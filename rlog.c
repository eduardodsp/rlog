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

#include "dlog/dlog.h"
#include "rlog.h"
#include "port/os/osal.h"
#include "port/com/tcpip/tcpip.h"
#include "format.h"

#define _RLOG_DEBUG_QUEUE_ 0
#define _RLOG_DEBUG_ 1

#define RLOG_MAIN_TASK_STACK_SIZE 3192
#define RLOG_COM_TASK_STACK_SIZE 1024
#define RLOG_TASK_PRIO 8


/**
 * @brief User defined log message queue size
 */
#ifndef RLOG_MSG_QUEUE_SIZE
    #define RLOG_MSG_QUEUE_SIZE 10
#endif

/**
 * @brief User defined server thread period in milliseconds
 */
#ifndef RLOG_THREAD_PERIOD_MS
    #define RLOG_THREAD_PERIOD_MS 10
#endif

#define THREAD_PERIOD_US (RLOG_THREAD_PERIOD_MS * 1000)
#define QUEUE_POLLING_PERIOD_US 0

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

#define MSG_QUEUE_SIZE RLOG_MSG_QUEUE_SIZE

#define EVENT_NEW_CONNECTION    ( 1 << 0 )
#define EVENT_NEW_MSG           ( 1 << 1 )
#define EVENTS_MASK             ( EVENT_NEW_CONNECTION | EVENT_NEW_MSG )

rlog_ifc_t server_ifc;

typedef enum
{
    STATE_INIT,
    STATE_DISCONNECTED,
    STATE_CONNECTED,
    
}STATES;

/**
 * @brief Ring buffer implementation
 * using queue
 */
struct queue_s
{
    unsigned int    tail;
    unsigned int    head;
    log_t           buffer[MSG_QUEUE_SIZE];
    unsigned int    cnt;
    unsigned int    ovf;
    unsigned int    max_cnt;
};

/**
 * @brief Message queue
 */
static struct queue_s   msg_queue;

/**
 * @brief  Message buffer
 */
static char msg_buffer[MSG_MAX_SIZE_CHAR] = { 0 };

/**
 * @brief  Auxiliary statically allocated buffer
 */
static char aux_buffer[MSG_MAX_SIZE_CHAR] = { 0 };

/**
 * @brief Server status
 */
static RLOG_STATUS server_status = RLOG_DEAD;

/**
 * @brief Heartbeat timer counter
 */
#if RLOG_SEND_HEARTBEAT
static int  heartbeat_timer = 0;
#endif

/**
 * @brief server thread handle
 */
static os_thread_t* main_task_handle;

/**
 * @brief COM listener thread handle
 */
static os_thread_t* com_task_handle;

/**
 * @brief mutual exclusion semaphore
 */
static os_mutex_t*  queue_lock;

/**
 * @brief Indicates a lost connection and wakes the TCP thread
 * to wait for a new client.
 */
static os_sem_t* lost_conn_sema;

/**
 * @brief Set of events to wake the main thread
 */
static os_event_t* wakeup_events;

#if RLOG_DLOG_ENABLE
/**
 * @brief If backup logging is enabled we shall use
 * dlog to store messages when disconnected.
 */
static dlog_t logger;
#endif

/**
 * @brief Initializes the queue
 */
static void queue_init(void);

/**
 * @brief Put a c string on the queue
 * @param msg Buffer holding the null-terminated string
 */
static void  queue_put(log_t log, const char* msg);

/**
 * @brief Composes a string based on a format string and args
 * and insert in the queue
 * @param log Log metadata
 * @param format Format string
 * @param args Argument list
 */
static void queue_putf(log_t log, const char* format,  va_list args);

/**
 * @brief Removes a message from the queue
 * 
 * @param msg Buffer to hold the null-terminated string
 * @return Length of the received message 
 */
static int  queue_get(char* msg);

/**
 * @brief Main thread to receive new log messages and dispatch
 * to the remote client if connected, or to a backup file if
 * disconnected from client. As soon as a new client connects to
 * the server it will dump all log messages in the backup file in 
 * FIFO order and only then it will dispatch the newer messages from 
 * RAM.
 */
static void main_thread(void* arg);

/**
 * @brief Thread to wait a new TCP connection. As soon
 * as a new client is connected it sends an event to
 * the main thread.
 */
static void com_thread(void* arg);

/**
 * @brief Function to copy a string in a safe way. 
 * Essentially does the same thing as strncpy but without padding
 * also adds a null char at the end if string is truncated.
 * 
 * @param dst Destination buffer
 * @param src Source buffer
 * @param n Maximum number of characters to copy until a null char is found.
 */
static
void fast_strncpy(char* dst, const char* src, size_t n)
{
    int i = 0;
    for(; i < n; i++) {
        dst[i] = src[i];
        if(src[i] == '\0')
            break;
    }
    
    if ( i >= n ) {
        dst[n] = '\0';
    }
}

static
void queue_init(void)
{
    msg_queue.head = 0;
    msg_queue.tail = 0;
    msg_queue.cnt = 0;
    msg_queue.ovf = 0;  
    msg_queue.max_cnt = 0; 
}

