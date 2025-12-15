/**
 ******************************************************************************
 * @file           : watchdog.c
 * @brief          : Task Watchdog Monitor Implementation
 ******************************************************************************
 * @description
 * Monitors registered tasks to detect hung or deadlocked tasks.
 *
 * Architecture:
 * - Each task registers and gets a unique ID
 * - Tasks call watchdog_feed(id) periodically
 * - Watchdog task wakes every WATCHDOG_CHECK_PERIOD_MS
 * - Checks all tasks: if time_since_last_feed > timeout → ALERT!
 *
 ******************************************************************************
 */

#include "watchdog.h"
#include <string.h>
#include <stdio.h>

/* Use print task for all watchdog output (UART3 - debug logging) */
#include "print_task.h"
#define WATCHDOG_PRINT(msg) print_message(msg)

/*============================================================================
 * Private Types
 *===========================================================================*/

/** Watchdog entry for each registered task */
typedef struct {
    char task_name[16];           // Task name (for debugging)
    uint32_t timeout_ms;          // Max time between feeds
    TickType_t last_feed_tick;    // Last time task fed watchdog
    BaseType_t registered;        // Is this slot in use?
} watchdog_entry_t;

/*============================================================================
 * Private Data
 *===========================================================================*/

/** Array of registered tasks */
static watchdog_entry_t watchdog_tasks[WATCHDOG_MAX_TASKS];

/** Number of registered tasks */
static uint8_t num_registered = 0;

/** Optional callback for timeout events */
static watchdog_callback_t timeout_callback = NULL;

/** Watchdog task handle */
static TaskHandle_t watchdog_task_handle = NULL;

/*============================================================================
 * Private Function Prototypes
 *===========================================================================*/

static void watchdog_task(void *parameters);

/*============================================================================
 * Public Functions
 *===========================================================================*/

/**
 * @brief  Initialize watchdog system
 */
void watchdog_init(void)
{
    // Clear all entries
    memset(watchdog_tasks, 0, sizeof(watchdog_tasks));
    num_registered = 0;
    timeout_callback = NULL;

    // Create watchdog monitor task
    BaseType_t status = xTaskCreate(
        watchdog_task,
        "Watchdog",
        WATCHDOG_TASK_STACK_SIZE,
        NULL,
        WATCHDOG_TASK_PRIORITY,
        &watchdog_task_handle
    );

    configASSERT(status == pdPASS);

    WATCHDOG_PRINT("\r\n[WATCHDOG] Initialized\r\n");
}

/**
 * @brief  Register a task with watchdog
 */
watchdog_id_t watchdog_register(const char *task_name, uint32_t timeout_ms)
{
    // Check if we have space
    if (num_registered >= WATCHDOG_MAX_TASKS) {
        WATCHDOG_PRINT("[WATCHDOG] ERROR: Max tasks reached!\r\n");
        return WATCHDOG_INVALID_ID;
    }

    // Find free slot
    watchdog_id_t id;
    for (id = 0; id < WATCHDOG_MAX_TASKS; id++) {
        if (!watchdog_tasks[id].registered) {
            break;
        }
    }

    if (id >= WATCHDOG_MAX_TASKS) {
        return WATCHDOG_INVALID_ID;
    }

    // Register task
    taskENTER_CRITICAL();
    {
        strncpy(watchdog_tasks[id].task_name, task_name, sizeof(watchdog_tasks[id].task_name) - 1);
        watchdog_tasks[id].task_name[sizeof(watchdog_tasks[id].task_name) - 1] = '\0';
        watchdog_tasks[id].timeout_ms = timeout_ms;
        watchdog_tasks[id].last_feed_tick = xTaskGetTickCount();
        watchdog_tasks[id].registered = pdTRUE;
        num_registered++;
    }
    taskEXIT_CRITICAL();

    // Log registration
    char msg[64];
    snprintf(msg, sizeof(msg), "[WATCHDOG] Registered '%s' (ID=%u, timeout=%lums)\r\n",
             task_name, id, timeout_ms);
    WATCHDOG_PRINT(msg);

    return id;
}

