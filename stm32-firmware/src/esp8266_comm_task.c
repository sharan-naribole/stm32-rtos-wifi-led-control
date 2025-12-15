/**
 ******************************************************************************
 * @file           : esp8266_comm_task.c
 * @brief          : ESP8266 Communication Task (UART2)
 ******************************************************************************
 * @description
 * Manages UART2 for ESP8266 communication using FreeRTOS Stream Buffers.
 * Parses LED_CMD: prefix messages and controls LED patterns.
 * Responds to ECHO_PING for connection monitoring.
 *
 * IMPORTANT: UART Allocation
 * - UART2 (huart2): ESP8266 communication (this task)
 * - UART3 (huart3): Debug logging (print_task)
 *
 * Architecture:
 * ┌─────────────┐     ┌──────────────┐     ┌──────────────────┐
 * │ UART2 RX IRQ│ ──> │ Stream Buffer│ ──> │ ESP8266 Comm Task│
 * │  (ISR)      │     │ (xStreamBuf) │     │    (Blocked)     │
 * └─────────────┘     └──────────────┘     └──────────────────┘
 *
 * Protocol:
 * - Receives: LED_CMD:X (where X = 1, 2, 3, or 4)
 * - Receives: PING (connection test from ESP8266)
 * - Receives: STM32_PONG (response to STM32_PING)
 * - Sends: OK:PatternX (acknowledgment)
 * - Sends: PONG (connection test response)
 * - Sends: STM32_PING (connection test to ESP8266)
 *
 * LED Commands:
 * - LED_CMD:1 → Pattern 1 (All LEDs ON)
 * - LED_CMD:2 → Pattern 2 (Different Frequency Blink)
 * - LED_CMD:3 → Pattern 3 (Same Frequency Blink)
 * - LED_CMD:4 → All LEDs OFF
 *
 * Hardware Connections:
 * - ESP8266 D1 (GPIO5) → STM32 PA3 (USART2 RX)
 * - ESP8266 D2 (GPIO4) → STM32 PA2 (USART2 TX)
 * - ESP8266 GND → STM32 GND
 *
 ******************************************************************************
 */

#include "esp8266_comm_task.h"
#include "led_effects.h"
#include "watchdog.h"
#include "print_task.h"
#include <string.h>
#include <stdio.h>

/* External UART handle */
extern UART_HandleTypeDef huart2;

/* FreeRTOS Stream Buffer for ISR-to-Task communication */
static StreamBufferHandle_t uart_stream_buffer = NULL;

/* Single-byte buffer for interrupt reception */
static uint8_t uart_rx_byte;

/* Command line buffer */
static char rx_buffer[UART_RX_BUFFER_SIZE];
static uint16_t rx_index = 0;

/* UART connection monitoring */
#define STM32_PING_INTERVAL_MS  10000  // STM32 pings every 10 seconds (base interval)
#define STM32_PING_JITTER_MS    2000   // Random jitter: 0-2000ms uniform distribution to avoid collision
#define STM32_PING_TIMEOUT_MS   1000   // 1 second timeout for response
static TickType_t last_ping_sent = 0;
static TickType_t last_pong_received = 0;
static BaseType_t waiting_for_pong = pdFALSE;
static BaseType_t uart_connection_ok = pdTRUE;
static uint32_t ping_random_seed = 0;

/**
 * @brief  Simple pseudo-random number generator for jitter
 * @param  max: Maximum value (exclusive)
 * @retval Random number in range [0, max)
 */
static uint32_t get_random_jitter(uint32_t max)
{
    // Simple Linear Congruential Generator (LCG)
    // Using constants from Numerical Recipes
    ping_random_seed = (ping_random_seed * 1664525UL + 1013904223UL);
    return (ping_random_seed % max);
}

/**
 * @brief  Parse and execute LED command, PING, or PONG response
 * @param  line: Received line to parse
 * @retval None
 */
