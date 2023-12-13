#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "format.h"

int make_log_string(char* str, log_t* log)
{
    int nchar = 0;
    char date[80] = { 0 };

#if RLOG_TIMESTAMP_ENABLE    
    struct tm * timeinfo;
    timeinfo = localtime ( &log->timestamp );
    strftime(date, sizeof(date), "%d-%m-%Y %H:%M:%S", timeinfo);
#endif

    switch(log->type)
    {
        case RLOG_INFO:
                nchar = snprintf(str, MSG_MAX_SIZE_CHAR,"%s [i]: %s \n", date, log->msg);
            break;

        case RLOG_WARNING:
                nchar = snprintf(str, MSG_MAX_SIZE_CHAR,"%s [!]: %s \n", date, log->msg);
            break;

        case RLOG_ERROR:
                nchar = snprintf(str, MSG_MAX_SIZE_CHAR,"%s [#]: %s \n", date, log->msg);
            break;

        default:
            return -3;
            break;
    }
   
    if( nchar < 0 || nchar > MSG_MAX_SIZE_CHAR )
        return -4;

    return nchar;
}
