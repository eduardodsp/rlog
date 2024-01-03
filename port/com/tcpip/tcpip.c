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

#include "../../os/osal.h"
#include "tcpip.h"

#ifdef DPRINTF
#include <stdio.h>
#endif

rlog_ifc_t rlog_default_tcpip_ifc = {
    .init       = &rlog_tcp_init,
    .connect    = &rlog_tcp_wait_conn,
    .send       = &rlog_tcp_send,
    .get_cli    = &rlog_tcp_get_client_ip,
};

static int socket_fd = -1;
static int cli_socket_fd = -1;
static char cli_ip[INET_ADDRSTRLEN] = "";
static struct sockaddr_in sock_addr = { 0 };

int rlog_tcp_init()
{
    socket_fd = socket(AF_INET , SOCK_STREAM , 0);

    if(socket_fd == -1)
        return -1;

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons( RLOG_TCPIP_PORT );

    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd, F_SETFL, flags | O_NONBLOCK);
    
    //Bind
	if(bind(socket_fd,(struct sockaddr *)&sock_addr , sizeof(sock_addr)) < 0)
	{
        close(socket_fd);
        socket_fd = -1;
		return -2;
	}

    // Now server is ready to listen and verification
    if ((listen(socket_fd, 1)) != 0)
    {
        close(socket_fd);
        socket_fd = -1;
        return -3;
    }

    return RLOG_COM_OK;
}

int rlog_tcp_wait_conn()
{
    int len;
    struct sockaddr_in client;

    if(socket_fd < 0)
        return -1;

    len = sizeof(client);

    cli_socket_fd = accept(socket_fd, (struct sockaddr *)&client, (socklen_t*)&len);

    if (cli_socket_fd < 0) {

        // accept returns EWOULDBLOCK if O_NONBLOCK is set for the socket and no connections are present to be accepted.
        if( errno == EWOULDBLOCK )
            return RLOG_COM_NO_CLIENT;

        // anything else assume is error!
        return -2;
    }

    struct in_addr ipAddr = client.sin_addr;
    inet_ntop( AF_INET, &ipAddr, cli_ip, INET_ADDRSTRLEN );

    return RLOG_COM_NEW_CLIENT;
}

int rlog_tcp_send(const void* buf, int len)
{
	int ret = 0;
    if(cli_socket_fd < 0)
        return -1;

    ret = send(cli_socket_fd,buf, len, MSG_DONTWAIT);

    if(ret == -1)
    {
    	if(errno == ENOTCONN || errno == ENETDOWN || errno == ECONNRESET)
    		return -2; //lost connection
    }

    return RLOG_COM_OK;
}

int rlog_tcp_recv(void* buf, int len)
{
    if(cli_socket_fd < 0)
        return -1;

    int ret = recv(cli_socket_fd , buf , len , MSG_DONTWAIT);

    return ret;
}

const char* rlog_tcp_get_client_ip()
{
    if (cli_socket_fd < 0)
        return NULL;

    return (const char*) cli_ip;
}

