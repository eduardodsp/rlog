#ifndef _FORMAT_H_
#define _FORMAT_H_

#include "rlog.h"
#include <stdbool.h>
#include <sys/time.h>

//#define RLOG_LOG_FORMAT_RFC5424 


#define MSG_MAX_SIZE_CHAR (RLOG_MAX_SIZE_CHAR + 40)


typedef struct log_format_t
{
    // timestamp at the moment the log is generated 
    time_t timestamp;

    // message type
    int type;

    char msg[RLOG_MAX_SIZE_CHAR];

#ifdef RLOG_LOG_FORMAT_RFC5424
    unsigned int prio;
#endif

}log_t;


int make_log_string(char* str, log_t* log);

#endif //_FORMAT_H_
