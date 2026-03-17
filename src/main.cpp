// =============================================================
// PrinterCam - 3D Printer Camera Monitor
// =============================================================
//
// A complete camera monitoring system for 3D printers built on
// the ESP32-CAM (AI Thinker) board with OV2640 camera sensor.
//
// Features:
//   - Live MJPEG video stream viewable in any browser
//   - Single JPEG snapshot endpoint
//   - Timelapse capture to microSD card
//   - Web dashboard with live controls
//   - WiFi setup via captive portal (AP mode)
//   - Credentials stored in Non-Volatile Storage (NVS)
//
// Endpoints:
//   GET  /                  - Dashboard (or WiFi setup in AP mode)
//   GET  /stream            - Live MJPEG video stream
//   GET  /capture           - Single JPEG snapshot
//   GET  /timelapse/start   - Start timelapse (?interval=30)
//   GET  /timelapse/stop    - Stop timelapse
//   GET  /timelapse/status  - JSON status info
//   POST /save-wifi         - Save WiFi credentials (from setup form)
//   GET  /wifi-reset        - Clear WiFi credentials and reboot
//
// =============================================================


// =============================================================
// SECTION 1: INCLUDES
// =============================================================
// Each #include brings in a library that provides specific
// functionality. Think of them as "toolboxes" we can use.

#include <Arduino.h>           // Core Arduino functions (Serial, delay, etc.)

// --- ESP32 Camera Driver ---
// This library talks to the OV2640 camera hardware.
// It handles capturing images and compressing them to JPEG.
#include "esp_camera.h"

// --- HTTP Server ---
// The ESP-IDF HTTP server handles incoming web requests.
// Unlike the simpler Arduino "WebServer" library, this one can
// handle multiple connections at once - critical for streaming
// video to one client while another checks the dashboard.
#include "esp_http_server.h"

// --- WiFi ---
// Lets us connect to WiFi networks (Station/STA mode) or
// create our own WiFi hotspot (Access Point/AP mode).
#include <WiFi.h>

// --- DNS Server ---
// In AP mode, this redirects ALL domain lookups to the ESP32.
// When someone connects to our hotspot and opens any URL,
// they get redirected to our WiFi setup page automatically.
// This is how "captive portals" work (like hotel WiFi login pages).
#include <DNSServer.h>

// --- Non-Volatile Storage (NVS) ---
// NVS stores small pieces of data in flash memory that survive
// reboots and power cycles. We use it to remember WiFi credentials
// so the camera reconnects automatically after a power outage.
// (The user mentioned "NVMe" - NVS is the correct ESP32 term.)
#include <Preferences.h>

// --- SD Card (MMC interface) ---
// The ESP32-CAM has a microSD card slot connected via the MMC
// (MultiMediaCard) bus. We use 1-bit mode which only needs 3
// GPIO pins, freeing up GPIO 4 (which is shared with the flash LED).
#include "SD_MMC.h"
#include "FS.h"

// --- Brownout Detector Registers ---
// The brownout detector monitors the ESP32's power supply voltage.
// If it drops too low, the detector triggers a reset to prevent
// data corruption. However, the ESP32-CAM draws lots of power
// during WiFi + camera operation, causing brief voltage dips that
// trigger false resets. We disable the detector to prevent this.
// (Make sure your USB cable and power supply are decent quality!)
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- Our project header files ---
#include "camera_pins.h"       // GPIO pin numbers for AI Thinker board
#include "html_content.h"      // HTML pages (dashboard, WiFi setup)


// =============================================================
// SECTION 2: CONFIGURATION CONSTANTS
// =============================================================
// Adjust these values to customize behavior without changing
// any logic. All times are in milliseconds unless noted.

// WiFi Access Point settings (used during first-time setup)
const char* AP_SSID     = "PrinterCam-Setup";  // Name of the setup WiFi network
const char* AP_PASSWORD = "printer123";         // Password for the setup network

// How long to try connecting to stored WiFi before giving up
// and falling back to AP mode (in milliseconds)
const unsigned long WIFI_TIMEOUT_MS = 15000;    // 15 seconds

// Default timelapse interval (in seconds)
const uint32_t DEFAULT_INTERVAL = 30;

// Folder on the SD card where timelapse images are saved
const char* TIMELAPSE_DIR = "/timelapse";

