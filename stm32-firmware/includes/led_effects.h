/**
 ******************************************************************************
 * @file           : led_effects.h
 * @brief          : LED Pattern Control Using FreeRTOS Software Timers
 ******************************************************************************
 * @description
 * This header defines the interface for LED pattern control using FreeRTOS
 * software timers. Provides multiple blinking patterns without blocking tasks.
 *
 * Implementation Strategy:
 * - Uses 2 auto-reload software timers (one per LED)
 * - Timers execute in Timer Service Task context (NOT interrupt!)
 * - Timer callbacks toggle GPIO pins directly
 * - Periods dynamically changed using xTimerChangePeriod()
 *
 * Hardware Mapping:
 * ┌──────────────┬──────────┬──────────┬─────────────┐
 * │ LED Name     │ Color    │ GPIO Pin │ Timer       │
 * ├──────────────┼──────────┼──────────┼─────────────┤
 * │ LD4          │ Green    │ PD12     │ led_timer1  │
 * │ LD3          │ Orange   │ PD13     │ led_timer2  │
 * └──────────────┴──────────┴──────────┴─────────────┘
 *
 * Available Patterns:
 * ┌──────────┬────────────────┬─────────────────────────────────┐
 * │ Pattern  │ Name           │ Description                     │
 * ├──────────┼────────────────┼─────────────────────────────────┤
 * │ NONE     │ All OFF        │ Both LEDs OFF (timers stopped)  │
 * │ 1        │ Always ON      │ Both LEDs ON (static, no timer) │
 * │ 2        │ Async Blink    │ Green: 100ms, Orange: 1000ms    │
 * │ 3        │ Sync Blink     │ Both: 100ms (synchronized)      │
 * └──────────┴────────────────┴─────────────────────────────────┘
 *
 * Thread Safety:
 * - Timer callbacks run in Timer Service Task (configTIMER_TASK_PRIORITY)
 * - HAL_GPIO_TogglePin() is atomic and ISR-safe
 * - No synchronization needed for pattern changes
 ******************************************************************************
 */

#ifndef __LED_EFFECTS_H
#define __LED_EFFECTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "FreeRTOS.h"
#include "timers.h"

/*============================================================================
 * Type Definitions
 *===========================================================================*/

/**
 * @brief  LED pattern enumeration
 *
 * Defines all available LED blinking patterns. Each pattern has different
 * timing and visual characteristics.
 *
 * LED_PATTERN_NONE (0):
 *   Both LEDs OFF
 *   Timers: Stopped (no CPU overhead)
 *   Use case: Power saving, application exit
 *
 * LED_PATTERN_1:
 *   Both LEDs always ON
 *   Timers: Stopped (static state, no periodic activity)
 *   Implementation: Direct GPIO write (HIGH)
 *   Use case: Maximum brightness indicator
 *
 * LED_PATTERN_2:
 *   Asynchronous blinking at different frequencies
 *   Timer1 (Green): 100ms period → toggle every 100ms (5 Hz blink rate)
 *   Timer2 (Orange): 1000ms period → toggle every 1s (0.5 Hz blink rate)
 *   Visual: Green blinks fast, Orange blinks slowly (10:1 ratio)
 *   Use case: Activity indicator with status differentiation
 *
 * LED_PATTERN_3:
 *   Synchronized blinking at same frequency
 *   Both timers: 100ms period (5 Hz blink rate)
 *   Visual: Both LEDs blink in sync (may start out of phase)
 *   Use case: Alert or attention-grabbing indicator
 *
 * @note Pattern changes are instantaneous - old pattern stops, new starts
 * @note Toggle period = 2 × blink period (ON + OFF time)
 */
typedef enum {
    LED_PATTERN_NONE = 0,   /**< All LEDs OFF (timers stopped) */
    LED_PATTERN_1,          /**< Always ON 2 LEDs (static) */
    LED_PATTERN_2,          /**< Different frequency: Green 100ms, Orange 1000ms */
    LED_PATTERN_3           /**< Same frequency: Both 100ms (synchronized) */
} LED_Pattern_t;

