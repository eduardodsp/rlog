/*
 * rlog_rtc.h
 *
 *  Created on: Nov 28, 2021
 *      Author: edsp
 */

#ifndef _RLOG_RTC_H_
#define _RLOG_RTC_H_

/**
 * @brief Returns the date time on a C string with format
 *        DD-MM-YYYY HH:MM:SS
 *
 * @param[out] sdate Output string
 */
void rtc_get_date(char* sdate);


#endif /* _RLOG_RTC_H_ */