// Camera JPEG quality settings
// Range: 0 (best quality, largest file) to 63 (worst quality, smallest file)
// 10-12 is a good sweet spot for VGA resolution
const int STREAM_JPEG_QUALITY  = 12;   // Used during live streaming (slightly smaller)
const int CAPTURE_JPEG_QUALITY = 10;   // Used for saved timelapse photos (slightly sharper)


// =============================================================
// SECTION 3: GLOBAL VARIABLES
// =============================================================
// These variables are accessible from any function in the program.
// They track the current state of the camera system.

// --- WiFi State ---
bool apMode = false;             // true = AP mode (setup), false = connected to WiFi

// --- Server ---
httpd_handle_t httpServer = NULL; // Handle to the HTTP server instance

// --- DNS Server (captive portal) ---
DNSServer dnsServer;             // Only active in AP mode

// --- Non-Volatile Storage ---
Preferences preferences;         // For reading/writing WiFi credentials

// --- Timelapse State ---
bool timelapseRunning = false;           // Is timelapse currently active?
uint32_t timelapseInterval = DEFAULT_INTERVAL; // Seconds between captures
uint32_t imageCounter = 0;              // Total images captured (also used for filenames)
unsigned long lastCaptureTime = 0;       // millis() timestamp of last capture

// --- SD Card State ---
bool sdCardReady = false;                // Did the SD card initialize successfully?


// =============================================================
// SECTION 4: CAMERA INITIALIZATION
// =============================================================

// Configure and start the OV2640 camera.
// Returns true on success, false on failure.
bool initCamera() {
    Serial.println("Initializing camera...");

    // The camera_config_t struct tells the camera driver:
    //   - Which GPIO pins connect to which camera signals
    //   - What resolution and quality to use
    //   - How many frame buffers to allocate
    camera_config_t config;

    // --- Pin Assignments ---
    // These map ESP32 GPIO numbers to camera functions.
    // Values come from camera_pins.h (specific to AI Thinker board).
    config.pin_pwdn     = PWDN_GPIO_NUM;     // Power-down control
    config.pin_reset    = RESET_GPIO_NUM;     // Hardware reset (-1 = unused)
    config.pin_xclk     = XCLK_GPIO_NUM;     // Clock output to camera
    config.pin_sccb_sda = SIOD_GPIO_NUM;     // I2C data (camera control)
    config.pin_sccb_scl = SIOC_GPIO_NUM;     // I2C clock (camera control)
    config.pin_d7       = Y9_GPIO_NUM;       // Data bus bit 7 (MSB)
    config.pin_d6       = Y8_GPIO_NUM;       // Data bus bit 6
    config.pin_d5       = Y7_GPIO_NUM;       // Data bus bit 5
    config.pin_d4       = Y6_GPIO_NUM;       // Data bus bit 4
    config.pin_d3       = Y5_GPIO_NUM;       // Data bus bit 3
    config.pin_d2       = Y4_GPIO_NUM;       // Data bus bit 2
    config.pin_d1       = Y3_GPIO_NUM;       // Data bus bit 1
    config.pin_d0       = Y2_GPIO_NUM;       // Data bus bit 0 (LSB)
    config.pin_vsync    = VSYNC_GPIO_NUM;    // Vertical sync
    config.pin_href     = HREF_GPIO_NUM;     // Horizontal reference
    config.pin_pclk     = PCLK_GPIO_NUM;     // Pixel clock

    // --- Clock and Peripheral Settings ---
    config.xclk_freq_hz = 20000000;          // 20MHz clock to camera sensor
    config.ledc_timer   = LEDC_TIMER_0;      // Timer for generating the clock
    config.ledc_channel = LEDC_CHANNEL_0;    // PWM channel for the clock
    config.pixel_format = PIXFORMAT_JPEG;    // Output JPEG (hardware-compressed)
    config.grab_mode    = CAMERA_GRAB_LATEST; // Always return the newest frame

    // --- Resolution and Buffer Settings ---
    // PSRAM (Pseudo-Static RAM) is 4MB of extra memory on the ESP32-CAM.
    // With PSRAM: we can use VGA (640x480) and 2 frame buffers
    // Without PSRAM: limited to QVGA (320x240) and 1 buffer
    if (psramFound()) {
        Serial.println("  PSRAM found - using VGA (640x480) with 2 frame buffers");
        config.frame_size  = FRAMESIZE_VGA;          // 640 x 480 pixels
        config.jpeg_quality = STREAM_JPEG_QUALITY;    // Quality 12
        config.fb_count    = 2;                       // 2 buffers for smooth streaming
        config.fb_location = CAMERA_FB_IN_PSRAM;      // Store buffers in PSRAM
    } else {
        Serial.println("  WARNING: No PSRAM - using QVGA (320x240) with 1 buffer");
        config.frame_size  = FRAMESIZE_QVGA;         // 320 x 240 pixels
        config.jpeg_quality = 15;                     // Lower quality to save RAM
        config.fb_count    = 1;                       // Single buffer
        config.fb_location = CAMERA_FB_IN_DRAM;       // Store in main RAM
    }

    // --- Start the camera driver ---
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("  ERROR: Camera init failed! (error code: 0x%x)\n", err);
        Serial.println("  Common fixes:");
        Serial.println("    - Reseat the camera ribbon cable (gold contacts face the board)");
        Serial.println("    - Check that the cable isn't torn or creased");
        Serial.println("    - Try a different USB cable (some don't provide enough power)");
        return false;
    }

    // --- Fine-tune image sensor settings ---
    // The sensor_t struct lets us adjust camera parameters in real time.
    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
        sensor->set_brightness(sensor, 0);      // Brightness: -2 to +2 (0 = neutral)
        sensor->set_contrast(sensor, 0);        // Contrast:   -2 to +2 (0 = neutral)
        sensor->set_saturation(sensor, 0);      // Saturation: -2 to +2 (0 = neutral)
        sensor->set_whitebal(sensor, 1);        // Auto white balance: ON
        sensor->set_awb_gain(sensor, 1);        // AWB gain: ON
        sensor->set_exposure_ctrl(sensor, 1);   // Auto exposure: ON
        sensor->set_aec2(sensor, 1);            // Advanced auto exposure: ON
        sensor->set_gain_ctrl(sensor, 1);       // Auto gain control: ON
    }

    Serial.println("  Camera initialized successfully!");
    return true;
}


