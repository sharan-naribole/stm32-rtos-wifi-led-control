# STM32CubeMX Configuration Guide
## Project: 015LedControlWifiServer

This document provides step-by-step instructions for configuring the STM32F407 project in STM32CubeMX.

---

## Overview

This project uses **TWO separate UART peripherals** for different purposes:

| UART | Purpose | Connected To | Pins |
|------|---------|--------------|------|
| **USART2** | ESP8266 Communication | ESP8266 Wi-Fi Module | PA2 (TX), PA3 (RX) |
| **USART3** | Debug Logging | Serial Terminal/PC | PD8 (TX), PB11 (RX) |

---

## Step 1: Open STM32CubeMX Project

1. Launch **STM32CubeMX**
2. Open existing project: `015LedControlWifiServer.ioc`
3. If starting fresh, select **STM32F407VGTx** microcontroller

---

## Step 2: Configure USART2 (ESP8266 Communication)

### 2.1 Enable USART2
- Navigate to: **Connectivity → USART2**
- Set **Mode**: `Asynchronous`
- This will automatically assign pins:
  - **PA2**: USART2_TX
  - **PA3**: USART2_RX

### 2.2 USART2 Parameters
Configure the following parameters:

| Parameter | Value | Notes |
|-----------|-------|-------|
| **Baud Rate** | `115200 Bits/s` | Must match ESP8266 |
| **Word Length** | `8 Bits` | Standard |
| **Parity** | `None` | Standard |
| **Stop Bits** | `1` | Standard |
| **Data Direction** | `Receive and Transmit` | Full duplex |
| **Over Sampling** | `16 Samples` | Standard |

### 2.3 USART2 Interrupts
- Navigate to **NVIC Settings** tab
- Enable: **USART2 global interrupt**
- Priority: Leave at default or set to `5` (medium priority)

---

## Step 3: Configure USART3 (Debug Logging)

### 3.1 Enable USART3
- Navigate to: **Connectivity → USART3**
- Set **Mode**: `Asynchronous`

### 3.2 Select USART3 Pins

USART3 has multiple pin options. **This project uses:**

**Current Configuration (PD8/PB11):**
- **PD8**: USART3_TX
- **PB11**: USART3_RX

**Alternative Options (if PD8/PB11 unavailable):**

**Option A:**
- **PB10**: USART3_TX
- **PB11**: USART3_RX

**Option B:**
- **PC10**: USART3_TX
- **PC11**: USART3_RX

> **Note**: Current hardware is configured for **PD8 (TX) and PB11 (RX)**. If you change pins, update all hardware connections and documentation accordingly.

### 3.3 USART3 Parameters
Configure the same parameters as USART2:

| Parameter | Value |
|-----------|-------|
| **Baud Rate** | `115200 Bits/s` |
| **Word Length** | `8 Bits` |
| **Parity** | `None` |
| **Stop Bits** | `1` |
| **Data Direction** | `Receive and Transmit` |
| **Over Sampling** | `16 Samples` |

### 3.4 USART3 Interrupts
- Navigate to **NVIC Settings** tab
- Enable: **USART3 global interrupt**
- Priority: Set to `4` (higher priority than USART2 for debug output)

---

## Step 4: GPIO Configuration (LEDs - Already Configured)

The following LEDs should already be configured:

| LED | Pin | GPIO Mode |
|-----|-----|-----------|
| **LD4 (Green)** | PD12 | GPIO_Output |
| **LD3 (Orange)** | PD13 | GPIO_Output |
| **LD5 (Red)** | PD14 | GPIO_Output |
| **LD6 (Blue)** | PD15 | GPIO_Output |

No changes needed unless starting from scratch.

---

## Step 5: Clock Configuration

Ensure the following clock settings:

| Parameter | Value | Notes |
|-----------|-------|-------|
| **HCLK** | `168 MHz` | System clock |
| **APB1 Timer clocks** | `84 MHz` | For USART2/3 |
| **APB2 Timer clocks** | `168 MHz` | For peripherals |

These should already be configured. Verify in **Clock Configuration** tab.

---

## Step 6: FreeRTOS Configuration (Already Configured)

The FreeRTOS settings should already be configured. Verify:

