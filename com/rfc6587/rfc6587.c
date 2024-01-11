/**
 * @file tcpip.c
 * @author edsp
 * @brief Default TCP APIs for client/server communications. 
 * This implementation support multiple clients however it does not 
 * support multilpe instances of the interface.
 * @version 1.0.0
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

#include <stdlib.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "../interfaces.h"
#include "../../os/osal.h"
#include "../../../rlog.h"

#ifndef _RLOG_TCPIP_DBG_
    #define _RLOG_TCPIP_DBG_ 1
#endif

#if _RLOG_TCPIP_DBG_
#include <stdio.h>
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)
#endif

/**
 * @brief RLOG protocol TCP Port 
 */
#ifndef RLOG_DEFAULT_TCPIP_PORT
    #define RLOG_DEFAULT_TCPIP_PORT  8888
#endif

/**
 * @brief Max number of TCP clients allowed.
 */
#ifndef RLOG_TCPIP_MAX_CLI
    #define RLOG_TCPIP_MAX_CLI 2
#endif

/**
 * @brief Initialize server TCP socket
 * 
 * @param me Not used.
 * @return true if TCP socket is ready and listening
 */
bool rfc6587_init(void* me);

/**
 * @brief Check if at least one client has connected and test all connections. 
 * Non blocking function!
 *
 * @param me Not used.
 * @return true if there is at least one client connected.
 */
bool rfc6587_poll(void* me);

/**
 * @brief Send data to all connected TCP clients
 * 
 * @param me Not used.
 * @param buf Buffer holding the message to be sent
 * @param len Length of the message in bytes
 * @return true if was able to send a message to at least one client 
 */
bool rfc6587_send(void* me, const void* buf, int len);

rlog_ifc_t rlog_rfc6587 = {
    .init       = &rfc6587_init,
    .poll       = &rfc6587_poll,
    .send       = &rfc6587_send,
    .deinit     = NULL,
    .ctx        = NULL,
};

static int my_socket = -1;
static char server_ip[] = "255.255.255.255";
static struct sockaddr_in sock_addr = { 0 };
static uint16_t rfc6587_port = 1514;
static bool connected = false;
static char tx[160] = "";

/**
 * @brief server thread handle
 */
static os_thread_t* thread_handle;
static void rfc6587_thread(void* arg);
static bool configured = false;
static bool initialized = false;

bool rlog_rfc6587_config(const char* ip, unsigned int port)
{
    if( initialized )
        return false;

    if( port ) {
        rfc6587_port = port;
    }
    
    if( ip == NULL ) 
    {
        DBG_PRINTF("[RLOG] rlog_rfc6587_config:: invalid IP!\n");
        return false;      
    }

    strncpy(server_ip, ip, sizeof(server_ip));
    configured = true;
    return true;
}

bool rfc6587_init(void* me)
{		
    if( initialized )
        return true;

    if( !configured )
        return false;

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(server_ip);
    sock_addr.sin_port = htons( rfc6587_port );

    thread_handle = os_thread_create("rfc6587", rfc6587_thread, NULL, 2048, 8);
    if( thread_handle == NULL ) {
        DBG_PRINTF("[RLOG] rlog_init failed to create thread\n");
        return false;
    }

    initialized = true;
    return true;
}

bool rfc6587_check_socket(int sockfd) 
{ 
    char sock_buf;
    if( recv(sockfd, &sock_buf, sizeof(char), MSG_PEEK | MSG_DONTWAIT) == 0 ) {
        // when a stream socket (i.e TCP) peer has performed an orderly shutdown, the return value will be 0.
        return false; 
    } 

    // since the client will never send anything the return -1 might mean
    // the socket is still open, but just in case lets test for ECONNRESET.
    // This needs double checking. It does not seem right but it is working :-)
    if ( errno == ECONNRESET ) { return false; }

    return true; 
} 

bool rfc6587_poll(void* me)
{	
    if( my_socket < 0 )
        return false;

    return connected;
}

bool rfc6587_send(void* me, const void* buf, int len)
{
    if( my_socket < 0 )
        return false;

    int nbytes = snprintf(tx, sizeof(tx), "%s\r\n", (const char*)buf);
    if( send(my_socket, tx, nbytes, MSG_DONTWAIT) < 0 )
    {
        DBG_PRINTF("[RLOG] rlog_tcp_send::send() failed %d\n", errno);
        rlogf(RLOG_INFO, "[RLOG] Lost connection to %s", server_ip);
        close(my_socket);
        my_socket = -1;
        connected = false;
        return false;
    }

	return true;
}

static
void rfc6587_thread(void* arg)
{                
    while( 1 )
    {         
        if( connected )
        {
            if( !rfc6587_check_socket(my_socket) )
            {
                rlogf(RLOG_INFO, "[RLOG] Lost connection to %s", server_ip);
                connected = false;
            }
        }

        if( !connected )
        {
            my_socket = socket(AF_INET , SOCK_STREAM , 0);

            if( my_socket == -1 ) 
            {
                DBG_PRINTF("[RLOG] rfc6587_init::socket() failed %d\n", errno);
            }
            else
            {
                // Send connection request to server
                if(connect(my_socket, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0){
                    connected = false;
                    os_sleep_us(1000 * 1000);
                } else {
                    rlogf(RLOG_INFO, "[RLOG] New connection to %s", server_ip);
                    connected = true;
                }
            }
        }
        os_sleep_us(10 * 1000);
    }
}