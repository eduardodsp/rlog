#ifndef _RLOG_UDPIP_H_
#define _RLOG_UDPIP_H_

#include <stdbool.h>

/**
 * @brief RLOG default UDP Port
 */
#ifndef RLOG_UDP_DEFAULT_PORT
    #define RLOG_UDP_DEFAULT_PORT  514
#endif

/**
 * @brief Configure server address and port.
 * 
 * @param addr C String containing server address, this could be an URL or an IP address.
 * @param port Port number.
 * @return true If successfully configured the interface
 * @return false If failed.
 */
bool rlog_udp_config(const char* ip, unsigned int port);

#endif