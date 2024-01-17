/**
 * @file rlog.c
 * @author edsp
 * @brief RLOG Server API implementation
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
 * 
 */

#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "rlog.h"
#include "port/os/osal.h"

#if RLOG_DLOG_ENABLE
    #define DLOG_LINE_MAX_SIZE MSG_MAX_SIZE_CHAR
    #include "dlog/dlog.h"
#endif

#ifndef _RLOG_DBG_
    #define _RLOG_DBG_    0
#endif

#ifndef _RLOG_PRINTF_
    #define _RLOG_PRINTF_  1
#endif

#if _RLOG_PRINTF_
#define DBG_PRINTF(...) printf(__VA_ARGS__)
#else
#define DBG_PRINTF(...)
#endif

/**
 * @brief Rlog thread stack size. Default 4096
 */
#ifndef RLOG_STACK_SIZE
    #define RLOG_STACK_SIZE 4096
#endif

/**
 * @brief Rlog thread priority. Default 8
 */
#ifndef RLOG_TASK_PRIO
    #define RLOG_TASK_PRIO 8
#endif

/**
 * @brief Message queue size. Default 10
 */
#ifndef RLOG_QUEUE_SIZE
    #define RLOG_QUEUE_SIZE 10
#endif

/**
 * @brief Enable (1) or disable (0) the sending of periodic heartbeat messages
 */
#ifndef RLOG_HEARTBEAT
    #define RLOG_HEARTBEAT 1
#endif

/**
 * @brief User defined heartbeat period in seconds
 */
#if RLOG_HEARTBEAT
    #ifndef RLOG_HEARTBEAT_PERIOD_SEC
        #define RLOG_HEARTBEAT_PERIOD_SEC 3600 /* 3600 = 1h */
    #endif
#endif

#ifndef RLOG_MAX_NUM_IFC
    #define RLOG_MAX_NUM_IFC 2
#endif

#define MSG_QUEUE_SIZE RLOG_QUEUE_SIZE

#define EVENT_NEW_MSG           ( 1 << 0 )
#define EVENTS_MASK             ( EVENT_NEW_MSG )


/**
 * @brief Communication interface control block
 */
static struct 
{
    rlog_ifc_t ifc[RLOG_MAX_NUM_IFC];
    bool up[RLOG_MAX_NUM_IFC];

} coms;
static unsigned char n_ifc = 0; //empty

/**
 * @brief Ring buffer implementation
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
 * @brief Heartbeat timer counter
 */
#if RLOG_HEARTBEAT
static int  heartbeat_timer = 0;
#endif

/**
 * @brief server thread handle
 */
static os_thread_t* thread_handle;

/**
 * @brief mutual exclusion semaphore
 */
static os_mutex_t*  queue_lock = NULL;

/**
 * @brief Set of events to wake the main thread
 */
static os_event_t* wakeup_events;

/**
 * @brief Signals the thread must terminate.
 */
static bool terminate = false;

/**
 * @brief Indicate the server has already been intialized
 */
static bool ready = false;

/**
 * @brief Device hostname, can be an IP or some other designator 
 */
static char hostname[20] = "-";

/**
 * @brief Log format
 */
static RLOG_FORMAT log_format = RLOG_RFC3164;

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
static void server_thread(void* arg);

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
    bool full = false;
    os_mutex_lock(queue_lock);

    if( msg_queue.cnt < MSG_QUEUE_SIZE ) {
        msg_queue.cnt++;
    } else {
        msg_queue.ovf++;
        full = true;
    }

    msg_queue.buffer[msg_queue.tail].timestamp = log.timestamp;
    msg_queue.buffer[msg_queue.tail].pri = log.pri;
    msg_queue.buffer[msg_queue.tail].proc = log.proc;
    fast_strncpy(msg_queue.buffer[msg_queue.tail].msg, msg, sizeof(msg_queue.buffer[msg_queue.tail].msg));

    if( (msg_queue.tail == msg_queue.head) && full )
        msg_queue.head = (msg_queue.head + 1) % MSG_QUEUE_SIZE;

    msg_queue.tail = (msg_queue.tail + 1) % MSG_QUEUE_SIZE;
    
    os_mutex_unlock(queue_lock);
}

