/*
 * hardware.cpp - Hardware initialization functions
 * Air Condition Receiver v2.7
 */

#include "types.h"
#include "icons.h"
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>

// NVS storage for WiFi credentials
Preferences preferences;

// UDP broadcast
WiFiUDP udp;
#define UDP_BROADCAST_PORT 5000
#define UDP_BROADCAST_INTERVAL_MS 10000
uint32_t lastUdpBroadcastMs = 0;

// ============================================
// LCD FUNCTIONS
// ============================================
#if ENABLE_LCD

// Prepares SPI for LCD
// NOTE: Arduino_GFX with is_shared_interface=true manages SPI transactions itself
// We only need to ensure CS pins are in correct state
void prepareLcdSpi() {
  // Ensure TOUCH_CS is HIGH (touch inactive)
  digitalWrite(TOUCH_CS, HIGH);
  
  // DO NOT call endTransaction() - XPT2046 library ends transactions itself
  // Calling endTransaction() without beginTransaction() is undefined behavior!
  
  // Short delay to stabilize the bus (optional)
  delayMicroseconds(10);
  
  // LCD_CS is managed by Arduino_GFX automatically
}

bool setupLCD() {
  DEBUG_LCD_PRINT("[LCD] Initializing ST7789V... ");
  
  // Backlight ON
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  delay(100);
  
  // Initialize GFX with moderate SPI frequency
  // 20 MHz - compromise between 8 MHz (safe) and 40 MHz (default)
  // For ~15cm wires, 20 MHz should work stably
  if (!gfx->begin(20000000)) {
    DEBUG_LCD_PRINTLN("FAILED!");
    lcd_ok = false;
    return false;
  }
  
  // Rotation 3 = landscape with correct orientation
  gfx->setRotation(3);
  gfx->fillScreen(COLOR_BLACK);
  
  lcd_ok = true;
  DEBUG_LCD_PRINTLN("OK");
  DEBUG_LCD_PRINTLN("[LCD] Rotation: 3 (Landscape 320x240), SPI: 20MHz");
  
  return true;
}

// Full LCD reinitialization - used only by triple-tap
void reinitLCD() {
  DEBUG_LCD_PRINTLN("[LCD] Manual reinit (triple-tap)...");
  
  // Hardware reset LCD
  pinMode(LCD_RST, OUTPUT);
  digitalWrite(LCD_RST, LOW);
  delay(20);
  digitalWrite(LCD_RST, HIGH);
  delay(150);
  
  // Backlight ON
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  
  // Full initialization with moderate SPI frequency (20 MHz)
  gfx->begin(20000000);
  gfx->setRotation(3);
  gfx->fillScreen(COLOR_BG_MAIN);
  
  screenNeedsRedraw = true;
  lcdLastReinitTime = millis();
  
  DEBUG_LCD_PRINTLN("[LCD] Reinit complete");
}

// ============================================
// LCD REPAIR - SPI artifacts mitigation
// ============================================

// Repairs LCD/SPI state before returning to Home Screen
// Called after transitions from screens that heavily use LCD
// (Detail Screen with chart rotation is the main source of artifacts)
void repairLcdState() {
  // 1. Ensure CS pins are in correct state (inactive)
  digitalWrite(TOUCH_CS, HIGH);
  
  // 2. Short pause to stabilize SPI bus
  //    Allows any ongoing operations to complete
  delayMicroseconds(100);
  
  // 3. We don't do fillScreen() - that would cause a flash
  //    Instead we rely on drawHomeScreen() to
  //    redraw everything "on top"
  
  DEBUG_LCD_PRINTLN("[LCD] SPI state repaired before Home Screen");
}

#endif // ENABLE_LCD

// ============================================
// TOUCH FUNCTIONS
// ============================================
#if ENABLE_TOUCH

