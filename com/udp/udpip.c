/**
 * @file udpip.c
 * @author edsp
 * @brief Default UDP APIs for client/server communications. 
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

#include "udpip.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "../interfaces.h"
#include "../../os/osal.h"
#include "../../../rlog.h"

#ifndef _RLOG_UDPIP_DBG_
    #define _RLOG_UDPIP_DBG_ 1
#endif

#if _RLOG_UDPIP_DBG_
#include <stdio.h>
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)
#endif

/**
 * @brief Rlog thread stack size. Default 4096
 */
#ifndef RFC6587_STACK_SIZE
    #define RFC6587_STACK_SIZE 2048
#endif

/**
 * @brief Rlog thread priority. Default 8
 */
#ifndef RFC6587_TASK_PRIO
    #define RFC6587_TASK_PRIO 8
#endif

/**
 * @brief Initialize server UDP socket
 * 
 * @param me Not used.
 * @return true if UDP socket is ready and listening
 */
bool rlog_udp_init(void* me);

/**
 * @brief Check if at least one client has connected and test all connections. 
 * Non blocking function!
 *
 * @param me Not used.
 * @return true if there is at least one client connected.
 */
bool rlog_udp_poll(void* me);

/**
 * @brief Send data to all connected UDP clients
 * 
 * @param me Not used.
 * @param buf Buffer holding the message to be sent
 * @param len Length of the message in bytes
 * @return true if was able to send a message to at least one client 
 */
bool rlog_udp_send(void* me, const void* buf, int len);

rlog_ifc_t rlog_udp_ifc = {
    .init       = &rlog_udp_init,
    .poll       = &rlog_udp_poll,
    .send       = &rlog_udp_send,
    .deinit     = NULL,
    .ctx        = NULL,
};

static int my_socket = -1;
static struct sockaddr_in sock_addr = { 0 };
static char server_ip[] = "255.255.255.255";
static uint16_t udp_port = RLOG_UDP_DEFAULT_PORT;
static bool configured = false;
static bool initialized = false;

bool rlog_udp_config(const char* ip, unsigned int port)
{ 
    if( port ) {
        udp_port = port;
    }
    
    if( ip == NULL ) 
    {
        DBG_PRINTF("[RLOG] rlog_udp_config:: invalid IP!\n");
        return false;      
    }

    strncpy(server_ip, ip, sizeof(server_ip));
    configured = true;
    return true;
}

bool rlog_udp_init(void* me)
{	

    if( initialized )
        return true;

    if( !configured )
        return false;

    my_socket = socket(AF_INET , SOCK_DGRAM , 0);

    if( my_socket == -1 ) 
    {
        DBG_PRINTF("[RLOG] rlog_udp_init::socket() failed %d\n", errno);
        return false;
    }

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(server_ip);
    sock_addr.sin_port = htons( udp_port );

    initialized = true;
    return true;
}

bool rlog_udp_poll(void* me)
{
    return (initialized && configured);
}

bool rlog_udp_send(void* me, const void* buf, int len)
{
    int ret = sendto(my_socket, buf, len, MSG_DONTWAIT, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
    if( ret < 1 )
    {
        if (ret < 0) {
            DBG_PRINTF("[RLOG] rlog_udp_send::send() failed %d\n", errno);
        }

        return false;
    }	
	return true;
}