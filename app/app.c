/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>>> AND MODIFIED BY <<<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * main() creates all the demo application tasks, then starts the scheduler.
 * The web documentation provides more details of the standard demo application
 * tasks, which provide no particular functionality but do provide a good
 * example of how to use the FreeRTOS API.
 *
 * In addition to the standard demo tasks, the following tasks and tests are
 * defined and/or created within this file:
 *
 * "Check" task - This only executes every five seconds but has a high priority
 * to ensure it gets processor time.  Its main function is to check that all the
 * standard demo tasks are still operational.  While no errors have been
 * discovered the check task will print out "OK" and the current simulated tick
 * time.  If an error is discovered in the execution of a task then the check
 * task will print out an appropriate error message.
 *
 */

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include "log.h"
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* Priorities at which the tasks are created. */
#define mainCHECK_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define mainQUEUE_POLL_PRIORITY (tskIDLE_PRIORITY + 1)
#define mainSEM_TEST_PRIORITY (tskIDLE_PRIORITY + 1)
#define mainBLOCK_Q_PRIORITY (tskIDLE_PRIORITY + 2)
#define mainCREATOR_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define mainINTEGER_TASK_PRIORITY (tskIDLE_PRIORITY)
#define mainGEN_QUEUE_TASK_PRIORITY (tskIDLE_PRIORITY)
#define mainFLOP_TASK_PRIORITY (tskIDLE_PRIORITY)
#define mainQUEUE_OVERWRITE_PRIORITY (tskIDLE_PRIORITY)

#define mainTIMER_TEST_PERIOD (50)

/*
 * Prototypes for the standard FreeRTOS callback/hook functions implemented
 * within this file.
 */
void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationTickHook(void);

/* The variable into which error messages are latched. */
/*static char *pcStatusMessage = "OK";*/

/*-----------------------------------------------------------*/
TaskHandle_t data_reader_handle;
TaskHandle_t processing_handle;
TaskHandle_t cli_handle;

QueueHandle_t queue_handle;

/// @brief This task prints a file line by line.
/// @param pvParameters file path string.
void data_reader_task(void *pvParameters)
{
    const char *filename = (const char *)pvParameters;
    FILE *file;
    if (NULL == (file = fopen(filename, "r")))
    {
        LOG(ERR, "Could not open file: %s", filename);
        exit(EXIT_FAILURE);
    }

    LOG(DBG, "Creating a new queue.");
    if (NULL == (queue_handle = xQueueCreate(10, sizeof(int))))
    {
        LOG(ERR, "Failed to create a queue.");
        exit(EXIT_FAILURE);
    }
    LOG(DBG, "Queue created.");

    LOG(INF, "Reading file: %s", filename);
    int num;
    while (1 == fscanf(file, "%d", &num))
    {
        LOG(INF, "Sending %d to the queue.", num);
        xQueueSend(queue_handle, &num, pdMS_TO_TICKS(1000));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    LOG(INF, "Closing file: %s", filename);
    fclose(file);
    vTaskSuspend(data_reader_handle);
}

void processing_task(void *pvParameters)
{
    int buffer, sum = 0, count = 0;
    while (1)
    {
        if (NULL != queue_handle)
        {
            if (pdTRUE == xQueueReceive(queue_handle, &buffer, pdMS_TO_TICKS(1000)))
            {
                LOG(INF, "Received %d from the queue.", buffer);
                sum += buffer;
                if (2 == count)
                {
                    if (200 < sum)
                        LOG(WRN, "Sum = %d is > 200.", sum);
                    sum = 0;
                }
                count = ++count % 3;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void cli_task(void *pvParameters)
{
}

int main(int argc, const char *argv[])
{
    LOG_init();
    if (argc == 2)
    {
        xTaskCreate(data_reader_task, "data_reader_task", configMINIMAL_STACK_SIZE, (void *)argv[1], mainCHECK_TASK_PRIORITY, &data_reader_handle);
        xTaskCreate(processing_task, "processing_task", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, &processing_handle);
    }
    else
    {
        char *filepath = "/gl/freertos-intro/data.csv";
        xTaskCreate(data_reader_task, "data_reader_task", configMINIMAL_STACK_SIZE, (void *)filepath, mainCHECK_TASK_PRIORITY, &data_reader_handle);
        xTaskCreate(processing_handle, "processing_task", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, &processing_handle);
        return -1;
    }
    vTaskStartScheduler();

    /* Should never get here unless there was not enough heap space to create
    the idle and other system tasks. */
    return 0;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook(void)
{
    /* vApplicationMallocFailedHook() will only be called if
    configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
    function that will get called if a call to pvPortMalloc() fails.
    pvPortMalloc() is called internally by the kernel whenever a task, queue,
    timer or semaphore is created.  It is also called by various parts of the
    demo application.  If heap_1.c or heap_2.c are used, then the size of the
    heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
    FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
    to query the size of free heap space that remains (although it does not
    provide information on how the remaining heap might be fragmented). */
    vAssertCalled(__LINE__, __FILE__);
}
/*-----------------------------------------------------------*/

void vApplicationIdleHook(void)
{
    /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
    to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
    task.  It is essential that code added to this hook function never attempts
    to block in any way (for example, call xQueueReceive() with a block time
    specified, or call vTaskDelay()).  If the application makes use of the
    vTaskDelete() API function (as this demo application does) then it is also
    important that vApplicationIdleHook() is permitted to return to its calling
    function, because it is the responsibility of the idle task to clean up
    memory allocated by the kernel to any task that has since been deleted. */

    /* Call the idle task processing used by the full demo.  The simple
    blinky demo does not use the idle task hook. */
    // vFullDemoIdleFunction();
}
/*-----------------------------------------------------------*/

void vApplicationTickHook(void)
{
    /* This function will be called by each tick interrupt if
    configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.  User code can be
    added here, but the tick hook is called from an interrupt context, so
    code must not attempt to block, and only the interrupt safe FreeRTOS API
    functions can be used (those that end in FromISR()). */
}
/*-----------------------------------------------------------*/

void vAssertCalled(unsigned long ulLine, const char *const pcFileName)
{
    taskENTER_CRITICAL();
    {
        printf("[ASSERT] %s:%lu\n", pcFileName, ulLine);
        fflush(stdout);
    }
    taskEXIT_CRITICAL();
    exit(-1);
}
/*-----------------------------------------------------------*/