bool setupTouch() {
  DEBUG_TOUCH_PRINT("[TOUCH] Initializing XPT2046... ");
  
  // Set TOUCH_CS as OUTPUT and HIGH (deselected)
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  
  // Touch uses sharedSPI - the same instance as LCD
  // SPI is already initialized by LCD (gfx->begin() calls _spi->begin())
  // Thanks to shared SPIClass instance, LCD and Touch have the same mutex
  // and properly synchronize when accessing the HSPI bus
  
  if (ts.begin(sharedSPI)) {
    ts.setRotation(1);
    touch_ok = true;
    digitalWrite(TOUCH_CS, HIGH);
    DEBUG_TOUCH_PRINTLN("OK (sharedSPI with LCD)");
    return true;
  } else {
    DEBUG_TOUCH_PRINTLN("FAILED!");
    touch_ok = false;
    return false;
  }
}

bool checkTouch(int16_t* x, int16_t* y) {
  if (!touch_ok) return false;
  
  // DO NOT check touch when LCD is drawing
  if (lcdDrawing) return false;
  
  touchActive = true;
  
  // XPT2046 library manages SPI transactions itself
  // Calls beginTransaction() and endTransaction() internally
  bool touched = ts.touched();
  
  if (touched) {
    TS_Point p = ts.getPoint();
    
    // Mapping for rotation 3 (landscape)
    // Corrected: X shift ~8px to the right -> increase upper range
    *x = map(p.x, 200, 4025, 0, 319);
    *y = map(p.y, 3800, 200, 0, 239);
    
    // Boundaries
    if (*x < 0) *x = 0;
    if (*x > 319) *x = 319;
    if (*y < 0) *y = 0;
    if (*y > 239) *y = 239;
  }
  
  // Ensure TOUCH_CS is HIGH (safety)
  digitalWrite(TOUCH_CS, HIGH);
  
  // DO NOT call endTransaction() - XPT2046 library already did!
  // Calling endTransaction() without beginTransaction() is undefined behavior
  
  touchActive = false;
  lastTouchEndTime = millis();
  
  return touched;
}

#endif // ENABLE_TOUCH

// ============================================
// LORA FUNCTIONS
// ============================================
#if ENABLE_LORA

// ============================================
// LORA INTERRUPT HANDLER
// ============================================

// ISR for LoRa interrupt - MUST be minimal!
// Only sets flag, everything else in main loop
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void onLoRaReceive(void) {
  loraPacketReceived = true;
  loraIrqCount++;  // Debug counter
}

