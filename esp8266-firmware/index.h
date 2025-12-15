/**
 ******************************************************************************
 * @file           : index.h
 * @brief          : HTML Web Interface for LED Control with Request Tracking
 ******************************************************************************
 * @description
 * This header file contains the HTML, CSS, and JavaScript for the web-based
 * LED control interface with request history tracking and ACK status display.
 *
 * Features:
 * - Responsive design (mobile-friendly)
 * - Modern gradient buttons
 * - AJAX requests for smooth interaction
 * - Status feedback with animations
 * - Request history with IP addresses and ACK status
 * - Auto-refresh every 5 seconds
 *
 * @note The PROGMEM keyword stores the HTML in flash memory instead of RAM,
 *       saving precious SRAM on the ESP8266.
 ******************************************************************************
 */

#ifndef INDEX_H
#define INDEX_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <title>STM32 LED Control</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }

    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 20px;
    }

    .container {
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
      padding: 40px 30px;
    }

    h1 {
      text-align: center;
      color: #333;
      margin-bottom: 10px;
      font-size: 28px;
    }

    .subtitle {
      text-align: center;
      color: #666;
      margin-bottom: 30px;
      font-size: 14px;
    }

    .button-grid {
      display: grid;
      gap: 15px;
    }

    button {
      padding: 20px;
      font-size: 18px;
      font-weight: 600;
      border: none;
      border-radius: 12px;
      cursor: pointer;
      transition: all 0.3s ease;
      color: white;
      box-shadow: 0 4px 15px rgba(0,0,0,0.2);
    }

    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 6px 20px rgba(0,0,0,0.3);
    }

    button:active {
      transform: translateY(0);
    }

    button:disabled {
      opacity: 0.5;
      cursor: not-allowed;
      transform: none !important;
    }

    .btn-pattern1 {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    }

    .btn-pattern2 {
      background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
    }

    .btn-pattern3 {
      background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
    }

    .btn-off {
      background: linear-gradient(135deg, #fa709a 0%, #fee140 100%);
    }

    #status {
      text-align: center;
      margin-top: 20px;
      padding: 15px;
      border-radius: 10px;
      font-weight: 600;
      display: none;
      animation: fadeIn 0.3s ease;
    }

    @keyframes fadeIn {
      from {
        opacity: 0;
        transform: translateY(-10px);
      }
      to {
        opacity: 1;
        transform: translateY(0);
      }
    }

    .success {
      background: #d4edda;
      color: #155724;
      border: 2px solid #c3e6cb;
    }

    .error {
      background: #f8d7da;
      color: #721c24;
      border: 2px solid #f5c6cb;
    }

    .pattern-icon {
      font-size: 24px;
      margin-right: 10px;
    }

    .info-box {
      background: #e7f3ff;
      border-left: 4px solid #2196F3;
      padding: 15px;
      margin-top: 20px;
      border-radius: 8px;
      font-size: 14px;
      color: #0c5460;
    }

    .requests-card {
      background: #f8fafc;
      border-radius: 15px;
      padding: 20px;
      margin-top: 20px;
      margin-bottom: 20px;
      border: 2px solid #e2e8f0;
    }

    .requests-card h2 {
      font-size: 16px;
      font-weight: 600;
      margin-bottom: 15px;
      color: #1e293b;
    }

    .request-item {
      background: white;
      border-radius: 10px;
      padding: 12px;
      margin-bottom: 10px;
      border: 1px solid #e2e8f0;
      font-size: 12px;
    }

    .request-item:last-child {
      margin-bottom: 0;
    }

    .request-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 8px;
    }

    .request-ip {
      font-weight: 600;
      color: #667eea;
      font-family: 'Courier New', monospace;
    }

    .request-time {
      font-size: 11px;
      color: #64748b;
    }

    .request-detail {
      color: #475569;
      margin: 4px 0;
      line-height: 1.4;
    }

    .request-detail strong {
      color: #1e293b;
    }

    .ack-status {
      display: inline-block;
      padding: 2px 8px;
      border-radius: 4px;
      font-size: 11px;
      font-weight: 600;
      margin-left: 5px;
    }

    .ack-ok {
      background: #d4edda;
      color: #155724;
    }

    .ack-pending {
      background: #fff3cd;
      color: #856404;
    }

    .ack-error {
      background: #f8d7da;
      color: #721c24;
    }

    .no-requests {
      text-align: center;
      color: #64748b;
      padding: 20px;
      font-style: italic;
    }

    .total-requests {
      text-align: center;
      font-size: 13px;
      color: #64748b;
      margin-top: 10px;
      padding-top: 10px;
      border-top: 1px solid #e2e8f0;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎨 LED Pattern Control</h1>
    <p class="subtitle">Control STM32F407 via Wi-Fi</p>

    <div class="button-grid">
      <button class="btn-pattern1" onclick="sendPattern('1')">
        <span class="pattern-icon">💡</span>All LEDs ON
      </button>
      <button class="btn-pattern2" onclick="sendPattern('2')">
        <span class="pattern-icon">⚡</span>Different Frequency Blink
      </button>
      <button class="btn-pattern3" onclick="sendPattern('3')">
        <span class="pattern-icon">✨</span>Same Frequency Blink
      </button>
      <button class="btn-off" onclick="sendPattern('4')">
        <span class="pattern-icon">🌙</span>All LEDs OFF
      </button>
    </div>

    <div id="status"></div>

    <div class="requests-card">
      <h2>Recent Requests & ACK Status</h2>
      <div id="requestsList">
        <div class="no-requests">Loading request history...</div>
      </div>
      <div class="total-requests" id="totalRequests"></div>
    </div>

    <div class="info-box">
      <strong>Connection:</strong> ESP8266 → STM32F407<br>
      <strong>Interface:</strong> UART 115200 baud<br>
      <strong>Auto-refresh:</strong> Every 5 seconds
    </div>
  </div>

  <script>
    let autoRefreshTimer = null;

    function sendPattern(pattern) {
      const statusDiv = document.getElementById('status');
      const patternNames = {
        '1': 'All LEDs ON',
        '2': 'Different Frequency Blink',
        '3': 'Same Frequency Blink',
        '4': 'All LEDs OFF'
      };

      const buttons = document.querySelectorAll('button');
      buttons.forEach(btn => btn.disabled = true);

      // Send HTTP request to ESP8266
      fetch('/pattern?p=' + pattern)
        .then(response => {
          if (!response.ok) throw new Error('Network error');
          return response.text();
        })
        .then(data => {
          // Show success message
          statusDiv.className = 'success';
          statusDiv.style.display = 'block';
          statusDiv.textContent = '✓ ' + patternNames[pattern] + ' activated!';
          setTimeout(() => statusDiv.style.display = 'none', 2500);

          // Update request history immediately
          setTimeout(updateRequests, 200);
        })
        .catch(error => {
          // Show error message
          statusDiv.className = 'error';
          statusDiv.style.display = 'block';
          statusDiv.textContent = '✗ Failed to send command';
          setTimeout(() => statusDiv.style.display = 'none', 2500);
        })
        .finally(() => {
          buttons.forEach(btn => btn.disabled = false);
        });
    }

    // Update request history
    function updateRequests() {
      fetch('/clients')
        .then(response => response.json())
        .then(data => {
          const requestsList = document.getElementById('requestsList');
          const totalRequests = document.getElementById('totalRequests');

          // Update total requests
          totalRequests.textContent = 'Total Requests: ' + data.totalRequests;

          // Clear current list
          requestsList.innerHTML = '';

          // Check if there are any requests
          if (data.recentRequests.length === 0) {
            requestsList.innerHTML = '<div class="no-requests">No requests yet</div>';
            return;
          }

          // Display each request
          data.recentRequests.forEach(request => {
            const requestDiv = document.createElement('div');
            requestDiv.className = 'request-item';

            // Parse device type from User-Agent
            let deviceType = 'Unknown';
            let deviceIcon = '🖥️';
            const ua = request.userAgent;

            if (ua.includes('iPhone')) {
              deviceType = 'iPhone';
              deviceIcon = '📱';
            } else if (ua.includes('iPad')) {
              deviceType = 'iPad';
              deviceIcon = '📱';
            } else if (ua.includes('Android')) {
              deviceType = 'Android';
              deviceIcon = '📱';
            } else if (ua.includes('Macintosh')) {
              deviceType = 'Mac';
              deviceIcon = '💻';
            } else if (ua.includes('Windows')) {
              deviceType = 'Windows PC';
              deviceIcon = '💻';
            } else if (ua.includes('Linux')) {
              deviceType = 'Linux';
              deviceIcon = '💻';
            }

            // Parse browser
            let browser = 'Unknown';
            if (ua.includes('Chrome') && !ua.includes('Edg')) {
              browser = 'Chrome';
            } else if (ua.includes('Safari') && !ua.includes('Chrome')) {
              browser = 'Safari';
            } else if (ua.includes('Firefox')) {
              browser = 'Firefox';
            } else if (ua.includes('Edg')) {
              browser = 'Edge';
            }

            // Parse ACK status
            let ackStatus = '';
            let ackClass = 'ack-pending';
            if (request.ack.startsWith('OK:')) {
              ackStatus = request.ack;
              ackClass = 'ack-ok';
            } else if (request.ack.startsWith('ERROR:')) {
              ackStatus = request.ack;
              ackClass = 'ack-error';
            } else if (request.ack === '') {
              ackStatus = 'Pending';
            } else {
              ackStatus = request.ack;
            }

            requestDiv.innerHTML = `
              <div class="request-header">
                <span class="request-ip">${deviceIcon} ${request.ip}</span>
                <span class="request-time">${request.uptime}</span>
              </div>
              <div class="request-detail"><strong>Device:</strong> ${deviceType} (${browser})</div>
              <div class="request-detail"><strong>Endpoint:</strong> ${request.endpoint}</div>
              <div class="request-detail"><strong>STM32 ACK:</strong> <span class="ack-status ${ackClass}">${ackStatus}</span></div>
            `;

            requestsList.appendChild(requestDiv);
          });
        })
        .catch(error => {
          console.error('Requests update failed:', error);
          document.getElementById('requestsList').innerHTML =
            '<div class="no-requests">Failed to load request history</div>';
        });
    }

    // Auto-update every 5 seconds
    function startAutoRefresh() {
      updateRequests();
      autoRefreshTimer = setInterval(() => {
        updateRequests();
      }, 5000);
    }

    // Initialize on page load
    window.addEventListener('load', () => {
      startAutoRefresh();
    });

    // Clean up on page unload
    window.addEventListener('beforeunload', () => {
      if (autoRefreshTimer) {
        clearInterval(autoRefreshTimer);
      }
    });
  </script>
</body>
</html>
)rawliteral";

#endif // INDEX_H
