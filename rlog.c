/**
 * @file rlog.c
 * @author edsp
 * @brief RLOG Server API implementation
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
 * 
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "rlog.h"
#include "platform/env/rlog_osal.h"
#include "platform/env/rlog_net.h"

#define _DEBUG_QUEUE_ 0
#define _DEBUG_THREAD_ 0

/**
 * @brief Enable (1) or Disable (0) timestamping of messages.
 */
#ifndef RLOG_TIMESTAMP_ENABLE
    #define RLOG_TIMESTAMP_ENABLE 1
#endif

/**
 * @brief User defined maximum size of log messages.
 */
#ifndef RLOG_MAX_SIZE_CHAR
    #define RLOG_MAX_SIZE_CHAR  100
#endif

/**
 * @brief User defined log message queue size
 */
#ifndef RLOG_MSG_QUEUE_SIZE
    #define RLOG_MSG_QUEUE_SIZE 4
#endif

/**
 * @brief User defined server thread period in milliseconds
 */
#ifndef RLOG_THREAD_PERIOD_MS
    #define RLOG_THREAD_PERIOD_MS 10
#endif

#define THREAD_PERIOD_US (RLOG_THREAD_PERIOD_MS * 1000)
#define QUEUE_POLLING_PERIOD_US 1000

/**
 * @brief Enable (1) or disable (0) the sending of periodic heartbeat messages
 */
#ifndef RLOG_SEND_HEARTBEAT
    #define RLOG_SEND_HEARTBEAT 0
#endif

/**
 * @brief User defined heartbeat period in seconds
 */
#if RLOG_SEND_HEARTBEAT
    #ifndef RLOG_HEARTBEAT_PERIOD_SEC
        #define RLOG_HEARTBEAT_PERIOD_SEC 5
    #endif
#endif

#if RLOG_SEND_HEARTBEAT
    #define HEARTBEAT_PERIOD_US (RLOG_HEARTBEAT_PERIOD_SEC * 1000000)
    #define HEARTBEAT_PERIOD_TICKS (HEARTBEAT_PERIOD_US / THREAD_PERIOD_US)
#endif

#define MSG_MAX_SIZE_CHAR (RLOG_MAX_SIZE_CHAR + 40) //includes date, tags and termination character
#define MSG_QUEUE_SIZE RLOG_MSG_QUEUE_SIZE


typedef char msg_t[MSG_MAX_SIZE_CHAR];

/**
 * @brief Ring buffer implementation
 * using queue
 */
struct queue_s
{
    unsigned int  tail;
    unsigned int  head;
    msg_t         buffer[MSG_QUEUE_SIZE];
    unsigned int  cnt;
    unsigned int  ovf;
    unsigned int  max_cnt;
};

/**
 * @brief Message queue
 */
static struct queue_s   smq;

/**
 * @brief  Tx buffer
 */
static char stx[MSG_MAX_SIZE_CHAR] = { 0 };

/**
 * @brief Server status
 */
static rlog_sts_e ssts = RLOG_STS_INVALID;

/**
 * @brief Heartbeat timer counter
 */
#if RLOG_SEND_HEARTBEAT
static int  shtb_tmr = 0;
#endif
/**
 * @brief server thread handle
 */
static osal_thread_t* sthread;

/**
 * @brief mutual exclusion semaphore
 */
static osal_mutex_t*  smutex;

/**
 * @brief Initializes the queue
 */
static void queue_init(void);

/**
 * @brief Put a c string on the queue
 * @param msg Buffer holding the null-terminated string
 */
static void  queue_put(char* msg);

/**
 * @brief Removes a message from the queue
 * 
 * @param msg Buffer to hold the null-terminated string
 * @return Length of the received message 
 */
static int  queue_get(char* msg);

void rlog_thread(void* arg);

void queue_init(void)
{
    smq.head = 0;
    smq.tail = 0;
    smq.cnt = 0;
    smq.ovf = 0;  
    smq.max_cnt = 0; 
}

void queue_put(char* msg)
{
    osal_lock_mutex(smutex);

    if( smq.cnt < MSG_QUEUE_SIZE )
    {
        smq.cnt++;
    }
    else
    {
        smq.ovf++;
    }

    strcpy(smq.buffer[smq.tail],msg);

    smq.tail = (smq.tail + 1) % MSG_QUEUE_SIZE;
    
    if( smq.cnt > smq.max_cnt )
        smq.max_cnt = smq.cnt;

    osal_unlock_mutex(smutex);

#if _DEBUG_QUEUE_        
        printf("%s \n", msg);
#endif  
}

