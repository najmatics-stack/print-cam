# PrinterCam Bill of Materials

---

## Required Parts

| # | Part | Specs | Qty | Est. Price | Status | Notes |
|---|------|-------|-----|------------|--------|-------|
| 1 | **ESP32-CAM (AI Thinker)** | ESP32 + OV2640 camera + 4MB PSRAM + 4MB flash + microSD slot | 1 | $5-8 | HAVE | OV2640 camera module comes pre-attached. Verify the board has **PSRAM** (most AI Thinker boards do; some cheap clones omit it). Without PSRAM, resolution drops to 320x240. |
| 2 | **ESP32-CAM-MB Programmer** | CH340 USB-to-serial adapter board | 1 | $2-4 (often bundled with #1) | HAVE | This is the daughter board you plug the ESP32-CAM into for USB programming. It has a CH340 or CP2102 USB-to-serial chip. Most kits bundle this with the ESP32-CAM. |
| 3 | **MicroSD Card** | FAT32 formatted, Class 10 or better | 1 | $5-10 | NEED | 8-32 GB is ideal. Cards over 32 GB ship as exFAT and must be reformatted to FAT32. Each timelapse photo is ~30-50 KB at VGA/quality-10, so 8 GB holds ~160,000+ photos. |
| 4 | **USB Cable (data-capable)** | Micro-USB male to USB-A male | 1 | $3-5 | CHECK | Must carry data, not just power. Many cheap/short cables are charge-only. If upload fails with "no serial port," try a different cable. Match the connector to your MB board (most are Micro-USB; some newer ones are USB-C). |

**Minimum cost (if buying all): ~$15-25**

---

## Optional But Recommended

| # | Part | Specs | Qty | Est. Price | Notes |
|---|------|-------|-----|------------|-------|
| 5 | **5V/2A USB Power Supply** | 5V DC, 2A minimum, Micro-USB or USB-C output | 1 | $5-10 | For long-term deployment. The ESP32-CAM draws ~310 mA during WiFi + camera streaming. Thin USB cables or weak laptop ports can cause voltage drops and brownout resets. A dedicated phone charger (5V/2A+) solves this. |
| 6 | **MicroSD Card Reader** | USB-A or USB-C to microSD | 1 | $5-8 | To transfer timelapse photos from the SD card to your computer. Many laptops have a built-in SD slot (use a microSD-to-SD adapter). |
| 7 | **Camera Mount** | 3D printed bracket for your printer model | 1 | ~$0.50 (filament) | Search Printables or Thingiverse for "ESP32-CAM mount" + your printer name (Ender 3, Prusa MK3, etc.). Many free designs available. |
| 8 | **Longer Camera Ribbon Cable** | 24-pin, 0.5mm pitch FFC cable, 10-20 cm | 1 | $2-3 | Only needed if the stock 6 cm cable is too short for your mounting position. The stock cable works fine for most setups. |

---

## What You Do NOT Need

| Item | Why Not |
|------|---------|
| Breadboard / jumper wires | The MB programmer board handles all connections |
| Soldering iron | No soldering required — everything is plug-and-play |
| FTDI adapter / other programmer | The MB board replaces this |
| External antenna | The onboard PCB antenna has sufficient range for most setups (10-15m through walls) |
| Level shifter | The MB board handles USB-to-3.3V conversion |
| Resistors / capacitors | All required passives are on-board |

---

## Where to Buy

The ESP32-CAM + MB programmer kit is widely available:

- **Amazon** — Search "ESP32-CAM with programmer" or "ESP32-CAM MB kit". Many Prime-eligible options.
- **AliExpress** — Cheapest option ($3-5 for the kit), 2-4 week shipping.
- **Banggood** — Similar to AliExpress pricing, sometimes faster shipping.
- **Adafruit / SparkFun / Mouser** — Higher quality, better documentation, higher price. Good if you want guaranteed genuine parts.

When buying, look for listings that explicitly say **"AI Thinker"** and **"with PSRAM"** or **"4MB PSRAM"** to ensure compatibility.

---

## SD Card Sizing Reference

| Card Size | Approx. Timelapse Photos | At 30s Interval |
|-----------|--------------------------|-----------------|
| 4 GB      | ~80,000 photos           | ~28 days continuous |
| 8 GB      | ~160,000 photos          | ~56 days continuous |
| 16 GB     | ~320,000 photos          | ~112 days continuous |
| 32 GB     | ~640,000 photos          | ~224 days continuous |

*Based on ~50 KB per VGA JPEG at quality 10. Actual size varies with scene complexity.*

---

## Power Consumption Reference

| State | Current Draw | Notes |
|-------|-------------|-------|
| Idle (WiFi connected, no stream) | ~80 mA | Camera in standby |
| Streaming MJPEG | ~260-310 mA | Continuous capture + WiFi TX |
| Timelapse capture (momentary) | ~310 mA | Brief spike during JPEG write |
| Flash LED on (not used by default) | ~310 mA additional | GPIO 4, up to 700 mA total |
| Deep sleep (not implemented) | ~6 mA | Could add for battery use |

*Measurements are approximate and vary by board revision. Total should stay under 500 mA for reliable USB operation.*
