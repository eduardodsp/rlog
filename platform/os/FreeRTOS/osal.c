/**
 * @file osal.c
 * @author edsp
 * @brief OSAL implementation for FreeRTOS
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

#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "../osal.h"
#include "../../target/rtc.h"

#define TIME_TO_TICKS(ms) \
   ((ms == OSAL_WAIT_FOREVER) ? portMAX_DELAY : (ms) / portTICK_PERIOD_MS)

typedef struct aux_timer
{
   TimerHandle_t handle;
   void(*fn) (osal_timer_t *, void * arg);
   void * arg;
} aux_timer_t;

osal_thread_t* osal_create_thread(
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
    if (xTaskCreate ((TaskFunction_t)task, NULL, stack, arg, prio, &hTask) != 1)
    	return NULL;
#endif

    return (osal_thread_t*)hTask;
}

int osal_destroy_thread(osal_thread_t* id)
{
    return 0;
}

osal_mutex_t * osal_create_mutex()
{
	SemaphoreHandle_t xMutex = NULL;
	xMutex = xSemaphoreCreateRecursiveMutex();
    return (osal_mutex_t*)xMutex;
}

int osal_destroy_mutex(osal_mutex_t* lock)
{
    if(lock == NULL)
        return -1;

    vSemaphoreDelete ((SemaphoreHandle_t)lock);

    return 0;
}

int osal_lock_mutex(osal_mutex_t* lock)
{
    if (xSemaphoreTakeRecursive ((SemaphoreHandle_t)lock, portMAX_DELAY) != 1)
        return -1;

    return 0;
}

int osal_unlock_mutex(osal_mutex_t* lock)
{
     if (xSemaphoreGiveRecursive ((SemaphoreHandle_t)lock) != 1) {
    	 return -1;
     }
    return 0;
}

/**
 * @brief Create waitable event
 * @return Pointer to event handle
 */
osal_event_t* osal_create_event()
{
    EventGroupHandle_t handle = xEventGroupCreate();
    return (osal_event_t *)handle;
}

bool osal_event_wait (osal_event_t * event, uint32_t mask, uint32_t * value, uint32_t time)
{
   *value = xEventGroupWaitBits (
      (EventGroupHandle_t)event,
      mask,
      pdFALSE,
      pdFALSE,
      TIME_TO_TICKS (time));

   *value &= mask;
   return *value == 0;
}

void osal_event_set (osal_event_t * event, uint32_t value)
{
   xEventGroupSetBits ((EventGroupHandle_t)event, value);
}

void osal_event_clr (osal_event_t * event, uint32_t value)
{
   xEventGroupClearBits ((EventGroupHandle_t)event, value);
}

void osal_event_destroy (osal_event_t * event)
{
   vEventGroupDelete ((EventGroupHandle_t)event);
}

static void timer_callback (TimerHandle_t xTimer)
{
   aux_timer_t * timer = (aux_timer_t *)pvTimerGetTimerID (xTimer);

   if (timer->fn)
      timer->fn (timer, timer->arg);
}

osal_timer_t* osal_timer_create(
                                uint32_t us,
                                void (*fn) (osal_timer_t * timer, void * arg),
                                void * arg,
                                bool oneshot
                                )
{
    aux_timer_t * timer;

    timer = malloc (sizeof (aux_timer_t));
    if(timer == NULL)
        return NULL;

    timer->fn  = fn;
    timer->arg = arg;
    
    timer->handle = xTimerCreate("osal_timer",
                                (us / portTICK_PERIOD_MS) / 1000,
                                oneshot ? pdFALSE : pdTRUE,
                                timer,
                                timer_callback);

    if(timer->handle == NULL)
        return NULL;

    return (osal_timer_t *)timer;
}

void osal_timer_start (osal_timer_t * timer)
{
    aux_timer_t* tmp = (aux_timer_t*)timer;
    xTimerStart(tmp->handle, 0);
}

void osal_timer_stop (osal_timer_t * timer)
{
    aux_timer_t* tmp = (aux_timer_t*)timer;
    xTimerStop (tmp->handle, portMAX_DELAY);
}

void osal_timer_destroy (osal_timer_t * timer)
{
    aux_timer_t* tmp = (aux_timer_t*)timer;

    xTimerDelete (tmp->handle, portMAX_DELAY);
    free (timer);
}

void osal_get_date(char* buffer)
{
    rtc_get_date(buffer);
}

void osal_sleep(uint32_t t)
{
	TickType_t xDelay = t / (portTICK_PERIOD_MS * 1000);
	vTaskDelay(xDelay);
}