static
void queue_putf(log_t log, const char* format,  va_list args)
{
    bool full = false;

    os_mutex_lock(queue_lock);

    if( msg_queue.cnt < MSG_QUEUE_SIZE ) {
        msg_queue.cnt++;
    } else {
        msg_queue.ovf++;
        full = true;
    }
    msg_queue.buffer[msg_queue.tail].timestamp = log.timestamp;
    msg_queue.buffer[msg_queue.tail].pri = log.pri;
    msg_queue.buffer[msg_queue.tail].proc = log.proc;
    vsnprintf(msg_queue.buffer[msg_queue.tail].msg, sizeof(msg_queue.buffer[msg_queue.tail].msg), format, args);

    if( (msg_queue.tail == msg_queue.head) && full )
         msg_queue.head = (msg_queue.head + 1) % MSG_QUEUE_SIZE;

    msg_queue.tail = (msg_queue.tail + 1) % MSG_QUEUE_SIZE;
    
    os_mutex_unlock(queue_lock);
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
    log.pri = msg_queue.buffer[msg_queue.head].pri;
    log.proc = msg_queue.buffer[msg_queue.tail].proc;

    fast_strncpy(log.msg, msg_queue.buffer[msg_queue.head].msg, sizeof(msg_queue.buffer[msg_queue.head].msg));

    msg_queue.head = (msg_queue.head + 1) % MSG_QUEUE_SIZE;
    msg_queue.cnt--;
    os_mutex_unlock(queue_lock);

    make_log_string(log_format, hostname, msg, &log);
    return strlen(msg);
}

bool rlog_init(const char* filepath, unsigned int size)
{
    if( ready ) 
    {
        DBG_PRINTF("[RLOG] rlog server already running \n");
        return false;
    }

    queue_init();

    queue_lock = os_mutex_create();
    if( queue_lock == NULL ) {
        DBG_PRINTF("[RLOG] rlog_init failed to create mutex\n");
        goto INIT_FAIL;
    }

    wakeup_events = os_event_create();        

#if RLOG_DLOG_ENABLE
    int err = dlog_open(&logger, filepath, size);
    if( err != DLOG_OK ) {
        DBG_PRINTF("[RLOG] rlog_init failed to open dlog. DLOG error %d\n", err);
        goto INIT_FAIL;            
    }
#endif

    thread_handle = os_thread_create("rlog", server_thread, NULL, RLOG_STACK_SIZE, RLOG_TASK_PRIO);
    if( thread_handle == NULL ) {
        DBG_PRINTF("[RLOG] rlog_init failed to create thread\n");
        goto INIT_FAIL;
    }

    ready = true;
    os_sleep_us(1000);
    return true;

INIT_FAIL:
    return false;    
}

void rlog_kill(void)
{
    terminate = true;
}

void rlog(RLOG_TYPE type, const char* msg)
{
    log_t log;
#if RLOG_TIMESTAMP_ENABLE
    time(&log.timestamp);
#endif
    log.pri = 8 + type;
    log.proc = os_thread_get_name(NULL);
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
    log.pri = 8 + type;
    log.proc = os_thread_get_name(NULL);

    va_start(args, format);
    queue_putf(log, format, args);
    va_end(args);
    os_event_set(wakeup_events, EVENT_NEW_MSG);
}

bool rlog_set_format(RLOG_FORMAT fmt)
{
    if( ready ) 
    {
        DBG_PRINTF("[RLOG] rlog server already running \n");
        return false;
    }

    if ( fmt >= RLOG_NO_FORMAT )
    {   
        DBG_PRINTF("[RLOG] Invalid format! \n"); 
        return false;
    }

    log_format = fmt;
    return true;
}


bool rlog_set_hostname(const char* name)
{
    if( ready ) 
    {
        DBG_PRINTF("[RLOG] rlog server already running \n");
        return false;
    }

    if( !name )
        return false;
        
    snprintf(hostname, sizeof(hostname), name);    
    return true;
}

bool rlog_install_interface( rlog_ifc_t interface )
{
    if( ready ) 
    {
        DBG_PRINTF("[RLOG] rlog server already running \n");
        return false;
    }

    if(interface.poll == NULL) {
        DBG_PRINTF("[RLOG] rlog_install_interface failed. Invalid parameter\n");
        return false;
    }

    if(interface.init == NULL) {
        DBG_PRINTF("[RLOG] rlog_install_interface failed. Invalid parameter\n");
        return false;
    }

    if(interface.send == NULL) {
        DBG_PRINTF("[RLOG] rlog_install_interface failed. Invalid parameter\n");
        return false;
    }

    if( n_ifc >= RLOG_MAX_NUM_IFC ) {
        DBG_PRINTF("[RLOG] rlog_install_interface failed. Too many interfaces!\n");
        return false;
    }

    // try to initialize the interface
    if( !interface.init(interface.ctx) )
    {
        DBG_PRINTF("[RLOG] rlog_install_interface failed to initialize interface\n");
        return false;
    }

    coms.ifc[n_ifc++] = interface;

    return true;
}

/**
 * @brief Poll all installed interfaces to see which is avaialble.
 * 
 * @return true If at least on interface is ready to receive messages
 * @return false If no interface can receive messages at the moment.
 */
bool rlog_poll()
{
    rlog_ifc_t p;
    bool up = false;

    for( int i=0; i < n_ifc; i++ )
    {
        p = coms.ifc[i];         
        coms.up[i] = p.poll(p.ctx);

        if( !up ) {
            up = coms.up[i];
        }
    }

    return up;   
}

