#pragma once

// ─────────────────────────────────────────────
//  WiFi credentials  (edit before flashing)
// ─────────────────────────────────────────────
#define WIFI_SSID      "Verizon_7STF4V"
#define WIFI_PASSWORD  "ran7-silk-sixth"
#define WIFI_MAX_RETRY 10

// ─────────────────────────────────────────────
//  Board: Goouuu ESP32-S3-CAM
//  Camera: OV3660
//
//  Pin mapping from official Goouuu pinout diagram.
//  Camera bus occupies GPIOs 4–18 plus GPIO13/15.
// ─────────────────────────────────────────────

// No dedicated power-down or hardware reset pins on this board
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1

// XCLK — master clock to sensor
#define CAM_PIN_XCLK    15   // GPIO15 — CAM_XCLK

// SCCB (I2C for sensor register config)
#define CAM_PIN_SIOD     4   // GPIO4  — CAM_SIOD (SDA)
#define CAM_PIN_SIOC     5   // GPIO5  — CAM_SIOC (SCL)

// Pixel data bus D0–D7 (8-bit parallel, Y2=LSB, Y9=MSB)
#define CAM_PIN_D0      11   // GPIO11 — CAM_Y2  (D0, LSB)
#define CAM_PIN_D1       9   // GPIO9  — CAM_Y3  (D1)
#define CAM_PIN_D2       8   // GPIO8  — CAM_Y4  (D2)
#define CAM_PIN_D3      10   // GPIO10 — CAM_Y5  (D3)
#define CAM_PIN_D4      12   // GPIO12 — CAM_Y6  (D4)
#define CAM_PIN_D5      18   // GPIO18 — CAM_Y7  (D5)
#define CAM_PIN_D6      17   // GPIO17 — CAM_Y8  (D6)
#define CAM_PIN_D7      16   // GPIO16 — CAM_Y9  (D7, MSB)

// Sync / clock signals
#define CAM_PIN_VSYNC    6   // GPIO6  — CAM_VYSNC
#define CAM_PIN_HREF     7   // GPIO7  — CAM_HREF
#define CAM_PIN_PCLK    13   // GPIO13 — CAM_PCLK

// ─────────────────────────────────────────────
//  Servo pins
//
//  Free GPIOs on this board (not used by camera,
//  PSRAM, USB-OTG, JTAG, SD card, or WS2812):
//
//    GPIO14 — right header, available PWM-capable
//    GPIO21 — right header, available PWM-capable
//
//  Connect servo signal wires to these pins.
//  Power servos from the 5V header pin, not 3V3.
// ─────────────────────────────────────────────
#define SERVO_PAN_GPIO   14  // GPIO14 — right header
#define SERVO_TILT_GPIO  21  // GPIO21 — right header

// LEDC channels & timers
#define SERVO_PAN_CHANNEL   LEDC_CHANNEL_0
#define SERVO_TILT_CHANNEL  LEDC_CHANNEL_1
#define SERVO_TIMER         LEDC_TIMER_0
#define SERVO_SPEED_MODE    LEDC_LOW_SPEED_MODE

// Servo PWM: 50 Hz, 14-bit resolution
#define SERVO_FREQ_HZ        50
#define SERVO_RESOLUTION     LEDC_TIMER_14_BIT   // 16384 ticks

// Pulse widths in µs (standard hobby servo)
#define SERVO_MIN_US         500
#define SERVO_MAX_US        2500
#define SERVO_CENTER_US     1500

// ─────────────────────────────────────────────
//  HTTP server
// ─────────────────────────────────────────────
#define HTTP_PORT            80
#define STREAM_BOUNDARY      "gc0p4Jq0M2Yt08jU534c0p"

// ─────────────────────────────────────────────
//  Camera frame / quality
// ─────────────────────────────────────────────
#define CAM_FRAME_SIZE   FRAMESIZE_QVGA   // 320x240 — smooth at 30fps over WiFi
#define CAM_JPEG_QUALITY  10              // 0–63, lower = better quality