uint16_t calculateCRC16(uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void formatMAC(uint8_t* mac, char* buffer) {
  sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Wysyła pakiet ACK i wraca do trybu RX
void sendAckPacket(uint8_t* mac, uint8_t seqNum) {
  if (!lora_ok || radio == nullptr) return;
  
  AckPacket ack;
  memcpy(ack.mac, mac, 6);
  ack.msgType = MSG_TYPE_ACK;
  ack.ackedSeqNum = seqNum;
  
  // Wyłącz callback przerwań na czas TX
  radio->clearPacketReceivedAction();
  
  // Daj czas nadajnikowi na przełączenie w tryb RX (~5ms)
  delay(5);
  
  // Wyślij ACK
  int state = radio->transmit((uint8_t*)&ack, sizeof(ack));
  
  if (state == RADIOLIB_ERR_NONE) {
    DEBUG_LORA_PRINT("[LORA] ACK sent for seq=");
    DEBUG_LORA_PRINTLN(seqNum);
  } else {
    DEBUG_LORA_PRINT("[LORA] ACK TX failed: ");
    DEBUG_LORA_PRINTLN(state);
  }
  
  // Przywróć callback i wróć do RX
  radio->setPacketReceivedAction(onLoRaReceive);
  radio->startReceive();
}

bool setupLoRa() {
  DEBUG_LORA_PRINTLN("[LORA] Initializing SX1262...");
  DEBUG_LORA_PRINT("[LORA] Pins: NSS=");
  DEBUG_LORA_PRINT(LORA_NSS);
  DEBUG_LORA_PRINT(" DIO1=");
  DEBUG_LORA_PRINT(LORA_DIO1);
  DEBUG_LORA_PRINT(" RST=");
  DEBUG_LORA_PRINT(LORA_RST);
  DEBUG_LORA_PRINT(" BUSY=");
  DEBUG_LORA_PRINTLN(LORA_BUSY);
  
  // LoRa uses FSPI on B2B pins
  loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  DEBUG_LORA_PRINTLN("[SPI] LoRa on FSPI");
  
  // Create RadioLib module with dedicated SPI
  Module* mod = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY, loraSPI);
  radio = new SX1262(mod);
  
  int state = radio->begin(
    868.0,    // freq MHz
    125.0,    // BW
    7,        // SF
    5,        // CR
    0x34,     // sync word
    14,       // power
    8         // preamble
  );
  
  if (state != RADIOLIB_ERR_NONE) {
    DEBUG_LORA_PRINT("FAILED! Error: ");
    DEBUG_LORA_PRINTLN(state);
    lora_ok = false;
    return false;
  }
  
  radio->setDio2AsRfSwitch(true);
  
  // Set interrupt callback for packet reception
  radio->setPacketReceivedAction(onLoRaReceive);
  
  // Enable continuous listening (continuous RX)
  state = radio->startReceive();
  if (state != RADIOLIB_ERR_NONE) {
    DEBUG_LORA_PRINT("[LORA] startReceive failed: ");
    DEBUG_LORA_PRINTLN(state);
    lora_ok = false;
    return false;
  }
  
  lora_ok = true;
  DEBUG_LORA_PRINTLN("OK (interrupt RX)");
  
  return true;
}

#endif // ENABLE_LORA

// ============================================
// WIFI FUNCTIONS
// ============================================

void setupWiFi() {
  DEBUG_WIFI_PRINT("[WIFI] Initializing... ");
  
  // Station mode
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  
  // Load saved credentials from NVS
  preferences.begin("wifi", true);  // read-only
  String savedSSID = preferences.getString("ssid", "");
  String savedPass = preferences.getString("pass", "");
  preferences.end();
  
  if (savedSSID.length() > 0) {
    DEBUG_WIFI_PRINT("Starting connection to ");
    DEBUG_WIFI_PRINTLN(savedSSID);
    
    strncpy(wifiSSID, savedSSID.c_str(), WIFI_SSID_MAX_LEN - 1);
    strncpy(wifiPassword, savedPass.c_str(), WIFI_PASS_MAX_LEN - 1);
    
    // Start connection - DO NOT WAIT (non-blocking)
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    
    // Connection will continue in background
    // Status checked in checkWiFiConnection()
    wifiConnected = false;
  } else {
    DEBUG_WIFI_PRINTLN("No saved credentials");
    wifiConnected = false;
  }
}

// Checks WiFi connection status (called in loop)
void checkWiFiConnection() {
  static bool wasConnecting = false;
  static uint32_t connectStartTime = 0;
  
  if (wifiConnected) {
    // Check if still connected
    if (WiFi.status() != WL_CONNECTED) {
      wifiConnected = false;
      ntpSynced = false;
      DEBUG_WIFI_PRINTLN("[WIFI] Connection lost");
    }
    return;
  }
  
  // Check if connection in progress
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    wifiRSSI = WiFi.RSSI();
    wifiConnectedSince = millis();
    wasConnecting = false;
    DEBUG_WIFI_PRINTLN("[WIFI] Connected!");
    DEBUG_WIFI_PRINT("[WIFI] IP: ");
    DEBUG_WIFI_PRINTLN(WiFi.localIP());
    
    // Start NTP after connection
    setupNTP();
    return;
  }
  
  // Start or retry connection (infinite retry with 15s timeout)
  if (strlen(wifiSSID) > 0 && !wasConnecting) {
    wasConnecting = true;
    connectStartTime = millis();
    DEBUG_WIFI_PRINTLN("[WIFI] Connecting...");
    WiFi.begin(wifiSSID, wifiPassword);
  }
  
  if (wasConnecting && millis() - connectStartTime > 15000) {
    wasConnecting = false;
    WiFi.disconnect();
    DEBUG_WIFI_PRINTLN("[WIFI] Connection timeout, will retry");
  }
}

