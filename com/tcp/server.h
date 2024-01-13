#ifndef _RLOG_TCP_SERVER_H_
#define _RLOG_TCP_SERVER_H_

#include <stdbool.h>

/**
 * @brief Default TCP server port 
 */
#ifndef RLOG_TCP_SERVER_PORT
    #define RLOG_TCP_SERVER_PORT  1514
#endif

bool rlog_tcp_server_config(unsigned int port);

#endif //_RLOG_TCP_SERVER_H_