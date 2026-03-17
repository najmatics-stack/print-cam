# PrinterCam Quick Setup Reference

> Compact reference card. See `SETUP_GUIDE.md` for detailed explanations and troubleshooting.

---

## Pre-flight Checklist

- [ ] ESP32-CAM (AI Thinker) board with OV2640 camera attached
- [ ] ESP32-CAM-MB programmer board (your "BT daughter board" — it's actually a USB-to-serial adapter, not Bluetooth)
- [ ] MicroSD card formatted as **FAT32** (8-32 GB recommended)
- [ ] USB data cable (Micro-USB, must carry data — not a charge-only cable)
- [ ] PlatformIO installed (VS Code extension or CLI)

---

## Hardware Assembly

```
1. Insert FAT32-formatted microSD card into the ESP32-CAM
   (slot is on the bottom, contacts face upward toward the camera)

2. Seat the ESP32-CAM onto the MB programmer board
   (antenna end points AWAY from the USB connector)

3. Connect USB cable: programmer board -> your computer
```

---

## Flash the Firmware

**Option A — CLI (already set up in this project):**

```bash
cd ~/Documents/claude-fun/print-cam
source .venv/bin/activate
pio run --target upload
```

**Option B — VS Code:**

1. Open the `print-cam` folder in VS Code
2. Click the **right arrow (->)** in the PlatformIO toolbar at the bottom
3. Wait for `SUCCESS`

**If upload fails:**
- Hold the **IO0** button on the programmer board, press **RST**, release both, then retry upload
- Try a different USB cable (charge-only cables won't work)
- Try a different USB port

---

## First Boot: WiFi Setup

**1. Open Serial Monitor** to see debug output:

```bash
source .venv/bin/activate
pio device monitor
```

Baud rate: **115200** (set automatically by `platformio.ini`).

**2. Connect to the setup WiFi** from your phone or laptop:

| Setting  | Value              |
|----------|--------------------|
| Network  | `PrinterCam-Setup` |
| Password | `printer123`       |

**3. Enter your home WiFi credentials:**

- A captive portal page should appear automatically
- If it doesn't, open a browser and go to **http://192.168.4.1**
- Enter your **2.4 GHz WiFi** network name and password (ESP32 does not support 5 GHz)
- Click **Connect**

**4. Note the IP address** from Serial Monitor after reboot:

```
==========================================
|   WiFi Connected Successfully!         |
|   IP Address: 192.168.1.XX             |
==========================================
```

---

## Access the Dashboard

Open **http://[YOUR_IP]** in any browser on the same network.

### All Endpoints

| Endpoint                          | Method | Description                        |
|-----------------------------------|--------|------------------------------------|
| `/`                               | GET    | Web dashboard with live stream     |
| `/stream`                         | GET    | Raw MJPEG stream (for OctoPrint)   |
| `/capture`                        | GET    | Single JPEG snapshot               |
| `/timelapse/start?interval=30`    | GET    | Start timelapse (seconds between)  |
| `/timelapse/stop`                 | GET    | Stop timelapse                     |
| `/timelapse/status`               | GET    | JSON: running, count, RSSI, SD     |
| `/save-wifi`                      | POST   | Save WiFi creds (setup form)       |
| `/wifi-reset`                     | GET    | Clear WiFi creds, reboot to AP     |

---

## Timelapse Workflow

1. Open dashboard, set interval (default: 30 seconds, range: 5-3600)
2. Click **Start** before your print begins
3. Click **Stop** when print is done
4. Power off, remove SD card, copy `/timelapse/` folder to your computer
5. Create a video with ffmpeg:

```bash
cd timelapse
ffmpeg -framerate 30 -i img_%05u.jpg -c:v libx264 -pix_fmt yuv420p timelapse.mp4
```

---

## Key Technical Details

| Parameter         | Value                                    |
|-------------------|------------------------------------------|
| Board             | `esp32cam` (AI Thinker)                  |
| Platform          | espressif32 v6.13.0                      |
| Framework         | Arduino (ESP-IDF under the hood)         |
| CPU               | ESP32 @ 240 MHz                          |
| Flash             | 4 MB (firmware uses 961 KB / 31%)        |
| RAM               | 320 KB (firmware uses 50 KB / 15%)       |
| PSRAM             | 4 MB (used for camera frame buffers)     |
| Resolution        | VGA 640x480 (QVGA 320x240 without PSRAM)|
| Stream quality    | JPEG quality 12                          |
| Timelapse quality | JPEG quality 10 (sharper)                |
| Frame buffers     | 2 (with PSRAM) / 1 (without)            |
| SD card mode      | 1-bit MMC (avoids GPIO 4 flash LED conflict) |
| Partition table   | `huge_app.csv` (3 MB app space)          |
| WiFi              | 2.4 GHz only (ESP32 limitation)          |
| Brownout detector | Disabled in firmware                     |

---

## Changing WiFi Networks

From the dashboard: click **Reset WiFi Settings** at the bottom. The camera reboots into AP mode so you can enter new credentials.

---

## Pro Tips

- **Static IP:** Assign a fixed IP to the ESP32's MAC address in your router's DHCP settings so the address never changes.
- **OctoPrint:** Use `http://[IP]/stream` as the webcam URL in OctoPrint settings.
- **Mount:** Search Printables/Thingiverse for "ESP32-CAM mount" for your specific printer.
- **Power:** For long-term deployment, use a dedicated 5V/2A power supply instead of USB through the programmer board.
