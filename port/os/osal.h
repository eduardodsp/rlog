/**
 * @file osal.h
 * @author edsp
 * @brief Operating system abstraction layer
 * @version 1.0.0
 * @date 2024-01-10
 * 
 * @copyright Copyright (c) 2024
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
#ifndef _OSAL_H_
#define _OSAL_H_

#include <stdbool.h>
#include <stdint.h>

#if 1
    #include <assert.h>
    #define OS_ASSERT(exp) assert (exp)
#else
    #define OS_ASSERT(exp)    
#endif

#define OS_WAIT_FOREVER 0xFFFFFFFF

typedef void os_thread_t;
typedef void os_mutex_t;
typedef void os_event_t;
typedef void os_timer_t;
typedef void os_sem_t;

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
os_thread_t* os_thread_create(
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
void os_thread_destroy(os_thread_t* id);

/**
 * @brief Create waitable event
 * @return Pointer to event handle
 */
os_event_t* os_event_create(void);

/**
 * @brief Wait for one or more event bits to be set.
 * 
 * @param evt Event handle
 * @param mask Event bits to wait
 * @param time Timeout in miliseconds
 * @return Test value to know which event bits were set
 */
uint32_t os_event_wait(os_event_t * event, uint32_t mask, uint32_t time);

/**
 * @brief Set an event bit
 * @param evt Event handle
 * @param value Bits to set
 */
void os_event_set(os_event_t * event, uint32_t value);

/**
 * @brief Clear an event bit
 * @param evt Event handle
 * @param value Bits to clear
 */
void os_event_clear(os_event_t * event, uint32_t value);

/**
 * @brief Destroy event object
 * @param evt Event handle
 */
void os_event_destroy(os_event_t * event);

/**
 * @brief Create a timer
 * @param us Timer period in microseconds
 * @param fn Function to call once timer expires   
 * @param arg Function argument
 * @param oneshot Enable or disable oneshot mode
 * @return Pointer to timer handle
 */
os_timer_t * os_timer_create(
                            uint32_t us,
                            void (*fn) (os_timer_t * timer, void * arg),
                            void * arg,
                            bool oneshot
                            );

/**
 * @brief Start a timer
 * @param timer Timer handle
 */
void os_timer_start(os_timer_t * timer);

/**
 * @brief Stop a timer
 * @param timer Timer handle
 */
void os_timer_stop(os_timer_t * timer);

/**
 * @brief Destroy a timer
 * @param timer Timer handle
 */
void os_timer_destroy(os_timer_t * timer);


/**
 * @brief Create a mutual exclusion semaphore (mutex)
 * @return Pointer to mutex handle
 */
os_mutex_t* os_mutex_create(void);

/**
 * @brief Destroy mutex
 * 
 * @param lock Pointer to mutex handle
 */
void os_mutex_destroy(os_mutex_t* lock);

/**
 * @brief Suspend calling thread until mutex is acquired.
 * 
 * @param lock Pointer to mutex handle
 * @return true if mutex was acquired
 * @return false in case of error
 */
bool os_mutex_lock(os_mutex_t* lock);

/**
 * @brief Release a mutex.
 * 
 * @param lock Pointer to mutex handle
 * @return true if mutex was released
 * @return false in case of error
 */
bool os_mutex_unlock(os_mutex_t* lock);

/**
 * @brief Create a counting semaphore.
 * 
 * @param max Sempahore max count
 * @param count Initial count
 * @return Semaphore handle
 */
os_sem_t * os_sem_create(size_t max, size_t count);

/**
 * @brief Wait for a semaphore until timeout
 * 
 * @param sem Semaphore handle
 * @param time Timeout in miliseconds
 * @return true if semaphore was acquired
 * @return false if timeout expired
 */
bool os_sem_wait (os_sem_t * sem, uint32_t time);

/**
 * @brief Signal a semaphore
 * 
 * @param sem Semaphore handle
 */
void os_sem_signal (os_sem_t * sem);

/**
 * @brief Destroy a semaphore
 * 
 * @param sem Semaphore handle
 */
void os_sem_destroy (os_sem_t * sem);

/**
 * @brief Returns the date time on a C string with format 
 *        DD-MM-YYYY HH:MM:SS
 * 
 * @param[out] buffer Output string
 */
void os_get_date(char* buffer);

/**
 * @brief Suspend execution for microsecond intervals
 * 
 * @param t time in microseconds
 */
void os_sleep_us(uint32_t t);

#endif //_OSAL_H_