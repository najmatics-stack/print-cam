// =============================================================
// AI Thinker ESP32-CAM - Camera Pin Definitions
// =============================================================
//
// The AI Thinker ESP32-CAM board uses specific GPIO pins to
// communicate with the OV2640 camera module. These pins carry:
//   - Image data (Y2-Y9): 8-bit parallel bus carrying pixel data
//   - Sync signals (VSYNC, HREF, PCLK): timing for frames/rows/pixels
//   - Clock (XCLK): 20MHz clock that drives the camera sensor
//   - I2C control bus (SIOD, SIOC): for configuring camera settings
//
// DO NOT CHANGE these values unless you're using a different
// ESP32-CAM board (e.g., TTGO T-Camera, Wrover Kit, M5Camera).
// Each board has its own pin mapping.
// =============================================================

#ifndef CAMERA_PINS_H
#define CAMERA_PINS_H

// --- Power and Reset ---
#define PWDN_GPIO_NUM     32   // Power-down pin: set HIGH to power OFF, LOW to power ON
#define RESET_GPIO_NUM    -1   // Hardware reset pin: -1 means not connected (software reset used instead)

// --- Clock ---
#define XCLK_GPIO_NUM      0   // External clock output to camera sensor (20MHz square wave)

// --- I2C Control Bus (SCCB) ---
// SCCB (Serial Camera Control Bus) is essentially I2C.
// Used to send configuration commands to the camera (resolution, quality, etc.)
#define SIOD_GPIO_NUM     26   // I2C Data line (SDA) - bidirectional data
#define SIOC_GPIO_NUM     27   // I2C Clock line (SCL) - clock signal

// --- 8-bit Parallel Data Bus (D0-D7) ---
// These 8 pins carry one byte of pixel data per clock cycle.
// The camera outputs raw image data on these pins, which the
// ESP32's built-in I2S peripheral captures at high speed.
#define Y9_GPIO_NUM       35   // D7 - most significant bit
#define Y8_GPIO_NUM       34   // D6
#define Y7_GPIO_NUM       39   // D5
#define Y6_GPIO_NUM       36   // D4
#define Y5_GPIO_NUM       21   // D3
#define Y4_GPIO_NUM       19   // D2
#define Y3_GPIO_NUM       18   // D1
#define Y2_GPIO_NUM        5   // D0 - least significant bit

// --- Synchronization Signals ---
// These timing signals tell the ESP32 when each frame/row starts
#define VSYNC_GPIO_NUM    25   // Vertical sync: pulses at the start of each new frame
#define HREF_GPIO_NUM     23   // Horizontal reference: HIGH during valid pixel data in a row
#define PCLK_GPIO_NUM     22   // Pixel clock: one tick per byte of pixel data

// --- Flash LED ---
// The bright white LED on the front of the ESP32-CAM board.
// Note: GPIO 4 is also SD_MMC DATA1 in 4-bit mode.
// We use SD card in 1-bit mode to avoid this conflict.
#define FLASH_LED_PIN      4

#endif // CAMERA_PINS_H
