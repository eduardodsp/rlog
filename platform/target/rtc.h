/**
 * @file rtc.h
 * @author edsp
 * @brief Real Time Clock abstraction layer
 * @version 1.0.0
 * @date 2022-09-29
 * 
 * @copyright Copyright (c) 2022
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
