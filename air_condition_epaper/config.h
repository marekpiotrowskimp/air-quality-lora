/*
 * config.h - Hardware configuration
 * Air Condition ePaper Display v0.2
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// DEBUG FLAGS
// ============================================
#define DEBUG_SERIAL  1   // Enable Serial debug output

// ============================================
// DEBUG MACROS
// ============================================
#if DEBUG_SERIAL
  #define DEBUG_INIT()       Serial.begin(115200)
  #define DEBUG_PRINT(x)     Serial.print(x)
  #define DEBUG_PRINTLN(x)   Serial.println(x)
  #define DEBUG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// ============================================
// ePaper Display Configuration
// Seeed Studio ePaper Breakout Board pinout
// for XIAO ESP32-S3
// ============================================
#define EPAPER_RST   1    // D0 - GPIO1 - Reset
#define EPAPER_CS    2    // D1 - GPIO2 - Chip Select
#define EPAPER_DC    4    // D3 - GPIO4 - Data/Command
#define EPAPER_BUSY  6    // D5 - GPIO6 - Busy
#define EPAPER_SCK   7    // D8 - GPIO7 - SPI Clock
#define EPAPER_MOSI  9    // D10 - GPIO9 - SPI MOSI

// ============================================
// Display Parameters
// 2.9" Monochrome ePaper (SSD1680)
// ============================================
#define DISPLAY_WIDTH   296
#define DISPLAY_HEIGHT  128

// ============================================
// WiFi Configuration
// ============================================
#include "secrets.h"   // WIFI_SSID, WIFI_PASSWORD — patrz secrets.h.example

#define WIFI_CONNECT_TIMEOUT_MS  15000  // 15s timeout

// ============================================
// UDP Configuration
// ============================================
#define UDP_PORT            5000
#define UDP_BUFFER_SIZE     2048

// ============================================
// Device limits (matching air_condition_receiver)
// ============================================
#define MAX_DEVICES         4
#define DEVICE_NAME_LEN     16

// ============================================
// Display refresh settings
// ============================================
#define DISPLAY_REFRESH_INTERVAL_MS   30000   // Minimum 30s between full refreshes
#define DATA_TIMEOUT_MS               120000  // 2 min - mark device as stale
#define PM_CHANGE_THRESHOLD           3       // Refresh when PM changes by this amount

// ============================================
// UI Layout constants
// ============================================
#define HEADER_HEIGHT       20
#define DEVICE_ROW_HEIGHT   26
#define MARGIN_X            4
#define MARGIN_Y            2

#endif // CONFIG_H