void setupNTP() {
  if (!wifiConnected) {
    DEBUG_WIFI_PRINTLN("[NTP] Skipped (no WiFi)");
    return;
  }
  
  DEBUG_WIFI_PRINTLN("[NTP] Starting sync...");
  
  configTzTime(NTP_TIMEZONE, NTP_SERVER);
  
  // Don't wait - synchronization in background
  // Status will be checked in updateTimeString()
}

void updateTimeString() {
  struct tm timeinfo;
  // When NTP already synced, getLocalTime works instantly
  // When not, use short timeout to not block UI
  uint32_t timeout = ntpSynced ? 100 : 10;
  
  if (getLocalTime(&timeinfo, timeout)) {
    // We have time - check if sensible (year > 2024)
    if (timeinfo.tm_year > 124) {  // tm_year = years since 1900
      if (!ntpSynced) {
        ntpSynced = true;
        DEBUG_WIFI_PRINTLN("[NTP] Synced!");
      }
      sprintf(currentTimeStr, "%02d:%02d:%02d", 
              timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
      strcpy(currentTimeStr, "--:--:--");
    }
  } else {
    strcpy(currentTimeStr, "--:--:--");
  }
}

// Formats uptime in readable form
String formatUptime(uint32_t ms) {
  uint32_t seconds = ms / 1000;
  uint32_t minutes = seconds / 60;
  uint32_t hours = minutes / 60;
  uint32_t days = hours / 24;
  
  char buf[32];
  if (days > 0) {
    sprintf(buf, "%lud %02lu:%02lu", days, hours % 24, minutes % 60);
  } else if (hours > 0) {
    sprintf(buf, "%lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
  } else {
    sprintf(buf, "%lu:%02lu", minutes, seconds % 60);
  }
  return String(buf);
}

// Scan available WiFi networks
void scanWiFiNetworks() {
  DEBUG_WIFI_PRINTLN("[WIFI] Scanning networks...");
  wifiScanning = true;
  wifiNetworkCount = 0;
  wifiSelectedNetwork = -1;
  
  int n = WiFi.scanNetworks();
  DEBUG_WIFI_PRINT("[WIFI] Found ");
  DEBUG_WIFI_PRINT(n);
  DEBUG_WIFI_PRINTLN(" networks");
  
  // Copy max WIFI_MAX_NETWORKS networks (sorted by RSSI)
  for (int i = 0; i < n && wifiNetworkCount < WIFI_MAX_NETWORKS; i++) {
    // Skip SSID duplicates
    bool duplicate = false;
    for (int j = 0; j < wifiNetworkCount; j++) {
      if (strcmp(wifiNetworks[j].ssid, WiFi.SSID(i).c_str()) == 0) {
        duplicate = true;
        break;
      }
    }
    if (duplicate) continue;
    
    strncpy(wifiNetworks[wifiNetworkCount].ssid, WiFi.SSID(i).c_str(), WIFI_SSID_MAX_LEN - 1);
    wifiNetworks[wifiNetworkCount].ssid[WIFI_SSID_MAX_LEN - 1] = '\0';
    wifiNetworks[wifiNetworkCount].rssi = WiFi.RSSI(i);
    wifiNetworks[wifiNetworkCount].encrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
    wifiNetworkCount++;
  }
  
  WiFi.scanDelete();
  wifiScanning = false;
  DEBUG_WIFI_PRINT("[WIFI] Listed ");
  DEBUG_WIFI_PRINT(wifiNetworkCount);
  DEBUG_WIFI_PRINTLN(" unique networks");
}

// Connect to WiFi network
void connectToWiFi(const char* ssid, const char* password) {
  DEBUG_WIFI_PRINT("[WIFI] Connecting to ");
  DEBUG_WIFI_PRINT(ssid);
  DEBUG_WIFI_PRINT("... ");
  
  // Disconnect if connected
  if (wifiConnected) {
    WiFi.disconnect();
    wifiConnected = false;
    delay(100);
  }
  
  // Save credentials
  strncpy(wifiSSID, ssid, WIFI_SSID_MAX_LEN - 1);
  strncpy(wifiPassword, password, WIFI_PASS_MAX_LEN - 1);
  
  WiFi.begin(ssid, password);
  
  // Wait for connection (max 10s)
  uint32_t startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < WIFI_CONNECT_TIMEOUT_MS) {
    delay(100);
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    wifiRSSI = WiFi.RSSI();
    wifiConnectedSince = millis();
    DEBUG_WIFI_PRINTLN("OK");
    DEBUG_WIFI_PRINT("[WIFI] IP: ");
    DEBUG_WIFI_PRINTLN(WiFi.localIP());
    
    // Save credentials to NVS
    saveWiFiCredentials(ssid, password);
    
    // Start NTP
    setupNTP();
  } else {
    DEBUG_WIFI_PRINTLN("FAILED");
    wifiConnected = false;
  }
}

// Disconnect WiFi
void disconnectWiFi() {
  DEBUG_WIFI_PRINTLN("[WIFI] Disconnecting...");
  WiFi.disconnect();
  wifiConnected = false;
  ntpSynced = false;
  strcpy(currentTimeStr, "--:--:--");
}

// Save credentials to NVS
void saveWiFiCredentials(const char* ssid, const char* password) {
  preferences.begin("wifi", false);  // read-write
  preferences.putString("ssid", ssid);
  preferences.putString("pass", password);
  preferences.end();
  DEBUG_WIFI_PRINTLN("[WIFI] Credentials saved to NVS");
}

// Open keyboard with context
void openKeyboard(KeyboardTarget target, const char* initialText, uint8_t maxLen) {
  kbCtx.target = target;
  kbCtx.maxLen = maxLen;
  kbCtx.shifted = false;
  kbCtx.returnScreen = SCREEN_WIFI_CONFIG;
  
  // Copy initial text
  strncpy(kbCtx.buffer, initialText, maxLen);
  kbCtx.buffer[maxLen] = '\0';
  kbCtx.cursorPos = strlen(kbCtx.buffer);
  
  currentScreen = SCREEN_KEYBOARD;
  screenNeedsRedraw = true;
  
  DEBUG_TOUCH_PRINT("[KB] Opened for ");
  DEBUG_TOUCH_PRINTLN(target == KB_TARGET_SSID ? "SSID" : "Password");
}

// Open keyboard for device name editing
void openKeyboardForDeviceName(int8_t deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) return;
  
  kbCtx.target = KB_TARGET_DEVICE_NAME;
  kbCtx.maxLen = DEVICE_NAME_LEN - 1;
  kbCtx.shifted = false;
  kbCtx.returnScreen = SCREEN_DETAIL;
  
  // Copy current name
  strncpy(kbCtx.buffer, devices[deviceIndex].name, DEVICE_NAME_LEN - 1);
  kbCtx.buffer[DEVICE_NAME_LEN - 1] = '\0';
  kbCtx.cursorPos = strlen(kbCtx.buffer);
  
  currentScreen = SCREEN_KEYBOARD;
  screenNeedsRedraw = true;
  
  DEBUG_TOUCH_PRINT("[KB] Opened for device name: ");
  DEBUG_TOUCH_PRINTLN(devices[deviceIndex].name);
}

