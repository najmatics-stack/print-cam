# PrinterCam Setup Guide

A step-by-step guide for absolute beginners. No prior ESP32 experience needed.

---

## What You Need

| Item | Notes |
|------|-------|
| ESP32-CAM (AI Thinker) | The camera board itself |
| ESP32-CAM-MB programmer | The USB daughter board (you called it the "BT board") |
| MicroSD card | FAT32 formatted, up to 32GB recommended |
| USB cable | Micro-USB or USB-C (depends on your programmer board) |
| Computer | Windows, macOS, or Linux |

> **About the "BT daughter board":** This is actually the ESP32-CAM-MB (Mother Board). It has a CH340 USB-to-serial chip that lets you program the ESP32 via USB. It's not a Bluetooth board — the "MB" stands for Mother Board. You plug the ESP32-CAM into this board for programming and serial debugging.

---

## Step 1: Install PlatformIO

PlatformIO is a development platform for embedded devices. The easiest way to use it is as a VS Code extension.

1. **Install VS Code**: Download from https://code.visualstudio.com/
2. **Install PlatformIO extension**:
   - Open VS Code
   - Click the Extensions icon in the left sidebar (or press `Cmd+Shift+X` on Mac)
   - Search for "PlatformIO IDE"
   - Click "Install"
   - Wait for it to finish (it downloads compilers and tools — this takes a few minutes)
   - Restart VS Code when prompted

---

## Step 2: Install USB Driver (if needed)

The ESP32-CAM-MB programmer uses a CH340 USB-to-serial chip. Most modern operating systems include the driver, but if your board isn't recognized:

- **macOS**: Usually works out of the box. If not, install the driver from https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver
- **Windows**: Download from https://www.wch-ic.com/downloads/CH341SER_EXE.html
- **Linux**: Usually built into the kernel. No action needed.

**How to check:** Plug in the board via USB. If you see a new serial port appear (like `/dev/cu.usbserial-*` on Mac or `COM3` on Windows), the driver is working.

---

## Step 3: Prepare the Hardware

