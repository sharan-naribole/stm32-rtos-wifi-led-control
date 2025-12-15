/**
 ******************************************************************************
 * @file           : ESP8266_LED_WebServer.ino
 * @brief          : ESP8266 Wi-Fi-to-UART Bridge for STM32 LED Control
 ******************************************************************************
 * @description
 * This sketch turns the ESP8266 into a Wi-Fi-to-UART bridge, allowing wireless
 * control of STM32F407 LED patterns through a web interface.
 *
 * Architecture:
 * [Mobile Browser] --Wi-Fi--> [ESP8266 Web Server] --UART--> [STM32F407]
 *
 * Features:
 * - Web server on port 80 (HTTP)
 * - Responsive HTML interface
 * - UART communication at 115200 baud
 * - Automatic Wi-Fi connection with status monitoring
 * - RESTful API for pattern control
 *
 * Hardware Connections (SoftwareSerial):
 * - ESP8266 D1 (GPIO5)  → STM32 PA3 (USART2 RX)  - SoftwareSerial TX
 * - ESP8266 D2 (GPIO4)  → STM32 PA2 (USART2 TX)  - SoftwareSerial RX
 * - ESP8266 GND         → STM32 GND
 * - ESP8266 USB         → Computer (for Serial Monitor debug)
 * - Both boards use 3.3V logic (compatible)
 *
 * Serial Ports:
 * - Serial (USB):       Debug messages (WIFI_DEBUG:) - visible in Serial Monitor
 * - SoftwareSerial:     LED commands (LED_CMD:) - sent to STM32 only
 *
 * Web Interface:
 * - Homepage:        http://ESP8266_IP/
 * - Pattern control: http://ESP8266_IP/pattern?p=<1-4>
 *
 * UART Protocol:
 * - Baud rate: 115200
 * - Data bits: 8
 * - Parity: None
 * - Stop bits: 1
 *
 * Message Routing:
 * - LED_CMD:x   → SoftwareSerial → STM32 (LED pattern commands)
 * - WIFI_DEBUG: → Serial (USB) → Serial Monitor (debug messages)
 *
 * Command Format:
 * - LED_CMD:1  (enter LED menu)
 * - LED_CMD:x  (select pattern 1-4)
 *
 * Benefits:
 * - See Wi-Fi status and IP address in Serial Monitor
 * - Monitor LED commands being sent to STM32
 * - Debug issues without disconnecting
 * - STM32 receives only clean LED commands
 *
 * @author  Your Name
 * @date    December 2024
 ******************************************************************************
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SoftwareSerial.h>
#include "index.h"  // HTML web interface

// ========================================
// Configuration Section - CHANGE THESE!
// ========================================

/**
 * @brief Wi-Fi network credentials
 * @note Replace with your home Wi-Fi details
 */
const char* WIFI_SSID = "YOUR_WIFI_SSID";     // Your Wi-Fi network name
const char* WIFI_PASSWORD = "YOUR_PASSWORD";  // Your Wi-Fi password

/**
 * @brief Web server configuration
 */
const int HTTP_PORT = 80;                        // Standard HTTP port

/**
 * @brief UART configuration for STM32 communication
 */
const unsigned long STM32_BAUD_RATE = 115200;    // SoftwareSerial to STM32
const unsigned long DEBUG_BAUD_RATE = 115200;    // USB Serial for debug
const int COMMAND_DELAY_MS = 50;                 // Delay between commands
const unsigned long ECHO_PING_INTERVAL_MS = 10000; // UART connection test interval (10 seconds base)
const unsigned long ECHO_PING_JITTER_MS = 2000;  // Random jitter: 0-2000ms uniform distribution
const unsigned long ECHO_TIMEOUT_MS = 1000;      // Timeout for ECHO response

/**
 * @brief SoftwareSerial pin configuration
 * @note D1 = GPIO5 (TX to STM32), D2 = GPIO4 (RX from STM32)
 */
#define STM32_RX_PIN D2  // GPIO4 - ESP8266 receives from STM32
#define STM32_TX_PIN D1  // GPIO5 - ESP8266 sends to STM32

// ========================================
// Request Tracking Structure
// ========================================

/**
 * @struct RequestLog
 * @brief Structure to store HTTP request information
 */
struct RequestLog {
  String ip;                 // Client IP address
  String endpoint;           // Requested endpoint
  unsigned long timestamp;   // Request timestamp (millis)
  String userAgent;          // User-Agent header for device detection
  String ack;                // Last ACK received from STM32
};

