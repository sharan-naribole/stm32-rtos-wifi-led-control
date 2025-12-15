# Hardware Setup Guide

Complete hardware setup instructions for the STM32 FreeRTOS Wi-Fi LED Controller project.

---

## 📦 Bill of Materials

### Required Components

| Component | Quantity | Specifications | Purpose |
|-----------|----------|----------------|---------|
| **STM32F407VG Discovery Board** | 1 | 168 MHz Cortex-M4, 1MB Flash, 192KB RAM | Main microcontroller with FreeRTOS |
| **ESP8266 NodeMCU** | 1 | ESP-12E module, Wi-Fi 802.11 b/g/n (2.4 GHz) | Wi-Fi-to-UART bridge |
| **USB-to-Serial Adapter** | 1 | 3.3V TTL, CP2102/CH340/FTDI | STM32 UART3 debug output |
| **USB Cable (Mini-B)** | 1 | Data transfer capable | STM32 ST-Link programming |
| **USB Cable (Micro-B)** | 1 | Data transfer capable | ESP8266 programming & Serial Monitor |
| **Jumper Wires (Male-Male)** | 3 | 3.3V logic compatible | ESP8266 ↔ STM32 UART connection |
| **Breadboard** | 1 | Optional | For organizing connections |

### Optional Components

| Component | Purpose |
|-----------|---------|
| **Logic Analyzer** | Debug UART protocol issues |
| **USB Hub (Powered)** | Power both boards + USB-Serial adapter |
| **Oscilloscope** | Measure UART signal integrity |

---

## 🔌 Pin Configuration

### STM32F407VG Discovery

**UART2 (ESP8266 Communication):**
| Pin | Function | Connection |
|-----|----------|------------|
| **PA2** | USART2 TX | → ESP8266 D2 (GPIO4) RX |
| **PA3** | USART2 RX | ← ESP8266 D1 (GPIO5) TX |

**UART3 (Debug Output):**
| Pin | Function | Connection |
|-----|----------|------------|
| **PD8** | USART3 TX | → USB-Serial Adapter RX |
| **PB11** | USART3 RX | ← USB-Serial Adapter TX (optional) |

**LED Outputs:**
| Pin | LED | Color | Function |
|-----|-----|-------|----------|
| **PD12** | LD4 | Green | Pattern indicator |
| **PD13** | LD3 | Orange | Pattern indicator |
| **PD14** | LD5 | Red | Pattern indicator |
| **PD15** | LD6 | Blue | Pattern indicator |

**Power & Ground:**
| Pin | Function |
|-----|----------|
| **GND** | Common ground (multiple pins available) |
| **3V3** | 3.3V output (DO NOT use for powering ESP8266 - insufficient current) |

### ESP8266 NodeMCU

**SoftwareSerial (STM32 Communication):**
| Pin Label | GPIO | Function | Connection |
|-----------|------|----------|------------|
| **D1** | GPIO5 | SoftwareSerial TX | → STM32 PA3 (USART2 RX) |
| **D2** | GPIO4 | SoftwareSerial RX | ← STM32 PA2 (USART2 TX) |

**Hardware Serial (USB Debug):**
| Pin Label | GPIO | Function | Connection |
|-----------|------|----------|------------|
| **RX** | GPIO3 | UART0 RX | USB (Serial Monitor) |
| **TX** | GPIO1 | UART0 TX | USB (Serial Monitor) |

**Power:**
| Pin | Function | Notes |
|-----|----------|-------|
| **VIN** | 5V input | Connect to USB 5V or external 5V supply |
| **3V3** | 3.3V output | Onboard regulator (max 500mA) |
| **GND** | Ground | Common ground with STM32 |

### USB-to-Serial Adapter

| Pin | Connection | Notes |
|-----|------------|-------|
| **RX** | STM32 PD8 (USART3 TX) | Required for debug output |
| **TX** | STM32 PB11 (USART3 RX) | Optional (not used in this project) |
| **GND** | STM32 GND | Critical for UART operation |
| **VCC** | Leave disconnected | STM32 powered via ST-Link USB |

---

## 🔧 Step-by-Step Wiring