static void process_led_command(char *line)
{
    // Debug: Log all received lines via print_task
    char debug_msg[128];
    snprintf(debug_msg, sizeof(debug_msg), "[ESP8266] ← Received: '%s'\r\n", line);
    print_message(debug_msg);

    // Check for PING message (UART connection test from ESP8266)
    if (strncmp(line, "PING", 4) == 0) {
        // Respond immediately to prove UART connection is alive
        // Retry up to 3 times if UART busy
        HAL_StatusTypeDef status;
        for (int retry = 0; retry < 3; retry++) {
            status = HAL_UART_Transmit(&huart2, (uint8_t*)"PONG\r\n", 6, 100);
            if (status == HAL_OK) break;
            vTaskDelay(pdMS_TO_TICKS(10)); // Wait 10ms before retry
        }
        if (status == HAL_OK) {
            print_message("[ESP8266] ← PING received, sent PONG\r\n");
        } else {
            print_message("[ESP8266] ERROR: Failed to send PONG\r\n");
        }
        return;
    }

    // Check for STM32_PONG response (reply to our STM32_PING)
    if (strncmp(line, "STM32_PONG", 10) == 0) {
        // ESP8266 is alive and responding
        if (!uart_connection_ok) {
            // Connection restored
            print_message("[ESP8266] ✓ UART connection restored!\r\n");
            uart_connection_ok = pdTRUE;
        }
        waiting_for_pong = pdFALSE;
        last_pong_received = xTaskGetTickCount();
        print_message("[ESP8266] ← STM32_PONG received\r\n");
        return;
    }

    // Check for LED_CMD: prefix
    if (strncmp(line, "LED_CMD:", 8) == 0) {
        // Extract command character after "LED_CMD:"
        char cmd = line[8];
        HAL_StatusTypeDef status;
        const char *ack_msg = NULL;
        const char *log_msg = NULL;

        switch(cmd) {
            case '1':
                led_effects_set_pattern(LED_PATTERN_1);
                ack_msg = "OK:Pattern1\r\n";
                log_msg = "[LED] Pattern 1: All LEDs ON\r\n";
                break;

            case '2':
                led_effects_set_pattern(LED_PATTERN_2);
                ack_msg = "OK:Pattern2\r\n";
                log_msg = "[LED] Pattern 2: Different Frequency Blink\r\n";
                break;

            case '3':
                led_effects_set_pattern(LED_PATTERN_3);
                ack_msg = "OK:Pattern3\r\n";
                log_msg = "[LED] Pattern 3: Same Frequency Blink\r\n";
                break;

            case '4':
                led_effects_set_pattern(LED_PATTERN_NONE);
                ack_msg = "OK:AllOFF\r\n";
                log_msg = "[LED] Pattern 4: All LEDs OFF\r\n";
                break;

            default:
                ack_msg = "ERROR:InvalidPattern\r\n";
                log_msg = "[LED] ERROR: Invalid pattern command\r\n";
                break;
        }

        // Send ACK with retry logic
        if (ack_msg != NULL) {
            for (int retry = 0; retry < 3; retry++) {
                status = HAL_UART_Transmit(&huart2, (uint8_t*)ack_msg, strlen(ack_msg), 100);
                if (status == HAL_OK) break;
                vTaskDelay(pdMS_TO_TICKS(10)); // Wait 10ms before retry
            }
            if (status != HAL_OK) {
                print_message("[LED] ERROR: Failed to send ACK to ESP8266\r\n");
            }
        }

        // Log to UART3
        if (log_msg != NULL) {
            print_message(log_msg);
        }
        return;
    }
}

/**
 * @brief  Initialize ESP8266 Communication Stream Buffer subsystem
 * @note   Must be called BEFORE starting the FreeRTOS scheduler
 * @retval None
 *
 * Setup:
 * 1. Create stream buffer for ISR-to-Task communication
 * 2. Start first interrupt-driven UART2 reception
 *
 * The stream buffer allows the ISR to deposit bytes and the task to
 * retrieve them in a thread-safe, lock-free manner.
 */
void esp8266_comm_task_init(void)
{
    // Create stream buffer (128 bytes storage, 1 byte trigger level)
    // Trigger level = 1 means task wakes immediately when ANY byte arrives
    uart_stream_buffer = xStreamBufferCreate(UART_STREAM_BUFFER_SIZE, 1);
    configASSERT(uart_stream_buffer != NULL);

    // Start first interrupt-based reception
    // HAL will call HAL_UART_RxCpltCallback when byte arrives
    HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);
}

/**
 * @brief  UART RX Complete Callback (called from ISR context)
 * @param  huart: UART handle
 * @retval None
 *
 * ISR Operation:
 * 1. Called automatically by HAL when byte received
 * 2. Deposit byte into stream buffer (ISR-safe)
 * 3. Stream buffer wakes up task if blocked
 * 4. Re-enable reception for next byte
 *
 * Thread Safety:
 * - Uses xStreamBufferSendFromISR (ISR-safe variant)
 * - Handles context switch if higher priority task woken
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        // Send byte to stream buffer (ISR-safe, lock-free)
        // If task is blocked reading, it will be woken immediately
        xStreamBufferSendFromISR(uart_stream_buffer,
                                 &uart_rx_byte,
                                 1,
                                 &xHigherPriorityTaskWoken);

        // Re-enable reception for next byte
        HAL_UART_Receive_IT(&huart2, &uart_rx_byte, 1);

        // Yield to higher priority task if woken
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
 * @brief  ESP8266 communication task
 * @param  parameters: Unused
 * @retval None (never returns)
 *
 * Task Operation:
 * 1. Register with watchdog monitor
 * 2. Read characters from stream buffer (finite 2s timeout)
 * 3. Feed watchdog on every iteration
 * 4. Buffer until newline (\n or \r)
 * 5. Parse LED_CMD: and ECHO_PING messages
 * 6. Execute LED pattern changes or respond to ping
 * 7. Send acknowledgment back to ESP8266 via UART2
 *
 * Efficiency:
 * - Task enters BLOCKED state when no data (yields CPU to other tasks)
 * - Woken immediately by ISR when byte arrives OR on 2s timeout
 * - Zero CPU waste (no polling loop)
 * - Watchdog monitored (5s timeout, feeds every 2s)
 */