static
void queue_put(log_t log, const char* msg)
{
    os_mutex_lock(queue_lock);

    if( msg_queue.cnt < MSG_QUEUE_SIZE )
        msg_queue.cnt++;
    else
        msg_queue.ovf++;

    msg_queue.buffer[msg_queue.tail].timestamp = log.timestamp;
    msg_queue.buffer[msg_queue.tail].type = log.type;
    fast_strncpy(msg_queue.buffer[msg_queue.tail].msg, msg, sizeof(msg_queue.buffer[msg_queue.tail].msg));
    msg_queue.tail = (msg_queue.tail + 1) % MSG_QUEUE_SIZE;
    
    if( msg_queue.cnt > msg_queue.max_cnt )
        msg_queue.max_cnt = msg_queue.cnt;

    os_mutex_unlock(queue_lock);

#if _RLOG_DEBUG_QUEUE_        
        printf("%s \n", msg);
#endif  
}

static
void queue_putf(log_t log, const char* format,  va_list args)
{
    os_mutex_lock(queue_lock);

    if( msg_queue.cnt < MSG_QUEUE_SIZE )
        msg_queue.cnt++;
    else
        msg_queue.ovf++;

    msg_queue.buffer[msg_queue.tail].timestamp = log.timestamp;
    msg_queue.buffer[msg_queue.tail].type = log.type;
    
    vsnprintf(msg_queue.buffer[msg_queue.tail].msg, sizeof(msg_queue.buffer[msg_queue.tail].msg), format, args);

    msg_queue.tail = (msg_queue.tail + 1) % MSG_QUEUE_SIZE;
    
    if( msg_queue.cnt > msg_queue.max_cnt )
        msg_queue.max_cnt = msg_queue.cnt;

    os_mutex_unlock(queue_lock);

#if _RLOG_DEBUG_QUEUE_        
        printf("%s \n", msg);
#endif  
}

static
int queue_get(char* msg)
{
    log_t log;

    os_mutex_lock(queue_lock);

    if( msg_queue.cnt == 0 )
    {   
        os_mutex_unlock(queue_lock); 
        return 0;
    }

    log.timestamp = msg_queue.buffer[msg_queue.head].timestamp;
    log.type = msg_queue.buffer[msg_queue.head].type;
    fast_strncpy(log.msg, msg_queue.buffer[msg_queue.head].msg, sizeof(msg_queue.buffer[msg_queue.head].msg));

    msg_queue.head = (msg_queue.head + 1) % MSG_QUEUE_SIZE;
    msg_queue.cnt--;
    os_mutex_unlock(queue_lock);

    make_log_string(msg, &log);
    return strlen(msg);
}


int rlog_init(const char* filepath, unsigned int size, rlog_ifc_t ifc)
{
    int err = 0;
    server_status = RLOG_INTIALIZING;

    if(ifc.connect == NULL) {
        err = -1;
        goto INIT_FAIL;
    }

    if(ifc.init == NULL) {
        err = -1;
        goto INIT_FAIL;
    }

    if(ifc.send == NULL) {
        err = -1;
        goto INIT_FAIL;
    }

    server_ifc = ifc;
    queue_init();

    queue_lock = os_mutex_create();
    if( queue_lock == NULL ) {
        err = -2;
        goto INIT_FAIL;
    }
    lost_conn_sema = os_sem_create(1, 0);
    if( lost_conn_sema == NULL ) {
        err = -3;
        goto INIT_FAIL;
    }

    wakeup_events = os_event_create();        

    main_task_handle = os_thread_create("rlog_main_tsk", main_thread, NULL, RLOG_MAIN_TASK_STACK_SIZE, RLOG_TASK_PRIO);
    if( main_task_handle == NULL ) {
        err = -4;
        goto INIT_FAIL;
    }
    
    com_task_handle = os_thread_create("rlog_com_tsk", com_thread, NULL, RLOG_COM_TASK_STACK_SIZE, RLOG_TASK_PRIO);
    if( com_task_handle == NULL ) {
        err = -5;
        goto INIT_FAIL;
    }

#if RLOG_DLOG_ENABLE
    if( dlog_open(&logger, filepath, size) != DLOG_OK ) {
        err = -6;
        goto INIT_FAIL;            
    }
#endif

    return RLOG_OK;

INIT_FAIL:
    server_status = RLOG_DEAD;
    return err;    
}

void rlog(RLOG_TYPE type, const char* msg)
{
    log_t log;
#if RLOG_TIMESTAMP_ENABLE
    time(&log.timestamp);
#endif
    log.type = type;
    queue_put(log, msg);
    os_event_set(wakeup_events, EVENT_NEW_MSG);
}

void rlogf(RLOG_TYPE type, const char* format, ...)
{
    va_list args;
    log_t log;

#if RLOG_TIMESTAMP_ENABLE
    time(&log.timestamp);
#endif
    log.type = type;
    va_start(args, format);
    queue_putf(log, format, args);
    va_end(args);
    os_event_set(wakeup_events, EVENT_NEW_MSG);
}