/**
 * @brief  Feed the watchdog (task is alive)
 */
void watchdog_feed(watchdog_id_t id)
{
    // Validate ID
    if (id >= WATCHDOG_MAX_TASKS || !watchdog_tasks[id].registered) {
        return;
    }

    // Update last feed time (atomic)
    taskENTER_CRITICAL();
    watchdog_tasks[id].last_feed_tick = xTaskGetTickCount();
    taskEXIT_CRITICAL();
}

/**
 * @brief  Set timeout callback
 */
void watchdog_set_callback(watchdog_callback_t callback)
{
    timeout_callback = callback;
}

/**
 * @brief  Get watchdog statistics
 */
BaseType_t watchdog_get_stats(watchdog_id_t id, uint32_t *last_feed_ms, uint32_t *timeout_ms)
{
    if (id >= WATCHDOG_MAX_TASKS || !watchdog_tasks[id].registered) {
        return pdFALSE;
    }

    TickType_t now = xTaskGetTickCount();
    TickType_t elapsed_ticks = now - watchdog_tasks[id].last_feed_tick;

    if (last_feed_ms) {
        *last_feed_ms = pdTICKS_TO_MS(elapsed_ticks);
    }

    if (timeout_ms) {
        *timeout_ms = watchdog_tasks[id].timeout_ms;
    }

    return pdTRUE;
}

/*============================================================================
 * Private Functions
 *===========================================================================*/

/**
 * @brief  Watchdog monitor task
 * @param  parameters: Unused
 *
 * Task Operation:
 * 1. Wake every WATCHDOG_CHECK_PERIOD_MS
 * 2. Check all registered tasks
 * 3. For each task: if (time_since_last_feed > timeout) → ALERT!
 * 4. Call callback or print warning
 */
static void watchdog_task(void *parameters)
{
    (void)parameters;

    TickType_t last_wake = xTaskGetTickCount();

    WATCHDOG_PRINT("[WATCHDOG] Monitor task started\r\n");

    while (1) {
        // Sleep for check period
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(WATCHDOG_CHECK_PERIOD_MS));

        // Get current time
        TickType_t now = xTaskGetTickCount();

        // Check all registered tasks
        for (uint8_t id = 0; id < WATCHDOG_MAX_TASKS; id++) {
            if (!watchdog_tasks[id].registered) {
                continue;  // Skip unregistered slots
            }

            // Calculate time since last feed
            TickType_t elapsed_ticks = now - watchdog_tasks[id].last_feed_tick;
            uint32_t elapsed_ms = pdTICKS_TO_MS(elapsed_ticks);

            // Check if timeout exceeded
            if (elapsed_ms > watchdog_tasks[id].timeout_ms) {
                // TIMEOUT DETECTED!

                if (timeout_callback) {
                    // Call user callback
                    timeout_callback(id, watchdog_tasks[id].task_name, elapsed_ms);
                } else {
                    // Default: print warning
                    char alert_msg[256];
                    snprintf(alert_msg, sizeof(alert_msg),
                             "\r\n*** WATCHDOG ALERT ***\r\n"
                             "Task: %s (ID=%u)\r\n"
                             "Last feed: %lu ms ago\r\n"
                             "Timeout: %lu ms\r\n"
                             "Status: HUNG or DEADLOCKED!\r\n\r\n",
                             watchdog_tasks[id].task_name,
                             id,
                             elapsed_ms,
                             watchdog_tasks[id].timeout_ms);
                    WATCHDOG_PRINT(alert_msg);
                }

                // Reset timer to avoid spam (task may be permanently hung)
                // This gives recovery mechanisms time to act
                watchdog_tasks[id].last_feed_tick = now;
            }
        }
    }
}
