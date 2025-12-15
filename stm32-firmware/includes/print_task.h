/**
 ******************************************************************************
 * @file           : print_task.h
 * @brief          : Dedicated Print Task for Debug Logging (UART3)
 ******************************************************************************
 * @description
 * This module implements a dedicated print task that owns UART3 exclusively
 * for debug logging and watchdog monitoring output to serial terminal.
 *
 * IMPORTANT: This is NOT for ESP8266 communication!
 * - UART2: ESP8266 communication (user_input task)
 * - UART3: Debug logging and watchdog output (print_task)
 *
 * Key Features:
 * - Exclusive UART3 ownership (no concurrent access issues)
 * - Non-blocking API for application tasks
 * - Queue-based message passing
 * - FIFO message ordering
 * - Watchdog monitoring integration
 *
 * Usage Example:
 * ```c
 * // Simple string printing to serial terminal
 * print_message("[APP] System initialized\r\n");
 *
 * // Character echo
 * print_char('A');
 *
 * // Formatted printing
 * char buffer[64];
 * snprintf(buffer, sizeof(buffer), "[SENSOR] Temp: %d°C\r\n", temp);
 * print_message(buffer);
 * ```
 ******************************************************************************
 */

#ifndef __PRINT_TASK_H
#define __PRINT_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*============================================================================
 * Configuration Constants
 *===========================================================================*/

/**
 * @brief  Maximum size of a single print message
 * @note   256 bytes is sufficient for debug messages
 */
#define PRINT_MESSAGE_MAX_SIZE 256

/**
 * @brief  Print message queue depth
 * @note   Number of messages that can be queued before blocking/dropping
 */
#define PRINT_QUEUE_DEPTH 5

/**
 * @brief  Print task priority
 * @note   Priority 3 (high priority for responsive debug logging)
 *         Higher than user_input task to ensure watchdog alerts are immediate
 */
#define PRINT_TASK_PRIORITY 3

/**
 * @brief  Print task stack size (in words)
 * @note   384 words = 1536 bytes (reduced to save RAM)
 */
#define PRINT_TASK_STACK_SIZE 384

/**
 * @brief  Timeout for enqueuing print messages (milliseconds)
 * @note   100ms timeout prevents deadlock if queue fills unexpectedly
 */
#define PRINT_ENQUEUE_TIMEOUT_MS 100

/*============================================================================
 * FreeRTOS Objects (Global Handles)
 *===========================================================================*/

/**
 * @brief  Print message queue handle
 */
extern QueueHandle_t print_queue;

/**
 * @brief  UART3 peripheral handle for debug logging
 * @note   UART3 is used for serial terminal debug output, NOT ESP8266
 */
extern UART_HandleTypeDef huart3;

/*============================================================================
 * Public Function Interfaces
 *===========================================================================*/

/**
 * @brief  Initialize print task and message queue
 * @note   MUST be called before starting the FreeRTOS scheduler
 * @retval None
 */
void print_task_init(void);

/**
 * @brief  Send a string message to the print queue (debug logging)
 * @param  message: Null-terminated string to print to serial terminal
 * @retval BaseType_t: pdPASS if message queued successfully, pdFAIL if timeout
 *
 * Thread Safety: Safe to call from any task
 *
 * Example:
 * ```c
 * print_message("[DEBUG] Entering sleep mode\r\n");
 * ```
 */
BaseType_t print_message(const char *message);

/**
 * @brief  Send a single character to the print queue
 * @param  c: Character to print to serial terminal
 * @retval BaseType_t: pdPASS if character queued successfully, pdFAIL if timeout
 *
 * Example:
 * ```c
 * print_char('A');  // Echo back character to serial terminal
 * ```
 */
BaseType_t print_char(char c);

/**
 * @brief  Print task handler (main task loop)
 * @param  parameters: Task parameters (unused, required by FreeRTOS API)
 * @retval None (task never returns)
 *
 * @note This task has EXCLUSIVE access to UART3 for debug logging
 *       No other task should call HAL_UART_Transmit(&huart3, ...) directly
 */
void print_task_handler(void *parameters);

#ifdef __cplusplus
}
#endif

#endif /* __PRINT_TASK_H */
