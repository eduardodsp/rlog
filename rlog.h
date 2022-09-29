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

/**
 * @brief Current RLOG version.
 * This follows the classic semantic versioning format
 */
#define RLOG_VERSION "1.0.0"

/**
 * @brief API success return 
 */
#define RLOG_OK 0

typedef enum
{
    RLOG_INFO = 0,
    RLOG_WARNING,
    RLOG_ERROR, 

}rlog_type_e;


typedef enum
{
    RLOG_STS_INVALID,
    RLOG_STS_INTIALIZING,
    RLOG_STS_RUNNING,
    RLOG_STS_DEAD,

}rlog_sts_e;

typedef struct rlog_server_stats_t
{
    /**
     * @brief Queue overflow counter
     */
    uint32_t    queue_ovf;

    /**
     * @brief Number of elements currently in the queue
     */    
    uint32_t    queue_cnt;

    /**
     * @brief Queue watermark. Maximum elements ever stored in the queue.
     */
    uint32_t    queue_max_cnt;

    /**
     * @brief Current server status
     */
    rlog_sts_e  status;

} rlog_server_stats_t;

/**
 * @brief Initializes rlog server.
 * @return 0 on success, negative number on failure.
 */
int rlog_init(void);

/**
 * @brief Insert a log message into the queue
 * 
 * @param tag Arbitrary string with up to 8 characters.
 * @param type Message type identifier, see @rlog_type_e.
 * @param msg Message to be logged.
 * @return 0 on success, negative number on failure.
 */
int rlog(char* tag, rlog_type_e type, char* msg);

/**
 * @brief Get server statistics 
 * @return Server runtime statistics for debuging 
 */
rlog_server_stats_t rlog_get_stats(void);

#endif //_RLOG_H_