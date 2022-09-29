/**
 * @file rlog.h
 * @author edsp
 * @brief RLOG Server API
 * @version 0.1
 * @date 2022-09-17
 * 
 * @copyright Copyright (c) 2022
 *  
 */

#ifndef _RLOG_H_
#define _RLOG_H_


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
    uint32_t    queue_ovf;
    uint32_t    queue_cnt;
    uint32_t    queue_max_cnt;
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
 * @return rlog_server_stats_t 
 */
rlog_server_stats_t rlog_get_stats(void);

#endif //_RLOG_H_