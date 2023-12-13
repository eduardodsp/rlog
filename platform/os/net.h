/**
 * @file net.h
 * @author edsp
 * @brief Network APIs
 * @version 1.0.0
 * @date 2021-11-06
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

/**
 * @brief Get a string with client IP address
 * 
 * @param buf Buffer to store the string
 * @param size Buffer size
 * @return int 
 */
const char* net_get_client_ip(void);

#endif //_RLOG_NET_H_