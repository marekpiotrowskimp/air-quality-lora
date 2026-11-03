/*
 * Air Condition ePaper Display v0.9
 * ==================================
 * 
 * Power optimizations:
 *   - CPU reduced to 80MHz
 *   - WiFi Modem Sleep enabled
 *   - Partial refresh for clock (every 1 min)
 *   - Full refresh only when data changes
 * 
 * Hardware:
 *   - XIAO ESP32-S3
 *   - Seeed Studio ePaper Breakout Board (105990172)
 *   - 2.9" Monochrome ePaper Display 296x128 (SSD1680)
 */

#include "config.h"
#include "splash_bitmap.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>

#define ENABLE_GxEPD2_GFX 1
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// Display 296x128
GxEPD2_BW<GxEPD2_290_T94_V2, GxEPD2_290_T94_V2::HEIGHT> display(
  GxEPD2_290_T94_V2(EPAPER_CS, EPAPER_DC, EPAPER_RST, EPAPER_BUSY)
);

// WiFi & UDP
WiFiUDP udp;
char udpBuffer[UDP_BUFFER_SIZE];

// Device Data
struct DeviceData {
  char name[DEVICE_NAME_LEN];
  bool active;
  float temperature;
  float humidity;
  uint16_t pm2_5;
  uint16_t pm10;
  bool valid;
};

DeviceData devices[MAX_DEVICES];
DeviceData prevDevices[MAX_DEVICES];
uint8_t deviceCount = 0;
uint8_t prevDeviceCount = 0;
uint16_t pressure = 0;
uint16_t prevPressure = 0;
char timeStr[9] = "--:--:--";
char prevTimeStr[9] = "";
uint8_t lastMinute = 255;

uint32_t lastFullRefresh = 0;
uint32_t lastClockRefresh = 0;
bool wifiConnected = false;
bool firstDataReceived = false;

// Power management
#define CLOCK_REFRESH_INTERVAL_MS   60000   // 1 minute for partial refresh
#define FULL_REFRESH_INTERVAL_MS    300000  // 5 min full refresh to prevent ghosting

// ============================================
// SETUP
// ============================================
void setup() {
  // Reduce CPU frequency for power saving
  setCpuFrequencyMhz(80);
  
  DEBUG_INIT();
  delay(500);
  DEBUG_PRINTLN("=== Air Condition ePaper v0.9 ===");
  DEBUG_PRINTF("CPU Freq: %d MHz\n", getCpuFrequencyMhz());
  
  memset(devices, 0, sizeof(devices));
  memset(prevDevices, 0, sizeof(prevDevices));
  
  SPI.begin(EPAPER_SCK, -1, EPAPER_MOSI, EPAPER_CS);
  
  display.init(115200);
  display.setRotation(1);
  display.setTextColor(GxEPD_BLACK);
  
  // Splash screen
  drawSplashWithText("AIR QUALITY", "MONITOR");
  delay(2000);
  
  drawSplashWithText("Connecting...", WIFI_SSID);
  
  connectWiFi();
  
  if (wifiConnected) {
    // Enable WiFi power saving (modem sleep)
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    DEBUG_PRINTLN("WiFi Modem Sleep enabled");
    
    udp.begin(UDP_PORT);
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org");
    drawSplashWithText("Connected!", WiFi.localIP().toString().c_str());
    delay(1500);
    drawWaitingScreen();
  } else {
    drawSplashWithText("No WiFi", "Check settings");
  }
  
  lastFullRefresh = millis();
  lastClockRefresh = millis();
  
  DEBUG_PRINTLN("Setup complete! Power optimizations active.");
}

