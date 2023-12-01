/**
 * @file osal.h
 * @author edsp
 * @brief Operating system abstraction layer
 * @version 1.0.0
 * @date 2021-11-06
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
#ifndef _RLOG_OSAL_H_
#define _RLOG_OSAL_H_

#include <stdbool.h>
#include <stdint.h>

#if 1
    #include <assert.h>
    #define OSAL_ASSERT(exp) assert (exp)
#else
    #define OSAL_ASSERT(exp)    
#endif

#define OSAL_WAIT_FOREVER 0xFFFFFFFF

typedef void osal_thread_t;
typedef void osal_mutex_t;
typedef void osal_event_t;
typedef void osal_timer_t;
typedef void osal_sem_t;

/**
 * @brief Create a thread.
 * 
 * @param name Thread name
 * @param task Function to execute
 * @param arg Argument to pass to task
 * @param stack Stack size in bytes
 * @param prio Thread priority
 * @return Pointer to thread handle
 */
osal_thread_t* osal_create_thread(
                                const char * name, 
                                void (*task)(void*), 
                                void* arg, 
                                uint32_t stack, 
                                uint32_t prio
                                );

/**
 * @brief Destroy a thread
 * 
 * @param id Pointer to thread handle
 */
void osal_destroy_thread(osal_thread_t* id);

/**
 * @brief Create waitable event
 * @return Pointer to event handle
 */
osal_event_t* osal_create_event(void);

/**
 * @brief Wait for one or more event bits to be set.
 * 
 * @param evt Event handle
 * @param mask Event bits to wait
 * @param time Timeout in miliseconds
 * @return Test value to know which event bits were set
 */
uint32_t osal_event_wait(osal_event_t * event, uint32_t mask, uint32_t time);

/**
 * @brief Set an event bit
 * @param evt Event handle
 * @param value Bits to set
 */
void osal_event_set(osal_event_t * event, uint32_t value);

/**
 * @brief Clear an event bit
 * @param evt Event handle
 * @param value Bits to clear
 */
void osal_event_clr(osal_event_t * event, uint32_t value);

/**
 * @brief Destroy event object
 * @param evt Event handle
 */
void osal_event_destroy(osal_event_t * event);

/**
 * @brief Create a timer
 * @param us Timer period in microseconds
 * @param fn Function to call once timer expires   
 * @param arg Function argument
 * @param oneshot Enable or disable oneshot mode
 * @return Pointer to timer handle
 */
osal_timer_t * osal_timer_create(
                                uint32_t us,
                                void (*fn) (osal_timer_t * timer, void * arg),
                                void * arg,
                                bool oneshot
                                );

/**
 * @brief Start a timer
 * @param timer Timer handle
 */
void osal_timer_start(osal_timer_t * timer);

/**
 * @brief Stop a timer
 * @param timer Timer handle
 */
void osal_timer_stop(osal_timer_t * timer);

/**
 * @brief Destroy a timer
 * @param timer Timer handle
 */
void osal_timer_destroy(osal_timer_t * timer);


/**
 * @brief Create a mutual exclusion semaphore (mutex)
 * @return Pointer to mutex handle
 */
osal_mutex_t* osal_create_mutex(void);

/**
 * @brief Destroy mutex
 * 
 * @param lock Pointer to mutex handle
 */
void osal_destroy_mutex(osal_mutex_t* lock);

/**
 * @brief Suspend calling thread until mutex is acquired.
 * 
 * @param lock Pointer to mutex handle
 * @return true if mutex was acquired
 * @return false in case of error
 */
bool osal_lock_mutex(osal_mutex_t* lock);

/**
 * @brief Release a mutex.
 * 
 * @param lock Pointer to mutex handle
 * @return true if mutex was released
 * @return false in case of error
 */
bool osal_unlock_mutex(osal_mutex_t* lock);

/**
 * @brief Create a counting semaphore.
 * 
 * @param max Sempahore max count
 * @param count Initial count
 * @return Semaphore handle
 */
osal_sem_t * osal_sem_create(size_t max, size_t count);

/**
 * @brief Wait for a semaphore until timeout
 * 
 * @param sem Semaphore handle
 * @param time Timeout in miliseconds
 * @return true if semaphore was acquired
 * @return false if timeout expired
 */
bool osal_sem_wait (osal_sem_t * sem, uint32_t time);

/**
 * @brief Signal a semaphore
 * 
 * @param sem Semaphore handle
 */
void osal_sem_signal (osal_sem_t * sem);

/**
 * @brief Destroy a semaphore
 * 
 * @param sem Semaphore handle
 */
void osal_sem_destroy (osal_sem_t * sem);

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