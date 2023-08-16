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
#include "mymqttclient.h"
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
TaskHandle_t cli_listener_handle;

#define PROCESSING_QUEUE_SIZE 30
QueueHandle_t processing_queue_handle;

typedef void (*cli_handler_t)(void *);

#define MAX_COMMAND_LENGTH 31
#define MAX_COMMAND_COUNT 10
int command_count = 0;

typedef struct
{
    char *cmd;
    cli_handler_t handler;
} cli_cmd_t;

cli_cmd_t commands[MAX_COMMAND_COUNT];

TimerHandle_t main_timer;
float main_timer_time;

void main_timer_callback(TimerHandle_t xTimer)
{
    main_timer_time += (0.001f);
}

/// @brief initialize main timer
void main_timer_init()
{
    if (NULL == (main_timer = xTimerCreate("timer", pdMS_TO_TICKS(1), pdTRUE, 0, main_timer_callback)))
    {
        LOG(ERR, "Couldn't create a timer.");
    }
}

/// @brief This task prints a file line by line.
/// @param pvParameters file path string.
void data_reader_task(void *pvParameters)
{
    //mqtt_init_client();
    const char *filename = (const char *)pvParameters;
    FILE *file;
    if (NULL == (file = fopen(filename, "r")))
    {
        LOG(ERR, "Could not open file: %s", filename);
        exit(EXIT_FAILURE);
    }

    LOG(DBG, "Creating a new processing queue.");
    if (NULL == (processing_queue_handle = xQueueCreate(PROCESSING_QUEUE_SIZE, sizeof(int))))
    {
        LOG(ERR, "Failed to create a processing queue.");
        exit(EXIT_FAILURE);
    }
    LOG(DBG, "Processing queue created.");

    LOG(INF, "Reading file: %s", filename);
    int num;
    while (1 == fscanf(file, "%d", &num))
    {
        LOG(INF, "Sending %d to the queue.", num);
        xQueueSend(processing_queue_handle, &num, pdMS_TO_TICKS(100000));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    LOG(INF, "Closing file: %s", filename);
    fclose(file);

    //mqtt_disconnect_client();

    vTaskSuspend(data_reader_handle);
}

/// @brief this task receives numbers from a que and checks if the sum of last 3 is > 200.
void processing_task(void *pvParameters)
{
    int buffer, sum = 0, count = 0, arr[3] = {0, 0, 0};
    while (1)
    {
        if (NULL != processing_queue_handle)
        {
            if (pdTRUE == xQueueReceive(processing_queue_handle, &buffer, pdMS_TO_TICKS(100000)))
            {
                LOG(INF, "Received %d from the queue.", buffer);
                arr[count] = buffer;
                for (int i = 0; i < 3; i++)
                {
                    sum += arr[i];
                }

                if (200 < sum)
                {
                    char msg[100];
                    sprintf(msg, "%d + %d + %d = %d > 200", arr[0], arr[1], arr[2], sum);
                    LOG(WRN, "%s", msg);
                    mqtt_publish_msg(msg);
                }
                sum = 0;

                count = ++count % 3;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/// @brief this task listens for commands
/// @param pvParameters
void cli_listener_task(void *pvParameters)
{
    char cmd[MAX_COMMAND_LENGTH];
    while (1)
    {
        if (1 == scanf(" %30s", cmd))
        {
            for (int i = 0; i < command_count; i++)
            {
                if (0 == strcmp(commands[i].cmd, cmd))
                {
                    LOG(DBG, "Executing command: %s", cmd);
                    commands[i].handler(NULL);
                    strcpy(cmd, "");
                    break;
                }
            }
            if (strcmp(cmd, ""))
                LOG(ERR, "%s is not a known command.", cmd);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/// @brief register a new command and link it with a handler
/// @param cmd command name string
/// @param handler handler of the command
void register_cmd(char *cmd, cli_handler_t handler)
{
    cli_cmd_t command = {cmd, handler};
    // Register the command to be listened for.
    if (command_count < MAX_COMMAND_COUNT)
    {
        commands[command_count++] = command;
    }
    else
    {
        LOG(ERR, "Can't register this command - too many commands - expand the command array");
    }
}

void test_handler(void *arg)
{
    LOG(INF, "Test scuccessfull");
}

void exit_handler(void *arg)
{
    LOG(WRN, "Exiting program");

    exit(EXIT_SUCCESS);
}

void timer_start(void *arg)
{
    LOG(DBG, "Starting the main timer.");
    xTimerStart(main_timer, 0);
}

void timer_stop(void *arg)
{
    xTimerStop(main_timer, 0);
    LOG(DBG, "Stopping the main timer at: %f", main_timer_time);
}

void stop_processing(void *arg)
{
    LOG(WRN, "Suspending processing task");
    vTaskSuspend(processing_handle);
}

void resume_processing(void *arg)
{
    LOG(WRN, "Resuming processing task");
    vTaskResume(processing_handle);
}

int main(int argc, char *argv[])
{
    LOG_init();
    main_timer_init();

    register_cmd("test", test_handler);
    register_cmd("exit", exit_handler);
    register_cmd("timer_start", timer_start);
    register_cmd("timer_stop", timer_stop);
    register_cmd("stop_processing", stop_processing);
    register_cmd("restart_processing", resume_processing);

    xTaskCreate(cli_listener_task, "cli_listener_task", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, &cli_listener_handle);
    char *filepath = "/gl/freertos-intro/data.csv";
    if (argc == 2)
        filepath = argv[1];
    xTaskCreate(data_reader_task, "data_reader_task", configMINIMAL_STACK_SIZE, (void *)filepath, mainCHECK_TASK_PRIORITY, &data_reader_handle);
    xTaskCreate(processing_task, "processing_task", configMINIMAL_STACK_SIZE, NULL, mainCHECK_TASK_PRIORITY, &processing_handle);

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