// ========================================
// Global Objects
// ========================================

ESP8266WebServer server(HTTP_PORT);
SoftwareSerial stm32Serial(STM32_RX_PIN, STM32_TX_PIN);  // RX, TX

/**
 * @brief Circular buffer for recent requests
 */
#define MAX_REQUESTS 10
RequestLog recentRequests[MAX_REQUESTS];
int requestIndex = 0;
unsigned long totalRequests = 0;

/**
 * @brief UART connection status
 */
bool uartConnectionOK = true;
unsigned long lastEchoPing = 0;
unsigned long lastEchoReceived = 0;
bool waitingForEcho = false;

/**
 * @brief Last ACK received from STM32
 */
String lastAckReceived = "";

// ========================================
// Function Declarations
// ========================================

void setupWiFi();
void setupWebServer();
void handleRoot();
void handlePattern();
void handleClients();
void handleNotFound();
void sendCommandToSTM32(String pattern);
void logRequest(String endpoint);
void checkUARTConnection();
void processSTM32Response();

// ========================================
// Setup Function (Runs Once)
// ========================================

void setup() {
  // Initialize USB Serial for debug output
  Serial.begin(DEBUG_BAUD_RATE);
  delay(100);

  // Initialize SoftwareSerial for STM32 communication
  stm32Serial.begin(STM32_BAUD_RATE);
  delay(100);

  // Print startup banner to Serial Monitor
  Serial.println("\r\n\r\n");
  Serial.println("========================================");
  Serial.println("  ESP8266 LED Control Web Server");
  Serial.println("  SoftwareSerial Mode");
  Serial.println("========================================");
  Serial.println("Serial Monitor: Debug messages");
  Serial.println("SoftwareSerial: STM32 commands");
  Serial.println("========================================");

  // Connect to Wi-Fi network
  setupWiFi();

  // Configure and start web server
  setupWebServer();

  Serial.println("========================================");
  Serial.println("  System Ready!");
  Serial.println("========================================");
}

// ========================================
// Main Loop (Runs Forever)
// ========================================

void loop() {
  // Handle incoming HTTP requests
  server.handleClient();

  // Process any responses from STM32
  processSTM32Response();

  // Check UART connection periodically
  checkUARTConnection();

  // Wi-Fi connection monitoring
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) {  // Check every 30 seconds
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("\n[WIFI] Connection lost! Reconnecting...");
      setupWiFi();
    } else {
      // Print status update
      Serial.print("[WIFI] Connected | IP: ");
      Serial.print(WiFi.localIP());
      Serial.print(" | Signal: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    }
    lastCheck = millis();
  }
}

// ========================================
// Wi-Fi Setup Function
// ========================================

void setupWiFi() {
  Serial.println("\n[WIFI] Starting Wi-Fi connection...");
  Serial.print("[WIFI] SSID: ");
  Serial.println(WIFI_SSID);

  // Disconnect any previous connection
  WiFi.disconnect();
  delay(100);

  // Set Wi-Fi mode to station (client)
  WiFi.mode(WIFI_STA);

  // Set hostname (optional)
  WiFi.hostname("ESP8266-STM32-Bridge");

  // Begin Wi-Fi connection
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Wait for connection with timeout
  Serial.print("[WIFI] Connecting");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  // Check connection status
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WIFI] ✓ Connected successfully!");
    Serial.println("[WIFI] --------------------------------");
    Serial.print("[WIFI] IP Address:  ");
    Serial.println(WiFi.localIP());
    Serial.print("[WIFI] MAC Address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("[WIFI] Gateway:     ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("[WIFI] Subnet Mask: ");
    Serial.println(WiFi.subnetMask());
    Serial.print("[WIFI] Signal:      ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.println("[WIFI] --------------------------------");
  } else {
    Serial.println("[WIFI] ✗ Connection Failed!");
    Serial.println("[WIFI] Troubleshooting:");
    Serial.println("[WIFI] 1. Check Wi-Fi credentials");
    Serial.println("[WIFI] 2. Ensure 2.4 GHz Wi-Fi (NOT 5 GHz)");
    Serial.println("[WIFI] 3. Check if router is nearby");
    Serial.println("[WIFI] 4. Verify router allows new devices");
  }
}

// ========================================
// Web Server Setup Function
// ========================================

