#include <stdlib.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"

#include "../rlog_osal.h"
#include "../rlog_net.h"

#ifdef DPRINTF
#include <stdio.h>
#endif

static int socket_fd = -1;
static int cli_socket_fd = -1;
static struct sockaddr_in sock_addr = { 0 };

int net_init()
{
    socket_fd = socket(AF_INET , SOCK_STREAM , 0);

    if(socket_fd == -1)
        return -1;

    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = INADDR_ANY;
    sock_addr.sin_port = htons( RLOG_NET_PORT );

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

    return 0;
}

int net_wait_conn()
{
    int len;
    struct sockaddr_in client;

    if(socket_fd < 0)
        return -1;

    len = sizeof(client);

    cli_socket_fd = accept(socket_fd, (struct sockaddr *)&client, (socklen_t*)&len);

    if (cli_socket_fd < 0)
        return -2;


    return 0;
}

int net_send(const void* buf, int len)
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

    return 0;
}

int net_recv(void* buf, int len)
{
    if(cli_socket_fd < 0)
        return -1;

    int ret = recv(cli_socket_fd , buf , len , MSG_DONTWAIT);

    return ret;
}
