#ifndef _FORMAT_H_
#define _FORMAT_H_

#include "rlog.h"
#include <stdbool.h>
#include <sys/time.h>

#define MSG_MAX_SIZE_CHAR (RLOG_MAX_SIZE_CHAR + 40)

typedef struct log_format_t
{
    time_t timestamp;
    int type;
    char msg[RLOG_MAX_SIZE_CHAR];
}log_t;


int make_log_string(char* str, log_t* log);

#endif //_FORMAT_H_
