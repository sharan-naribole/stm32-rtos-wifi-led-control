/**
 ******************************************************************************
 * @file           : print_task.c
 * @brief          : Dedicated Print Task for Debug Logging (UART3)
 ******************************************************************************
 * @description
 * This module implements a dedicated print task that owns UART3 exclusively
 * for debug logging to serial terminal. UART2 is reserved for ESP8266.
 *
 * UART Allocation:
 * - UART2: ESP8266 communication (managed by user_input task)
 * - UART3: Debug logging and watchdog output (managed by print_task)
 *
 * Architecture Benefits:
 * - Eliminates priority inversion (queue is faster than mutex)
 * - Better separation of concerns (tasks don't need UART knowledge)
 * - Centralized debug output control
 * - Non-blocking for application tasks
 ******************************************************************************
 */

#include "print_task.h"
#include "watchdog.h"
#include <string.h>
#include <stdio.h>

/* FreeRTOS Objects */
QueueHandle_t print_queue = NULL;        // Message queue for print requests

/**
 * @brief  UART3 peripheral handle for debug logging
 * @note   Defined in main.c and initialized in MX_USART3_UART_Init()
 */
extern UART_HandleTypeDef huart3;

/**
 * @brief  Initialize print task and message queue
 * @note   Must be called BEFORE starting the scheduler
 * @retval None
 */
void print_task_init(void)
{
    // Create message queue for print requests
    print_queue = xQueueCreate(PRINT_QUEUE_DEPTH, PRINT_MESSAGE_MAX_SIZE);
    configASSERT(print_queue != NULL);

    // Create print task
    // Priority 3: Higher than user tasks to ensure responsive debug logging
    BaseType_t status = xTaskCreate(print_task_handler,
                                    "Print_Task",
                                    PRINT_TASK_STACK_SIZE,
                                    NULL,
                                    PRINT_TASK_PRIORITY,
                                    NULL);
    configASSERT(status == pdPASS);
}

/**
 * @brief  Send a message to the print queue
 * @param  message: Null-terminated string to print to serial terminal
 * @retval BaseType_t: pdPASS if queued successfully, pdFAIL if timeout
 */
BaseType_t print_message(const char *message)
{
    char buffer[PRINT_MESSAGE_MAX_SIZE];

    // Validate input
    if (message == NULL) {
        return pdFAIL;
    }

    // Safety check: If print_task not initialized, silently fail
    // This allows other tasks to call print_message even if print_task is disabled
    if (print_queue == NULL) {
        return pdFAIL;
    }

    // Copy message to local buffer with size limit
    strncpy(buffer, message, PRINT_MESSAGE_MAX_SIZE - 1);
    buffer[PRINT_MESSAGE_MAX_SIZE - 1] = '\0';  // Ensure null termination

    // Send to queue (copies buffer into queue storage)
    BaseType_t result = xQueueSend(print_queue, buffer, pdMS_TO_TICKS(PRINT_ENQUEUE_TIMEOUT_MS));

    return result;
}

/**
 * @brief  Send a single character to the print queue
 * @param  c: Character to print to serial terminal
 * @retval BaseType_t: pdPASS if queued successfully, pdFAIL if timeout
 */
BaseType_t print_char(char c)
{
    char buffer[2];

    // Safety check: If print_task not initialized, silently fail
    if (print_queue == NULL) {
        return pdFAIL;
    }

    // Create 2-byte message: character + null terminator
    buffer[0] = c;
    buffer[1] = '\0';

    // Send to queue
    BaseType_t result = xQueueSend(print_queue, buffer, pdMS_TO_TICKS(PRINT_ENQUEUE_TIMEOUT_MS));

    return result;
}

/**
 * @brief  Print task main loop - processes messages from queue
 * @param  parameters: Task parameters (unused)
 * @retval None (task never returns)
 *
 * Task Behavior:
 * - Blocks waiting for messages in queue
 * - Transmits messages to UART3 (serial terminal)
 * - Feeds watchdog periodically
 *
 * IMPORTANT: This task uses UART3 for debug output
 *            UART2 is reserved for ESP8266 communication
 */
void print_task_handler(void *parameters)
{
    char message_buffer[PRINT_MESSAGE_MAX_SIZE];

    // Register with watchdog (5 second timeout)
    watchdog_id_t wd_id = watchdog_register("Print_Task", 5000);
    if (wd_id == WATCHDOG_INVALID_ID) {
        // Silently fail - can't use print_message here (would cause recursion)
    }

    // Send startup message to serial terminal
    const char *startup_msg = "\r\n[PRINT_TASK] Debug logging initialized on UART3\r\n";
    HAL_UART_Transmit(&huart3, (uint8_t*)startup_msg, strlen(startup_msg), HAL_MAX_DELAY);

    while (1) {
        // Block waiting for message with finite timeout (2 seconds)
        // Timeout allows periodic watchdog feeding even when no print activity
        if (xQueueReceive(print_queue, message_buffer, pdMS_TO_TICKS(2000)) == pdPASS) {
            // Message received - transmit to UART3 (serial terminal)
            HAL_UART_Transmit(&huart3,
                            (uint8_t *)message_buffer,
                            strlen(message_buffer),
                            HAL_MAX_DELAY);
        }

        // Feed watchdog to prove task is alive
        if (wd_id != WATCHDOG_INVALID_ID) {
            watchdog_feed(wd_id);
        }
    }
}