void esp8266_comm_task_handler(void *parameters)
{
    uint8_t received_char;

    // Send startup message to ESP8266
    const char *startup = "\r\nSTM32 LED Controller Ready (Stream Buffer Mode)\r\n";
    HAL_UART_Transmit(&huart2, (uint8_t*)startup, strlen(startup), 1000);

    // Initialize random seed for ping jitter using current tick count
    ping_random_seed = xTaskGetTickCount();

    // Register with watchdog (5 second timeout = 2.5× the 2s blocking period)
    watchdog_id_t wd_id = watchdog_register("ESP8266_Comm", 5000);
    if (wd_id == WATCHDOG_INVALID_ID) {
        print_message("[ESP8266] Failed to register with watchdog!\r\n");
    }

    while (1) {
        // Get current tick count for timing
        TickType_t now = xTaskGetTickCount();

        // Check if it's time to send ping to ESP8266
        // Add random jitter (0-2000ms) to avoid collision with ESP8266 pings
        static uint32_t next_ping_jitter = 0;
        if (last_ping_sent == 0) {
            // First ping - generate initial jitter
            next_ping_jitter = get_random_jitter(STM32_PING_JITTER_MS);
        }

        uint32_t ping_interval_with_jitter = STM32_PING_INTERVAL_MS + next_ping_jitter;
        if ((now - last_ping_sent) >= pdMS_TO_TICKS(ping_interval_with_jitter)) {
            // Send STM32_PING to ESP8266 with retry logic
            HAL_StatusTypeDef status;
            for (int retry = 0; retry < 3; retry++) {
                status = HAL_UART_Transmit(&huart2, (uint8_t*)"STM32_PING\r\n", 12, 100);
                if (status == HAL_OK) break;
                vTaskDelay(pdMS_TO_TICKS(10)); // Wait 10ms before retry
            }

            if (status == HAL_OK) {
                last_ping_sent = now;
                waiting_for_pong = pdTRUE;
                print_message("[ESP8266] → Sending STM32_PING...\r\n");
                // Generate new jitter for next ping
                next_ping_jitter = get_random_jitter(STM32_PING_JITTER_MS);
            } else {
                print_message("[ESP8266] ERROR: Failed to send STM32_PING\r\n");
            }
        }

        // Check for ping timeout
        if (waiting_for_pong && ((now - last_ping_sent) >= pdMS_TO_TICKS(STM32_PING_TIMEOUT_MS))) {
            if (uart_connection_ok) {
                // Connection appears broken (first time)
                uart_connection_ok = pdFALSE;
                print_message("[ESP8266] ✗ ALERT: No STM32_PONG response!\r\n");
                print_message("[ESP8266] UART connection may be broken\r\n");
            }
            // Reset waiting flag so we can detect the next ping timeout
            waiting_for_pong = pdFALSE;
        }

        // Read one byte from stream buffer with finite timeout
        // When data is available, returns immediately (doesn't wait full timeout)
        // When buffer empty, timeout allows periodic watchdog feeding and ping checking
        // Short timeout (100ms) ensures responsive ping detection
        size_t received = xStreamBufferReceive(uart_stream_buffer,
                                               &received_char,
                                               1,
                                               pdMS_TO_TICKS(100));

        // Feed watchdog to prove task is alive
        // Fed on every iteration (whether data received or timeout)
        if (wd_id != WATCHDOG_INVALID_ID) {
            watchdog_feed(wd_id);
        }

        // If timeout (no data received), continue to next iteration
        if (received == 0) {
            continue;
        }

        // Data received - check how much is still in buffer for diagnostics
        size_t bytes_available = xStreamBufferBytesAvailable(uart_stream_buffer);
        if (bytes_available > 64) {
            // Buffer filling up - log warning once
            static BaseType_t buffer_warning_shown = pdFALSE;
            if (!buffer_warning_shown) {
                print_message("[ESP8266] WARNING: Stream buffer filling up, ESP8266 sending too fast!\r\n");
                buffer_warning_shown = pdTRUE;
            }
        }

        // Data received - process the character
        {
            // Check for line endings
            if (received_char == '\n' || received_char == '\r') {
                if (rx_index > 0) {
                    // Null-terminate the string
                    rx_buffer[rx_index] = '\0';

                    // Process the command
                    process_led_command(rx_buffer);

                    // Reset buffer
                    rx_index = 0;
                }
            }
            // Buffer overflow protection
            else if (rx_index >= (UART_RX_BUFFER_SIZE - 1)) {
                // Buffer full - discard and reset
                rx_index = 0;
                HAL_UART_Transmit(&huart2, (uint8_t*)"ERROR:BufferOverflow\r\n", 22, 100);
                print_message("[ESP8266] ERROR: RX buffer overflow!\r\n");
            }
            // Normal character - add to buffer
            else {
                rx_buffer[rx_index++] = received_char;
            }
        }
    }
}
