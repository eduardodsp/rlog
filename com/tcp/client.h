#ifndef _RLOG_TCP_CLIENT_H_
#define _RLOG_TCP_CLIENT_H_

#include <stdbool.h>

/**
 * @brief Configure server address and port.
 * 
 * @param addr C String containing server address, this could be an URL or an IP address.
 * @param port Port number.
 * @return true If successfully configured the interface
 * @return false If failed.
 */
bool rlog_tcpcli_config(const char* addr, unsigned int port);

#endif