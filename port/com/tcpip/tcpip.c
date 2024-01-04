/**
 * @file tcp.c
 * @author edsp
 * @brief Default TCP/IP IP APIs for client/server communications
 * @version 1.0.0
 * @date 2022-09-29
 * 
 * @copyright Copyright (c) 2022
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

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "../interfaces.h"
#include "../../os/osal.h"
#include "../../../rlog.h"

#define TCPIP_DBG 0

#if TCPIP_DBG
#include <stdio.h>
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)
#endif


/**
 * @brief User defined RLOG protocol TCP Port 
 */
#define RLOG_TCPIP_PORT  8888

/**
 * @brief Initialize server TCP socket
 * @return 0 on success, negative number on failure.
 */
bool rlog_tcp_init(void);

/**
 * @brief Check if client has connected. Non blocking function!
 * @return 0 on success, negative number on failure.
 */
bool rlog_tcp_poll(void);

/**
 * @brief Send data to TCP client
 * 
 * @param buf Buffer holding the message to be sent
 * @param len Length of the message in bytes
 * @return 0 on success, negative number on failure.
 */
bool rlog_tcp_send(const void* buf, int len);


rlog_ifc_t rlog_default_tcpip_ifc = {
    .init       = &rlog_tcp_init,
    .poll       = &rlog_tcp_poll,
    .send       = &rlog_tcp_send,
};

static int socket_fd = -1;
static int cli_socket_fd = -1;
static char cli_ip[INET_ADDRSTRLEN] = "";
static struct sockaddr_in sock_addr = { 0 };

bool rlog_tcp_init()
{
    socket_fd = socket(AF_INET , SOCK_STREAM , 0);

    if(socket_fd == -1) 
    {
        DBG_PRINTF("[RLOG] rlog_tcp_init::socket() failed %d\n", errno);
        return false;
    }

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons( RLOG_TCPIP_PORT );

    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    //Bind
	if(bind(socket_fd,(struct sockaddr *)&sock_addr , sizeof(sock_addr)) < 0)
	{
        DBG_PRINTF("[RLOG] rlog_tcp_init::bind() failed %d\n", errno);
        close(socket_fd);
        socket_fd = -1;
		return false;
	}

    // Now server is ready to listen and verification
    if ((listen(socket_fd, 1)) != 0)
    {
        DBG_PRINTF("[RLOG] rlog_tcp_init::listen() failed %d\n", errno);
        close(socket_fd);
        socket_fd = -1;
        return false;
    }

    return true;
}

bool rlog_tcp_poll()
{
    int len;
    struct sockaddr_in client;

    // check if still connected
    if( cli_socket_fd > 0)
        return true;

    if(socket_fd < 0)
        return false;

    len = sizeof(client);

    cli_socket_fd = accept(socket_fd, (struct sockaddr *)&client, (socklen_t*)&len);

    if (cli_socket_fd < 0) 
    {
        // accept returns EWOULDBLOCK if O_NONBLOCK is set for the socket and no connections are present to be accepted
        // so thats not an actual error
        if( errno != EWOULDBLOCK ) {
            DBG_PRINTF("[RLOG] rlog_tcp_poll::accept() failed %d\n", errno);
        }
        
        return false;
    }

    struct in_addr ipAddr = client.sin_addr;
    inet_ntop( AF_INET, &ipAddr, cli_ip, INET_ADDRSTRLEN );

    rlogf(RLOG_INFO, "[RLOG] New connection from %s", cli_ip);
    return true;
}

bool rlog_tcp_send(const void* buf, int len)
{
	int ret = 0;
    if(cli_socket_fd < 0)
        return false;

    ret = send(cli_socket_fd, buf, len, MSG_DONTWAIT);

    if(ret == -1)
    {
        DBG_PRINTF("[RLOG] rlog_tcp_send::send() failed %d\n", errno);
        cli_socket_fd = -1;
        rlogf(RLOG_WARNING, "[RLOG] Lost connection from %s", cli_ip);
        return false; //lost connection
    }

    return true;
}