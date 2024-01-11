/**
 * @file osal.c
 * @author edsp
 * @brief OSAL implementation for FreeRTOS
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

#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include <time.h>

#include "../osal.h"

#define TIME_TO_TICKS(ms) \
   ((ms == OS_WAIT_FOREVER) ? portMAX_DELAY : (ms) / portTICK_PERIOD_MS)

typedef struct aux_timer
{
   TimerHandle_t handle;
   void(*fn) (os_timer_t *, void * arg);
   void * arg;
} aux_timer_t;

os_thread_t* os_thread_create(
                                const char * name,
                                void (*task)(void*), 
                                void* arg, 
                                uint32_t stack, 
                                uint32_t prio
                                )
{
    TaskHandle_t hTask = NULL;

#if 0
   /* stacksize in freertos is not in bytes but in stack depth, it should be
    * divided by the stack width */
    configSTACK_DEPTH_TYPE stacksize = stack / sizeof (configSTACK_DEPTH_TYPE);

    if (xTaskCreate ((TaskFunction_t)task, NULL, stacksize, arg, prio, &hTask) != 1)
    	return NULL;
#else
    if (xTaskCreate((TaskFunction_t)task, NULL, stack, arg, prio, &hTask) != 1)
    	return NULL;
#endif

    return (os_thread_t*)hTask;
}

void os_thread_destroy(os_thread_t* id)
{
    vTaskDelete( id );
}

os_mutex_t * os_mutex_create()
{
	SemaphoreHandle_t xMutex = NULL;
	xMutex = xSemaphoreCreateRecursiveMutex();
    OS_ASSERT(xMutex != NULL);
    return (os_mutex_t*)xMutex;
}

void os_mutex_destroy(os_mutex_t* lock)
{
    OS_ASSERT(lock != NULL);
    vSemaphoreDelete((SemaphoreHandle_t)lock);
}

bool os_mutex_lock(os_mutex_t* lock)
{
    if (xSemaphoreTakeRecursive((SemaphoreHandle_t)lock, portMAX_DELAY) != 1)
        return false;

    return true;
}

bool os_mutex_unlock(os_mutex_t* lock)
{
     if (xSemaphoreGiveRecursive((SemaphoreHandle_t)lock) != 1) {
    	 return false;
     }
    return true;
}

/**
 * @brief Create waitable event
 * @return Pointer to event handle
 */
os_event_t* os_event_create()
{
    EventGroupHandle_t handle = xEventGroupCreate();
    OS_ASSERT(handle != NULL);
    return (os_event_t *)handle;
}

uint32_t os_event_wait(os_event_t * event, uint32_t mask, uint32_t time)
{
   return xEventGroupWaitBits((EventGroupHandle_t)event, mask, pdFALSE, pdFALSE, TIME_TO_TICKS (time));
}

void os_event_set(os_event_t * event, uint32_t value)
{
   xEventGroupSetBits ((EventGroupHandle_t)event, value);
}

void os_event_clear(os_event_t * event, uint32_t value)
{
   xEventGroupClearBits((EventGroupHandle_t)event, value);
}

void os_event_destroy(os_event_t * event)
{
   vEventGroupDelete((EventGroupHandle_t)event);
}

static void timer_callback(TimerHandle_t xTimer)
{
   aux_timer_t * timer = (aux_timer_t *)pvTimerGetTimerID (xTimer);

   if (timer->fn)
      timer->fn (timer, timer->arg);
}

os_timer_t* os_timer_create(
                                uint32_t us,
                                void (*fn) (os_timer_t * timer, void * arg),
                                void * arg,
                                bool oneshot
                                )
{
    aux_timer_t * timer;

    timer = malloc (sizeof (aux_timer_t));
    OS_ASSERT(timer != NULL);

    timer->fn  = fn;
    timer->arg = arg;
    
    timer->handle = xTimerCreate("os_timer",
                                (us / portTICK_PERIOD_MS) / 1000,
                                oneshot ? pdFALSE : pdTRUE,
                                timer,
                                timer_callback);

    OS_ASSERT(timer != NULL);
    return (os_timer_t *)timer;
}

void os_timer_start(os_timer_t * timer)
{
    aux_timer_t* tmp = (aux_timer_t*)timer;
    xTimerStart(tmp->handle, 0);
}

void os_timer_stop(os_timer_t * timer)
{
    aux_timer_t* tmp = (aux_timer_t*)timer;
    xTimerStop(tmp->handle, portMAX_DELAY);
}

void os_timer_destroy(os_timer_t * timer)
{
    aux_timer_t* tmp = (aux_timer_t*)timer;

    xTimerDelete(tmp->handle, portMAX_DELAY);
    free (timer);
}

os_sem_t * os_sem_create (size_t max, size_t count)
{
   SemaphoreHandle_t handle = xSemaphoreCreateCounting (max, count);
   return (os_sem_t *)handle;
}

bool os_sem_wait(os_sem_t * sem, uint32_t time)
{
   if (xSemaphoreTake((SemaphoreHandle_t)sem, TIME_TO_TICKS(time)) == pdTRUE)
   {
      /* Did not timeout */
      return true;
   }

   /* Timed out */
   return false;
}

void os_sem_signal(os_sem_t * sem)
{
   xSemaphoreGive((SemaphoreHandle_t)sem);
}

void os_sem_destroy(os_sem_t * sem)
{
   vSemaphoreDelete((SemaphoreHandle_t)sem);
}

void os_get_date(char* buffer)
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    sprintf(buffer, "%02d-%02d-%d %02d:%02d:%02d",timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
}

void os_sleep_us(uint32_t t)
{
    if(t == 0)
    {
        taskYIELD();
    }
    else
    {
	    TickType_t xDelay = t / (portTICK_PERIOD_MS * 1000);
	    vTaskDelay(xDelay);
    }
}
