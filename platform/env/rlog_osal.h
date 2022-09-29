/**
 * @file rlog_osal.h
 * @author edsp
 * @brief Operating system abstraction layer
 * @version 0.1
 * @date 2021-11-06
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#ifndef _RLOG_OSAL_H_
#define _RLOG_OSAL_H_

#include <stdint.h>


typedef void osal_thread_t;
typedef void osal_mutex_t;

/**
 * @brief Create a thread.
 * 
 * @param id Pointer to thread handle
 * @param task Function to execute
 * @param arg Argument to pass to task
 * @return 0 on success, negative number on failure
 */
int osal_create_thread(osal_thread_t** id, void (*task)(void*), void* arg);

/**
 * @brief Destroy a thread
 * 
 * @param id Pointer to thread handle
 * @return 0 on success, negative number on failure
 */
int osal_destroy_thread(osal_thread_t* id);

/**
 * @brief Create a mutual exclusion semaphore (mutex)
 * 
 * @param lock Pointer to mutex handle
 * @return 0 on success, negative number on failure
 */
int osal_create_mutex(osal_mutex_t** lock);

/**
 * @brief Destroy mutex
 * 
 * @param lock Pointer to mutex handle
 * @return int 0 if sucessful, negative if fail
 */
int osal_destroy_mutex(osal_mutex_t* lock);

/**
 * @brief Suspend calling thread until mutex is acquired.
 * 
 * @param lock Pointer to mutex handle
 * @return 0 on success, negative number on failure
 */
int osal_lock_mutex(osal_mutex_t* lock);

/**
 * @brief Release a mutex.
 * 
 * @param lock Pointer to mutex handle
 * @return 0 on success, negative number on failure
 */
int osal_unlock_mutex(osal_mutex_t* lock);

/**
 * @brief Returns the date time on a C string with format 
 *        DD-MM-YYYY HH:MM:SS
 * 
 * @param[out] buffer Output string
 */
void osal_get_date(char* buffer);

/**
 * @brief Suspend execution for microsecond intervals
 * 
 * @param t time in microseconds
 */
void osal_sleep(uint32_t t);

#endif //_RLOG_OSAL_H_