// =============================================================
// SECTION 5: SD CARD INITIALIZATION
// =============================================================

// Initialize the microSD card and prepare the timelapse directory.
// Returns true on success, false on failure.
bool initSDCard() {
    Serial.println("Initializing SD card...");

    // Start SD card in 1-bit MMC mode.
    // Why 1-bit mode? The ESP32-CAM has a hardware conflict:
    //   - GPIO 4 is used for the flash LED
    //   - GPIO 4 is also DATA1 in 4-bit SD mode
    // Using 1-bit mode only needs DATA0 (GPIO 2), CLK (GPIO 14),
    // and CMD (GPIO 15). This frees GPIO 4 for the LED.
    //
    // The second parameter (true) enables 1-bit mode.
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("  WARNING: SD card mount failed!");
        Serial.println("  Possible causes:");
        Serial.println("    - No microSD card inserted");
        Serial.println("    - Card not formatted as FAT32");
        Serial.println("    - Card contacts dirty or card damaged");
        Serial.println("    - Card inserted while board was booting (try removing and reinserting)");
        return false;
    }

    // Verify a card is actually present
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("  WARNING: No SD card detected in slot!");
        return false;
    }

    // Print card information
    const char* typeStr = (cardType == CARD_MMC)  ? "MMC"  :
                          (cardType == CARD_SD)   ? "SD"   :
                          (cardType == CARD_SDHC) ? "SDHC" : "Unknown";
    Serial.printf("  Card type: %s\n", typeStr);
    Serial.printf("  Card size: %llu MB\n", SD_MMC.cardSize() / (1024 * 1024));
    Serial.printf("  Free space: %llu MB\n",
        (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024));

    // Create the /timelapse directory if it doesn't exist yet
    if (!SD_MMC.exists(TIMELAPSE_DIR)) {
        if (SD_MMC.mkdir(TIMELAPSE_DIR)) {
            Serial.println("  Created /timelapse directory on SD card");
        } else {
            Serial.println("  WARNING: Failed to create /timelapse directory!");
        }
    }

    // Scan existing timelapse images to find the highest number.
    // This way we continue the sequence instead of overwriting old photos.
    // For example, if img_00042.jpg already exists, we start at img_00043.jpg.
    File dir = SD_MMC.open(TIMELAPSE_DIR);
    if (dir && dir.isDirectory()) {
        File entry;
        while ((entry = dir.openNextFile())) {
            const char* name = entry.name();
            uint32_t num = 0;
            // Try to parse the image number from the filename
            if (sscanf(name, "/timelapse/img_%05u.jpg", &num) == 1 ||
                sscanf(name, "img_%05u.jpg", &num) == 1) {
                if (num >= imageCounter) {
                    imageCounter = num + 1;  // Resume after the highest number
                }
            }
            entry.close();
        }
        dir.close();
    }

    if (imageCounter > 0) {
        Serial.printf("  Found existing images. Resuming from #%u\n", imageCounter);
    }

    Serial.println("  SD card initialized successfully!");
    return true;
}