### 6.1 Kernel Settings
- **TOTAL_HEAP_SIZE**: At least `20480` bytes (20 KB)
- **configUSE_TIMERS**: `Enabled`
- **configTIMER_TASK_PRIORITY**: `2`
- **configTIMER_QUEUE_LENGTH**: `10`

### 6.2 Include Parameters
- **vTaskDelayUntil**: `Enabled`
- **xTaskGetTickCount**: `Enabled`

---

## Step 7: Generate Code

1. Click **Project → Generate Code** (or press `Ctrl+Shift+G`)
2. Choose **Generate Code** option
3. STM32CubeMX will update:
   - `main.c`
   - `stm32f4xx_it.c`
   - `stm32f4xx_hal_msp.c`
   - Other peripheral files

4. **IMPORTANT**: The generated code will create:
   - `extern UART_HandleTypeDef huart2;` (ESP8266)
   - `extern UART_HandleTypeDef huart3;` (Debug logging)

---

## Step 8: Hardware Connections

### 8.1 USART2 ↔ ESP8266
Connect the following wires:

| STM32 Pin | ESP8266 Pin | Description |
|-----------|-------------|-------------|
| **PA2** (USART2_TX) | **D2** (GPIO4) | STM32 → ESP8266 |
| **PA3** (USART2_RX) | **D1** (GPIO5) | ESP8266 → STM32 |
| **GND** | **GND** | Common ground |

> **Note**: 3.3V logic level - STM32 and ESP8266 are compatible

### 8.2 USART3 ↔ USB-to-Serial Adapter
Connect to a USB-to-Serial adapter for debug logging:

| STM32 Pin | USB-Serial Adapter |
|-----------|--------------------|
| **PD8** (USART3_TX) | **RX** |
| **PB11** (USART3_RX) | **TX** (optional - only needed for receiving commands) |
| **GND** | **GND** |

> **Note**: For debug output only, you only need to connect **PD8 → Adapter RX** and **GND → GND**

---

## Step 9: Verify Generated Code

After code generation, verify the following files exist:

1. **main.c**: Contains `MX_USART2_UART_Init()` and `MX_USART3_UART_Init()`
2. **stm32f4xx_hal_msp.c**: Contains `HAL_UART_MspInit()` for both UARTs
3. **stm32f4xx_it.c**: Contains interrupt handlers:
   - `USART2_IRQHandler()`
   - `USART3_IRQHandler()`

---

## Step 10: Build and Flash

1. Open the project in **STM32CubeIDE**
2. Build the project (`Ctrl+B`)
3. Flash to STM32F407 board
4. Open two serial terminal windows:
   - **Terminal 1**: Connect to USART3 (debug output) at 115200 baud
   - **ESP8266**: Will communicate via USART2 automatically

---

## Troubleshooting

### USART3 Not Working
- Verify pins are correctly assigned in Pin Configuration view
- Check that USART3 clock is enabled in RCC configuration
- Ensure USART3 global interrupt is enabled in NVIC

### Pin Conflicts
- Current configuration uses PD8/PB11
- If these pins are unavailable, select alternative pins (PB10/PB11 or PC10/PC11)
- Update hardware connections and all documentation accordingly

### Missing huart3 Declaration
- Ensure USART3 is enabled in CubeMX before generating code
- Check `main.c` for `UART_HandleTypeDef huart3;` declaration

---

## Summary

This configuration creates a dual-UART system:

```
┌─────────────────────────────────────────────────────────┐
│                    STM32F407                            │
│                                                         │
│  USART2 (PA2/PA3) ←→ ESP8266 Wi-Fi Module              │
│     - LED Commands                                     │
│     - PING/PONG                                        │
│     - Pattern Control                                  │
│                                                         │
│  USART3 (PD8/PB11) ←→ Serial Terminal (PC)            │
│     - Watchdog Alerts                                  │
│     - Debug Logging                                    │
│     - System Status                                    │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## Questions?

If you encounter issues:
1. Verify all pins are correctly assigned in CubeMX Pin Configuration
2. Check Clock Configuration matches specifications
3. Ensure both USART interrupts are enabled
4. Verify hardware connections match pin assignments

**Last Updated**: December 2024
