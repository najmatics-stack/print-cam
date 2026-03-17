# PrinterCam

ESP32-CAM based 3D printer camera. Live MJPEG stream, timelapse capture to microSD, WiFi setup via captive portal. View your prints remotely via Tailscale.

## Features

- **Live streaming** — MJPEG stream viewable in any browser or OctoPrint
- **Timelapse capture** — Automatic photo capture to microSD at configurable intervals
- **Web dashboard** — Status, controls, and camera view from your browser
- **WiFi captive portal** — First-time setup with no serial monitor needed
- **Remote access** — View from anywhere via Tailscale subnet routing

## Hardware

- ESP32-CAM (AI Thinker) with OV2640 camera
- ESP32-CAM-MB programmer board
- MicroSD card (FAT32, 8-32GB)
- USB data cable

See [BOM.md](BOM.md) for the full bill of materials.

## Quick Start

```bash
# Flash firmware
cd print-cam
source .venv/bin/activate
pio run --target upload

# Monitor serial output
pio device monitor
```

1. Connect to **PrinterCam-Setup** WiFi (password: `printer123`)
2. Enter your home WiFi credentials in the captive portal
3. Open the IP address shown in Serial Monitor

## Endpoints

| Endpoint | Description |
|----------|-------------|
| `/` | Web dashboard |
| `/stream` | MJPEG live stream |
| `/capture` | Single JPEG snapshot |
| `/timelapse/start?interval=30` | Start timelapse |
| `/timelapse/stop` | Stop timelapse |
| `/timelapse/status` | JSON status |

## Documentation

- [SETUP_GUIDE.md](SETUP_GUIDE.md) — Detailed setup walkthrough
- [QUICK_SETUP.md](QUICK_SETUP.md) — Quick reference card
- [REMOTE_ACCESS.md](REMOTE_ACCESS.md) — Tailscale remote access setup
- [BOM.md](BOM.md) — Bill of materials and hardware details

## Creating Timelapse Videos

```bash
ffmpeg -framerate 30 -i img_%05u.jpg -c:v libx264 -pix_fmt yuv420p timelapse.mp4
```

## License

MIT