/**
 * @brief Send message to all interfaces that are ready to receive
 * 
 * @param buf Buffer with message to be sent
 * @param len Size of the message in bytes
 * @return true If at least one interface has received the message
 * @return false If no interface has received the message
 */
bool rlog_send(const void* buf, int len)
{
    rlog_ifc_t p;
    unsigned char ok = 0;

    for( int i=0; i < n_ifc; i++ )
    {
        if( coms.up[i] )
        {
            p = coms.ifc[i]; 

            if ( p.send(p.ctx, buf, len) )
                ok++;            
        }         
    }

    return ( ok > 0 );
}

/**
 * @brief Deinitialize all installed interfaces 
 */
void rlog_deinit()
{
    rlog_ifc_t p;

    for( int i=0; i < n_ifc; i++ )
    {
        p = coms.ifc[i]; 

        if( p.deinit ) {
            p.deinit(p.ctx);
        }

    }
}

#if _RLOG_DBG_   
static
void rlog_print_dbg_stats()
{
 
    // frequency divider so not to pollute stdout
    static uint32_t dbg_ctr = 0;

    uint32_t    queue_ovf;
    uint32_t    queue_cnt;
    uint32_t    queue_max_cnt;

    if( dbg_ctr++ < 50)
        return;

    dbg_ctr = 0;
    os_mutex_lock(queue_lock);
    queue_ovf = msg_queue.ovf;
    queue_cnt = msg_queue.cnt;
    queue_max_cnt = msg_queue.max_cnt;    
    os_mutex_unlock(queue_lock);

    DBG_PRINTF("[RLOG] Queue overflows: %d\n", queue_ovf);
    DBG_PRINTF("[RLOG] Queue count: %d\n", queue_cnt);
    DBG_PRINTF("[RLOG] Queue watermark: %d\n", queue_max_cnt);

}
#endif

#define SERVER_BANNER "RLOG Server v"RLOG_VERSION " up and running!"
#define EVENT_TIMEOUT_SEC 1
#define EVENT_TIMEOUT (EVENT_TIMEOUT_SEC * 1000)
#define QUEUE_POLLING_PERIOD_US 0

#if RLOG_HEARTBEAT
    #define HEARTBEAT_PERIOD_TICKS (RLOG_HEARTBEAT_PERIOD_SEC / EVENT_TIMEOUT_SEC)
#endif

static
void dump_dlog_to_remote()
{
#if RLOG_DLOG_ENABLE
    //dispatch all enqueued log messages
    while( dlog_peek(&logger, msg_buffer, sizeof(msg_buffer)) )
    {        
        if( rlog_send(msg_buffer, strlen(msg_buffer)) ) 
        { 
            dlog_next(&logger); 
        }
        else
        {
            break;
        }      
        os_sleep_us(QUEUE_POLLING_PERIOD_US);
    }
#endif
}

static 
void dump_queue_to_remote()
{
    //dispatch all enqueued log messages
    int len = queue_get(msg_buffer);
    while( len )
    {        
        if( !rlog_send(msg_buffer, len) )
        {
            // failed to send, put it on dlog for later
            dlog_put(&logger, msg_buffer);
            break;
        }           
        os_sleep_us(QUEUE_POLLING_PERIOD_US);
        len = queue_get(msg_buffer);
    }
}

static 
void dump_queue_to_dlog()
{
#if RLOG_DLOG_ENABLE                                     
    while( queue_get(msg_buffer) )
    {
        dlog_put(&logger, msg_buffer);
        os_sleep_us(QUEUE_POLLING_PERIOD_US);
    }
#endif                   
}

static
void send_heartbeat(void)
{
#if RLOG_HEARTBEAT
    heartbeat_timer++;
    if( heartbeat_timer > HEARTBEAT_PERIOD_TICKS ) 
    {
        rlog(RLOG_INFO,"Heartbeat.. rlog server is still running!");
        heartbeat_timer = 0;
    }
#endif                
}

static
void server_thread(void* arg)
{                
    uint32_t evts = 0; 

    rlog(RLOG_INFO, SERVER_BANNER);
    while( !terminate )
    {         
        evts = os_event_wait(wakeup_events, EVENTS_MASK, EVENT_TIMEOUT);
        os_event_clear(wakeup_events, evts);

        if( rlog_poll() )
        {
            // check backlog
            dump_dlog_to_remote();

            if( !evts )
                send_heartbeat();

            if( evts & EVENT_NEW_MSG )
                dump_queue_to_remote();
        }
        else
        {
            dump_queue_to_dlog();
        }

#if _RLOG_DBG_
        rlog_print_dbg_stats();
#endif
    }

    rlog_deinit(); 
}