### 3a. Format the MicroSD Card
1. Insert the microSD card into your computer (use an adapter if needed)
2. Format it as **FAT32**
   - **macOS**: Open Disk Utility → Select the card → Erase → Format: MS-DOS (FAT)
   - **Windows**: Right-click in File Explorer → Format → File system: FAT32
   - **Linux**: `sudo mkfs.vfat /dev/sdX1` (replace sdX1 with your card's device)
3. Eject the card safely

### 3b. Insert the SD Card into the ESP32-CAM
- The microSD slot is on the bottom of the ESP32-CAM board
- Insert the card with contacts facing up (toward the camera)
- It clicks into place

### 3c. Attach the Programmer Board
- Line up the pins of the ESP32-CAM with the socket on the programmer board
- Press firmly until seated (the antenna should point away from the USB port)
- Plug the USB cable into the programmer board, then into your computer

---

## Step 4: Open the Project

1. Open VS Code
2. Click **File → Open Folder** (or `Cmd+O` on Mac)
3. Navigate to the `print-cam` folder and open it
4. PlatformIO should automatically detect the project (you'll see the PlatformIO toolbar at the bottom of VS Code)

---

## Step 5: Upload the Firmware

1. Make sure the ESP32-CAM + programmer board is connected via USB
2. In VS Code, click the **right-arrow button (→)** in the bottom toolbar (or press `Cmd+Shift+B` and select "Upload")
   - Alternatively: open the PlatformIO sidebar, expand the `esp32cam` environment, and click "Upload"
3. PlatformIO will:
   - Download the ESP32 toolchain (first time only, takes a few minutes)
   - Compile the code
   - Upload it to the ESP32-CAM
4. Wait for the upload to complete. You should see `SUCCESS` in the terminal.

### If Upload Fails

**"No serial port" error:**
- Make sure the USB cable is plugged in
- Make sure the CH340 driver is installed (Step 2)
- Try a different USB cable (some cables are charge-only and don't carry data)

**"Failed to connect" or timeout error:**
- Some programmer boards need you to hold the **IO0 button** while pressing the **RST button**, then release both. This puts the ESP32 into programming mode.
- Try again after doing this

**"A fatal error occurred: No serial data received":**
- Reseat the ESP32-CAM on the programmer board
- Try a different USB port

---

## Step 6: Open the Serial Monitor

The Serial Monitor shows debug messages from the ESP32 — very useful for seeing what's happening!

1. In VS Code, click the **plug icon** in the bottom toolbar
   - Or: PlatformIO sidebar → `esp32cam` → "Monitor"
2. You should see boot messages and "PrinterCam" ASCII art
3. The monitor will tell you whether the camera and SD card initialized successfully

> **Baud rate:** Make sure the Serial Monitor is set to **115200** baud (it should be automatic since we set `monitor_speed = 115200` in `platformio.ini`)

---

## Step 7: First Boot — WiFi Setup

On first boot (or if no WiFi credentials are stored), the ESP32-CAM starts in **Access Point mode**:

1. **On your phone or laptop**, open WiFi settings
2. Connect to the network: **`PrinterCam-Setup`**
3. Password: **`printer123`**
4. A setup page should appear automatically (captive portal)
   - If it doesn't, open a browser and go to: **http://192.168.4.1**
5. Enter your **home WiFi network name** (SSID) and **password**
6. Click **Connect**
7. The ESP32 saves the credentials and reboots

### After Reboot

1. The ESP32 will connect to your home WiFi network
2. Watch the Serial Monitor — it will print the assigned IP address:
   ```
   ==========================================
   |   WiFi Connected Successfully!         |
   |   IP Address: 192.168.1.42             |
   |   Signal: -55 dBm                      |
   ==========================================
   ```
3. Note this IP address — you'll use it to access the camera

---

## Step 8: Access the Dashboard

1. Open a browser on any device connected to the same WiFi network
2. Navigate to: **http://[IP_ADDRESS_FROM_STEP_7]**
   - Example: `http://192.168.1.42`
3. You should see the PrinterCam dashboard with:
   - Live video stream
   - Timelapse controls
   - Status information

### Available Endpoints

| URL | Description |
|-----|-------------|
| `http://IP/` | Main dashboard |
| `http://IP/stream` | Raw MJPEG stream (use in OctoPrint, etc.) |
| `http://IP/capture` | Single JPEG snapshot |
| `http://IP/timelapse/start?interval=30` | Start timelapse (30s interval) |
| `http://IP/timelapse/stop` | Stop timelapse |
| `http://IP/timelapse/status` | JSON status data |

---

## Using the Timelapse

1. Make sure a microSD card is inserted before booting
2. Open the dashboard in a browser
3. Set the desired interval (in seconds) — default is 30
4. Click **Start** to begin capturing
5. Photos are saved to `/timelapse/img_00001.jpg`, `img_00002.jpg`, etc.
6. Click **Stop** when your print is done
7. Power off the ESP32, remove the SD card, and copy the images to your computer

### Creating a Timelapse Video from Photos

Once you have the photos on your computer, you can create a video using **ffmpeg**:

```bash
ffmpeg -framerate 30 -i img_%05u.jpg -c:v libx264 -pix_fmt yuv420p timelapse.mp4
```

This creates a 30fps video from all the numbered images.

---

## Troubleshooting

### Camera shows no image / black screen
- Check the ribbon cable connecting the camera module to the board
- The gold contacts on the ribbon cable should face the board (not outward)
- Make sure the cable isn't torn or creased at the connector

### SD card not detected
- Format the card as FAT32 (not exFAT or NTFS)
- Try a different microSD card (some high-capacity cards aren't compatible)
- Remove the card, reboot the ESP32, then insert the card and reboot again
- GPIO 2 (SD data pin) can sometimes prevent booting — if the ESP32 won't start with the SD card in, remove the card, flash the firmware, then reinsert

### Can't connect to WiFi
- Make sure you entered the correct SSID and password (case-sensitive!)
- The ESP32 only supports **2.4 GHz WiFi** (not 5 GHz)
- If your router has separate 2.4 GHz and 5 GHz networks, connect to the 2.4 GHz one
- To re-enter WiFi credentials: click "Reset WiFi Settings" on the dashboard, or reflash the firmware

### Stream is laggy or low framerate
- Move the ESP32-CAM closer to your WiFi router
- Reduce the number of devices streaming simultaneously (MJPEG uses bandwidth)
- Check WiFi signal strength on the dashboard (above -70 dBm is good)

### ESP32 keeps rebooting
- Use a good quality USB cable (some thin cables cause voltage drops)
- Use a USB port that can supply enough current (500mA+)
- Consider powering via a dedicated 5V power supply instead of USB

### "Brownout detector was triggered" in Serial Monitor
- This means the power supply voltage dropped too low momentarily
- The firmware disables the brownout detector, but if you see this message, your power supply is marginal
- Use a better USB cable or a dedicated 5V/2A power supply

---

## Tips

- **Static IP**: For convenience, set a static IP for the ESP32 in your router's DHCP settings. That way the address doesn't change after a reboot.
- **OctoPrint Integration**: Use the `/stream` URL as a webcam source in OctoPrint for direct integration.
- **Mounting**: 3D print a mount for the ESP32-CAM! Search Thingiverse or Printables for "ESP32-CAM mount" — there are many options for Ender 3, Prusa, and other printers.
- **Remote Access**: See `REMOTE_ACCESS.md` for how to view your camera from outside your home network using Tailscale.
