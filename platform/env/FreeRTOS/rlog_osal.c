/**
 * @file rlog_osal.c
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

#include "../rlog_osal.h"
#include "../../target/rlog_rtc.h"

#define USE_STATIC_ALLOCATION 0

/* Dimensions of the buffer that the task being created will use as its stack.
NOTE:  This is the number of words the stack will hold, not the number of
bytes.  For example, if each stack item is 32-bits, and this is set to 100,
then 400 bytes (100 * 32-bits) will be allocated. */
#define RLOG_TASK_STACK_SIZE 2048
#define RLOG_TASK_PRIO 8

#if USE_STATIC_ALLOCATION
/* Buffer that the task being created will use as its stack.  Note this is
an array of StackType_t variables.  The size of StackType_t is dependent on
the RTOS port. */
StackType_t xStack[ RLOG_TASK_STACK_SIZE ];
TaskHandle_t hTask = NULL;

/* Structure that will hold the TCB of the task being created. */
StaticTask_t xTaskBuffer;
SemaphoreHandle_t xSemaphore = NULL;
StaticSemaphore_t xMutexBuffer;
#endif

int osal_create_thread(osal_thread_t** id, void (*task)(void*), void* arg)
{
	uint32_t stack = RLOG_TASK_STACK_SIZE * sizeof(StackType_t);
	char name[] = "rlogTask";

#if USE_STATIC_ALLOCATION
    hTask = xTaskCreateStatic ((TaskFunction_t)task, name, stack, arg, RLOG_TASK_PRIO, xStack, &xTaskBuffer);
    if(hTask == NULL)
    	return -1;

    *id = (osal_thread_t*)hTask;
#else

    TaskHandle_t hTask = NULL;
    if (xTaskCreate ((TaskFunction_t)task, name, (uint16_t)stack, arg, RLOG_TASK_PRIO, &hTask) != 1)
    	return -1;

    *id = (osal_thread_t*)hTask;
#endif
    return 0;
}

int osal_destroy_thread(osal_thread_t* id)
{
    return 0;
}

int osal_create_mutex(osal_mutex_t **lock)
{
#if USE_STATIC_ALLOCATION
    //to-do
#else
	SemaphoreHandle_t xMutex = NULL;

	xMutex = xSemaphoreCreateRecursiveMutex();
	if(xMutex == NULL)
		return -1;

	*lock = (osal_mutex_t*)xMutex;
#endif
    return 0;
}

int osal_destroy_mutex(osal_mutex_t* lock)
{
    if(lock == NULL)
        return -1;
#if USE_STATIC_ALLOCATION
    //to-do
#else

    vSemaphoreDelete ((SemaphoreHandle_t)lock);

#endif
    return 0;
}

int osal_lock_mutex(osal_mutex_t* lock)
{
#if USE_STATIC_ALLOCATION
    //to-do
#else

	 if (xSemaphoreTakeRecursive ((SemaphoreHandle_t)lock, portMAX_DELAY) != 1) {
		 return -1;
	 }

#endif
    return 0;
}
int osal_unlock_mutex(osal_mutex_t* lock)
{
#if USE_STATIC_ALLOCATION
    //to-do
#else

     if (xSemaphoreGiveRecursive ((SemaphoreHandle_t)lock) != 1) {
    	 return -1;
     }

#endif
    return 0;
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
