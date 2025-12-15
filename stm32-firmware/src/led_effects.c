/**
 ******************************************************************************
 * @file           : led_effects.c
 * @brief          : LED Pattern Control Using FreeRTOS Software Timers
 ******************************************************************************
 * @description
 * This module implements various LED blinking patterns using FreeRTOS software
 * timers. It provides a clean interface for pattern selection without blocking
 * any tasks.
 *
 * Available Patterns:
 * ┌──────────┬────────────────────────────────────────────┐
 * │ Pattern  │ Description                                │
 * ├──────────┼────────────────────────────────────────────┤
 * │ NONE     │ Both LEDs OFF (timers stopped)             │
 * │ 1        │ Both LEDs always ON (static, no timers)    │
 * │ 2        │ Green: 100ms, Orange: 1000ms (async blink) │
 * │ 3        │ Both: 100ms (synchronized blink)           │
 * └──────────┴────────────────────────────────────────────┘
 *
 * Implementation:
 * - Uses 2 auto-reload software timers (one per LED)
 * - Timers run in Timer Service Task (separate from app tasks)
 * - Timer callbacks toggle GPIO pins directly
 * - Periods are dynamically changed using xTimerChangePeriod()
 *
 * Hardware:
 * - LED_GREEN (LD4) on GPIO PD12
 * - LED_ORANGE (LD3) on GPIO PD13
 *
 * @note Software timers execute in timer service task context
 *       GPIO operations in callbacks are safe (ISR-safe HAL functions)
 ******************************************************************************
 */

#include "led_effects.h"
#include "FreeRTOS.h"
#include "timers.h"

/* Software timer handles */
static TimerHandle_t led_timer1 = NULL;  // Controls LED_GREEN (LD4)
static TimerHandle_t led_timer2 = NULL;  // Controls LED_ORANGE (LD3)

/* Current active pattern */
static LED_Pattern_t current_pattern = LED_PATTERN_NONE;

void led_effects_init(void)
{
    // Create software timers for LED control
    // Timer 1: for LED1 (Green - LD4)
    led_timer1 = xTimerCreate("LED_Timer1",
                              pdMS_TO_TICKS(100),
                              pdTRUE,  // Auto-reload
                              (void *)0,
                              led_timer1_callback);

    // Timer 2: for LED2 (Orange - LD3)
    led_timer2 = xTimerCreate("LED_Timer2",
                              pdMS_TO_TICKS(100),
                              pdTRUE,  // Auto-reload
                              (void *)0,
                              led_timer2_callback);

    // Ensure both LEDs start OFF
    HAL_GPIO_WritePin(GPIOD, LED_GREEN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, LED_ORANGE_PIN, GPIO_PIN_RESET);
}

/**
 * @brief  Set LED pattern - Main control function
 * @param  pattern: Desired LED pattern (LED_PATTERN_NONE, 1, 2, or 3)
 * @retval None
 *
 * Pattern Implementation Details:
 *
 * PATTERN_NONE (0):
 * - Stop both timers
 * - Turn OFF both LEDs
 * - No CPU overhead (timers inactive)
 *
 * PATTERN_1 (Always ON):
 * - Stop both timers (not needed)
 * - Set both LEDs HIGH
 * - Static state, no periodic activity
 *
 * PATTERN_2 (Different Frequencies):
 * - Timer1: 100ms period (Green LED toggles every 100ms)
 * - Timer2: 1000ms period (Orange LED toggles every 1 second)
 * - Creates visually distinct blinking pattern
 *
 * PATTERN_3 (Same Frequency):
 * - Both timers: 100ms period
 * - LEDs blink in sync (can start out of phase)
 * - Creates fast synchronized blinking
 *
 * @note Always stops existing timers first to prevent glitches
 * @note xTimerChangePeriod() also starts the timer if stopped
 */
