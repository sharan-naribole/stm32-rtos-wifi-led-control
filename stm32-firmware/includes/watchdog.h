/**
 ******************************************************************************
 * @file           : watchdog.h
 * @brief          : Task Watchdog Monitor - Deadlock Detection
 ******************************************************************************
 * @description
 * Simple watchdog system to detect hung or deadlocked tasks.
 *
 * How it works:
 * 1. Each task registers with watchdog_register()
 * 2. Tasks must call watchdog_feed(id) periodically
 * 3. Watchdog task monitors all registered tasks
 * 4. If a task doesn't feed within threshold → alert!
 *
 * Example usage:
 * ```c
 * // In task initialization:
 * watchdog_id_t my_id = watchdog_register("MyTask", 5000);  // 5 second timeout
 *
 * // In task loop:
 * while(1) {
 *     // Do work...
 *     watchdog_feed(my_id);  // Prove I'm alive
 *     vTaskDelay(1000);
 * }
 * ```
 ******************************************************************************
 */

#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

/*============================================================================
 * Configuration
 *===========================================================================*/

/** Maximum number of tasks that can be monitored */
#define WATCHDOG_MAX_TASKS  3

/** Watchdog monitor task priority (should be high) */
#define WATCHDOG_TASK_PRIORITY  4

/** Watchdog monitor task stack size (words) */
#define WATCHDOG_TASK_STACK_SIZE  256

/** How often watchdog checks all tasks (ms) */
#define WATCHDOG_CHECK_PERIOD_MS  1000

/*============================================================================
 * Types
 *===========================================================================*/

/** Watchdog task ID (returned from registration) */
typedef uint8_t watchdog_id_t;

/** Invalid watchdog ID */
#define WATCHDOG_INVALID_ID  0xFF

/** Watchdog event callback type */
typedef void (*watchdog_callback_t)(watchdog_id_t id, const char *task_name, uint32_t last_feed_ms);

/*============================================================================
 * Public API
 *===========================================================================*/

/**
 * @brief  Initialize watchdog system
 * @note   Call BEFORE starting FreeRTOS scheduler
 * @retval None
 *
 * Creates the watchdog monitor task that will check all registered
 * tasks periodically.
 */
void watchdog_init(void);

/**
 * @brief  Register a task with the watchdog
 * @param  task_name: Name of task (for debugging)
 * @param  timeout_ms: Max time between feeds before alert (milliseconds)
 * @retval Watchdog ID to use with watchdog_feed(), or WATCHDOG_INVALID_ID if failed
 *
 * Call this during task initialization. The returned ID must be saved
 * and used to feed the watchdog periodically.
 *
 * Example:
 * ```c
 * watchdog_id_t my_id = watchdog_register("UART_Task", 5000);
 * if (my_id == WATCHDOG_INVALID_ID) {
 *     // Registration failed!
 * }
 * ```
 */
watchdog_id_t watchdog_register(const char *task_name, uint32_t timeout_ms);

/**
 * @brief  Feed the watchdog (prove task is alive)
 * @param  id: Watchdog ID from watchdog_register()
 * @retval None
 *
 * Call this periodically from your task (before timeout expires).
 * This resets the watchdog timer for your task.
 *
 * Example:
 * ```c
 * while(1) {
 *     // Do work...
 *     watchdog_feed(my_id);  // Feed before timeout!
 *     vTaskDelay(1000);
 * }
 * ```
 */
void watchdog_feed(watchdog_id_t id);

/**
 * @brief  Set callback for watchdog timeout events
 * @param  callback: Function to call when task timeout detected
 * @retval None
 *
 * Optional: Set a callback to be notified when a task fails to feed.
 * If not set, watchdog will print to UART (if print task available).
 *
 * Callback signature:
 * void my_callback(watchdog_id_t id, const char *task_name, uint32_t last_feed_ms)
 *
 * Example:
 * ```c
 * void watchdog_alert(watchdog_id_t id, const char *name, uint32_t last_ms) {
 *     printf("WATCHDOG: Task %s hasn't fed in %lu ms!\n", name, last_ms);
 *     // Take action: restart task, log event, etc.
 * }
 *
 * watchdog_set_callback(watchdog_alert);
 * ```
 */
void watchdog_set_callback(watchdog_callback_t callback);

/**
 * @brief  Get statistics for a registered task
 * @param  id: Watchdog ID
 * @param  last_feed_ms: [OUT] Time since last feed (ms)
 * @param  timeout_ms: [OUT] Configured timeout (ms)
 * @retval pdTRUE if valid ID, pdFALSE otherwise
 *
 * Query watchdog status for debugging/monitoring.
 */
BaseType_t watchdog_get_stats(watchdog_id_t id, uint32_t *last_feed_ms, uint32_t *timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* __WATCHDOG_H */
