#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "rlog.h"
#include "format.h"

static char date[80] = { 0 };

int make_rfc3164_string(char* hostname, char* str, log_t* log)
{
    int nchar = 0;

#if RLOG_TIMESTAMP_ENABLE    
    struct tm * timeinfo;
    timeinfo = localtime ( &log->timestamp );
    strftime(date, sizeof(date), "%b %d %H:%M:%S", timeinfo);
#endif
    if( strlen(log->proc) ) {

        char * pcTmp  = log->proc;
        while (*pcTmp) {
            if (*pcTmp == ' ') *pcTmp = '_';
            ++pcTmp;
        }

        nchar = snprintf(str, MSG_MAX_SIZE_CHAR,"<%d>%s %s %s: %s\r\n", log->pri, date, hostname, log->proc, log->msg);
    } else {
        nchar = snprintf(str, MSG_MAX_SIZE_CHAR,"<%d>%s %s -: %s\r\n", log->pri, date, hostname, log->msg);
    }
   
    if( nchar < 0 || nchar > MSG_MAX_SIZE_CHAR )
        return -1;

    return nchar;
}

int make_rfc5424_log(char* hostname, char* str, log_t* log)
{
    int nchar = 0;

#if RLOG_TIMESTAMP_ENABLE    
    struct tm * timeinfo;
    timeinfo = localtime ( &log->timestamp );
    strftime(date, sizeof(date), "%Y-%m-%dT%H:%M:%S", timeinfo);
#endif
    if( strlen(log->proc ) ) {

        char * pcTmp  = log->proc;
        while (*pcTmp) {
            if (*pcTmp == ' ') *pcTmp = '_';
            ++pcTmp;
        }

        nchar = snprintf(str, MSG_MAX_SIZE_CHAR,"<%d>1 %s %s %s - - %s\r\n", log->pri, date, hostname, log->proc, log->msg);
    } else {
        nchar = snprintf(str, MSG_MAX_SIZE_CHAR,"<%d>1 %s %s - - - %s\r\n", log->pri, date, hostname, log->msg);
    }
   
    if( nchar < 0 || nchar > MSG_MAX_SIZE_CHAR )
        return -1;

    return nchar;
}

int make_log_string(int format, char* hostname, char* str, log_t* log)
{
    switch(format)
    {
        case RLOG_RFC3164:
        return make_rfc3164_string(hostname, str, log);
        break;
        case RLOG_RFC5424:
        return make_rfc5424_log(hostname, str, log);
        break;
    }

    return 0;
}
