/*
 * types.h - Data structures and extern declarations
 * Air Condition Receiver v2.7
 */

#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>
#include "config.h"

// ============================================
// LORA PROTOCOL - Packet structure
// ============================================
#if ENABLE_LORA

struct __attribute__((packed)) SensorPacket {
  uint8_t mac[6];        // 0-5:   WiFi MAC ESP32
  uint8_t msgType;       // 6:     Message type
  uint8_t seqNum;        // 7:     Sequence number
  int16_t temperature;   // 8-9:   Temperature × 100
  uint16_t humidity;     // 10-11: Humidity × 100
  uint16_t pressure;     // 12-13: Pressure hPa
  uint16_t pm1_0;        // 14-15: PM1.0 µg/m³
  uint16_t pm2_5;        // 16-17: PM2.5 µg/m³
  uint16_t pm10;         // 18-19: PM10 µg/m³
  uint8_t status;        // 20:    Status flags
  uint16_t crc;          // 21-22: CRC-16/CCITT
};

// ACK Packet structure (8 bytes) - sent by receiver to confirm DATA
struct __attribute__((packed)) AckPacket {
  uint8_t mac[6];        // MAC of device we're acknowledging
  uint8_t msgType;       // MSG_TYPE_ACK
  uint8_t ackedSeqNum;   // Sequence number being acknowledged
};

#endif

// ============================================
// DEVICE DATA STRUCTURE
// ============================================
// IMPORTANT: Default initializers ensure all fields have sensible values
// even before initDevice() is called. This prevents LCD artifacts at startup
// caused by uninitialized historyCount/historyIndex values.
struct DeviceData {
  uint8_t mac[6] = {0};                   // Device MAC address
  char name[DEVICE_NAME_LEN] = {0};       // Name (e.g. "Device E8D4")
  bool active = false;                    // Is device active
  
  // Sensor data
  float temperature = 0.0f;               // Temperature °C
  float humidity = 0.0f;                  // Humidity %
  uint16_t pressure = 0;                  // Pressure hPa
  uint16_t pm1_0 = 0;                     // PM1.0 µg/m³
  uint16_t pm2_5 = 0;                     // PM2.5 µg/m³
  uint16_t pm10 = 0;                      // PM10 µg/m³
  uint8_t status = 0;                     // Status flags
  
  // LoRa data
  float rssi = 0.0f;                      // Signal strength dBm
  float snr = 0.0f;                       // SNR dB
  uint32_t lastSeenMs = 0;                // Last packet time (millis)
  uint8_t lastSeqNum = 0;                 // Last sequence number
  uint32_t packetsReceived = 0;           // Received packets count
  
  // PM history (circular buffer) - for all types
  uint16_t pm1History[HISTORY_SIZE] = {0};    // PM1.0 averaged
  uint16_t pm25History[HISTORY_SIZE] = {0};   // PM2.5 averaged
  uint16_t pm10History[HISTORY_SIZE] = {0};   // PM10 averaged
  uint8_t historyIndex = 0;               // Shared write index
  uint8_t historyCount = 0;               // Shared stored points count
  uint32_t lastHistoryUpdateMs = 0;       // Last history write time
  
  // Accumulation for averaging - all PM types
  uint32_t pm1Sum = 0;                    // PM1.0 values sum
  uint32_t pm25Sum = 0;                   // PM2.5 values sum
  uint32_t pm10Sum = 0;                   // PM10 values sum
  uint16_t pmSampleCount = 0;             // Shared sample count
};

// ============================================
// SCREEN NAVIGATION
// ============================================
enum Screen {
  SCREEN_HOME,          // Main screen with 2×2 tiles
  SCREEN_DETAIL,        // Device details
  SCREEN_EDIT_NAME,     // Name editing (TODO: phase 2)
  SCREEN_SETTINGS,      // System settings
  SCREEN_WIFI_CONFIG,   // WiFi configuration
  SCREEN_KEYBOARD       // On-screen keyboard
};

// ============================================
// WIFI NETWORK STRUCTURE
// ============================================
struct WiFiNetwork {
  char ssid[WIFI_SSID_MAX_LEN];
  int8_t rssi;
  bool encrypted;
};

// ============================================
// KEYBOARD INPUT CONTEXT
// ============================================
enum KeyboardTarget {
  KB_TARGET_SSID,
  KB_TARGET_PASSWORD,
  KB_TARGET_DEVICE_NAME
};