int queue_get(char* msg)
{
	int len = 0;
    osal_lock_mutex(smutex);

    if( smq.cnt == 0 )
    {   
        osal_unlock_mutex(smutex); 
        return len;
    }

    strcpy(msg,smq.buffer[smq.head]);

    smq.head = (smq.head + 1) % MSG_QUEUE_SIZE;

    smq.cnt--;

    osal_unlock_mutex(smutex);

    len = strlen(msg);

    return len;
}

int rlog_init(void)
{

    int  ierr = 0;

    ssts = RLOG_STS_INTIALIZING;

    queue_init();

    ierr = osal_create_mutex((osal_mutex_t*) &smutex);

    if( ierr < 0 )
        return -1;
    
    ierr = osal_create_thread((osal_thread_t*)&sthread, rlog_thread, NULL);
    
    if( ierr < 0 )
        return -2;
    
    return 0;
}

int rlog(char* tag, rlog_type_e type, char* msg)
{
    char log[MSG_MAX_SIZE_CHAR] = {0};
    char date[21] = { 0 };
    int nchar = 0;

    if( strlen(tag) > 8 )
        return -1;

    if( strlen(msg) > RLOG_MAX_SIZE_CHAR )
        return -2;

#if RLOG_TIMESTAMP_ENABLE
    osal_get_date(date);
#endif

    switch(type)
    {
        case RLOG_INFO:
            nchar = snprintf(log, MSG_MAX_SIZE_CHAR,"%s [i][%s]: %s \n",date, tag, msg);
            break;

        case RLOG_WARNING:
            nchar =snprintf(log, MSG_MAX_SIZE_CHAR,"%s [!][%s]: %s \n",date, tag, msg);
            break;

        case RLOG_ERROR:
            nchar = snprintf(log, MSG_MAX_SIZE_CHAR,"%s [#][%s]: %s \n",date, tag, msg);
            break;

        default:
            return -3;
            break;

    }
    
    if( nchar < 0 || nchar > MSG_MAX_SIZE_CHAR )
        return -4;

    queue_put(log);
    return 0;
}

rlog_server_stats_t rlog_get_stats(void)
{
    rlog_server_stats_t stats;

    osal_lock_mutex(smutex);
    stats.status    = ssts;
    stats.queue_ovf = smq.ovf;
    stats.queue_cnt = smq.cnt;
    stats.queue_max_cnt = smq.max_cnt;    
    osal_unlock_mutex(smutex);

    return stats;
}

#define SERVER_BANNER "RLOG Server v"RLOG_VERSION " up and running!"

void rlog_thread(void* arg)
{    
    int conn = 0;
    int ierr = 0;
    
    ssts = RLOG_STS_RUNNING;

    ierr = net_init();

    if ( ierr < 0 )
    {
#if _DEBUG_THREAD_        
        printf("rlog_thread net_init err: %d \n", ierr);
#endif        
        ssts = RLOG_STS_DEAD;
        return;
    }
    
    rlog("RLOG",RLOG_INFO, SERVER_BANNER);
        
    while(1)
    { 
                      
        ierr = net_wait_conn();
        if( ierr < 0 )
        {
        	osal_sleep(THREAD_PERIOD_US);
        	continue;
        }
        
        conn = 1;
        while( conn )
        {
        	//dispatch all enqueued log messages
            while( queue_get(stx) )
            {
            	if( net_send(stx,strlen(stx)) < 0 )
            	{
                    rlog("RLOG",RLOG_WARNING,"Lost connection");
                    conn = 0;
                    break;
            	}
            	 osal_sleep(QUEUE_POLLING_PERIOD_US);
            }

#if RLOG_SEND_HEARTBEAT
            if( shtb_tmr > HEARTBEAT_PERIOD_TICKS )
            {
                rlog("RLOG",RLOG_INFO,"Heartbeat");
                shtb_tmr = 0;
            }
            shtb_tmr++;
#endif
            osal_sleep(THREAD_PERIOD_US);
        }        
    }    
}
