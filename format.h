#ifndef _FORMAT_H_
#define _FORMAT_H_

#include <stdint.h>
#include <stdbool.h>
#include <sys/time.h>

/**
 * @brief User defined maximum size of log messages.
 */
#ifndef RLOG_MAX_SIZE_CHAR
    #define RLOG_MAX_SIZE_CHAR  80
#endif

#define MSG_MAX_SIZE_CHAR (RLOG_MAX_SIZE_CHAR + 80)

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

typedef struct log_format_t
{
    time_t timestamp;
    uint8_t pri;    
    char msg[RLOG_MAX_SIZE_CHAR];
}log_t;


int make_log_string(char* hostname, char* str, log_t* log);

#endif //_FORMAT_H_