// ============================================
// LOOP - Power optimized (no light sleep - was causing restarts)
// ============================================
void loop() {
  // WiFi reconnection
  if (WiFi.status() != WL_CONNECTED) {
    if (wifiConnected) {
      wifiConnected = false;
      drawWaitingScreen();
    }
    static uint32_t lastReconnect = 0;
    if (millis() - lastReconnect > 30000) {
      lastReconnect = millis();
      connectWiFi();
      if (wifiConnected) {
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        udp.begin(UDP_PORT);
        drawWaitingScreen();
      }
    }
  }
  
  // UDP data reception
  if (wifiConnected) {
    int packetSize = udp.parsePacket();
    if (packetSize > 0) {
      int len = udp.read(udpBuffer, UDP_BUFFER_SIZE - 1);
      if (len > 0) {
        udpBuffer[len] = '\0';
        parseJsonData(udpBuffer);
      }
    }
  }
  
  // Clock update with partial refresh (every 1 minute)
  if (firstDataReceived && millis() - lastClockRefresh > CLOCK_REFRESH_INTERVAL_MS) {
    updateTimeString();
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 100)) {
      if (timeinfo.tm_min != lastMinute) {
        lastMinute = timeinfo.tm_min;
        
        // Check if full refresh needed (every 5 min to prevent ghosting)
        if (millis() - lastFullRefresh > FULL_REFRESH_INTERVAL_MS) {
          drawMainScreen();  // Full refresh
          lastFullRefresh = millis();
          DEBUG_PRINTLN("Full refresh (anti-ghosting)");
        } else {
          partialRefreshClock();  // Partial refresh only clock area
          DEBUG_PRINTLN("Partial clock refresh");
        }
        lastClockRefresh = millis();
      }
    }
  }
  
  delay(100);  // Simple delay instead of light sleep
}

// ============================================
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
  }
  wifiConnected = (WiFi.status() == WL_CONNECTED);
}

// ============================================
void parseJsonData(const char* json) {
  JsonDocument doc;
  if (deserializeJson(doc, json)) return;
  
  const char* type = doc["type"];
  if (!type || strcmp(type, "air_quality_data") != 0) return;
  
  JsonArray devArray = doc["devices"];
  deviceCount = 0;
  pressure = 0;
  
  for (JsonObject dev : devArray) {
    if (deviceCount >= MAX_DEVICES) break;
    
    DeviceData* d = &devices[deviceCount];
    
    const char* name = dev["name"];
    if (name) {
      strncpy(d->name, name, sizeof(d->name) - 1);
      d->name[sizeof(d->name) - 1] = '\0';
    }
    
    d->active = dev["active"] | false;
    d->temperature = dev["temperature"] | 0.0f;
    d->humidity = dev["humidity"] | 0.0f;
    d->pm2_5 = dev["pm2_5"] | 0;
    d->pm10 = dev["pm10"] | 0;
    d->valid = true;
    
    if (d->active && pressure == 0) {
      pressure = dev["pressure"] | 0;
    }
    
    deviceCount++;
  }
  
  if (hasDataChanged()) {
    memcpy(prevDevices, devices, sizeof(devices));
    prevDeviceCount = deviceCount;
    prevPressure = pressure;
    drawMainScreen();
    lastFullRefresh = millis();
    lastClockRefresh = millis();
    firstDataReceived = true;
  }
}

// ============================================
bool hasDataChanged() {
  if (!firstDataReceived) return true;
  if (deviceCount != prevDeviceCount) return true;
  if (abs((int)pressure - (int)prevPressure) > 1) return true;
  
  for (uint8_t i = 0; i < deviceCount; i++) {
    if (devices[i].active != prevDevices[i].active) return true;
    if (strcmp(devices[i].name, prevDevices[i].name) != 0) return true;
    if (fabs(devices[i].temperature - prevDevices[i].temperature) >= 0.5) return true;
    if (fabs(devices[i].humidity - prevDevices[i].humidity) >= 2.0) return true;
    if (abs((int)devices[i].pm2_5 - (int)prevDevices[i].pm2_5) >= PM_CHANGE_THRESHOLD) return true;
    if (abs((int)devices[i].pm10 - (int)prevDevices[i].pm10) >= PM_CHANGE_THRESHOLD) return true;
  }
  return false;
}