/*============================================================================
 * Public Function Interfaces
 *===========================================================================*/

/**
 * @brief  Initialize LED effects subsystem
 * @retval None
 *
 * Initialization Steps:
 * 1. Creates led_timer1 for Green LED (LD4/PD12)
 * 2. Creates led_timer2 for Orange LED (LD3/PD13)
 * 3. Sets both LEDs to OFF state
 *
 * Timer Configuration:
 * - Type: Auto-reload (periodic execution)
 * - Initial period: 100ms (can be changed per pattern)
 * - Callbacks: led_timer1_callback, led_timer2_callback
 *
 * @note Must be called BEFORE starting FreeRTOS scheduler
 * @note Timers are created but NOT started (LEDs remain OFF)
 */
void led_effects_init(void);

/**
 * @brief  Set active LED pattern
 * @param  pattern: Desired pattern from LED_Pattern_t enum
 * @retval None
 *
 * Pattern Switching Logic:
 * 1. Stop all running timers (clean slate)
 * 2. Configure LEDs and timers based on selected pattern
 * 3. Start timers if needed (patterns 2 and 3)
 *
 * Pattern Implementation:
 * PATTERN_NONE:
 *   - Timers: Stopped
 *   - LEDs: Both OFF
 *
 * PATTERN_1:
 *   - Timers: Stopped (not needed)
 *   - LEDs: Both ON (static)
 *
 * PATTERN_2:
 *   - Timer1: 100ms period (Green LED)
 *   - Timer2: 1000ms period (Orange LED)
 *   - LEDs: Initially OFF, then toggled by timers
 *
 * PATTERN_3:
 *   - Timer1: 100ms period (Green LED)
 *   - Timer2: 100ms period (Orange LED)
 *   - LEDs: Initially OFF, then toggled by timers
 *
 * @note Can be called from any task (command handler typically)
 * @note Pattern change is immediate - no gradual transition
 * @note xTimerChangePeriod() also starts timer if stopped
 */
void led_effects_set_pattern(LED_Pattern_t pattern);

/**
 * @brief  Timer 1 callback - Controls Green LED (LD4/PD12)
 * @param  xTimer: Timer handle (unused, required by FreeRTOS API)
 * @retval None
 *
 * Execution Context:
 * - Called by Timer Service Task (NOT hardware interrupt!)
 * - Priority: configTIMER_TASK_PRIORITY (from FreeRTOSConfig.h)
 * - Can safely call FreeRTOS APIs (non-ISR versions)
 *
 * Behavior:
 * - Toggles Green LED state (ON → OFF or OFF → ON)
 * - Called periodically based on timer period (100ms or 1000ms)
 * - Auto-reload timer: executes indefinitely until stopped
 *
 * @note HAL_GPIO_TogglePin() is atomic and ISR-safe
 * @note Keep callback short to avoid blocking timer service task
 */
void led_timer1_callback(TimerHandle_t xTimer);

/**
 * @brief  Timer 2 callback - Controls Orange LED (LD3/PD13)
 * @param  xTimer: Timer handle (unused, required by FreeRTOS API)
 * @retval None
 *
 * Execution Context:
 * - Called by Timer Service Task (NOT hardware interrupt!)
 * - Priority: configTIMER_TASK_PRIORITY (from FreeRTOSConfig.h)
 * - Can safely call FreeRTOS APIs (non-ISR versions)
 *
 * Behavior:
 * - Toggles Orange LED state (ON → OFF or OFF → ON)
 * - Called periodically based on timer period (100ms or 1000ms)
 * - Auto-reload timer: executes indefinitely until stopped
 *
 * @note HAL_GPIO_TogglePin() is atomic and ISR-safe
 * @note Keep callback short to avoid blocking timer service task
 */
void led_timer2_callback(TimerHandle_t xTimer);

#ifdef __cplusplus
}
#endif

#endif /* __LED_EFFECTS_H */
