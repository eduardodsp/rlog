#ifndef _RLOG_UDPIP_H_
#define _RLOG_UDPIP_H_

#include <stdbool.h>

/**
 * @brief RLOG default UDP Port
 */
#ifndef RLOG_UDP_DEFAULT_PORT
    #define RLOG_UDP_DEFAULT_PORT  514
#endif

bool rlog_udp_config(const char* ip, unsigned int port);

#endif