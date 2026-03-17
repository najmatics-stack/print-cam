// =============================================================
// HTML Pages served by the ESP32
// =============================================================
//
// These are stored as string constants in flash memory.
// R"rawliteral(...)rawliteral" is a C++ "raw string literal" -
// it lets us write multi-line strings with quotes and special
// characters without needing escape sequences.
//
// We have three pages:
//   1. WIFI_SETUP_HTML   - Captive portal for entering WiFi credentials
//   2. DASHBOARD_HTML    - Main camera dashboard with live stream
//   3. WIFI_SAVED_HTML   - Confirmation page after saving WiFi settings
// =============================================================

#ifndef HTML_CONTENT_H
#define HTML_CONTENT_H

// =============================================================
// WiFi Setup Page (shown when in AP mode)
// =============================================================
// This page is served when the camera is in Access Point mode.
// The user connects to "PrinterCam-Setup" WiFi and sees this
// form where they enter their home WiFi network credentials.
const char WIFI_SETUP_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>PrinterCam WiFi Setup</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #1a1a2e; color: #e0e0e0; padding: 20px;
        }
        .container { max-width: 400px; margin: 40px auto; }
        h1 { color: #00d4ff; margin-bottom: 8px; font-size: 24px; }
        p.sub { color: #888; margin-bottom: 24px; font-size: 14px; }
        form {
            background: #16213e; padding: 24px;
            border-radius: 12px;
        }
        label {
            display: block; margin-bottom: 6px;
            font-size: 14px; color: #aaa;
        }
        input[type="text"], input[type="password"] {
            width: 100%; padding: 12px; margin-bottom: 16px;
            border: 1px solid #333; border-radius: 8px;
            background: #0f3460; color: #fff; font-size: 16px;
        }
        input:focus { border-color: #00d4ff; outline: none; }
        button {
            width: 100%; padding: 14px;
            background: #00d4ff; color: #1a1a2e;
            border: none; border-radius: 8px;
            font-size: 16px; font-weight: bold; cursor: pointer;
        }
        button:hover { background: #00b8d9; }
        .note {
            margin-top: 16px; font-size: 12px;
            color: #666; text-align: center;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>PrinterCam Setup</h1>
        <p class="sub">Connect your printer camera to WiFi</p>
        <form action="/save-wifi" method="POST">
            <label for="ssid">WiFi Network Name (SSID)</label>
            <input type="text" id="ssid" name="ssid"
                   placeholder="Your WiFi network" required>
            <label for="password">WiFi Password</label>
            <input type="password" id="password" name="password"
                   placeholder="Your WiFi password">
            <button type="submit">Connect</button>
        </form>
        <p class="note">
            The camera will reboot and connect to your network.<br>
            Check the Serial Monitor for the assigned IP address.
        </p>
    </div>
</body>
</html>
)rawliteral";


// =============================================================
// Main Dashboard Page (shown when connected to WiFi)
// =============================================================
// This is the main interface you see when you open the camera's
// IP address in a browser. It shows:
//   - Live MJPEG video stream from the camera
//   - Timelapse controls (start/stop/interval)
//   - Status info (photo count, WiFi signal, SD card space)
//
// The JavaScript at the bottom polls /timelapse/status every
// 2 seconds to keep the dashboard updated in real time.
const char DASHBOARD_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>PrinterCam</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif;
            background: #1a1a2e; color: #e0e0e0; padding: 16px;
        }
        .container { max-width: 800px; margin: 0 auto; }
        h1 { color: #00d4ff; margin-bottom: 16px; font-size: 22px; }

        /* Live video stream container */
        .stream-box {
            background: #000; border-radius: 12px;
            overflow: hidden; margin-bottom: 16px; text-align: center;
        }
        .stream-box img {
            width: 100%; max-width: 640px;
            display: block; margin: 0 auto;
        }

        /* Card containers for sections */
        .card {
            background: #16213e; border-radius: 12px;
            padding: 16px; margin-bottom: 12px;
        }
        .card h2 {
            font-size: 16px; color: #00d4ff; margin-bottom: 12px;
        }

        /* Status grid - 2x2 grid of stats */
        .status-grid {
            display: grid; grid-template-columns: 1fr 1fr; gap: 8px;
        }
        .stat {
            background: #0f3460; border-radius: 8px; padding: 10px;
        }
        .stat-label {
            font-size: 11px; color: #888; text-transform: uppercase;
        }
        .stat-value {
            font-size: 18px; font-weight: bold; margin-top: 2px;
        }
        .running { color: #4ade80; }
        .stopped { color: #f87171; }

        /* Control buttons and inputs */
        .controls {
            display: flex; gap: 8px;
            align-items: center; flex-wrap: wrap;
        }
        .btn {
            padding: 10px 20px; border: none; border-radius: 8px;
            font-size: 14px; font-weight: bold; cursor: pointer;
        }
        .btn-start { background: #4ade80; color: #1a1a2e; }
        .btn-stop  { background: #f87171; color: #1a1a2e; }
        .btn-wifi  { background: #f59e0b; color: #1a1a2e; }
        .btn:hover { opacity: 0.85; }
        .btn:active { transform: scale(0.97); }
        input[type="number"] {
            width: 80px; padding: 10px;
            border: 1px solid #333; border-radius: 8px;
            background: #0f3460; color: #fff; font-size: 14px;
        }
        .interval-label { font-size: 13px; color: #888; }

        /* Footer area */
        .footer { text-align: center; margin-top: 16px; }
        .footer p { margin-top: 8px; font-size: 12px; color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>PrinterCam</h1>

        <!-- Camera View -->
        <div class="stream-box">
            <img id="stream" src="/capture" alt="Camera View" />
        </div>
        <div class="card" style="padding:10px;text-align:center;">
            <button class="btn" id="mode-btn" style="background:#6366f1;color:#fff;" onclick="toggleMode()">Switch to Live Stream</button>
        </div>

        <!-- Status Cards -->
        <div class="card">
            <h2>Status</h2>
            <div class="status-grid">
                <div class="stat">
                    <div class="stat-label">Timelapse</div>
                    <div class="stat-value" id="tl-status">--</div>
                </div>
                <div class="stat">
                    <div class="stat-label">Photos Taken</div>
                    <div class="stat-value" id="tl-count">--</div>
                </div>
                <div class="stat">
                    <div class="stat-label">WiFi Signal</div>
                    <div class="stat-value" id="rssi">--</div>
                </div>
                <div class="stat">
                    <div class="stat-label">SD Card Free</div>
                    <div class="stat-value" id="sd-free">--</div>
                </div>
            </div>
        </div>

        <!-- Timelapse Controls -->
        <div class="card">
            <h2>Timelapse Controls</h2>
            <div class="controls">
                <button class="btn btn-start" onclick="startTL()">Start</button>
                <button class="btn btn-stop" onclick="stopTL()">Stop</button>
                <input type="number" id="interval" value="30" min="5" max="3600" />
                <span class="interval-label">sec interval</span>
            </div>
        </div>

        <!-- WiFi Settings -->
        <div class="card footer">
            <button class="btn btn-wifi" onclick="resetWiFi()">
                Reset WiFi Settings
            </button>
            <p id="wifi-msg"></p>
        </div>
    </div>

    <script>
        // -------------------------------------------------------
        // Dashboard JavaScript
        // -------------------------------------------------------
        // This code runs in the browser and communicates with the
        // ESP32 by making HTTP requests (fetch) to the API endpoints.

        // Poll the /timelapse/status endpoint every 2 seconds
        // to keep the dashboard info up to date
        function updateStatus() {
            fetch('/timelapse/status')
                .then(function(response) { return response.json(); })
                .then(function(data) {
                    // Update timelapse status with color coding
                    var statusEl = document.getElementById('tl-status');
                    statusEl.textContent = data.running ? 'Running' : 'Stopped';
                    statusEl.className = 'stat-value ' + (data.running ? 'running' : 'stopped');

                    // Update other stats
                    document.getElementById('tl-count').textContent = data.count;
                    document.getElementById('rssi').textContent = data.rssi + ' dBm';
                    document.getElementById('sd-free').textContent = data.sd_free_mb + ' MB';
                })
                .catch(function() {
                    // Silently ignore errors (camera may be rebooting)
                });
        }

        // Start timelapse with the interval from the input field
        function startTL() {
            var interval = document.getElementById('interval').value;
            fetch('/timelapse/start?interval=' + interval)
                .then(function() { setTimeout(updateStatus, 300); });
        }

        // Stop timelapse
        function stopTL() {
            fetch('/timelapse/stop')
                .then(function() { setTimeout(updateStatus, 300); });
        }

        // Reset WiFi credentials and reboot into AP mode
        function resetWiFi() {
            if (confirm('Reset WiFi settings and reboot into setup mode?')) {
                fetch('/wifi-reset').then(function() {
                    document.getElementById('wifi-msg').textContent = 'Rebooting into setup mode...';
                });
            }
        }

        // Snapshot vs stream mode
        var liveMode = false;
        var snapTimer = null;

        function refreshSnapshot() {
            var img = document.getElementById('stream');
            img.src = '/capture?t=' + Date.now();
        }

        function toggleMode() {
            var img = document.getElementById('stream');
            var btn = document.getElementById('mode-btn');
            liveMode = !liveMode;
            if (liveMode) {
                if (snapTimer) { clearInterval(snapTimer); snapTimer = null; }
                img.src = '/stream';
                btn.textContent = 'Switch to Snapshot';
            } else {
                img.src = '/capture?t=' + Date.now();
                snapTimer = setInterval(refreshSnapshot, 2000);
                btn.textContent = 'Switch to Live Stream';
            }
        }

        // Start in snapshot mode - refresh every 2 seconds
        snapTimer = setInterval(refreshSnapshot, 2000);

        // Run immediately on page load, then every 2 seconds
        updateStatus();
        setInterval(updateStatus, 2000);
    </script>
</body>
</html>
)rawliteral";


// =============================================================
// WiFi Saved Confirmation Page
// =============================================================
// Shown briefly after the user submits WiFi credentials.
// The ESP32 reboots immediately after sending this page.
const char WIFI_SAVED_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>WiFi Saved</title>
    <style>
        body {
            font-family: -apple-system, sans-serif;
            background: #1a1a2e; color: #e0e0e0;
            display: flex; align-items: center; justify-content: center;
            min-height: 100vh; margin: 0;
        }
        .msg { text-align: center; padding: 20px; }
        h1 { color: #4ade80; margin-bottom: 12px; }
        p { color: #888; margin-bottom: 8px; }
    </style>
</head>
<body>
    <div class="msg">
        <h1>WiFi Credentials Saved!</h1>
        <p>The camera is rebooting and will connect to your network.</p>
        <p>Check the Serial Monitor for the assigned IP address.</p>
        <p style="margin-top: 20px; font-size: 14px; color: #555;">
            Once connected, open the IP address shown in Serial Monitor.
        </p>
    </div>
</body>
</html>
)rawliteral";

#endif // HTML_CONTENT_H
