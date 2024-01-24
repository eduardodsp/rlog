#ifndef _RLOG_FORMAT_H_
#define _RLOG_FORMAT_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>
#include "../port/os/osal.h"

/**
 * @brief User defined maximum size of log messages.
 */
#ifndef RLOG_MAX_SIZE_CHAR
    #define RLOG_MAX_SIZE_CHAR  80
#endif

#define MSG_MAX_SIZE_CHAR (RLOG_MAX_SIZE_CHAR + 80)

typedef enum {

    RLOG_RFC3164 = 0,
    RLOG_RFC5424 = 1,
    RLOG_NO_FORMAT,

}RLOG_FORMAT;

typedef enum
{
    RLOG_EMERGENCY  = 0,    // Emergency: system is unusable
    RLOG_ALERT      = 1,    // Alert: action must be taken immediately
    RLOG_CRIT       = 2,    // Critical: critical conditions
    RLOG_ERROR      = 3,    // Error: error conditions
    RLOG_WARNING    = 4,    // Warning: warning conditions
    RLOG_NOTICE     = 5,    // Notice: normal but significant condition
    RLOG_INFO       = 6,    // Informational: informational messages
    RLOG_DEBUG      = 7,    // Debug: debug-level message

}RLOG_TYPE;

typedef struct log_t
{
    time_t timestamp;
    uint8_t pri;
    char proc[16];  
    char msg[RLOG_MAX_SIZE_CHAR];
}log_t;

int make_log_string(int format, char* hostname, char* str, log_t* log);


#endif //_RLOG_FORMAT_H_