// =============================================================
// SECTION 6: WIFI MANAGEMENT
// =============================================================

// Attempt to connect to a previously stored WiFi network.
// Returns true if connected, false if we should fall back to AP mode.
bool connectToWiFi() {
    Serial.println("Checking for stored WiFi credentials...");

    // Open the "printercam" namespace in NVS (read-only)
    preferences.begin("printercam", true);
    String ssid = preferences.getString("ssid", "");
    String password = preferences.getString("password", "");
    preferences.end();

    // If no credentials are stored, go straight to AP mode
    if (ssid.isEmpty()) {
        Serial.println("  No credentials found. Will start AP mode.");
        return false;
    }

    // Try to connect to the stored network
    Serial.printf("  Connecting to \"%s\"", ssid.c_str());
    WiFi.mode(WIFI_STA);  // Station mode = connect to an existing network
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait up to WIFI_TIMEOUT_MS for the connection to succeed.
    // WiFi.status() returns WL_CONNECTED when connected.
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startTime > WIFI_TIMEOUT_MS) {
            Serial.println("\n  Connection timed out! Will start AP mode.");
            WiFi.disconnect();
            return false;
        }
        delay(500);
        Serial.print(".");  // Print dots while waiting
    }

    // Connected! Print the IP address prominently so the user can find it.
    Serial.println();
    Serial.println();
    Serial.println("  ==========================================");
    Serial.println("  |   WiFi Connected Successfully!         |");
    Serial.printf( "  |   IP Address: %-23s|\n", WiFi.localIP().toString().c_str());
    Serial.printf( "  |   Signal: %d dBm %19s|\n", WiFi.RSSI(), "");
    Serial.println("  ==========================================");
    Serial.println();
    Serial.printf("  Open in browser: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.println();

    return true;
}

// Start the ESP32 as a WiFi Access Point for initial configuration.
// Users connect to this network and see the WiFi setup page.
void startAPMode() {
    apMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);

    // Start DNS server: intercept ALL domain lookups and point them
    // to the ESP32's AP IP address (192.168.4.1 by default).
    // This creates the "captive portal" effect.
    dnsServer.start(53, "*", WiFi.softAPIP());

    Serial.println();
    Serial.println("  ==========================================");
    Serial.println("  |   ACCESS POINT MODE - WiFi Setup       |");
    Serial.printf( "  |   Network:  %-25s|\n", AP_SSID);
    Serial.printf( "  |   Password: %-25s|\n", AP_PASSWORD);
    Serial.printf( "  |   Open: http://%-21s|\n", WiFi.softAPIP().toString().c_str());
    Serial.println("  ==========================================");
    Serial.println();
    Serial.println("  1. Connect your phone/laptop to the network above");
    Serial.println("  2. A setup page should appear automatically");
    Serial.printf( "  3. If not, open http://%s in your browser\n", WiFi.softAPIP().toString().c_str());
    Serial.println("  4. Enter your home WiFi name and password");
    Serial.println();
}


// =============================================================
// SECTION 7: TIMELAPSE FUNCTIONS
// =============================================================

// Get free space on the SD card in megabytes.
// Returns 0.0 if the SD card isn't ready.
float getSDFreeSpaceMB() {
    if (!sdCardReady) return 0.0;
    uint64_t freeBytes = SD_MMC.totalBytes() - SD_MMC.usedBytes();
    return (float)freeBytes / (1024.0 * 1024.0);
}

