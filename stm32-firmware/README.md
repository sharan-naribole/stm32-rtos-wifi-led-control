# STM32F407 FreeRTOS Firmware

![STM32](https://img.shields.io/badge/STM32-03234B?style=flat&logo=stmicroelectronics&logoColor=white)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-00979D?style=flat&logo=freertos&logoColor=white)
![C](https://img.shields.io/badge/C-A8B9CC?style=flat&logo=c&logoColor=black)

STM32F407VG Discovery firmware implementing dual-UART architecture with FreeRTOS, dedicated print task, watchdog monitoring, and robust UART communication with ESP8266 Wi-Fi module.

---

## 📁 File Structure

```
stm32-firmware/
├── README.md                          ← This file
├── STM32CUBEMX_CONFIGURATION.md       ← CubeMX setup guide
├── 015LedControlWifiServer.ioc        ← STM32CubeMX project file
├── src/                               ← Source files
│   ├── main.c                         ← Application entry point
│   ├── esp8266_comm_task.c            ← UART2 ESP8266 communication task
│   ├── print_task.c                   ← UART3 debug logging task
│   ├── watchdog.c                     ← Task deadlock detection
│   ├── led_effects.c                  ← LED pattern control (software timers)
│   ├── stm32f4xx_it.c                 ← Interrupt handlers
│   └── stm32f4xx_hal_msp.c            ← HAL MSP initialization
└── includes/                          ← Header files
    ├── main.h                         ← Main configuration
    ├── esp8266_comm_task.h
    ├── print_task.h
    ├── watchdog.h
    ├── led_effects.h
    ├── FreeRTOSConfig.h               ← RTOS configuration
    └── stm32f4xx_it.h
```

---

## 🏗️ Architecture Overview

### FreeRTOS Task Hierarchy

```
┌──────────────────────────────────────────────────────────────────┐
│  Priority 4: Watchdog Task (256 words stack)                    │
│  - Monitors ESP8266_Comm and Print_Task                         │
│  - Detects deadlocks/hung tasks                                 │
│  - Alerts via UART3 when timeout exceeded                       │
└──────────────────────────────────────────────────────────────────┘
           ↓ Monitors
┌──────────────────────────────────────────────────────────────────┐
│  Priority 3: Print_Task (384 words stack)                       │
│  - Exclusive UART3 ownership (thread-safe logging)              │
│  - Message queue: 5 entries × 256 bytes                         │
│  - Timeout: 5000ms watchdog                                     │
└──────────────────────────────────────────────────────────────────┘
           ↓ Logs from
┌──────────────────────────────────────────────────────────────────┐
│  Priority 2: ESP8266_Comm Task (256 words stack)                │
│  - UART2 RX/TX via stream buffer (128 bytes)                    │
│  - Processes LED_CMD:X, PING/PONG messages                      │
│  - UART retry logic (3 attempts, 10ms delay)                    │
│  - Random ping jitter (0-2000ms) to avoid collisions            │
│  - Timeout: 5000ms watchdog                                     │
└──────────────────────────────────────────────────────────────────┘
           ↓ Controls
┌──────────────────────────────────────────────────────────────────┐
│  Priority 2: Timer Service Task                                  │
│  - LED software timers (pattern control)                         │
│  - 4 patterns: All ON, Different Freq, Same Freq, All OFF       │
└──────────────────────────────────────────────────────────────────┘
```

### UART Allocation

| UART | Purpose | Pins | Direction | Baud Rate |
|------|---------|------|-----------|-----------|
| **USART2** | ESP8266 Communication | PA2 (TX), PA3 (RX) | Bidirectional | 115200 |
| **USART3** | Debug Logging | PD8 (TX), PB11 (RX) | Output only | 115200 |

**⚠️ CRITICAL:** UART3 TX is **PD8**, not PB10 (common documentation error)

---

## 🔧 Build & Flash

### Prerequisites

- **STM32CubeIDE** v1.10.0+ ([Download](https://www.st.com/en/development-tools/stm32cubeide.html))
- **STM32CubeMX** v6.0.0+ (optional, for modifying `.ioc` file)
- **ST-Link** drivers (usually included with STM32CubeIDE)

### Import Project into STM32CubeIDE

1. **File → Import → General → Existing Projects into Workspace**
2. Select `stm32-firmware` folder
3. Click **Finish**

### Configure Peripherals (Optional)

If you need to modify UART pins or add peripherals:

1. Open `015LedControlWifiServer.ioc` in STM32CubeMX
2. Make changes in graphical interface
3. **Project → Generate Code**
4. Rebuild in STM32CubeIDE

See [`STM32CUBEMX_CONFIGURATION.md`](STM32CUBEMX_CONFIGURATION.md) for detailed setup.

### Build

```
Project → Build Project (Ctrl+B / Cmd+B)
```

**Expected Output:**
```
Building target: 015LedControlWifiServer.elf
   text    data     bss     dec     hex filename
  35100     100   53932   89132   15c0c 015LedControlWifiServer.elf
Finished building: default.size.stdout
```

**Memory Analysis:**
- **Flash:** ~35 KB / 1 MB (3.4% usage) ✅
- **BSS:** ~53 KB (global variables) ✅
- **Heap:** 50 KB (FreeRTOS - configured in `FreeRTOSConfig.h`) ✅
- **Total RAM:** ~103 KB / 128 KB (80% usage, 25 KB free for stacks) ✅

### Flash to Board

1. Connect STM32F407 Discovery via USB (ST-Link)
2. **Run → Debug (F11)** or **Run → Run (Ctrl+F11)**
3. Wait for "Download verified successfully"

### Verify Operation

Connect a USB-to-Serial adapter to STM32 UART3 (PD8 TX → USB adapter RX, GND → GND) and open a serial terminal at 115200 baud.

#### Boot Sequence

**Expected UART3 output (115200 baud):**
```
========================================
STM32F407 LED Controller Boot Test
========================================
[BOOT] UART3 hardware: OK
[BOOT] System clock: 168 MHz
[BOOT] UART2 (ESP8266): 115200 baud
[BOOT] UART3 (Debug): 115200 baud
[BOOT] Starting FreeRTOS initialization...
[BOOT] LED effects initialized
[BOOT] Print task initialized
[BOOT] ESP8266 comm initialized (stream buffer created)
[BOOT] ESP8266_Comm task created
[BOOT] Watchdog initialized
[BOOT] Starting FreeRTOS scheduler NOW...
========================================

[PRINT_TASK] Debug logging initialized on UART3
[WATCHDOG] Initialized
[WATCHDOG] Monitor task started
[WATCHDOG] Registered 'Print_Task' (ID=0, timeout=5000ms)
[WATCHDOG] Registered 'ESP8266_Comm' (ID=1, timeout=5000ms)
```

#### Runtime Operation

**LED Command Processing:**
```
[ESP8266] ← Received: 'LED_CMD:1'
[LED] Pattern 1: All LEDs ON
[ESP8266] → Sent: 'OK:Pattern1'

[ESP8266] ← Received: 'LED_CMD:2'
[LED] Pattern 2: Different Frequency Blink
[LED] Green LED: 100ms period
[LED] Orange LED: 1000ms period
[ESP8266] → Sent: 'OK:Pattern2'

[ESP8266] ← Received: 'LED_CMD:3'
[LED] Pattern 3: Same Frequency Blink
[LED] Green + Orange: 100ms period
[ESP8266] → Sent: 'OK:Pattern3'

[ESP8266] ← Received: 'LED_CMD:4'
[LED] All LEDs OFF
[ESP8266] → Sent: 'OK:AllOFF'
```

**PING/PONG Connection Monitoring:**
```
[ESP8266] ← Received: 'PING'
[ESP8266] → Sent: 'PONG'

[ESP8266] Sending STM32_PING...
[ESP8266] Waiting for STM32_PONG response...
[ESP8266] ← Received: 'STM32_PONG'
[ESP8266] ✓ Connection confirmed (latency: 8ms)
```

**Watchdog Monitoring (Normal Operation):**
```
[WATCHDOG] Print_Task: Last feed 1245ms ago (OK)
[WATCHDOG] ESP8266_Comm: Last feed 892ms ago (OK)
```

**Watchdog Alert (Connection Lost):**
```
*** WATCHDOG ALERT ***
Task: ESP8266_Comm (ID=1)
Last feed: 5234 ms ago
Timeout: 5000 ms
Status: HUNG/DEADLOCK SUSPECTED
***********************
```

**Buffer Overflow Protection:**
```
[ESP8266] WARNING: Stream buffer usage high (72/128 bytes)
[ESP8266] ERROR: Stream buffer overflow! (128 bytes full)
[ESP8266] Discarding oldest data to prevent deadlock
```

**UART Retry Logic:**
```
[ESP8266] Failed to send PONG (attempt 1/3)
[ESP8266] Retry delay: 10ms
[ESP8266] Failed to send PONG (attempt 2/3)
[ESP8266] Retry delay: 10ms
[ESP8266] ✓ PONG sent successfully (attempt 3/3)
```

---

## 📡 Communication Protocol

### UART2 Messages (ESP8266 ↔ STM32)

**Received from ESP8266:**
| Message | Action | Response |
|---------|--------|----------|
| `LED_CMD:1\r\n` | Set Pattern 1 (All LEDs ON) | `OK:Pattern1\r\n` |
| `LED_CMD:2\r\n` | Set Pattern 2 (Different Freq) | `OK:Pattern2\r\n` |
| `LED_CMD:3\r\n` | Set Pattern 3 (Same Freq) | `OK:Pattern3\r\n` |
| `LED_CMD:4\r\n` | All LEDs OFF | `OK:AllOFF\r\n` |
| `PING\r\n` | Connection test from ESP8266 | `PONG\r\n` |
| `STM32_PONG\r\n` | Response to STM32's ping | (No response) |

**Sent to ESP8266:**
| Message | Frequency | Purpose |
|---------|-----------|---------|
| `STM32_PING\r\n` | 10s + (0-2s jitter) | Connection health check |
| `PONG\r\n` | On demand (response to PING) | Acknowledge ESP8266 alive |

### UART3 Debug Logs

**Log Categories:**
- `[BOOT]` - Boot sequence messages
- `[PRINT_TASK]` - Print task initialization
- `[WATCHDOG]` - Watchdog events (registration, alerts)
- `[ESP8266]` - UART2 communication events (RX/TX, PING/PONG)
- `[LED]` - LED pattern changes
- `ERROR:` - Error conditions (UART failures, buffer overflows)

---

## 🧩 Module Descriptions

### esp8266_comm_task.c

**Purpose:** Manages all UART2 communication with ESP8266 Wi-Fi module.

**Key Features:**
- Stream buffer for ISR-to-task RX (128 bytes)
- UART retry logic (3 attempts, 10ms delay between retries)
- Random PING jitter (0-2000ms) to avoid TX collisions
- Buffer overflow protection
- Line-based command parsing

**API:**
```c
void esp8266_comm_task_init(void);
void esp8266_comm_task_handler(void *parameters);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);  // ISR callback
```

### print_task.c

**Purpose:** Thread-safe debug logging to UART3 via message queue.

**Key Features:**
- Exclusive UART3 ownership (prevents concurrent HAL_UART_Transmit calls)
- Non-blocking API for application tasks
- Message queue (5 entries × 256 bytes)
- Watchdog monitored (5000ms timeout)

**API:**
```c
void print_task_init(void);
BaseType_t print_message(const char *message);
BaseType_t print_char(char c);
void print_task_handler(void *parameters);
```

**Usage Example:**
```c
print_message("[APP] System initialized\r\n");

char buffer[64];
snprintf(buffer, sizeof(buffer), "[SENSOR] Temp: %d°C\r\n", temp);
print_message(buffer);
```

### watchdog.c

**Purpose:** Detects hung or deadlocked tasks.

**Key Features:**
- Monitors up to 3 tasks (configurable)
- Per-task timeout configuration
- Alerts via print_task when timeout exceeded
- No-allocation design (static task array)

**API:**
```c
void watchdog_init(void);
watchdog_id_t watchdog_register(const char *task_name, uint32_t timeout_ms);
void watchdog_feed(watchdog_id_t id);
```

**Usage Example:**
```c
// In task initialization:
watchdog_id_t my_id = watchdog_register("MyTask", 5000);  // 5s timeout

// In task loop:
while(1) {
    // Do work...
    watchdog_feed(my_id);  // Prove task is alive
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```

### led_effects.c

**Purpose:** Controls 4 LED patterns using FreeRTOS software timers.

**LED Mapping:**
- **LD4 (Green)** - PD12
- **LD3 (Orange)** - PD13
- **LD5 (Red)** - PD14
- **LD6 (Blue)** - PD15

**Patterns:**
| Pattern | Description | LEDs Active |
|---------|-------------|-------------|
| `LED_PATTERN_1` | All LEDs ON (solid) | All 4 |
| `LED_PATTERN_2` | Different frequency blink | Green (100ms), Orange (1000ms) |
| `LED_PATTERN_3` | Same frequency blink | Green + Orange (100ms) |
| `LED_PATTERN_NONE` | All LEDs OFF | None |

**API:**
```c
void led_effects_init(void);
void led_effects_set_pattern(led_pattern_t pattern);
```

---

## ⚙️ Configuration

### FreeRTOSConfig.h

**Key Settings:**
```c
#define configTOTAL_HEAP_SIZE     ( ( size_t ) ( 50 * 1024 ) )  // 50 KB heap
#define configTICK_RATE_HZ        1000                          // 1ms tick
#define configMAX_PRIORITIES      5                             // 0-4 priority levels
#define configMINIMAL_STACK_SIZE  130                           // Words (520 bytes)
```

**Heap Size Rationale:**
- Original: 75 KB (caused crashes - memory exhaustion)
- Reduced to: **50 KB** (stable operation)
- Allows ~25 KB for task stacks and ISR stack

---

## 🐛 Debugging

### Enable Debug Logs

Debug logs are **always enabled** and sent to UART3. Connect USB-Serial adapter:
- **RX** → STM32 **PD8** (USART3 TX)
- **GND** → STM32 **GND**

Open serial terminal:
```
screen /dev/ttyUSB0 115200  # Linux
screen /dev/tty.usbserial-XXXX 115200  # macOS
```

### Common Issues

**1. No UART3 Output**
- **Cause:** Wrong pin (PB10 instead of PD8) or disconnected
- **Fix:** Verify wiring, check `.ioc` pin assignment

**2. System Crashes After Flash**
```
Download verified successfully
Target is not responding, retrying...
```
- **Cause:** Memory exhaustion (heap + BSS + stacks > 128 KB)
- **Fix:** Check build output for BSS size, reduce heap if needed

**3. Watchdog Alerts**
```
*** WATCHDOG ALERT ***
Task: ESP8266_Comm (ID=1)
Last feed: 5234 ms ago
```
- **Cause:** Task blocked/hung
- **Fix:** Check UART2 for hardware issue, verify ESP8266 connected

**4. UART TX Errors**
```
[ESP8266] ERROR: Failed to send PONG
```
- **Cause:** UART busy (concurrent transmit)
- **Fix:** Retry logic already implemented (3 attempts), check for excessive traffic

---

## 📊 Performance Metrics

### Task Timing

| Event | Frequency | Duration |
|-------|-----------|----------|
| Watchdog check | 1000ms | <1ms |
| Print queue check | 2000ms | <1ms (blocking) |
| ESP8266 stream read | 100ms | <1ms (blocking) |
| STM32_PING transmission | 10s + (0-2s jitter) | ~1ms |

### Memory Footprint

**Code (.text):** ~35 KB
**Read-only data (.rodata):** Included in .text
**Initialized data (.data):** ~100 bytes
**Uninitialized data (.bss):** ~53 KB
- Print queue buffers: 5 × 256 = 1.28 KB
- UART stream buffer: 128 bytes
- Watchdog task array: 3 × ~50 bytes = 150 bytes
- UART RX buffer: 64 bytes

---

## 📚 References

- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [STM32F4 HAL User Manual](https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf)
- [STM32F407VG Datasheet](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [STM32CubeMX User Guide](https://www.st.com/resource/en/user_manual/um1718-stm32cubemx-for-stm32-configuration-and-initialization-c-code-generation-stmicroelectronics.pdf)

---

## 📝 License

MIT License - See [LICENSE](../LICENSE) for details.