### Step 1: ESP8266 ↔ STM32 UART Connection

**⚠️ CRITICAL: Cross TX/RX connections**

```
ESP8266 NodeMCU                    STM32F407 Discovery
─────────────────                  ────────────────────
      D1 (GPIO5) TX ──────────────> PA3 (USART2 RX)
      D2 (GPIO4) RX <────────────── PA2 (USART2 TX)
      GND ──────────────────────────> GND
```

**Wiring Instructions:**
1. Connect **ESP8266 D1** to **STM32 PA3** (jumper wire)
2. Connect **ESP8266 D2** to **STM32 PA2** (jumper wire)
3. Connect **ESP8266 GND** to **STM32 GND** (jumper wire)

**Pin Locations:**
- **STM32 PA2/PA3**: Located on the outer header (labeled P1), top-left section
- **ESP8266 D1/D2**: Labeled on the NodeMCU silkscreen (left side)
- **STM32 GND**: Multiple GND pins available on all headers

### Step 2: USB-to-Serial Adapter ↔ STM32 Debug Output

```
USB-Serial Adapter                 STM32F407 Discovery
──────────────────                 ────────────────────
      RX ────────────────────────> PD8 (USART3 TX)
      TX (optional) <────────────── PB11 (USART3 RX)
      GND ──────────────────────────> GND
```

**Wiring Instructions:**
1. Connect **Adapter RX** to **STM32 PD8** (jumper wire)
2. Connect **Adapter GND** to **STM32 GND** (jumper wire)
3. Leave **Adapter VCC** disconnected (STM32 powered via ST-Link)
4. Leave **Adapter TX** disconnected (UART3 is output-only in this project)

**⚠️ Common Mistake:** Using **PB10** instead of **PD8**. UART3 TX is **PD8**, not PB10!

**Pin Locations:**
- **STM32 PD8**: Located on header P2 (bottom-right section)
- **STM32 PB11**: Located on header P1 (top-left section) - optional

### Step 3: Power Connections

**STM32F407 Discovery:**
- Connect **Mini-USB cable** to ST-Link connector (top of board)
- Plug USB into computer
- Green LED (LD1) should illuminate
- This powers the entire STM32 board

**ESP8266 NodeMCU:**
- Connect **Micro-USB cable** to onboard USB connector
- Plug USB into computer or powered USB hub
- Blue LED (GPIO2) may flash during boot
- This powers the ESP8266 and enables Serial Monitor

**USB-to-Serial Adapter:**
- Connect **Micro-USB or USB-A cable** (depending on adapter type)
- Plug USB into computer
- Adapter LED should illuminate (if present)
- This powers the adapter for Serial Monitor

---

## 📐 Complete Wiring Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                            COMPUTER                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────────────┐ │
│  │ STM32CubeIDE │  │ Arduino IDE  │  │ Serial Terminal (screen)      │ │
│  │ (Flash/Debug)│  │ (Upload/Mon) │  │ (UART3 Debug)                 │ │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────────────────────┘ │
│         │                 │                  │                          │
└─────────┼─────────────────┼──────────────────┼──────────────────────────┘
          │                 │                  │
          │ USB             │ USB              │ USB
          │ Mini-B          │ Micro-B          │ (CP2102/CH340)
          │                 │                  │
          ↓                 ↓                  ↓
┌────────────────────┐ ┌────────────────────┐ ┌─────────────────────┐
│  STM32F407         │ │   ESP8266          │ │ USB-Serial Adapter  │
│  Discovery         │ │   NodeMCU          │ │ (3.3V TTL)          │
│                    │ │                    │ │                     │
│  ┌──────────────┐  │ │  ┌──────────────┐  │ │  RX ───────────────┼──┐
│  │   ST-Link    │  │ │  │  ESP-12E     │  │ │  TX (not used)     │  │
│  │   (Program)  │  │ │  │  Wi-Fi       │  │ │  GND ──────────────┼──┼──┐
│  └──────────────┘  │ │  └──────────────┘  │ └─────────────────────┘  │  │
│                    │ │                    │                          │  │
│  PA2 (UART2 TX)────┼─┼──> D2 (GPIO4 RX)  │                          │  │
│  PA3 (UART2 RX) <──┼─┼──── D1 (GPIO5 TX)  │                          │  │
│  PD8 (UART3 TX)────┼─┼────────────────────┼──────────────────────────┘  │
│  GND ──────────────┼─┼──── GND            │                             │
│                    │ │                    │                             │
│  ┌──────────────┐  │ │                    │                             │
│  │  4x LEDs     │  │ │  Wi-Fi 2.4 GHz     │                             │
│  │  PD12-PD15   │  │ │  802.11 b/g/n      │                             │
│  └──────────────┘  │ │  (Client Mode)     │                             │
│                    │ │  HTTP Server :80   │ <───── Wi-Fi ──────> 📱💻  │
└────────────────────┘ └────────────────────┘           Router 192.168.x.x │
                                                                            │
                                          GND Common ──────────────────────┘