// Capture a single photo and save it to the SD card.
// Called periodically by the main loop when timelapse is running.
void captureTimelapsePhoto() {
    if (!sdCardReady) {
        Serial.println("Timelapse: SD card not ready, skipping.");
        return;
    }

    // Temporarily increase JPEG quality for the saved photo.
    // Stream quality (12) is fine for live viewing, but we want
    // slightly sharper images (quality 10) saved to the SD card.
    sensor_t *sensor = esp_camera_sensor_get();
    if (sensor) {
        sensor->set_quality(sensor, CAPTURE_JPEG_QUALITY);
    }

    // Capture a frame from the camera
    // esp_camera_fb_get() returns a pointer to a frame buffer
    // containing the JPEG image data
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Timelapse: Camera capture failed!");
        if (sensor) sensor->set_quality(sensor, STREAM_JPEG_QUALITY);
        return;
    }

    // Build the filename: /timelapse/img_00001.jpg, img_00002.jpg, etc.
    // %05u means "unsigned integer, padded with zeros to 5 digits"
    char filename[40];
    snprintf(filename, sizeof(filename), "%s/img_%05u.jpg",
             TIMELAPSE_DIR, imageCounter);

    // Write the JPEG data to the SD card
    File file = SD_MMC.open(filename, FILE_WRITE);
    if (file) {
        file.write(fb->buf, fb->len);  // Write raw JPEG bytes
        file.close();
        Serial.printf("Timelapse: Saved %s (%u bytes)\n", filename, fb->len);
        imageCounter++;
    } else {
        Serial.printf("Timelapse: ERROR - Could not open %s for writing!\n", filename);
    }

    // IMPORTANT: Return the frame buffer to the camera driver.
    // If we don't, the driver runs out of buffers and stops working.
    esp_camera_fb_return(fb);

    // Restore stream quality
    if (sensor) {
        sensor->set_quality(sensor, STREAM_JPEG_QUALITY);
    }
}


// =============================================================
// SECTION 8: HTTP REQUEST HANDLERS
// =============================================================
// Each handler function processes one type of web request.
// They receive an httpd_req_t* (the request) and must return
// ESP_OK on success or ESP_FAIL on error.

// ----- GET / -----
// Serves the main page. In AP mode, shows WiFi setup form.
// When connected to WiFi, shows the camera dashboard.
static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    if (apMode) {
        return httpd_resp_send(req, WIFI_SETUP_HTML, strlen(WIFI_SETUP_HTML));
    } else {
        return httpd_resp_send(req, DASHBOARD_HTML, strlen(DASHBOARD_HTML));
    }
}

// ----- POST /save-wifi -----
// Receives WiFi credentials from the setup form, saves them
// to NVS (Non-Volatile Storage), and reboots.
static esp_err_t save_wifi_handler(httpd_req_t *req) {
    // Read the form data from the POST request body
    // HTML forms send data as: ssid=MyNetwork&password=MyPassword
    char body[256] = {0};
    int received = httpd_req_recv(req, body, sizeof(body) - 1);
    if (received <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    body[received] = '\0';

    // Parse the SSID and password from the URL-encoded form data
    char ssid[64] = {0};
    char password[64] = {0};

    // Find "ssid=" in the form data
    char *ssid_start = strstr(body, "ssid=");
    if (ssid_start) {
        ssid_start += 5;  // Skip past "ssid="
        char *ssid_end = strchr(ssid_start, '&');
        size_t len = ssid_end ? (size_t)(ssid_end - ssid_start) : strlen(ssid_start);
        if (len >= sizeof(ssid)) len = sizeof(ssid) - 1;
        strncpy(ssid, ssid_start, len);
    }

    // Find "password=" in the form data
    char *pass_start = strstr(body, "password=");
    if (pass_start) {
        pass_start += 9;  // Skip past "password="
        char *pass_end = strchr(pass_start, '&');
        size_t len = pass_end ? (size_t)(pass_end - pass_start) : strlen(pass_start);
        if (len >= sizeof(password)) len = sizeof(password) - 1;
        strncpy(password, pass_start, len);
    }

    // URL-decode the values.
    // Web forms encode special characters:
    //   spaces become '+' or '%20'
    //   other special chars become '%XX' (hex code)
    // We need to decode these back to the original characters.
    auto urlDecode = [](char *str) {
        char *src = str;
        char *dst = str;
        while (*src) {
            if (*src == '+') {
                // '+' represents a space in form data
                *dst++ = ' ';
                src++;
            } else if (*src == '%' && src[1] && src[2]) {
                // '%XX' is a hex-encoded character
                char hex[3] = { src[1], src[2], 0 };
                *dst++ = (char)strtol(hex, NULL, 16);
                src += 3;
            } else {
                *dst++ = *src++;
            }
        }
        *dst = '\0';
    };

    urlDecode(ssid);
    urlDecode(password);

    Serial.printf("Saving WiFi credentials: SSID=\"%s\"\n", ssid);

    // Save to Non-Volatile Storage (persists across reboots)
    preferences.begin("printercam", false);  // false = read-write mode
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // Send the success page to the user's browser
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, WIFI_SAVED_HTML, strlen(WIFI_SAVED_HTML));

    // Wait a moment for the response to reach the browser, then reboot
    Serial.println("Credentials saved. Rebooting in 2 seconds...");
    delay(2000);
    ESP.restart();

    return ESP_OK;  // Never reached (ESP reboots above)
}