struct KeyboardContext {
  char buffer[WIFI_PASS_MAX_LEN];  // Edited text
  uint8_t cursorPos;               // Cursor position
  uint8_t maxLen;                  // Max length
  bool shifted;                    // Shift active
  KeyboardTarget target;           // What we're editing
  Screen returnScreen;             // Return screen
};

// ============================================
// EXTERN DECLARATIONS - Library objects
// ============================================
#include <SPI.h>

// Shared SPI instance for LCD and Touch (HSPI)
// Both libraries use the same mutex so they properly
// synchronize when accessing the SPI bus
extern SPIClass sharedSPI;

#if ENABLE_LCD
#include <Arduino_GFX_Library.h>
extern Arduino_DataBus *bus;
extern Arduino_GFX *gfx;
#endif

#if ENABLE_TOUCH
#include <XPT2046_Touchscreen.h>
extern XPT2046_Touchscreen ts;
#endif

#if ENABLE_LORA
#include <RadioLib.h>
extern SPIClass loraSPI;
extern SX1262 *radio;
#endif

// ============================================
// EXTERN DECLARATIONS - Status flags
// ============================================
extern bool lcd_ok;
extern bool touch_ok;
extern bool lora_ok;

// ============================================
// EXTERN DECLARATIONS - LoRa statistics
// ============================================
extern uint32_t packetsReceived;
extern uint32_t packetsCrcError;

// ============================================
// EXTERN DECLARATIONS - Legacy (compatibility)
// ============================================
#if ENABLE_LORA
extern SensorPacket lastPacket;
extern bool hasData;
extern float lastRssi;
extern float lastSnr;
extern uint32_t lastPacketTime;
#endif

// ============================================
// EXTERN DECLARATIONS - Multi-device system
// ============================================
extern DeviceData devices[MAX_DEVICES];
extern uint8_t deviceCount;

// ============================================
// EXTERN DECLARATIONS - Screen navigation
// ============================================
extern Screen currentScreen;
extern int8_t selectedDeviceIndex;
extern bool screenNeedsRedraw;

// ============================================
// EXTERN DECLARATIONS - WiFi/NTP status
// ============================================
extern bool wifiConnected;
extern bool lastWifiIconState;  // For tracking WiFi icon state change
extern char wifiSSID[WIFI_SSID_MAX_LEN];
extern char wifiPassword[WIFI_PASS_MAX_LEN];
extern int8_t wifiRSSI;
extern uint32_t wifiConnectedSince;
extern bool ntpSynced;
extern char currentTimeStr[12];  // "HH:MM:SS" or "--:--:--"

// ============================================
// EXTERN DECLARATIONS - WiFi scan
// ============================================
extern WiFiNetwork wifiNetworks[WIFI_MAX_NETWORKS];
extern uint8_t wifiNetworkCount;
extern bool wifiScanning;
extern int8_t wifiSelectedNetwork;  // -1 = no selection

// ============================================
// EXTERN DECLARATIONS - Keyboard
// ============================================
extern KeyboardContext kbCtx;
extern char kbPressedKey;  // Last pressed key
extern const char* kbRowsLower[];
extern const char* kbRowsUpper[];

// ============================================
// EXTERN DECLARATIONS - Chart rotation
// ============================================
extern uint8_t chartPMType;
extern uint32_t lastChartSwitch;

// ============================================
// EXTERN DECLARATIONS - SPI conflict prevention
// ============================================
extern volatile bool lcdDrawing;
extern volatile bool touchActive;
extern volatile bool loraActive;
extern volatile bool loraPacketReceived;
extern volatile uint32_t loraIrqCount;  // ISR call counter (debug)
extern bool loraDataUpdated;
extern uint32_t lastTouchEndTime;

// ============================================
// EXTERN DECLARATIONS - Keyboard touch feedback
// ============================================
extern int16_t lastKbTouchX;
extern int16_t lastKbTouchY;
extern uint32_t lastKbTouchTime;

// ============================================
// EXTERN DECLARATIONS - USB/LCD reinit
// ============================================
extern uint32_t lcdLastReinitTime;

// ============================================
// EXTERN DECLARATIONS - Animation state
// ============================================
extern bool alarmPulseState;
extern uint32_t lastAlarmPulseMs;
extern uint8_t scanAnimState;
extern uint32_t lastScanAnimMs;