void setupWebServer() {
  Serial.println("\n[SERVER] Configuring web server...");

  // Register URL handlers
  server.on("/", HTTP_GET, handleRoot);
  server.on("/pattern", HTTP_GET, handlePattern);
  server.on("/clients", HTTP_GET, handleClients);
  server.onNotFound(handleNotFound);

  // Start server
  server.begin();

  Serial.println("[SERVER] Web server started on port " + String(HTTP_PORT));
  Serial.print("[SERVER] Access at: http://");
  Serial.println(WiFi.localIP());
}

// ========================================
// Handler: Serve Main HTML Page
// ========================================

void handleRoot() {
  logRequest("/");
  Serial.println("[HTTP] GET / - Serving homepage");
  server.send_P(200, "text/html", INDEX_HTML);
}

// ========================================
// Handler: Process Pattern Commands
// ========================================

void handlePattern() {
  // Check if pattern parameter exists
  if (!server.hasArg("p")) {
    Serial.println("[HTTP] GET /pattern - ERROR: Missing parameter");
    server.send(400, "text/plain", "ERROR: Missing 'p' parameter");
    return;
  }

  // Get pattern number
  String pattern = server.arg("p");

  // Validate pattern (must be 1-4)
  if (pattern != "1" && pattern != "2" && pattern != "3" && pattern != "4") {
    Serial.println("[HTTP] GET /pattern - ERROR: Invalid pattern");
    server.send(400, "text/plain", "ERROR: Invalid pattern (must be 1-4)");
    return;
  }

  Serial.println("[HTTP] GET /pattern?p=" + pattern);

  // Send commands to STM32 (this updates lastAckReceived)
  sendCommandToSTM32(pattern);

  // Log the request AFTER getting ACK from STM32
  // This ensures the correct ACK is captured in the request log
  String endpoint = "/pattern?p=" + pattern;
  logRequest(endpoint);

  // Send success response to browser
  String response = "Pattern " + pattern + " sent to STM32";
  server.send(200, "text/plain", response);

  Serial.println("[HTTP] Response sent to browser");
}

// ========================================
// Handler: Client Request History (JSON)
// ========================================

void handleClients() {
  Serial.println("[HTTP] GET /clients - Serving client history");

  // Build JSON response
  String json = "{\"totalRequests\":" + String(totalRequests) + ",\"recentRequests\":[";

  // Add recent requests in reverse order (newest first)
  bool firstEntry = true;
  for (int i = 0; i < MAX_REQUESTS; i++) {
    int idx = (requestIndex - 1 - i + MAX_REQUESTS) % MAX_REQUESTS;

    // Only include entries that have been populated
    if (recentRequests[idx].ip.length() > 0) {
      if (!firstEntry) json += ",";
      firstEntry = false;

      unsigned long uptime = (millis() - recentRequests[idx].timestamp) / 1000;

      json += "{";
      json += "\"ip\":\"" + recentRequests[idx].ip + "\",";
      json += "\"endpoint\":\"" + recentRequests[idx].endpoint + "\",";
      json += "\"userAgent\":\"" + recentRequests[idx].userAgent + "\",";
      json += "\"ack\":\"" + recentRequests[idx].ack + "\",";
      json += "\"uptime\":\"" + String(uptime) + "s ago\"";
      json += "}";
    }
  }

  json += "]}";

  // Send JSON response
  server.send(200, "application/json", json);
}

// ========================================
// Handler: 404 Not Found
// ========================================

void handleNotFound() {
  String message = "404: Page Not Found\n\n";
  message += "URI: " + server.uri() + "\n";
  message += "Method: " + String((server.method() == HTTP_GET) ? "GET" : "POST") + "\n";

  Serial.println("[HTTP] 404 - " + server.uri());
  server.send(404, "text/plain", message);
}

// ========================================
// Send Command to STM32 via UART
// ========================================

void sendCommandToSTM32(String pattern) {
  Serial.println("[STM32] Sending LED command...");

  // Clear previous ACK before sending new command
  lastAckReceived = "";

  // Send pattern command directly (no menu mode needed)
  Serial.print("[STM32] → Sending: LED_CMD:");
  Serial.print(pattern);
  stm32Serial.print("LED_CMD:");
  stm32Serial.println(pattern);
  Serial.println(" [SENT]");

  // Wait for ACK to arrive (max 500ms)
  // Process incoming data so ACK gets captured
  unsigned long startWait = millis();
  while (lastAckReceived.length() == 0 && (millis() - startWait < 500)) {
    processSTM32Response();  // Process incoming messages
    delay(10);  // Small delay to avoid tight loop
  }

  if (lastAckReceived.length() == 0) {
    Serial.println("[STM32] Warning: No ACK received");
  }
}

// ========================================
// Log Client Request (Circular Buffer)
// ========================================

