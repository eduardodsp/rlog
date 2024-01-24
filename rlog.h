/**
 * @file rlog.h
 * @author edsp
 * @brief RLOG Server API
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
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

#include "com/interfaces.h"
#include "format/format.h"

/**
 * @brief Current RLOG version.
 * This follows the classic semantic versioning format
 */
#define RLOG_VERSION "1.0.0"

/**
 * @brief Enable (1) or Disable (0) timestamping of messages.
 */
#ifndef RLOG_TIMESTAMP_ENABLE
    #define RLOG_TIMESTAMP_ENABLE 1
#endif

#ifndef RLOG_DLOG_ENABLE
    #define RLOG_DLOG_ENABLE 1
#endif

/**
 * @brief RLOG configuration structure
 */
typedef struct rlog_cfg_t
{
    /**
     * @brief Device name. 
     * The string must have a maximum size of 20 characters including termination and
     * MUST NOT have spaces!
     */
    const char* name;

    /**
     * @brief Rlog thread priority
     */
    unsigned int priority;
    
    /**
     * @brief Fullpath for backup file, including filename. Only used if
     * RLOG_DLOG_ENABLE is set to 1, else it is ignored
     */
    const char* filepath;
    
    /**
     * @brief Size of backup file in number of message entries. Only used if
     * RLOG_DLOG_ENABLE is set to 1, else it is ignored
     */    
    unsigned int nlogs;

    /**
     * @brief Message format see \ref RLOG_FORMAT. Default value is RLOG_RFC3164
     */
    RLOG_FORMAT format;

    /**
     * @brief Log level. Will only log messages with level equal or smaller than this.
     * Ex: Setting this to RLOG_DEBUG will enable all levels whereas setting this
     * to RLOG_CRIT will only allow RLOG_CRIT, RLOG_ALERT and RLOG_EMERGENCY 
     * messages.
     */
    RLOG_LEVEL level;

}rlog_cfg_t;

/**
 * @brief Initializes rlog server.
 * @param cfg Configuration options
 * @return true if succesfull initialized the server
 * @return false if failed
 */
bool rlog_init(rlog_cfg_t cfg);

/**
 * @brief Kills the server and de-initialize all installed interfaces;
 */
void rlog_kill(void);

/**
 * @brief Insert a log message into the queue
 * 
 * @param type Message type identifier, see @RLOG_LEVEL.
 * @param msg Message to be logged.
 */
void rlog(RLOG_LEVEL type, const char* msg);

/**
 * @brief Composes a string based on format and variable arguments
 * and insert into the queue
 * 
 * @param type Message type identifier, see @RLOG_LEVEL.
 * @param format Message format to be used to create a new message.
 * @param ... format arguments
 */
void rlogf(RLOG_LEVEL type, const char* format, ...);

/**
 * @brief Install a new interface instance, See \ref rlog_ifc_t for more details.
 * This function will first attempt to intialize the interface using the provided 
 * init function and only then it will install the function pointers.
 * 
 * @param interface  Interface descriptor
 * @return true If interface was installed and initialized.
 * @return false If failed to install the interface.
 */
bool rlog_install_interface(rlog_ifc_t interface);

#endif //_RLOG_H_