rlog_server_stats_t rlog_get_stats(void)
{
    rlog_server_stats_t stats;

    os_mutex_lock(queue_lock);
    stats.status    = server_status;
    stats.queue_ovf = msg_queue.ovf;
    stats.queue_cnt = msg_queue.cnt;
    stats.queue_max_cnt = msg_queue.max_cnt;    
    os_mutex_unlock(queue_lock);

    return stats;
}

#define SERVER_BANNER "RLOG Server v"RLOG_VERSION " up and running!"

#if RLOG_SEND_HEARTBEAT
    #define EVENT_TIMEOUT 10
#else
    #define EVENT_TIMEOUT OS_WAIT_FOREVER
#endif

static
void main_thread(void* arg)
{                
    uint32_t evts = 0; 
    STATES state = STATE_INIT;
    int err = DLOG_OK;

    while( 1 )
    { 
        server_status = RLOG_SLEEPING;
        evts = os_event_wait(wakeup_events, EVENTS_MASK, EVENT_TIMEOUT);
        os_event_clear(wakeup_events, evts);

        if( !evts )
        {
#if RLOG_SEND_HEARTBEAT
            heartbeat_timer++;
            if( heartbeat_timer > HEARTBEAT_PERIOD_TICKS ) {
                rlog("RLOG",RLOG_INFO,"Heartbeat");
                heartbeat_timer = 0;
            }
#endif
        }

        if( evts & EVENT_NEW_CONNECTION )
        {
            state = STATE_CONNECTED;
#if RLOG_DLOG_ENABLE
            // dump all messages from dlog to rlog-cli
            do
            {
                err = dlog_get(&logger, msg_buffer, sizeof(msg_buffer));                
                if ( err == DLOG_OK )
                {
                    err = server_ifc.send(msg_buffer, strlen(msg_buffer));
                    if( err != RLOG_COM_OK )
                    {
#if _RLOG_DEBUG_                    

                        printf("[RLOG] rlog_ifc_t::send() error %d\n", err);
#endif
                        os_sem_signal(lost_conn_sema);
                        state = STATE_DISCONNECTED;
                        break;
                    }
                    os_sleep_us(QUEUE_POLLING_PERIOD_US);                    
                }
                else if ( err != DLOG_EMPTY_QUEUE )
                {
#if _RLOG_DEBUG_                    
                    printf("[RLOG] dlog_get() error %d\n", err);
#endif
                }

            server_status = RLOG_DUMPING_DLOG;
            } while( err == DLOG_OK );
#endif            
        } 

        if( evts & EVENT_NEW_MSG )
        {
            switch( state )
            {
                case STATE_INIT:
                    rlog(RLOG_INFO, SERVER_BANNER);
                    state = STATE_DISCONNECTED;
                break;
                case STATE_DISCONNECTED:
                {
                    os_sem_signal(lost_conn_sema);
#if RLOG_DLOG_ENABLE                                     
                    while( queue_get(msg_buffer) )
                    {
                        server_status = RLOG_DUMPING_RAM_QUEUE;
                        dlog_put(&logger, msg_buffer);    
                        os_sleep_us(QUEUE_POLLING_PERIOD_US);
                    }
#endif                    
                }
                break;
                case STATE_CONNECTED:
                {
                    //dispatch all enqueued log messages
                    while( queue_get(msg_buffer) )
                    {
                        server_status = RLOG_DUMPING_RAM_QUEUE;
                        err = server_ifc.send(msg_buffer,strlen(msg_buffer));
                        if( err != RLOG_COM_OK )
                        {
#if _RLOG_DEBUG_                            
                            printf("[RLOG] rlog_ifc_t::send() error %d\n", err);
#endif
                            os_sem_signal(lost_conn_sema);
                            state = STATE_DISCONNECTED;
                            break;
                        }
                        os_sleep_us(QUEUE_POLLING_PERIOD_US);
                    }
                }
                break;
            }    
        } 
    }
    server_status = RLOG_DEAD;    
}

static
void com_thread(void* arg)
{
    const char* ip;
    int err = 0;

    err = server_ifc.init(); 
    if ( err != RLOG_COM_OK )
    {
#if _RLOG_DEBUG_                    
        printf("[RLOG] rlog_ifc_t::init() error: %d \n", err);
#endif
        return;
    }
            
    while( 1 )
    {                       
        err = server_ifc.connect(); 
        if ( err == RLOG_COM_OK ) 
        {
            if( server_ifc.get_cli != NULL ) 
            {
                ip = server_ifc.get_cli();
                if( ip != NULL ) 
                {
                    snprintf(aux_buffer, sizeof(aux_buffer), "[RLOG] New connection from %s", ip);
                    rlog(RLOG_INFO, aux_buffer);
                }
            }
            os_event_set(wakeup_events, EVENT_NEW_CONNECTION);
            os_sem_wait(lost_conn_sema, OS_WAIT_FOREVER);
            rlog(RLOG_WARNING,"[RLOG] Lost connection");
        }
        else
        {
#if _RLOG_DEBUG_                    
            printf("[RLOG] rlog_ifc_t::connect() error: %d \n", err);
#endif            
            os_sleep_us(10 * 1000 /*10ms*/);
        }
    }
}