// ----- GET /wifi-reset -----
// Clears stored WiFi credentials and reboots into AP mode.
// Useful when you want to move the camera to a different network.
static esp_err_t wifi_reset_handler(httpd_req_t *req) {
    Serial.println("WiFi reset requested. Clearing stored credentials...");

    preferences.begin("printercam", false);
    preferences.clear();  // Erase all stored values in our namespace
    preferences.end();

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "WiFi credentials cleared. Rebooting into setup mode...");

    delay(1000);
    ESP.restart();

    return ESP_OK;
}

// ----- GET /stream -----
// Sends a continuous MJPEG (Motion JPEG) video stream.
//
// How MJPEG streaming works:
//   1. We set the HTTP content type to "multipart/x-mixed-replace"
//   2. This tells the browser: "I'm going to send multiple parts
//      separated by a boundary string"
//   3. Each part is a complete JPEG image with its own Content-Type header
//   4. The browser replaces the previous image with each new one,
//      creating the illusion of video
//   5. The loop continues until the client disconnects
//
// MJPEG is simple and works in all browsers without JavaScript.
// Just put it in an <img> tag: <img src="/stream" />
static esp_err_t stream_handler(httpd_req_t *req) {
    esp_err_t res = ESP_OK;
    camera_fb_t *fb = NULL;

    // These strings define the MJPEG multipart protocol
    static const char *STREAM_CONTENT_TYPE =
        "multipart/x-mixed-replace;boundary=frame";
    static const char *STREAM_BOUNDARY = "\r\n--frame\r\n";
    static const char *STREAM_PART =
        "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

    // Set the response content type for multipart streaming
    res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) return res;

    // Allow cross-origin requests (so the stream works even if
    // accessed from a different port or domain)
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    Serial.println("Stream: Client connected");

    // Continuously capture and send frames
    while (true) {
        // Grab a frame from the camera
        fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Stream: Camera capture failed!");
            res = ESP_FAIL;
            break;
        }

        // Build the header for this frame (content type + size)
        char partHeader[64];
        size_t headerLen = snprintf(partHeader, sizeof(partHeader),
                                    STREAM_PART, fb->len);

        // Send the boundary separator (marks start of new frame)
        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY,
                                    strlen(STREAM_BOUNDARY));
        if (res != ESP_OK) break;

        // Send the frame header
        res = httpd_resp_send_chunk(req, partHeader, headerLen);
        if (res != ESP_OK) break;

        // Send the actual JPEG image data
        res = httpd_resp_send_chunk(req, (const char *)fb->buf, fb->len);
        if (res != ESP_OK) break;

        // Return the frame buffer to the camera driver
        // (so it can be reused for the next capture)
        esp_camera_fb_return(fb);
        fb = NULL;
    }

    // If we exited the loop while holding a frame buffer, return it
    if (fb) {
        esp_camera_fb_return(fb);
    }

    Serial.println("Stream: Client disconnected");
    return res;
}

// ----- GET /capture -----
// Returns a single JPEG snapshot (not a stream).
// Useful for testing or grabbing a quick photo.
static esp_err_t capture_handler(httpd_req_t *req) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition",
                       "inline; filename=capture.jpg");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);

    esp_camera_fb_return(fb);
    return res;
}

