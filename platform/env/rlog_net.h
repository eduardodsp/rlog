/**
 * @file rlog_net.h
 * @author edsp
 * @brief Network APIs
 * @version 0.1
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef _RLOG_NET_H_
#define _RLOG_NET_H_

/**
 * @brief User defined RLOG protocol TCP Port 
 */
#define RLOG_NET_PORT  8888

/**
 * @brief Initialize server TCP socket
 * @return 0 on success, negative number on failure.
 */
int net_init();

/**
 * @brief Wait for connection
 * @return 0 on success, negative number on failure.
 */
int net_wait_conn();

/**
 * @brief Send data to TCP client
 * 
 * @param buf Buffer holding the message to be sent
 * @param len Length of the message in bytes
 * @return 0 on success, negative number on failure.
 */
int net_send(const void* buf, int len);

/**
 * @brief Receive data from client
 * 
 * @param buf buffer to store received message
 * @param len size of the buffer
 * @return Number of bytes received, -1 for error
 */
int net_recv(void* buf, int len);

#endif //_RLOG_NET_H_