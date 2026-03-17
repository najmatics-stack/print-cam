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
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>PrinterCam</title>
<style>*{box-sizing:border-box;margin:0;padding:0}body{font-family:-apple-system,sans-serif;background:#1a1a2e;color:#e0e0e0;padding:16px}.c{max-width:800px;margin:0 auto}h1{color:#00d4ff;margin-bottom:16px;font-size:22px}.sb{background:#000;border-radius:12px;overflow:hidden;margin-bottom:16px;text-align:center;min-height:120px}.sb img{width:100%;max-width:640px;display:block;margin:0 auto}.cd{background:#16213e;border-radius:12px;padding:16px;margin-bottom:12px}.cd h2{font-size:16px;color:#00d4ff;margin-bottom:12px}.sg{display:grid;grid-template-columns:1fr 1fr;gap:8px}.st{background:#0f3460;border-radius:8px;padding:10px}.sl{font-size:11px;color:#888;text-transform:uppercase}.sv{font-size:18px;font-weight:bold;margin-top:2px}.gr{color:#4ade80}.rd{color:#f87171}.ct{display:flex;gap:8px;align-items:center;flex-wrap:wrap}.b{padding:10px 20px;border:none;border-radius:8px;font-size:14px;font-weight:bold;cursor:pointer}input[type=number]{width:80px;padding:10px;border:1px solid #333;border-radius:8px;background:#0f3460;color:#fff;font-size:14px}.il{font-size:13px;color:#888}.ld{color:#888;padding:40px;font-size:14px}</style>
</head><body><div class="c"><h1>PrinterCam</h1>
<div class="sb"><div class="ld" id="ph">Loading camera...</div><img id="stream" alt="Camera" style="display:none"></div>
<div class="cd" style="padding:10px;text-align:center"><div class="ct" style="justify-content:center">
<button class="b" style="background:#6366f1;color:#fff" onclick="snap()">Refresh</button>
<button class="b" id="ab" style="background:#333;color:#fff" onclick="toggleA()">Auto: Off</button>
<button class="b" id="mb" style="background:#333;color:#fff" onclick="toggleM()">Live Stream</button>
</div></div>
<div class="cd"><h2>Status</h2><div class="sg">
<div class="st"><div class="sl">Timelapse</div><div class="sv" id="ts">--</div></div>
<div class="st"><div class="sl">Photos</div><div class="sv" id="tc">--</div></div>
<div class="st"><div class="sl">WiFi Signal</div><div class="sv" id="rs">--</div></div>
<div class="st"><div class="sl">SD Free</div><div class="sv" id="sf">--</div></div>
</div></div>
<div class="cd"><h2>Timelapse</h2><div class="ct">
<button class="b" style="background:#4ade80;color:#1a1a2e" onclick="startTL()">Start</button>
<button class="b" style="background:#f87171;color:#1a1a2e" onclick="stopTL()">Stop</button>
<input type="number" id="iv" value="30" min="5" max="3600"><span class="il">sec</span>
</div></div>
<div class="cd" style="text-align:center;margin-top:16px">
<button class="b" style="background:#f59e0b;color:#1a1a2e" onclick="resetW()">Reset WiFi</button>
<p id="wm" style="margin-top:8px;font-size:12px;color:#666"></p>
</div></div>
<script>
var img=document.getElementById('stream'),ph=document.getElementById('ph'),st=null,am=false,lm=false;
function show(){ph.style.display='none';img.style.display='block'}
function snap(){img.onload=show;img.src='/capture?t='+Date.now()}
function us(){fetch('/timelapse/status').then(function(r){return r.json()}).then(function(d){var e=document.getElementById('ts');e.textContent=d.running?'Running':'Stopped';e.className='sv '+(d.running?'gr':'rd');document.getElementById('tc').textContent=d.count;document.getElementById('rs').textContent=d.rssi+' dBm';document.getElementById('sf').textContent=d.sd_free_mb+' MB'}).catch(function(){})}
function startTL(){fetch('/timelapse/start?interval='+document.getElementById('iv').value).then(function(){setTimeout(us,500)})}
function stopTL(){fetch('/timelapse/stop').then(function(){setTimeout(us,500)})}
function resetW(){if(confirm('Reset WiFi and reboot?'))fetch('/wifi-reset').then(function(){document.getElementById('wm').textContent='Rebooting...'})}
function toggleA(){am=!am;var b=document.getElementById('ab');if(lm)toggleM();if(am){st=setInterval(snap,5000);b.textContent='Auto: On';b.style.background='#4ade80';b.style.color='#1a1a2e'}else{if(st){clearInterval(st);st=null}b.textContent='Auto: Off';b.style.background='#333';b.style.color='#fff'}}
function toggleM(){var b=document.getElementById('mb');lm=!lm;if(lm){if(st){clearInterval(st);st=null}am=false;var a=document.getElementById('ab');a.textContent='Auto: Off';a.style.background='#333';a.style.color='#fff';img.onload=show;img.src='/stream';b.textContent='Stop Stream';b.style.background='#f87171'}else{snap();b.textContent='Live Stream';b.style.background='#333';b.style.color='#fff'}}
snap();setTimeout(us,2000);setInterval(us,10000);
</script></body></html>
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
