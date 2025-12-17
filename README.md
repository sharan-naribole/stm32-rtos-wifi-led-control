# STM32 FreeRTOS Wi-Fi LED Controller

![STM32](https://img.shields.io/badge/STM32-03234B?style=flat&logo=stmicroelectronics&logoColor=white)
![FreeRTOS](https://img.shields.io/badge/FreeRTOS-00979D?style=flat&logo=freertos&logoColor=white)
![ESP8266](https://img.shields.io/badge/ESP8266-000000?style=flat&logo=espressif&logoColor=white)
![Arduino](https://img.shields.io/badge/Arduino-00979D?style=flat&logo=Arduino&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Status](https://img.shields.io/badge/status-active-success.svg)

IoT LED controller demonstrating embedded systems best practices: STM32F407 with FreeRTOS, ESP8266 Wi-Fi bridge, dual-UART architecture, stream buffers, watchdog monitoring, UART retry logic, collision-free PING/PONG, thread-safe logging, and RESTful web API with ACK tracking.

---

## 🎯 Project Overview

This project showcases a complete IoT LED control system combining STM32F407 microcontroller running FreeRTOS with ESP8266 Wi-Fi module. It demonstrates professional embedded systems engineering with robust error handling, real-time task management, and wireless web interface control.

### Key Features

**STM32 FreeRTOS Firmware:**
- ✅ **Dual-UART Architecture** - Separate ESP8266 communication (UART2) and debug logging (UART3)
- ✅ **Thread-Safe Logging** - Dedicated print task with message queue for UART3
- ✅ **Watchdog Monitoring** - Detects hung/deadlocked tasks with configurable timeouts
- ✅ **Stream Buffer RX** - Efficient ISR-to-task communication for UART2
- ✅ **UART Retry Logic** - Prevents dropped messages with automatic retry (3 attempts)
- ✅ **Bidirectional PING/PONG** - Connection health monitoring with random jitter
- ✅ **LED Pattern Control** - 4 patterns via software timers
- ✅ **Memory Optimized** - 50KB heap, stable operation with watchdog + print task

**ESP8266 Wi-Fi Bridge:**
- ✅ **mDNS Support** - Access via `esp8266-led.local` instead of IP addresses
- ✅ **Web Server** - Responsive HTML interface with auto-refresh
- ✅ **RESTful API** - JSON endpoints for pattern control and status
- ✅ **Request Tracking** - Circular buffer storing last 10 requests with IP/device info
- ✅ **ACK Status Display** - Real-time STM32 acknowledgment tracking on webpage
- ✅ **Device Detection** - Automatic identification of client devices
- ✅ **Collision Prevention** - Random ping jitter (0-2s) avoids UART conflicts

---

## 📸 Web Interface

![Web Interface](docs/images/web-interface.png)
*Real-time LED control with request history and ACK status tracking*

---

## 🏗️ System Architecture

### High-Level Overview

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         CLIENT DEVICES                                  │
│              (Laptop, Smartphone, Tablet, Desktop)                      │
│                    HTTP GET /pattern?p=<1-4>                           │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │ Wi-Fi (2.4 GHz)
                                 ↓
┌─────────────────────────────────────────────────────────────────────────┐
│       ESP8266 NodeMCU (Wi-Fi Bridge) - esp8266-led.local               │
│  ┌──────────────┐    ┌───────────────┐    ┌────────────────────────┐  │
│  │  Web Server  │ ←→ │ Request Track │ ←→ │  SoftwareSerial TX/RX │  │
│  │  (Port 80)   │    │  (10 entries) │    │   (115200 baud)        │  │
│  │  + mDNS      │    │ + ACK Display │    │                        │  │
│  └──────────────┘    └───────────────┘    └────────────────────────┘  │
└────────────────────────────────┬────────────────────────────────────────┘
                                 │ UART (LED_CMD:x, PING/PONG)
                                 ↓
┌─────────────────────────────────────────────────────────────────────────┐
│                   STM32F407 Discovery (FreeRTOS)                        │
│  ┌────────────────┐  ┌──────────────┐  ┌─────────────┐  ┌───────────┐ │
│  │ ESP8266_Comm   │  │  Print Task  │  │  Watchdog   │  │ LED       │ │
│  │ Task (Pri 2)   │  │  (Pri 3)     │  │  (Pri 4)    │  │ Effects   │ │
│  │ - UART2 RX/TX  │→│ - UART3 TX   │←│ - Monitor   │  │ Timers    │ │
│  │ - Stream Buf   │  │ - Msg Queue  │  │ - Alerts    │  │ (4 patt)  │ │
│  └────────────────┘  └──────────────┘  └─────────────┘  └───────────┘ │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  UART Allocation:                                                │  │
│  │  • UART2 (PA2/PA3): ESP8266 communication                        │  │
│  │  • UART3 (PD8/PB11): Debug logging to serial terminal            │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

### Communication Protocol

**UART2 (ESP8266 ↔ STM32):**
- `LED_CMD:1` → STM32: Pattern 1 (All LEDs ON)
- `LED_CMD:2` → STM32: Pattern 2 (Different Freq Blink)
- `LED_CMD:3` → STM32: Pattern 3 (Same Freq Blink)
- `LED_CMD:4` → STM32: Pattern 4 (All LEDs OFF)
- `OK:Pattern1` → ESP8266: Acknowledgment
- `PING` ↔ `PONG`: Bidirectional connection test (10s + 0-2s jitter)
- `STM32_PING` ↔ `STM32_PONG`: STM32-initiated connection test

**UART3 (STM32 → Serial Terminal):**
- All debug messages via print_task
- Watchdog alerts
- PING/PONG status
- LED pattern changes
- Error logging

---

## 🔧 Hardware Setup

### Bill of Materials

| Component | Quantity | Notes |
|-----------|----------|-------|
| STM32F407VG Discovery Board | 1 | 168 MHz Cortex-M4, 1MB Flash, 192KB RAM |
| ESP8266 NodeMCU | 1 | Wi-Fi module with SoftwareSerial |
| USB-to-Serial Adapter | 1 | For STM32 UART3 debug output |
| Jumper Wires | 6 | 3.3V logic compatible |

### Wiring Connections

**ESP8266 ↔ STM32 (UART2):**
```
ESP8266 NodeMCU          STM32F407 Discovery
─────────────────        ────────────────────
D1 (GPIO5) TX     ───>   PA3 (USART2 RX)
D2 (GPIO4) RX     <───   PA2 (USART2 TX)
GND               ───>   GND
```

**USB-Serial Adapter ↔ STM32 (UART3 - Debug):**
```
USB-Serial Adapter       STM32F407 Discovery
──────────────────       ────────────────────
RX                <───   PD8 (USART3 TX)
TX (optional)     ───>   PB11 (USART3 RX)
GND               ───>   GND
```

**Power:**
- STM32: USB cable (ST-Link)
- ESP8266: USB cable or 3.3V from breadboard power supply

---

## 🚀 Quick Start

### 1. STM32 Firmware Setup

**Prerequisites:**
- STM32CubeIDE (v1.10.0+)
- STM32CubeMX (for `.ioc` file configuration)

**Build & Flash:**
```bash
cd stm32-firmware
# Open in STM32CubeIDE
# Project → Build Project
# Run → Debug (F11)
```

See [`stm32-firmware/README.md`](stm32-firmware/README.md) for detailed instructions.

### 2. ESP8266 Firmware Setup

**Prerequisites:**
- Arduino IDE (v1.8.x or v2.x)
- ESP8266 Board Package

**Configure & Upload:**
1. Open `esp8266-firmware/ESP8266_LED_WebServer.ino`
2. **Edit Wi-Fi credentials:**
   ```cpp
   const char* WIFI_SSID = "YOUR_WIFI_SSID";
   const char* WIFI_PASSWORD = "YOUR_PASSWORD";
   ```
3. Tools → Board → ESP8266 NodeMCU
4. Sketch → Upload

See [`esp8266-firmware/README.md`](esp8266-firmware/README.md) for detailed instructions.

### 3. Connect Hardware

Follow wiring diagram above, then power on both boards.

### 4. Access Web Interface

**Method 1: Using mDNS (Recommended)**
1. Open browser: `http://esp8266-led.local/`
2. Click LED pattern buttons
3. Monitor UART3 for STM32 debug logs

**Method 2: Using IP Address**
1. Open ESP8266 Serial Monitor (115200 baud)
2. Find IP address in output: `[WIFI] Connected | IP: 192.168.x.x`
3. Open browser: `http://192.168.x.x/`
4. Click LED pattern buttons
5. Monitor UART3 for STM32 debug logs

---

## 📊 System Performance

### Memory Usage (STM32F407)

| Resource | Usage | Available | Utilization |
|----------|-------|-----------|-------------|
| Flash | 35 KB | 1 MB | 3.4% |
| RAM (BSS) | 53 KB | 128 KB | 41.4% |
| Heap (FreeRTOS) | 50 KB | - | Configured |
| Stack | ~25 KB | - | Remaining |

### Task Configuration

| Task | Priority | Stack | Timeout | Status |
|------|----------|-------|---------|--------|
| Watchdog | 4 | 256 words | N/A | Monitoring |
| Print_Task | 3 | 384 words | 5000ms | ✅ Registered |
| ESP8266_Comm | 2 | 256 words | 5000ms | ✅ Registered |
| Timer Service | 2 | Default | N/A | Active |
| Idle | 0 | Minimal | N/A | Sleep mode |

---

## 🐛 Troubleshooting

### No UART3 Output

**Check:**
- UART3 TX pin: **PD8** (not PB10!)
- USB-Serial adapter connected: PD8 → RX, GND → GND
- Serial terminal: 115200 baud, 8N1
- Correct serial port selected

### ESP8266 Connection Alerts

**Symptom:** `[ESP8266] ✗ ALERT: No STM32_PONG response!`

**Solutions:**
- Verify wiring: TX → RX crossed
- Check GND connected
- Ensure both at 115200 baud
- Verify ESP8266 firmware uploaded

### LED Commands Not Working

**Check UART3 logs for:**
- `[ESP8266] ← Received: 'LED_CMD:X'`
- `[LED] Pattern X: ...`
- `ERROR: Failed to send ACK` (indicates UART collision)

### STM32 Crashes on Boot

**If system hangs after flash:**
- Check heap size in `FreeRTOSConfig.h` (should be 50KB)
- Verify print_task and watchdog memory allocation
- Review build output for BSS size (<64KB recommended)

---

## 📚 Documentation

- **[STM32 Firmware README](stm32-firmware/README.md)** - FreeRTOS architecture, task details, API reference, serial output examples
- **[ESP8266 Firmware README](esp8266-firmware/README.md)** - Web server, REST API, request tracking, serial monitor logs
- **[STM32CubeMX Configuration](stm32-firmware/STM32CUBEMX_CONFIGURATION.md)** - Peripheral setup guide
- **[Hardware Setup Guide](docs/hardware-setup.md)** - Detailed wiring, pin configuration, and hardware troubleshooting
- **[Architecture Deep Dive](docs/architecture.md)** - System design decisions, issues faced, and performance optimizations

---

## 🎓 Learning Outcomes

This project demonstrates:

**Embedded Systems:**
- FreeRTOS task management and scheduling
- Hardware abstraction layer (HAL) usage
- Interrupt-driven UART with stream buffers
- Memory optimization techniques
- Watchdog timer implementation

**Communication Protocols:**
- Dual-UART architecture
- Thread-safe message passing (queues)
- Bidirectional health monitoring (PING/PONG)
- Collision prevention with random jitter
- Error handling and retry logic

**Web Development:**
- RESTful API design
- Responsive HTML/CSS/JavaScript
- AJAX for real-time updates
- Client tracking and analytics

**Debugging & Reliability:**
- Debug logging strategies
- Deadlock detection
- Error visibility (UART3 logs)
- Production-ready error handling

---

## 🔮 Future Enhancements

- [ ] Add OTA (Over-The-Air) firmware updates for ESP8266
- [ ] Implement RGB LED support with color picker
- [ ] Add MQTT integration for cloud connectivity
- [ ] Create mobile app (React Native / Flutter)
- [ ] Add authentication to web interface
- [ ] Implement data logging to SD card
- [ ] Add temperature/humidity sensor integration

---

## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- **STMicroelectronics** - STM32CubeMX, HAL library, STM32F4 Discovery board
- **FreeRTOS** - Real-time operating system kernel
- **Espressif** - ESP8266 Wi-Fi module and SDK
- **Arduino Community** - ESP8266 board package and examples

---

## 📧 Contact

For questions, issues, or contributions, please open an issue on GitHub.
