/*
 * globals.cpp - Global variable definitions
 * Air Condition Receiver v2.7
 * 
 * NOTE: All arrays and structures have explicit initializers = {}
 * to prevent issues with uninitialized memory at startup.
 */

#include "types.h"

// ============================================
// SHARED SPI INSTANCE FOR LCD AND TOUCH
// ============================================
// IMPORTANT: We use ONE SPIClass instance for both devices
// so they share the same mutex and synchronize properly!
// Previously LCD (Arduino_ESP32SPI) and Touch (SPIClass) had
// separate mutexes despite both using HSPI hardware.
SPIClass sharedSPI(HSPI);

// ============================================
// LIBRARY OBJECTS - LCD
// ============================================
#if ENABLE_LCD
// LCD uses Arduino_HWSPI which accepts an external SPIClass pointer
// This way LCD and Touch share the same SPI mutex!
Arduino_DataBus *bus = new Arduino_HWSPI(
  LCD_DC,      // DC
  LCD_CS,      // CS
  LCD_SCLK,    // SCK
  LCD_MOSI,    // MOSI
  LCD_MISO,    // MISO - needed for Touch on the same SPI
  &sharedSPI,  // Pointer to shared SPI instance
  true         // is_shared_interface = TRUE (shared with Touch)
);

Arduino_GFX *gfx = new Arduino_ST7789(
  bus,
  LCD_RST,   // RST
  0,         // rotation (set in setup)
  true,      // IPS panel
  240,       // width
  320,       // height
  0,         // col_offset
  0          // row_offset
);
#endif

// ============================================
// LIBRARY OBJECTS - TOUCH
// ============================================
#if ENABLE_TOUCH
// Touch uses the same sharedSPI as LCD (initialized in setupTouch)
XPT2046_Touchscreen ts(TOUCH_CS);
#endif

// ============================================
// LIBRARY OBJECTS - LORA
// ============================================
#if ENABLE_LORA
// LoRa uses SEPARATE pins on B2B - FSPI
SPIClass loraSPI(FSPI);
SX1262 *radio = nullptr;
#endif

// ============================================
// STATUS FLAGS
// ============================================
bool lcd_ok = false;
bool touch_ok = false;
bool lora_ok = false;

// ============================================
// LORA STATISTICS
// ============================================
uint32_t packetsReceived = 0;
uint32_t packetsCrcError = 0;

// ============================================
// LEGACY - for compatibility
// ============================================
#if ENABLE_LORA
SensorPacket lastPacket = {};  // Explicit zero initialization
bool hasData = false;
float lastRssi = 0;
float lastSnr = 0;
uint32_t lastPacketTime = 0;
#endif

// ============================================
// MULTI-DEVICE SYSTEM
// ============================================
// Explicit initialization = {} for safety (although struct has default initializers)
DeviceData devices[MAX_DEVICES] = {};
uint8_t deviceCount = 0;

// ============================================
// SCREEN NAVIGATION
// ============================================
Screen currentScreen = SCREEN_HOME;
int8_t selectedDeviceIndex = -1;  // -1 = no selection
bool screenNeedsRedraw = true;

// ============================================
// CHART ROTATION
// ============================================
uint8_t chartPMType = 1;        // 0=PM1.0, 1=PM2.5, 2=PM10
uint32_t lastChartSwitch = 0;

// ============================================
// SPI CONFLICT PREVENTION
// ============================================
volatile bool lcdDrawing = false;   // true when LCD is drawing
volatile bool touchActive = false;  // true when touch is communicating
volatile bool loraActive = false;   // true when LoRa is communicating via SPI
bool loraDataUpdated = false;       // flag: new LoRa data waiting to be displayed
uint32_t lastTouchEndTime = 0;      // time when last touch communication ended

// ============================================
// LORA INTERRUPT FLAG
// ============================================
volatile bool loraPacketReceived = false;  // Interrupt flag - packet received
volatile uint32_t loraIrqCount = 0;        // ISR call counter (debug)

// ============================================
// KEYBOARD TOUCH FEEDBACK
// ============================================
int16_t lastKbTouchX = -1;          // last X position of keyboard click
int16_t lastKbTouchY = -1;          // last Y position of keyboard click
uint32_t lastKbTouchTime = 0;       // time of last click

// ============================================
// USB/LCD REINIT
// ============================================
uint32_t lcdLastReinitTime = 0;

// ============================================
// WIFI/NTP STATUS
// ============================================
bool wifiConnected = false;
bool lastWifiIconState = false;  // For tracking WiFi icon state changes
char wifiSSID[WIFI_SSID_MAX_LEN] = "";
char wifiPassword[WIFI_PASS_MAX_LEN] = "";
int8_t wifiRSSI = 0;
uint32_t wifiConnectedSince = 0;
bool ntpSynced = false;
char currentTimeStr[12] = "--:--:--";

// ============================================
// WIFI SCAN
// ============================================
WiFiNetwork wifiNetworks[WIFI_MAX_NETWORKS] = {};  // Explicit initialization
uint8_t wifiNetworkCount = 0;
bool wifiScanning = false;
int8_t wifiSelectedNetwork = -1;

// ============================================
// KEYBOARD CONTEXT
// ============================================
KeyboardContext kbCtx = {
  {0},     // buffer - full array initialization (not just first char)
  0,       // cursorPos
  32,      // maxLen
  false,   // shifted
  KB_TARGET_SSID,     // target
  SCREEN_WIFI_CONFIG  // returnScreen
};
char kbPressedKey = 0;

// ============================================
// KEYBOARD LAYOUT
// ============================================
const char* kbRowsLower[] = {
  "1234567890",
  "qwertyuiop",
  "asdfghjkl",
  "zxcvbnm.,"
};
const char* kbRowsUpper[] = {
  "!@#$%^&*()",
  "QWERTYUIOP",
  "ASDFGHJKL",
  "ZXCVBNM_-"
};

// ============================================
// ANIMATION STATE
// ============================================
bool alarmPulseState = false;
uint32_t lastAlarmPulseMs = 0;
uint8_t scanAnimState = 0;
uint32_t lastScanAnimMs = 0;