// Save device name to NVS
void saveDeviceName(int8_t deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) return;
  
  // Create key based on device MAC (last 4 bytes)
  char key[12];
  sprintf(key, "n%02X%02X%02X%02X", 
          devices[deviceIndex].mac[2],
          devices[deviceIndex].mac[3],
          devices[deviceIndex].mac[4],
          devices[deviceIndex].mac[5]);
  
  preferences.begin("devices", false);  // read-write
  preferences.putString(key, devices[deviceIndex].name);
  preferences.end();
  
  DEBUG_PRINT("[NVS] Device name saved: ");
  DEBUG_PRINT(key);
  DEBUG_PRINT(" = ");
  DEBUG_PRINTLN(devices[deviceIndex].name);
}

// Load device name from NVS (if exists)
void loadDeviceName(int8_t deviceIndex) {
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) return;
  
  // Create key based on device MAC
  char key[12];
  sprintf(key, "n%02X%02X%02X%02X", 
          devices[deviceIndex].mac[2],
          devices[deviceIndex].mac[3],
          devices[deviceIndex].mac[4],
          devices[deviceIndex].mac[5]);
  
  preferences.begin("devices", true);  // read-only
  String savedName = preferences.getString(key, "");
  preferences.end();
  
  if (savedName.length() > 0) {
    strncpy(devices[deviceIndex].name, savedName.c_str(), DEVICE_NAME_LEN - 1);
    devices[deviceIndex].name[DEVICE_NAME_LEN - 1] = '\0';
    DEBUG_PRINT("[NVS] Device name loaded: ");
    DEBUG_PRINTLN(devices[deviceIndex].name);
  }
}