// ----- GET /timelapse/start?interval=30 -----
// Starts the timelapse capture. Optionally accepts an 'interval'
// query parameter to set the capture frequency (in seconds).
static esp_err_t timelapse_start_handler(httpd_req_t *req) {
    // Check for an 'interval' query parameter
    char query[64] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char value[16] = {0};
        if (httpd_query_key_value(query, "interval", value, sizeof(value)) == ESP_OK) {
            uint32_t newInterval = atoi(value);
            // Validate: minimum 5 seconds, maximum 1 hour
            if (newInterval >= 5 && newInterval <= 3600) {
                timelapseInterval = newInterval;
                Serial.printf("Timelapse: Interval set to %u seconds\n",
                              timelapseInterval);
            }
        }
    }

    timelapseRunning = true;
    lastCaptureTime = millis();  // Start timing from now
    Serial.printf("Timelapse: Started (every %u seconds)\n", timelapseInterval);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "Timelapse started");
    return ESP_OK;
}

// ----- GET /timelapse/stop -----
// Stops the timelapse capture.
static esp_err_t timelapse_stop_handler(httpd_req_t *req) {
    timelapseRunning = false;
    Serial.printf("Timelapse: Stopped (%u photos captured)\n", imageCounter);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "Timelapse stopped");
    return ESP_OK;
}

// ----- GET /timelapse/status -----
// Returns JSON with current system status.
// The dashboard JavaScript polls this every 2 seconds.
//
// Example response:
// {"running":true,"count":42,"interval":30,"rssi":-65,"sd_free_mb":14832.5}
static esp_err_t timelapse_status_handler(httpd_req_t *req) {
    // Build JSON manually (no library needed for simple JSON)
    char json[256];
    snprintf(json, sizeof(json),
        "{\"running\":%s,\"count\":%u,\"interval\":%u,\"rssi\":%d,\"sd_free_mb\":%.1f}",
        timelapseRunning ? "true" : "false",
        imageCounter,
        timelapseInterval,
        apMode ? 0 : (int)WiFi.RSSI(),  // RSSI only meaningful when connected
        getSDFreeSpaceMB()
    );

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}


// =============================================================
// SECTION 9: WEB SERVER SETUP
// =============================================================

