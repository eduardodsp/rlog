/*
 * rlog_rtc.c
 *
 *  Created on: Nov 28, 2021
 *      Author: edsp
 */

#include <esp_system.h>
#include <time.h>
#include <sys/time.h>

#include "../rlog_rtc.h"


void rtc_get_date(char* sdate)
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    sprintf(sdate, "%02d-%02d-%d %02d:%02d:%02d",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}