void led_effects_set_pattern(LED_Pattern_t pattern)
{
    // Safety: Stop all running timers before pattern change
    // This prevents orphaned timers from previous pattern
    if (led_timer1 != NULL) {
        xTimerStop(led_timer1, 0);  // 0 = don't block if timer queue full
    }
    if (led_timer2 != NULL) {
        xTimerStop(led_timer2, 0);
    }

    // Update pattern state
    current_pattern = pattern;

    // Configure LEDs and timers based on selected pattern
    switch (pattern) {
        case LED_PATTERN_1:
            // Pattern 1: Always ON 2 LEDs
            HAL_GPIO_WritePin(GPIOD, LED_GREEN_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(GPIOD, LED_ORANGE_PIN, GPIO_PIN_SET);
            break;

        case LED_PATTERN_2:
            // Pattern 2: Different frequency blinking
            // Creates visual contrast - one fast, one slow
            HAL_GPIO_WritePin(GPIOD, LED_GREEN_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, LED_ORANGE_PIN, GPIO_PIN_RESET);

            // Configure periods: Green=100ms, Orange=1000ms (10:1 ratio)
            xTimerChangePeriod(led_timer1, pdMS_TO_TICKS(100), 0);
            xTimerChangePeriod(led_timer2, pdMS_TO_TICKS(1000), 0);

            // Start both timers (xTimerChangePeriod also starts, but explicit for clarity)
            xTimerStart(led_timer1, 0);
            xTimerStart(led_timer2, 0);
            break;

        case LED_PATTERN_3:
            // Pattern 3: Synchronized fast blinking
            // Both LEDs toggle at same rate (may be out of phase initially)
            HAL_GPIO_WritePin(GPIOD, LED_GREEN_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, LED_ORANGE_PIN, GPIO_PIN_RESET);

            // Both timers: 100ms period (fast blink)
            xTimerChangePeriod(led_timer1, pdMS_TO_TICKS(100), 0);
            xTimerChangePeriod(led_timer2, pdMS_TO_TICKS(100), 0);

            // Start both timers
            xTimerStart(led_timer1, 0);
            xTimerStart(led_timer2, 0);
            break;

        default:
            // PATTERN_NONE or invalid: Turn off all LEDs
            // Timers already stopped at function entry
            HAL_GPIO_WritePin(GPIOD, LED_GREEN_PIN, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, LED_ORANGE_PIN, GPIO_PIN_RESET);
            break;
    }
}

/**
 * @brief  Timer 1 Callback - Controls Green LED (LD4)
 * @param  xTimer: Timer handle (unused but required by FreeRTOS API)
 * @retval None
 *
 * Execution Context:
 * - Runs in Timer Service Task context (NOT interrupt!)
 * - Priority determined by configTIMER_TASK_PRIORITY
 * - Can safely call FreeRTOS APIs (non-ISR versions)
 *
 * @note This is an auto-reload timer - callback executes periodically
 * @note HAL_GPIO_TogglePin() is atomic and ISR-safe
 */
void led_timer1_callback(TimerHandle_t xTimer)
{
    // Toggle Green LED state (ON->OFF or OFF->ON)
    HAL_GPIO_TogglePin(GPIOD, LED_GREEN_PIN);
}

/**
 * @brief  Timer 2 Callback - Controls Orange LED (LD3)
 * @param  xTimer: Timer handle (unused but required by FreeRTOS API)
 * @retval None
 *
 * Execution Context:
 * - Runs in Timer Service Task context (NOT interrupt!)
 * - Priority determined by configTIMER_TASK_PRIORITY
 * - Can safely call FreeRTOS APIs (non-ISR versions)
 *
 * @note This is an auto-reload timer - callback executes periodically
 * @note HAL_GPIO_TogglePin() is atomic and ISR-safe
 */
void led_timer2_callback(TimerHandle_t xTimer)
{
    // Toggle Orange LED state (ON->OFF or OFF->ON)
    HAL_GPIO_TogglePin(GPIOD, LED_ORANGE_PIN);
}