// Register all URL endpoints and start the HTTP server.
void startWebServer() {
    // Configure the HTTP server
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;  // We register 8 handlers (leave some room)
    config.stack_size = 12288;     // Stack size per handler thread (bytes)
    config.max_open_sockets = 3;   // Limit concurrent connections to prevent overload

    // Allow the server to purge the oldest connection when all
    // slots are full. This is important because the /stream handler
    // holds a connection open indefinitely.
    config.lru_purge_enable = true;

    Serial.println("Starting web server on port 80...");

    if (httpd_start(&httpServer, &config) != ESP_OK) {
        Serial.println("ERROR: Failed to start web server!");
        return;
    }

    // --- Register all URL handlers ---
    // Each handler maps a URL path + HTTP method to a function.
    // When a browser requests that URL, the corresponding function runs.

    // Dashboard / WiFi setup page
    httpd_uri_t index_uri = {
        .uri = "/", .method = HTTP_GET,
        .handler = index_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(httpServer, &index_uri);

    // Save WiFi credentials (form submission)
    httpd_uri_t save_wifi_uri = {
        .uri = "/save-wifi", .method = HTTP_POST,
        .handler = save_wifi_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(httpServer, &save_wifi_uri);

    // WiFi reset
    httpd_uri_t wifi_reset_uri = {
        .uri = "/wifi-reset", .method = HTTP_GET,
        .handler = wifi_reset_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(httpServer, &wifi_reset_uri);

    // Live MJPEG video stream
    httpd_uri_t stream_uri = {
        .uri = "/stream", .method = HTTP_GET,
        .handler = stream_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(httpServer, &stream_uri);

    // Single JPEG snapshot
    httpd_uri_t capture_uri = {
        .uri = "/capture", .method = HTTP_GET,
        .handler = capture_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(httpServer, &capture_uri);

    // Timelapse start
    httpd_uri_t tl_start_uri = {
        .uri = "/timelapse/start", .method = HTTP_GET,
        .handler = timelapse_start_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(httpServer, &tl_start_uri);

    // Timelapse stop
    httpd_uri_t tl_stop_uri = {
        .uri = "/timelapse/stop", .method = HTTP_GET,
        .handler = timelapse_stop_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(httpServer, &tl_stop_uri);

    // Timelapse status (JSON)
    httpd_uri_t tl_status_uri = {
        .uri = "/timelapse/status", .method = HTTP_GET,
        .handler = timelapse_status_handler, .user_ctx = NULL
    };
    httpd_register_uri_handler(httpServer, &tl_status_uri);

    Serial.println("Web server started! Available endpoints:");
    Serial.println("  GET  /                  Dashboard");
    Serial.println("  GET  /stream            Live video (MJPEG)");
    Serial.println("  GET  /capture           Single JPEG snapshot");
    Serial.println("  GET  /timelapse/start   Start timelapse (?interval=N)");
    Serial.println("  GET  /timelapse/stop    Stop timelapse");
    Serial.println("  GET  /timelapse/status  JSON status");
}


// =============================================================
// SECTION 10: SETUP - Runs once at boot
// =============================================================
// This is the entry point. The ESP32 calls setup() once when it
// powers on or reboots. We initialize all hardware and services here.

void setup() {
    // --- Step 1: Disable the brownout detector ---
    // MUST be done first, before anything that draws power.
    // Without this, the ESP32-CAM may randomly reboot during
    // WiFi transmission or camera captures.
    WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

    // --- Step 2: Start Serial communication ---
    // Serial is the text output you see in the Serial Monitor.
    // 115200 is the baud rate (speed) - must match monitor_speed
    // in platformio.ini.
    Serial.begin(115200);
    Serial.setDebugOutput(true);  // Also print ESP-IDF debug messages
    Serial.println();
    Serial.println("==========================================");
    Serial.println("  PrinterCam - 3D Printer Camera Monitor");
    Serial.println("==========================================");
    Serial.println();

    // --- Step 3: Initialize the camera ---
    if (!initCamera()) {
        Serial.println("FATAL: Camera failed! Cannot continue.");
        Serial.println("The flash LED will blink to indicate an error.");
        // Blink the flash LED rapidly to signal a camera error
        // (useful when you don't have Serial Monitor connected)
        pinMode(FLASH_LED_PIN, OUTPUT);
        while (true) {
            digitalWrite(FLASH_LED_PIN, HIGH);
            delay(300);
            digitalWrite(FLASH_LED_PIN, LOW);
            delay(300);
        }
    }

    // --- Step 4: Initialize the SD card ---
    sdCardReady = initSDCard();
    if (!sdCardReady) {
        Serial.println("NOTE: Continuing without SD card.");
        Serial.println("      Live streaming will work, but timelapse");
        Serial.println("      photos cannot be saved.\n");
    }

    // --- Step 5: Connect to WiFi (or start AP mode) ---
    if (!connectToWiFi()) {
        startAPMode();
    }

    // --- Step 6: Start the web server ---
    startWebServer();

    Serial.println();
    Serial.println("Setup complete! PrinterCam is ready.");
    Serial.println("==========================================");
    Serial.println();
}


// =============================================================
// SECTION 11: LOOP - Runs continuously after setup()
// =============================================================
// The loop() function is called over and over forever.
// We use it for two things:
//   1. Processing DNS requests in AP mode (captive portal)
//   2. Capturing timelapse photos at the configured interval

void loop() {
    // --- Handle captive portal DNS in AP mode ---
    // The DNS server intercepts domain lookups and redirects them
    // to the ESP32, so any URL the user visits shows our setup page.
    if (apMode) {
        dnsServer.processNextRequest();
    }

    // --- Handle timelapse captures ---
    // If timelapse is running and the SD card is ready, check if
    // enough time has passed since the last capture.
    if (timelapseRunning && sdCardReady) {
        unsigned long now = millis();
        // Cast timelapseInterval to unsigned long to prevent overflow
        // when multiplying by 1000 (seconds to milliseconds)
        if (now - lastCaptureTime >= (unsigned long)timelapseInterval * 1000UL) {
            captureTimelapsePhoto();
            lastCaptureTime = now;
        }
    }

    // Small delay to prevent watchdog timer resets.
    // The ESP32 has a watchdog that reboots the chip if the main
    // loop runs too long without yielding to background tasks
    // (WiFi, TCP/IP stack, etc.). delay(1) gives them time to run.
    delay(1);
}
