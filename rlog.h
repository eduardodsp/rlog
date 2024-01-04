/**
 * @file rlog.h
 * @author edsp
 * @brief RLOG Server API
 * @date 2022-09-17
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
 */

#ifndef _RLOG_H_
#define _RLOG_H_

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include "port/com/interfaces.h"

/**
 * @brief Current RLOG version.
 * This follows the classic semantic versioning format
 */
#define RLOG_VERSION "2.4.0"

/**
 * @brief User defined maximum size of log messages.
 */
#ifndef RLOG_MAX_SIZE_CHAR
    #define RLOG_MAX_SIZE_CHAR  80
#endif

/**
 * @brief Enable (1) or Disable (0) timestamping of messages.
 */
#ifndef RLOG_TIMESTAMP_ENABLE
    #define RLOG_TIMESTAMP_ENABLE 1
#endif

#ifndef RLOG_DLOG_ENABLE
    #define RLOG_DLOG_ENABLE 1
#endif

typedef enum
{
    RLOG_INFO = 0,
    RLOG_WARNING,
    RLOG_ERROR, 

}RLOG_TYPE;

/**
 * @brief Initializes rlog server.
 * @param filepath Fullpath for backup file, including filename. Only used if
 * RLOG_DLOG_ENABLE is set to 1, else it is ignored
 * @param size Size of backup file in number of message entries. Only used if
 * RLOG_DLOG_ENABLE is set to 1, else it is ignored
 * @param ifc Communication interface to be used. If set to RLOG_DEFAULT_IFC then
 * standard TCP/IP protocol will be used. See \ref rlog_ifc_t for more details.
 * @return true if succesfull initialized the server
 * @return false if failed
 */
bool rlog_init(const char* filepath, unsigned int size, rlog_ifc_t ifc);

/**
 * @brief Insert a log message into the queue
 * 
 * @param type Message type identifier, see @RLOG_TYPE.
 * @param msg Message to be logged.
 */
void rlog(RLOG_TYPE type, const char* msg);

/**
 * @brief Composes a string based on format and variable arguments
 * and insert into the queue
 * 
 * @param type Message type identifier, see @RLOG_TYPE.
 * @param format Message format to be used to create a new message.
 * @param ... format arguments
 */
void rlogf(RLOG_TYPE type, const char* format, ...);

/**
 * @brief Initialize interface structure. See \ref rlog_ifc_t for more details
 * 
 * @param ifc Pointer to interface descriptor strucutre
 * @param ifc_init Function pointer to intialize communication interface
 * @param ifc_poll Function pointer wait for client connection
 * @param ifc_send Function pointer send messages to the client.
 */
inline void rlog_init_interface( 
    rlog_ifc_t* ifc, 
    bool (*ifc_init)(void),
    bool (*ifc_poll)(void),
    bool (*ifc_send)(const void* buf, int len)
    )
{
    ifc->init = ifc_init;
    ifc->poll = ifc_poll;
    ifc->send = ifc_send;
}

#endif //_RLOG_H_