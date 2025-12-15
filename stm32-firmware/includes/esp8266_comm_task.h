/**
 ******************************************************************************
 * @file           : esp8266_comm_task.h
 * @brief          : ESP8266 Communication Task - Header
 ******************************************************************************
 * @description
 * Manages UART2 for ESP8266 communication using FreeRTOS Stream Buffers.
 *
 * IMPORTANT: UART Allocation
 * - UART2 (huart2): ESP8266 communication (this task)
 * - UART3 (huart3): Debug logging (print_task)
 *
 * Architecture:
 * - Interrupt-based reception (HAL_UART_Receive_IT)
 * - Stream buffer for ISR-to-Task communication
 * - TRUE task blocking (yields CPU while waiting)
 * - Processes LED_CMD: messages from ESP8266
 * - Responds to PING for connection monitoring
 * - Sends STM32_PING to test ESP8266 connection
 *
 ******************************************************************************
 */

#ifndef __ESP8266_COMM_TASK_H
#define __ESP8266_COMM_TASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

/* Configuration */
#define UART_RX_BUFFER_SIZE       64   // Buffer for incoming command lines
#define UART_STREAM_BUFFER_SIZE   128  // Stream buffer size (bytes)

/* Initialization function - call before starting scheduler */
void esp8266_comm_task_init(void);

/* Task function */
void esp8266_comm_task_handler(void *parameters);

#ifdef __cplusplus
}
#endif

#endif /* __ESP8266_COMM_TASK_H */