// ============================================
// UDP BROADCAST FUNCTIONS
// ============================================

// Sends all device data as JSON via UDP broadcast
void sendUdpBroadcast() {
  if (!wifiConnected) return;
  
  // Check if 10 seconds passed since last send
  if (millis() - lastUdpBroadcastMs < UDP_BROADCAST_INTERVAL_MS) return;
  lastUdpBroadcastMs = millis();
  
  // Get receiver MAC
  uint8_t receiverMac[6];
  WiFi.macAddress(receiverMac);
  char receiverMacStr[18];
  sprintf(receiverMacStr, "%02X:%02X:%02X:%02X:%02X:%02X",
          receiverMac[0], receiverMac[1], receiverMac[2],
          receiverMac[3], receiverMac[4], receiverMac[5]);
  
  // Get timestamp
  time_t now;
  time(&now);
  
  // Build JSON
  String json = "{";
  json += "\"type\":\"air_quality_data\",";
  json += "\"receiver_mac\":\"" + String(receiverMacStr) + "\",";
  json += "\"timestamp\":" + String((unsigned long)now) + ",";
  json += "\"devices\":[";
  
  bool firstDevice = true;
  for (uint8_t i = 0; i < deviceCount; i++) {
    DeviceData* dev = &devices[i];
    
    if (!firstDevice) json += ",";
    firstDevice = false;
    
    // Format device MAC
    char devMacStr[18];
    sprintf(devMacStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            dev->mac[0], dev->mac[1], dev->mac[2],
            dev->mac[3], dev->mac[4], dev->mac[5]);
    
    // Calculate time since last packet
    uint32_t lastSeenSec = (millis() - dev->lastSeenMs) / 1000;
    
    json += "{";
    json += "\"mac\":\"" + String(devMacStr) + "\",";
    json += "\"name\":\"" + String(dev->name) + "\",";
    json += "\"active\":" + String(dev->active ? "true" : "false") + ",";
    json += "\"temperature\":" + String(dev->temperature, 2) + ",";
    json += "\"humidity\":" + String(dev->humidity, 1) + ",";
    json += "\"pressure\":" + String(dev->pressure) + ",";
    json += "\"pm1_0\":" + String(dev->pm1_0) + ",";
    json += "\"pm2_5\":" + String(dev->pm2_5) + ",";
    json += "\"pm10\":" + String(dev->pm10) + ",";
    json += "\"lora_rssi\":" + String(dev->rssi, 1) + ",";
    json += "\"lora_snr\":" + String(dev->snr, 1) + ",";
    json += "\"last_seen_sec\":" + String(lastSeenSec);
    json += "}";
  }
  
  json += "]}";
  
  // Send UDP broadcast — 255.255.255.255 reaches all subnets/interfaces
  IPAddress broadcastIP(255, 255, 255, 255);
  
  udp.beginPacket(broadcastIP, UDP_BROADCAST_PORT);
  udp.print(json);
  udp.endPacket();
  
  DEBUG_WIFI_PRINT("[UDP] Broadcast sent to ");
  DEBUG_WIFI_PRINT(broadcastIP);
  DEBUG_WIFI_PRINT(":");
  DEBUG_WIFI_PRINT(UDP_BROADCAST_PORT);
  DEBUG_WIFI_PRINT(" (");
  DEBUG_WIFI_PRINT(json.length());
  DEBUG_WIFI_PRINTLN(" bytes)");
}
