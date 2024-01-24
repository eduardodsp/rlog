/**
 * @file server.c
 * @author edsp
 * @brief Default TCP server interface implementation. 
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
#include <sys/socket.h>

#include "server.h"
#include "../interfaces.h"
#include "../../port/os/osal.h"
#include "../../rlog.h"

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
bool rlog_tcp_init(void* me);

/**
 * @brief Check if at least one client has connected and test all connections. 
 * Non blocking function!
 *
 * @param me Not used.
 * @return true if there is at least one client connected.
 */
bool rlog_tcp_poll(void* me);

/**
 * @brief Send data to all connected TCP clients
 * 
 * @param me Not used.
 * @param buf Buffer holding the message to be sent
 * @param len Length of the message in bytes
 * @return true if was able to send a message to at least one client 
 */
bool rlog_tcp_send(void* me, const void* buf, int len);

rlog_ifc_t rlog_tcp_server_ifc = {
    .init       = &rlog_tcp_init,
    .poll       = &rlog_tcp_poll,
    .send       = &rlog_tcp_send,
    .deinit     = NULL,
    .ctx        = NULL,
};

typedef struct rlog_tcp_cli_t
{
	int socket;
	struct in_addr ip;
	char ip_str[INET_ADDRSTRLEN];
	
}rlog_tcp_cli_t;

static int server_socket = -1;
static uint16_t server_port = RLOG_TCP_SERVER_PORT;
static struct sockaddr_in sock_addr = { 0 };
static rlog_tcp_cli_t cli[RLOG_TCPIP_MAX_CLI];
static bool initialized = false;

bool rlog_tcp_server_config(unsigned int port)
{
    if( initialized )
        return false;

    if( port ) {
        server_port = port;
    }
    
    return true;
}


bool rlog_tcp_init(void* me)
{	
    if( initialized ) {
        return true;
    }
    
	for (int i=0; i < RLOG_TCPIP_MAX_CLI; i++)
	{			
		cli[i].socket = -1;
		strcpy(cli[i].ip_str, "0.0.0.0");
	}
	
    server_socket = socket(AF_INET , SOCK_STREAM , 0);

    if( server_socket == -1 ) 
    {
        DBG_PRINTF("[RLOG] rlog_tcp_init::socket() failed %d\n", errno);
        return false;
    }

    //set server socket to allow multiple connections,  
    //this is just a good habit, it will work without this  
    int opt = 1;   
    if( setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )   
    {   
        DBG_PRINTF("[RLOG] rlog_tcp_init::bind() failed %d\n", errno);
        close(server_socket);
        server_socket = -1;
		return false;   
    }  

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons( server_port );

    int flags = fcntl(server_socket, F_GETFL, 0);
    fcntl(server_socket, F_SETFL, flags | O_NONBLOCK);
    
	if( bind(server_socket,(struct sockaddr *)&sock_addr , sizeof(sock_addr)) < 0 )
	{
        DBG_PRINTF("[RLOG] rlog_tcp_init::bind() failed %d\n", errno);
        close(server_socket);
        server_socket = -1;
		return false;
	}

    if( listen(server_socket, RLOG_TCPIP_MAX_CLI) != 0 )
    {
        DBG_PRINTF("[RLOG] rlog_tcp_init::listen() failed %d\n", errno);
        close(server_socket);
        server_socket = -1;
        return false;
    }

    initialized = true;
    return true;
}

bool rlog_tcp_check_socket(int sockfd) 
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

bool rlog_tcp_poll(void* me)
{
    int len;
    struct sockaddr_in client;
    int fd = -1;

    if( server_socket < 0 ) {
        return false;
    }

    for (int i=0; i < RLOG_TCPIP_MAX_CLI; i++)
    {			
        if ( cli[i].socket > 0 )
        {
            if( !rlog_tcp_check_socket(cli[i].socket) )
            {
                // we lost the connection but haven't detected till now
                rlogf(RLOG_WARNING, "[RLOG] Lost connection from %s", cli[i].ip_str);	
                cli[i].socket = -1;
            }				
        }			
    }

    len = sizeof(client);
    fd = accept(server_socket, (struct sockaddr *)&client, (socklen_t*)&len);

	if( fd > 0 ) // accept() was successful	
	{
        // find available socket			
		for (int i=0; i < RLOG_TCPIP_MAX_CLI; i++)
		{			
			if ( cli[i].socket == -1 )
			{
				// assume new client
				cli[i].socket = fd;
				struct in_addr ipAddr = client.sin_addr;
				inet_ntop( AF_INET, &ipAddr, cli[i].ip_str, INET_ADDRSTRLEN );
				rlogf(RLOG_INFO, "[RLOG] New connection from %s", cli[i].ip_str);
				return true;				
			}
		}
		
		// should never pass here, but in any case..
		DBG_PRINTF("[RLOG] rlog_tcp_poll: a new connection was accepted but no socket is available! \n");		
		return false;		
	}
	else // accept() failed 
	{
        // if errno == EWOULDBLOCK it only means that no one is trying to connect and thats ok, however..
        if( errno != EWOULDBLOCK ) 
        {
            // something went terribly wrong, print!
            DBG_PRINTF("[RLOG] rlog_tcp_poll::accept() failed %d\n", errno);
        }				

		for (int i=0; i < RLOG_TCPIP_MAX_CLI; i++)
		{			
			if ( cli[i].socket > 0 )
			{
                //we have at least one client still connected, so carry on..
                return true;
			}
		}	
	}
	
    // we don't have any client so nothing to do..
    return false;
}

bool rlog_tcp_send(void* me, const void* buf, int len)
{
	int ret = 0;
	unsigned int logs_sent = 0;
	
	for (int i=0; i < RLOG_TCPIP_MAX_CLI; i++)
	{
		if( cli[i].socket > 0 )
		{
			ret = send(cli[i].socket, buf, len, MSG_DONTWAIT);
			
			if( ret == -1 )
			{
				DBG_PRINTF("[RLOG] rlog_tcp_send::send() failed %d\n", errno);
				cli[i].socket = -1;
				rlogf(RLOG_INFO, "[RLOG] Lost connection from %s", cli[i].ip_str);
			}
			else{
				logs_sent++;
			}
		}		
	}
	
    // if we were able to send to at least 1 client its a success!
	if( logs_sent )
		return true;
	
    // no client received this log, we must warn the daemon to do some backup
	return false;
}