// ============================================
void updateTimeString() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 500)) {
    if (timeinfo.tm_year >= 124) {
      sprintf(timeStr, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
  }
}

// ============================================
// Partial refresh - only clock area in header
// Much faster and uses less power than full refresh
// ============================================
void partialRefreshClock() {
  updateTimeString();
  strcpy(prevTimeStr, timeStr);
  
  // Clock area: right side of header, approximately x=200 to x=296, y=0 to y=19
  // We'll refresh a bit more to be safe
  int16_t clockX = 180;
  int16_t clockY = 0;
  int16_t clockW = 116;  // 296 - 180
  int16_t clockH = 20;
  
  display.setPartialWindow(clockX, clockY, clockW, clockH);
  display.firstPage();
  
  do {
    display.fillRect(clockX, clockY, clockW, clockH, GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);
    
    // Build header right part
    char header[22];
    sprintf(header, "%dhPa %s %s", 
            pressure > 0 ? pressure : 0,
            wifiConnected ? "OK" : "--",
            timeStr);
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(header, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(296 - w - 4, 15);
    display.print(header);
    
  } while (display.nextPage());
}

// ============================================
// SPLASH SCREEN with lung icon and text overlay
// ============================================
void drawSplashWithText(const char* line1, const char* line2) {
  display.setFullWindow();
  display.firstPage();
  
  do {
    display.fillScreen(GxEPD_WHITE);
    
    // Draw lung icon on left side
    int iconX = 30;
    int iconY = (128 - LUNG_ICON_HEIGHT) / 2;
    display.drawBitmap(iconX, iconY, lung_icon_64x64, LUNG_ICON_WIDTH, LUNG_ICON_HEIGHT, GxEPD_BLACK);
    
    // Draw decorative lines
    for (int x = 0; x < 296; x += 2) {
      int y = 8 + sin(x * 0.03) * 4;
      display.drawPixel(x, y, GxEPD_BLACK);
    }
    for (int x = 0; x < 296; x += 2) {
      int y = 120 + sin(x * 0.03 + 1.5) * 4;
      display.drawPixel(x, y, GxEPD_BLACK);
    }
    
    int16_t x1, y1;
    uint16_t w, h;
    
    display.setFont(&FreeSansBold12pt7b);
    display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
    int tx = 110 + (186 - w) / 2;
    int ty = 55;
    
    display.fillRect(tx - 6, ty - h - 3, w + 12, h + 8, GxEPD_WHITE);
    display.drawRect(tx - 6, ty - h - 3, w + 12, h + 8, GxEPD_BLACK);
    display.setCursor(tx, ty);
    display.print(line1);
    
    display.setFont(&FreeSans9pt7b);
    display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
    tx = 110 + (186 - w) / 2;
    ty = 85;
    
    display.fillRect(tx - 4, ty - h - 2, w + 8, h + 6, GxEPD_WHITE);
    display.setCursor(tx, ty);
    display.print(line2);
    
  } while (display.nextPage());
}

// ============================================
// WAITING SCREEN
// ============================================
void drawWaitingScreen() {
  display.setFullWindow();
  display.firstPage();
  
  do {
    display.fillScreen(GxEPD_WHITE);
    
    for (int y = 0; y < 128; y += 10) {
      for (int x = 0; x < 296; x += 10) {
        display.drawPixel(x, y, GxEPD_BLACK);
      }
    }
    
    display.setFont(&FreeSans9pt7b);
    int16_t x1, y1;
    uint16_t w, h;
    
    if (wifiConnected) {
      const char* t1 = "Waiting for data...";
      display.getTextBounds(t1, 0, 0, &x1, &y1, &w, &h);
      int tx = (296 - w) / 2;
      int ty = 40;
      display.fillRect(tx - 8, ty - h - 4, w + 16, h + 10, GxEPD_WHITE);
      display.drawRect(tx - 8, ty - h - 4, w + 16, h + 10, GxEPD_BLACK);
      display.setCursor(tx, ty);
      display.print(t1);
      
      char t2[32];
      sprintf(t2, "UDP port: %d", UDP_PORT);
      display.getTextBounds(t2, 0, 0, &x1, &y1, &w, &h);
      tx = (296 - w) / 2;
      ty = 70;
      display.fillRect(tx - 6, ty - h - 3, w + 12, h + 8, GxEPD_WHITE);
      display.setCursor(tx, ty);
      display.print(t2);
      
      String ip = WiFi.localIP().toString();
      display.getTextBounds(ip.c_str(), 0, 0, &x1, &y1, &w, &h);
      tx = (296 - w) / 2;
      ty = 100;
      display.fillRect(tx - 6, ty - h - 3, w + 12, h + 8, GxEPD_WHITE);
      display.drawRect(tx - 6, ty - h - 3, w + 12, h + 8, GxEPD_BLACK);
      display.setCursor(tx, ty);
      display.print(ip);
    } else {
      const char* t1 = "No WiFi";
      display.getTextBounds(t1, 0, 0, &x1, &y1, &w, &h);
      int tx = (296 - w) / 2;
      int ty = 65;
      display.fillRect(tx - 8, ty - h - 4, w + 16, h + 10, GxEPD_WHITE);
      display.drawRect(tx - 8, ty - h - 4, w + 16, h + 10, GxEPD_BLACK);
      display.setCursor(tx, ty);
      display.print(t1);
    }
  } while (display.nextPage());
}

// ============================================
// MAIN SCREEN - Full refresh
// ============================================
void drawMainScreen() {
  updateTimeString();
  strcpy(prevTimeStr, timeStr);
  
  display.setFullWindow();
  display.firstPage();
  
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold9pt7b);
    
    // Header
    display.setCursor(2, 15);
    display.print("AIR Q.");
    
    char header[22];
    sprintf(header, "%dhPa %s %s", 
            pressure > 0 ? pressure : 0,
            wifiConnected ? "OK" : "--",
            timeStr);
    
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(header, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(296 - w - 4, 15);
    display.print(header);
    
    display.drawLine(0, 19, 296, 19, GxEPD_BLACK);
    
    // Device rows
    if (deviceCount == 0) {
      display.setCursor(80, 75);
      display.print("No devices");
    } else {
      for (uint8_t i = 0; i < deviceCount && i < MAX_DEVICES; i++) {
        int y = 20 + i * 27;
        drawDeviceRow(i, y);
      }
    }
    
  } while (display.nextPage());
}

// ============================================
// Device row
// ============================================
void drawDeviceRow(uint8_t idx, int y) {
  DeviceData* d = &devices[idx];
  int textY = y + 18;
  
  display.setFont(&FreeMonoBold9pt7b);
  
  char line[28];
  
  char name[8];
  strncpy(name, d->name, 7);
  name[7] = '\0';
  for (int i = 0; name[i]; i++) {
    name[i] = toupper(name[i]);
  }
  
  if (!d->active) {
    sprintf(line, "%-7s --offline--", name);
  } else {
    char temp[6];
    if (d->temperature >= 10.0 || d->temperature <= -10.0) {
      sprintf(temp, "%dC", (int)round(d->temperature));
    } else {
      sprintf(temp, "%.1fC", d->temperature);
    }
    
    int hum = (int)round(d->humidity);
    
    sprintf(line, "%-7s %5s %2d%% P:%d/%d", 
            name, temp, hum, d->pm2_5, d->pm10);
  }
  
  display.setCursor(2, textY);
  display.print(line);
  
  if (idx < deviceCount - 1) {
    display.drawLine(2, y + 26, 294, y + 26, GxEPD_BLACK);
  }
}
