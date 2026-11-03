/*
 * config.h - Hardware configuration and constants
 * Air Condition Receiver v2.7
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// FEATURE FLAGS
// ============================================
#define ENABLE_LCD    1   // LCD display ST7789V
#define ENABLE_TOUCH  1   // Touch XPT2046
#define ENABLE_LORA   1   // LoRa SX1262

// ============================================
// DEBUG FLAGS - disable in production (0) for stable startup
// After enabling DEBUG_SERIAL, you can enable detailed logs
// ============================================
#define DEBUG_SERIAL  0   // Main flag - Serial.begin() and basic logs
#define DEBUG_TOUCH   0   // Touch logs (coordinates, actions)
#define DEBUG_LORA    0   // LoRa logs (received packets, RSSI)
#define DEBUG_WIFI    0   // WiFi logs (connection, NTP, UDP)
#define DEBUG_LCD     0   // LCD logs (redraw, screens)

// ============================================
// DEBUG MACROS
// ============================================
#if DEBUG_SERIAL
  #define DEBUG_INIT()       do { Serial.begin(115200); Serial.setTxTimeoutMs(0); } while(0)
  #define DEBUG_PRINT(x)     Serial.print(x)
  #define DEBUG_PRINTLN(x)   Serial.println(x)
#else
  #define DEBUG_INIT()
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
#endif

#if DEBUG_SERIAL && DEBUG_TOUCH
  #define DEBUG_TOUCH_PRINT(x)   Serial.print(x)
  #define DEBUG_TOUCH_PRINTLN(x) Serial.println(x)
#else
  #define DEBUG_TOUCH_PRINT(x)
  #define DEBUG_TOUCH_PRINTLN(x)
#endif

#if DEBUG_SERIAL && DEBUG_LORA
  #define DEBUG_LORA_PRINT(x)    Serial.print(x)
  #define DEBUG_LORA_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_LORA_PRINT(x)
  #define DEBUG_LORA_PRINTLN(x)
#endif

#if DEBUG_SERIAL && DEBUG_WIFI
  #define DEBUG_WIFI_PRINT(x)    Serial.print(x)
  #define DEBUG_WIFI_PRINTLN(x)  Serial.println(x)
#else
  #define DEBUG_WIFI_PRINT(x)
  #define DEBUG_WIFI_PRINTLN(x)
#endif

#if DEBUG_SERIAL && DEBUG_LCD
  #define DEBUG_LCD_PRINT(x)     Serial.print(x)
  #define DEBUG_LCD_PRINTLN(x)   Serial.println(x)
#else
  #define DEBUG_LCD_PRINT(x)
  #define DEBUG_LCD_PRINTLN(x)
#endif

// ============================================
// PIN DEFINITIONS - LCD
// ============================================
#define LCD_CS      1    // GPIO1 (D0) - Chip Select LCD
#define LCD_DC      2    // GPIO2 (D1) - Data/Command
#define LCD_RST     3    // GPIO3 (D2) - Reset
#define LCD_BL      4    // GPIO4 (D3) - Backlight
#define LCD_SCLK    5    // GPIO5 (D4) - SPI Clock
#define LCD_MOSI    6    // GPIO6 (D5) - SPI MOSI
#define LCD_MISO    44   // GPIO44 (D7) - SPI MISO (for touch)

// Touch XPT2046
#define TOUCH_CS    43   // GPIO43 (D6) - Chip Select Touch

// ============================================
// PIN DEFINITIONS - LoRa SX1262 (B2B)
// ============================================
#define LORA_NSS    41   // CS
#define LORA_SCK     7   // SCK
#define LORA_MOSI    9   // MOSI
#define LORA_MISO    8   // MISO
#define LORA_RST    42   // RESET
#define LORA_BUSY   40   // BUSY
#define LORA_DIO1   39   // IRQ

// ============================================
// UI COLORS (RGB565)
// ============================================
#define COLOR_BG_MAIN       0x1082  // dark navy
#define COLOR_BG_TILE       0x1124  // navy blue
#define COLOR_BG_HEADER     0x0861  // very dark

#define COLOR_ACCENT        0x04B4  // blue
#define COLOR_ACCENT2       0x06DF  // cyan

#define COLOR_TEXT          0xFFFF  // white
#define COLOR_TEXT_SEC      0xA514  // gray
#define COLOR_TEXT_VALUE    0x07F2  // neon green

#define COLOR_STATUS_OK     0x07E0  // green
#define COLOR_STATUS_WARN   0xFD20  // orange
#define COLOR_STATUS_DANGER 0xF800  // red

// Standard colors
#define COLOR_BLACK         0x0000
#define COLOR_WHITE         0xFFFF
#define COLOR_RED           0xF800
#define COLOR_GREEN         0x07E0
#define COLOR_BLUE          0x001F
#define COLOR_CYAN          0x07FF
#define COLOR_PRESSURE      0xFD20  // orange for pressure

// ============================================
// MULTI-DEVICE CONFIGURATION
// ============================================
#define MAX_DEVICES         4           // Maximum number of devices
#define DEVICE_NAME_LEN     16          // Device name length
#define HISTORY_SIZE        24          // 24 history points (24h)
#define DEVICE_TIMEOUT_MS   1800000     // 30 minutes timeout
#define HISTORY_INTERVAL_MS 3600000     // 1 hour between history points

// ============================================
// TOUCH ACTIONS
// ============================================
#define ACTION_NONE         0
#define ACTION_TILE_0       1
#define ACTION_TILE_1       2
#define ACTION_TILE_2       3
#define ACTION_TILE_3       4
#define ACTION_SETTINGS     10
#define ACTION_BACK         11
#define ACTION_EDIT         12
#define ACTION_WIFI_CONFIG  13
#define ACTION_WIFI_SCAN    14
#define ACTION_WIFI_CONNECT 15
#define ACTION_WIFI_DISCONNECT 16
#define ACTION_KEY_PRESS    20
#define ACTION_KEY_OK       21
#define ACTION_KEY_CANCEL   22
#define ACTION_KEY_BACKSPACE 23
#define ACTION_KEY_SHIFT    24
#define ACTION_SELECT_NETWORK 25
#define ACTION_EDIT_NAME    26

// ============================================
// LAYOUT CONSTANTS
// ============================================
#define HEADER_HEIGHT    32
#define FOOTER_HEIGHT    16
#define TILE_WIDTH       152
#define TILE_HEIGHT      95
#define TILE_MARGIN      5
#define TILE_START_Y     37

// ============================================
// LORA PROTOCOL
// ============================================
#define MSG_TYPE_DATA    0x01
#define MSG_TYPE_STATUS  0x02
#define MSG_TYPE_ACK     0x03  // Potwierdzenie odbioru

// ============================================
// WIFI CONFIGURATION
// ============================================
#define WIFI_CONNECT_TIMEOUT_MS  10000   // 10s connection timeout
#define WIFI_MAX_NETWORKS        5       // Max networks to display
#define WIFI_SSID_MAX_LEN        32
#define WIFI_PASS_MAX_LEN        64

// ============================================
// NTP CONFIGURATION
// ============================================
#define NTP_SERVER           "pool.ntp.org"
#define NTP_TIMEZONE         "CET-1CEST,M3.5.0,M10.5.0/3"  // Poland

// ============================================
// SETTINGS SCREEN LAYOUT
// ============================================
#define SETTINGS_ROW_HEIGHT     28
#define SETTINGS_LABEL_X        10
#define SETTINGS_VALUE_X        120
#define SETTINGS_START_Y        38

// ============================================
// ANIMATION CONSTANTS
// ============================================
#define STARTUP_BAR_X           40
#define STARTUP_BAR_Y           140
#define STARTUP_BAR_W           240
#define STARTUP_BAR_H           20

#define ALARM_PULSE_INTERVAL_MS   500
#define TOUCH_FEEDBACK_MS         50
#define SCAN_ANIM_INTERVAL_MS     300

#endif // CONFIG_H