void logRequest(String endpoint) {
  // Get client IP
  String clientIP = server.client().remoteIP().toString();

  // Get User-Agent header
  String userAgent = server.header("User-Agent");
  if (userAgent.length() == 0) {
    userAgent = "Unknown";
  }

  // Store in circular buffer
  recentRequests[requestIndex].ip = clientIP;
  recentRequests[requestIndex].endpoint = endpoint;
  recentRequests[requestIndex].timestamp = millis();
  recentRequests[requestIndex].userAgent = userAgent;
  recentRequests[requestIndex].ack = lastAckReceived;

  // Increment counters
  requestIndex = (requestIndex + 1) % MAX_REQUESTS;
  totalRequests++;

  // Log to Serial Monitor
  Serial.println("[CLIENT] --------------------------------");
  Serial.print("[CLIENT] IP: ");
  Serial.println(clientIP);
  Serial.print("[CLIENT] Endpoint: ");
  Serial.println(endpoint);
  Serial.print("[CLIENT] Total Requests: ");
  Serial.println(totalRequests);
  Serial.println("[CLIENT] --------------------------------");
}

// ========================================
// Check UART Connection (PING/PONG Test)
// ========================================

void checkUARTConnection() {
  unsigned long now = millis();

  // Send PING every 10 seconds with random jitter (0-2000ms) to avoid collision with STM32 pings
  static unsigned long nextPingJitter = 0;
  if (lastEchoPing == 0) {
    // First ping - generate initial jitter
    nextPingJitter = random(0, ECHO_PING_JITTER_MS);
  }

  unsigned long pingIntervalWithJitter = ECHO_PING_INTERVAL_MS + nextPingJitter;
  if (now - lastEchoPing >= pingIntervalWithJitter) {
    lastEchoPing = now;

    Serial.println("[UART] --------------------------------");
    Serial.println("[UART] → Sending PING to STM32...");
    stm32Serial.println("PING");
    waitingForEcho = true;
    lastEchoReceived = now;

    // Generate new jitter for next ping
    nextPingJitter = random(0, ECHO_PING_JITTER_MS);
  }

  // Check for PONG timeout
  if (waitingForEcho && (now - lastEchoReceived > ECHO_TIMEOUT_MS)) {
    if (uartConnectionOK) {
      Serial.println("[UART] ✗ ALERT: No PONG from STM32!");
      Serial.println("[UART] UART connection may be broken");
      Serial.println("[UART] --------------------------------");
      uartConnectionOK = false;
    }
  }
}

// ========================================
// Process STM32 Responses
// ========================================

void processSTM32Response() {
  static String rxBuffer = "";

  while (stm32Serial.available()) {
    char c = stm32Serial.read();

    if (c == '\n' || c == '\r') {
      if (rxBuffer.length() > 0) {
        // Check for STM32_PING (connection test from STM32)
        if (rxBuffer.startsWith("STM32_PING")) {
          // Respond immediately with PONG
          stm32Serial.println("STM32_PONG");
          Serial.println("[UART] --------------------------------");
          Serial.println("[UART] ← STM32_PING received");
          Serial.println("[UART] → Sent STM32_PONG response");
          Serial.println("[UART] --------------------------------");
        }
        // Check for PONG (reply to our PING)
        else if (rxBuffer.startsWith("PONG")) {
          if (!uartConnectionOK) {
            Serial.println("[UART] ✓ UART connection restored!");
          }
          uartConnectionOK = true;
          waitingForEcho = false;
          Serial.println("[UART] ← PONG received");
          Serial.println("[UART] ✓ Connection confirmed");
          Serial.println("[UART] --------------------------------");
        }
        // Check for acknowledgments
        else if (rxBuffer.startsWith("OK:")) {
          lastAckReceived = rxBuffer;  // Save ACK for request tracking
          Serial.print("[STM32] ← ACK: ");
          Serial.println(rxBuffer);
        }
        // Check for errors
        else if (rxBuffer.startsWith("ERROR:")) {
          lastAckReceived = rxBuffer;  // Save ERROR as ACK for request tracking
          Serial.print("[STM32] ← ERROR: ");
          Serial.println(rxBuffer);
        }
        // Other messages
        else {
          Serial.print("[STM32] ← ");
          Serial.println(rxBuffer);
        }

        rxBuffer = "";
      }
    } else {
      rxBuffer += c;
      // Buffer overflow protection
      if (rxBuffer.length() > 128) {
        rxBuffer = "";
      }
    }
  }
}
