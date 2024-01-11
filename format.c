#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "rlog.h"
#include "format.h"

int make_log_string(char* hostname, char* str, log_t* log)
{
    int nchar = 0;
    char date[80] = { 0 };

#if RLOG_TIMESTAMP_ENABLE    
    struct tm * timeinfo;
    timeinfo = localtime ( &log->timestamp );

    strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%S", timeinfo);
#endif

    nchar = snprintf(str, MSG_MAX_SIZE_CHAR,"<%d>1 %s %s - - - %s", log->pri, date, hostname, log->msg);
   
    if( nchar < 0 || nchar > MSG_MAX_SIZE_CHAR )
        return -1;

    return nchar;
}