// ============================================
// FUNCTION DECLARATIONS - Hardware
// ============================================
#if ENABLE_LCD
bool setupLCD();
void prepareLcdSpi();
void reinitLCD();
void repairLcdState();
#endif

#if ENABLE_TOUCH
bool setupTouch();
bool checkTouch(int16_t* x, int16_t* y);
#endif

#if ENABLE_LORA
bool setupLoRa();
bool receivePacket();
void sendAckPacket(uint8_t* mac, uint8_t seqNum);
uint16_t calculateCRC16(uint8_t* data, size_t len);
void formatMAC(uint8_t* mac, char* buffer);
void onLoRaReceive(void);  // ISR callback for LoRa interrupt
#endif

// ============================================
// FUNCTION DECLARATIONS - Device Manager
// ============================================
void generateDefaultName(uint8_t* mac, char* name);
void initDevice(int8_t index, uint8_t* mac);
int8_t findOrAddDevice(uint8_t* mac);
void updateDeviceData(int8_t index, SensorPacket* pkt, float rssi, float snr);
void updateHistoryIfNeeded(int8_t index);
void checkDeviceTimeouts();
uint8_t getActiveDeviceCount();

// ============================================
// FUNCTION DECLARATIONS - UI Screens
// ============================================
#if ENABLE_LCD
void drawStartupScreen();
void drawMainScreen();
void drawHomeScreen();
void drawDetailScreen(int8_t deviceIndex);
void drawSettingsScreen();
void drawWifiConfigScreen();
void drawKeyboardScreen();
void updateDetailChartOnly(int8_t deviceIndex);
void drawTouchDot(int16_t x, int16_t y);

// UI Helpers
void getTilePosition(uint8_t tilePos, int* x, int* y);
uint16_t getPM25StatusColor(uint16_t pm25);
uint16_t getPM10StatusColor(uint16_t pm10);
uint16_t getPM1StatusColor(uint16_t pm1);
uint16_t lightenColor(uint16_t color);
void drawIcon(int x, int y, const uint16_t* icon, int w, int h, uint16_t tintColor);
void safeDrawIcon(int x, int y, const uint16_t* icon, int w, int h, uint16_t tintColor);
void drawEmptyTile(uint8_t tilePos);
void drawDeviceTile(int8_t deviceIndex, uint8_t tilePos);
void drawHeader();
void drawFooter();
void drawPMChart(int8_t deviceIndex, int chartX, int chartY, int chartW, int chartH, uint8_t pmType);
void updateHeaderTime();
void updateHeaderWifiIcon();

// Animation functions
void drawStartupProgress(uint8_t step, const char* text);
void updateAlarmPulse();
void updateAlarmTiles();
void drawTileFeedback(uint8_t tileIndex);
void drawButtonFeedback(int x, int y, int w, int h);
void updateScanAnimation();
#endif

// ============================================
// FUNCTION DECLARATIONS - Touch Handler
// ============================================
#if ENABLE_TOUCH
bool pointInRect(int16_t x, int16_t y, int rx, int ry, int rw, int rh);
uint8_t getTouchActionHome(int16_t x, int16_t y);
uint8_t getTouchActionDetail(int16_t x, int16_t y);
uint8_t getTouchActionSettings(int16_t x, int16_t y);
uint8_t getTouchActionWifiConfig(int16_t x, int16_t y);
uint8_t getTouchActionKeyboard(int16_t x, int16_t y);
void handleTouch(int16_t x, int16_t y);
#endif

// ============================================
// FUNCTION DECLARATIONS - WiFi/NTP
// ============================================
void setupWiFi();
void setupNTP();
void updateTimeString();
String formatUptime(uint32_t ms);
void scanWiFiNetworks();
void connectToWiFi(const char* ssid, const char* password);
void disconnectWiFi();
void saveWiFiCredentials(const char* ssid, const char* password);
void openKeyboard(KeyboardTarget target, const char* initialText, uint8_t maxLen);
void openKeyboardForDeviceName(int8_t deviceIndex);
void saveDeviceName(int8_t deviceIndex);
void loadDeviceName(int8_t deviceIndex);
void checkWiFiConnection();
void sendUdpBroadcast();

#endif // TYPES_H