```

**Legend:**
- `───>` : UART transmit direction
- `<───` : UART receive direction
- `─────` : Power/ground connection
- `📱💻` : Mobile/desktop web clients

---

## ⚡ Power Considerations

### Current Draw

| Device | Idle Current | Active Current | Peak Current |
|--------|-------------|----------------|--------------|
| STM32F407 | ~50mA | ~100mA | ~150mA |
| ESP8266 (NodeMCU) | ~70mA | ~150mA | ~350mA (Wi-Fi TX) |
| USB-Serial Adapter | ~20mA | ~30mA | ~40mA |
| **Total** | ~140mA | ~280mA | ~540mA |

### Power Supply Options

**Option 1: Individual USB Ports (Recommended)**
- Each board powered by separate USB port
- Most reliable and safe
- No current sharing issues
- Total: 3 USB ports required

**Option 2: Powered USB Hub**
- All devices connected to powered USB hub
- Hub must support **>2A total current**
- Convenient for development
- Total: 1 USB port + powered hub

**❌ DO NOT:**
- Power ESP8266 from STM32 3V3 pin (insufficient current - max 100mA)
- Chain power between boards (creates ground loops)
- Use low-quality USB cables (voltage drop issues)

---

## 🔍 Signal Integrity

### UART Voltage Levels

**Both STM32 and ESP8266 use 3.3V logic:**
- Logic HIGH: 2.4V - 3.3V
- Logic LOW: 0V - 0.8V
- **Direct connection is SAFE** (no level shifters needed)

### Wiring Best Practices

**Keep UART wires short:**
- Maximum length: **30cm (12 inches)** recommended
- Longer wires increase capacitance and noise susceptibility
- At 115200 baud, bit time is ~8.7μs (vulnerable to reflections)

**Ground connection is critical:**
- Common GND provides voltage reference for UART signaling
- Without GND: Communication fails (random characters, timeouts)
- GND wire should be same length as TX/RX wires

**Avoid parallel routing:**
- Do not bundle UART wires with power wires
- Keep away from switching power supplies
- Minimize crosstalk between TX/RX lines

---

## ✅ Verification Checklist

### Before Powering On

- [ ] ESP8266 D1 (TX) connected to STM32 PA3 (RX) ✓
- [ ] ESP8266 D2 (RX) connected to STM32 PA2 (TX) ✓
- [ ] ESP8266 GND connected to STM32 GND ✓
- [ ] USB-Serial adapter RX connected to STM32 PD8 ✓
- [ ] USB-Serial adapter GND connected to STM32 GND ✓
- [ ] No 5V connections between boards (3.3V logic only) ✓
- [ ] All USB cables support data transfer (not power-only) ✓

### After Powering On

- [ ] STM32 green LED (LD1) illuminated ✓
- [ ] ESP8266 blue LED blinks during boot ✓
- [ ] USB-Serial adapter LED illuminated ✓
- [ ] No smoke, burning smell, or excessive heat ✓

### Firmware Upload

- [ ] STM32CubeIDE detects ST-Link debugger ✓
- [ ] STM32 firmware flashes successfully ✓
- [ ] Arduino IDE detects ESP8266 COM port ✓
- [ ] ESP8266 firmware uploads successfully ✓

### Serial Output

- [ ] STM32 UART3 shows boot sequence (115200 baud) ✓
- [ ] ESP8266 Serial Monitor shows Wi-Fi connection (115200 baud) ✓
- [ ] STM32 receives PING from ESP8266 (visible in UART3 logs) ✓
- [ ] ESP8266 receives PONG from STM32 (visible in Serial Monitor) ✓

### Web Interface

- [ ] ESP8266 connects to Wi-Fi network ✓
- [ ] Web browser can access ESP8266 IP address ✓
- [ ] LED pattern buttons respond (no HTTP errors) ✓
- [ ] STM32 UART3 shows LED_CMD messages ✓
- [ ] Web page displays ACK status (OK:Pattern1, etc.) ✓
- [ ] LEDs on STM32 board change patterns ✓

---

## 🐛 Hardware Troubleshooting

### No UART3 Output on Serial Terminal

**Symptom:** Serial terminal shows nothing, even after STM32 reset

**Solutions:**
1. **Verify pin connection:**
   - Measure voltage on STM32 PD8 with multimeter
   - Should toggle between 0V and 3.3V during operation
   - If stuck at 0V or 3.3V: Firmware not running or wrong pin

2. **Check baud rate:**
   - Must be **115200 baud, 8N1** (8 data bits, no parity, 1 stop bit)
   - Common mistake: Using 9600 baud (shows garbled characters)

3. **Verify USB-Serial adapter:**
   - Test adapter with loopback (short RX to TX)
   - Type in serial terminal - should echo back
   - If no echo: Driver issue or faulty adapter

4. **Check GND connection:**
   - Measure continuity between STM32 GND and adapter GND
   - Should read <1Ω resistance
   - If open circuit: Loose wire or bad breadboard contact

---

### ESP8266 Not Receiving STM32 Commands

**Symptom:** ESP8266 Serial Monitor shows "No PONG from STM32" alerts

**Solutions:**
1. **Verify TX/RX crossover:**
   - STM32 PA2 (TX) → ESP8266 D2 (RX) ✓
   - STM32 PA3 (RX) ← ESP8266 D1 (TX) ✓
   - Common mistake: Connecting TX→TX, RX→RX

2. **Check voltage levels:**
   - Measure STM32 PA2 (should toggle 0V-3.3V)
   - Measure ESP8266 D2 with oscilloscope during STM32 transmit
   - If no signal: Broken wire or loose connection

3. **Test loopback mode:**
   - On STM32: Short PA2 to PA3 (TX to RX)
   - Firmware should echo UART2 messages to UART3
   - If echo works: STM32 UART OK, problem is wiring/ESP8266

4. **Verify baud rate match:**
   - STM32: 115200 baud (hardcoded in firmware)
   - ESP8266: 115200 baud (defined in `STM32_BAUD_RATE`)
   - Mismatch causes garbled data

---

### STM32 Not Receiving ESP8266 Commands

**Symptom:** STM32 UART3 logs show no "Received: LED_CMD" messages from web interface

**Solutions:**
1. **Verify web interface:**
   - Check ESP8266 Serial Monitor for HTTP requests
   - Look for `[HTTP] GET /pattern?p=X`
   - If no HTTP requests: Network issue, wrong IP address

2. **Check UART transmission:**
   - ESP8266 Serial Monitor should show `[STM32] → Sending: LED_CMD:X [SENT]`
   - If missing: ESP8266 firmware issue or SoftwareSerial problem

3. **Verify wiring direction:**
   - ESP8266 D1 (TX) → STM32 PA3 (RX) ✓
   - Swap if reversed

4. **Check for buffer overflow:**
   - STM32 stream buffer is 128 bytes
   - Rapid commands can overflow buffer
   - Look for `ERROR: Stream buffer overflow` in UART3 logs

---

### Wi-Fi Connection Failed

**Symptom:** ESP8266 Serial Monitor shows "Connection Failed"

**Solutions:**
1. **Verify credentials:**
   - Check `WIFI_SSID` and `WIFI_PASSWORD` in ESP8266 code
   - Re-upload firmware after editing credentials

2. **Ensure 2.4 GHz network:**
   - ESP8266 does NOT support 5 GHz Wi-Fi
   - Check router settings for 2.4 GHz band

3. **Check signal strength:**
   - Move ESP8266 closer to router
   - RSSI should be > -70 dBm for reliable operation
   - Serial Monitor shows RSSI value after connection

4. **Router compatibility:**
   - Some routers block new device connections (MAC filtering)
   - Check router settings for "Allow new devices"
   - Disable AP isolation if enabled

---

### LEDs Not Changing Patterns

**Symptom:** STM32 receives LED_CMD but LEDs don't change

**Solutions:**
1. **Verify LED command logs:**
   - UART3 should show `[LED] Pattern X: <description>`
   - If missing: LED effects module not initialized

2. **Check LED visibility:**
   - Pattern 1: All LEDs solid ON (should be obvious)
   - Pattern 2: Green (100ms), Orange (1000ms) blinking
   - Pattern 3: Green + Orange (100ms) synchronized
   - Pattern 4: All LEDs OFF

3. **Test LED hardware:**
   - All 4 LEDs (LD3-LD6) should be visible on board
   - Located near USB connector on STM32 Discovery
   - If LEDs never worked: Hardware fault or GPIO config issue

---

### Intermittent UART Errors

**Symptom:** Occasional watchdog timeouts or missing ACKs

**Solutions:**
1. **Check power stability:**
   - Measure 3.3V rail on both boards with oscilloscope
   - Should be stable ±5% (3.13V - 3.47V)
   - Voltage droops indicate insufficient USB current

2. **Reduce UART traffic:**
   - PING interval is 10-12s (should not saturate UART)
   - If custom modifications: Verify command spacing

3. **Verify ground integrity:**
   - Measure AC voltage between STM32 GND and ESP8266 GND
   - Should be <50mV RMS
   - Higher values indicate ground loop or poor connection

4. **Check for EMI:**
   - Move boards away from switching power supplies
   - Use shielded USB cables if available
   - Add 100nF ceramic capacitor between 3.3V and GND (close to each IC)

---

## 🔧 Advanced Hardware Debugging

### Using a Logic Analyzer

**Recommended for:**
- Verifying UART protocol timing
- Debugging collision issues
- Measuring exact baud rate

**Connection:**
1. Connect logic analyzer CH0 to STM32 PA2 (TX to ESP8266)
2. Connect logic analyzer CH1 to ESP8266 D1 (TX to STM32)
3. Connect logic analyzer GND to common GND
4. Set trigger: UART, 115200 baud, 8N1
5. Capture during LED command

**What to look for:**
- Bit timing: Each bit should be 8.68μs (115200 baud)
- Start bit: Logic LOW (0V)
- Stop bit: Logic HIGH (3.3V)
- No collisions: TX lines should never transmit simultaneously
- Clean edges: Rise/fall time <1μs

### Using an Oscilloscope

**Recommended for:**
- Measuring signal integrity
- Detecting reflections or ringing
- Verifying voltage levels

**Setup:**
1. CH1: STM32 PA2 (UART2 TX)
2. CH2: ESP8266 D2 (UART2 RX - STM32's signal)
3. Trigger: Edge, CH1, rising edge, 1.65V threshold
4. Timebase: 10μs/div (captures ~2 UART bits)
5. Voltage: 1V/div (shows 0V-3.3V range)

**Expected waveform:**
- Clean square wave (minimal overshoot <10%)
- Rise/fall time: <500ns
- No ringing or oscillations
- CH2 should match CH1 (minimal delay/distortion)

---

## 📚 Component Datasheets

- [STM32F407VG Datasheet](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [ESP8266 Technical Reference](https://www.espressif.com/sites/default/files/documentation/esp8266-technical_reference_en.pdf)
- [CP2102 USB-UART Bridge Datasheet](https://www.silabs.com/documents/public/data-sheets/CP2102-9.pdf)

---

## 🔗 Related Documentation

- [Main README](../README.md) - Project overview
- [STM32 Firmware README](../stm32-firmware/README.md) - STM32 software details
- [ESP8266 Firmware README](../esp8266-firmware/README.md) - ESP8266 software details
- [Architecture Deep Dive](architecture.md) - System design decisions
