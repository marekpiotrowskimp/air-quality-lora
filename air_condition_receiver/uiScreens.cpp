/*
 * uiScreens.cpp - UI screen drawing functions
 * Air Condition Receiver v2.7
 */

#include "types.h"
#include "icons.h"
#include <WiFi.h>

#if ENABLE_LCD

// ============================================
// UI HELPER FUNCTIONS
// ============================================

// Calculates tile position (0-3) → x,y
void getTilePosition(uint8_t tilePos, int* x, int* y) {
  // Tile 0: top left, Tile 1: top right
  // Tile 2: bottom left, Tile 3: bottom right
  *x = TILE_MARGIN + (tilePos % 2) * (TILE_WIDTH + TILE_MARGIN);
  *y = TILE_START_Y + (tilePos / 2) * (TILE_HEIGHT + TILE_MARGIN);
}

// Returns PM2.5 status color
uint16_t getPM25StatusColor(uint16_t pm25) {
  if (pm25 > 75) return COLOR_STATUS_DANGER;
  if (pm25 > 35) return COLOR_STATUS_WARN;
  return COLOR_STATUS_OK;
}

// Returns PM10 status color
uint16_t getPM10StatusColor(uint16_t pm10) {
  if (pm10 > 100) return COLOR_STATUS_DANGER;
  if (pm10 > 50) return COLOR_STATUS_WARN;
  return COLOR_STATUS_OK;
}

// Returns PM1.0 status color
uint16_t getPM1StatusColor(uint16_t pm1) {
  if (pm1 > 50) return COLOR_STATUS_DANGER;
  if (pm1 > 25) return COLOR_STATUS_WARN;
  return COLOR_STATUS_OK;
}

// Lightens RGB565 color by ~30%
uint16_t lightenColor(uint16_t color) {
  uint8_t r = (color >> 11) & 0x1F;
  uint8_t g = (color >> 5) & 0x3F;
  uint8_t b = color & 0x1F;
  r = r + (31 - r) * 3 / 10;
  g = g + (63 - g) * 3 / 10;
  b = b + (31 - b) * 3 / 10;
  return (r << 11) | (g << 5) | b;
}

// Draws 16x16 icon from PROGMEM (0x0000 = transparent)
// OLD VERSION - pixel-by-pixel, may cause artifacts after gfx->print()
void drawIcon(int x, int y, const uint16_t* icon, int w, int h, uint16_t tintColor) {
  for (int py = 0; py < h; py++) {
    for (int px = 0; px < w; px++) {
      uint16_t pixel = pgm_read_word(&icon[py * w + px]);
      if (pixel != 0x0000) {
        // If white (0xFFFF), use tintColor
        gfx->drawPixel(x + px, y + py, (pixel == 0xFFFF) ? tintColor : pixel);
      }
    }
  }
}

// Safe icon drawing 16x16 - uses draw16bitRGBBitmap() instead of drawPixel()
// Handles transparency (0x0000) and tint color (replaces 0xFFFF with tintColor)
// This method is more reliable and doesn't depend on GFX state after print()
void safeDrawIcon(int x, int y, const uint16_t* icon, int w, int h, uint16_t tintColor) {
  // Buffer for processed icon (max 16x16 = 256 pixels)
  static uint16_t iconBuffer[256];
  
  // Validate size
  if (w * h > 256) return;
  
  // First fill background (for transparency) - draw rectangle with background color
  // Then draw bitmap with replaced colors
  
  // Prepare buffer with replaced colors
  bool hasTransparent = false;
  for (int i = 0; i < w * h; i++) {
    uint16_t pixel = pgm_read_word(&icon[i]);
    if (pixel == 0x0000) {
      hasTransparent = true;
      iconBuffer[i] = 0x0000;  // Will be handled by mask
    } else if (pixel == 0xFFFF) {
      iconBuffer[i] = tintColor;  // White -> tint color
    } else {
      iconBuffer[i] = pixel;  // Other colors unchanged
    }
  }
  
  // If icon has transparent pixels, we must draw pixel-by-pixel
  // but we do it safely - without mixing with text state
  if (hasTransparent) {
    // Draw only non-transparent pixels
    for (int py = 0; py < h; py++) {
      for (int px = 0; px < w; px++) {
        int idx = py * w + px;
        if (iconBuffer[idx] != 0x0000) {
          gfx->drawPixel(x + px, y + py, iconBuffer[idx]);
        }
      }
    }
  } else {
    // No transparency - can use fast draw16bitRGBBitmap
    gfx->draw16bitRGBBitmap(x, y, iconBuffer, w, h);
  }
}

// ============================================
// STARTUP SCREEN
// ============================================

void drawStartupScreen() {
  if (!lcd_ok) return;
  
  DEBUG_LCD_PRINTLN("[LCD] Drawing startup screen...");
  
  // Test - does fillScreen work after setting rotation?
  DEBUG_LCD_PRINTLN("[LCD] Test fillScreen BLUE...");
  gfx->fillScreen(COLOR_BLUE);
  delay(500);
  
  DEBUG_LCD_PRINTLN("[LCD] Test fillScreen BLACK...");
  gfx->fillScreen(COLOR_BLACK);
  delay(200);
  
  // Test fillRect
  DEBUG_LCD_PRINTLN("[LCD] Test fillRect RED at 10,10...");
  gfx->fillRect(10, 10, 100, 50, COLOR_RED);
  delay(500);
  
  DEBUG_LCD_PRINTLN("[LCD] Test fillRect GREEN at 150,10...");
  gfx->fillRect(150, 10, 100, 50, COLOR_GREEN);
  delay(500);
  
  DEBUG_LCD_PRINTLN("[LCD] Startup screen drawn.");
}

// ============================================
// STARTUP PROGRESS SCREEN (Animation)
// ============================================

// Draws startup progress screen with loading bar
// step: 0-3 (LCD, Touch, LoRa, WiFi)
// text: current initialization step description
void drawStartupProgress(uint8_t step, const char* text) {
  if (!lcd_ok) return;
  
  lcdDrawing = true;
  
  // Clear screen on first step
  if (step == 0) {
    gfx->fillScreen(COLOR_BG_MAIN);
    
    // Title
    gfx->setTextColor(COLOR_ACCENT2);
    gfx->setTextSize(2);
    gfx->setCursor(45, 60);
    gfx->print("AIR QUALITY");
    
    gfx->setTextColor(COLOR_TEXT_SEC);
    gfx->setTextSize(1);
    gfx->setCursor(115, 90);
    gfx->print("MONITOR v2.7");
    
    // Progress bar frame
    gfx->drawRect(STARTUP_BAR_X - 1, STARTUP_BAR_Y - 1, 
                  STARTUP_BAR_W + 2, STARTUP_BAR_H + 2, COLOR_TEXT_SEC);
  }
  
  // Clear previous progress bar fill
  gfx->fillRect(STARTUP_BAR_X, STARTUP_BAR_Y, STARTUP_BAR_W, STARTUP_BAR_H, COLOR_BG_MAIN);
  
  // Calculate progress (0-100%)
  int progress = (step + 1) * 25;  // 25%, 50%, 75%, 100%
  int fillW = (STARTUP_BAR_W * progress) / 100;
  
  // Draw progress bar fill
  if (fillW > 0) {
    gfx->fillRect(STARTUP_BAR_X, STARTUP_BAR_Y, fillW, STARTUP_BAR_H, COLOR_ACCENT);
  }
  
  // Clear text area
  gfx->fillRect(0, STARTUP_BAR_Y + 30, 320, 20, COLOR_BG_MAIN);
  
  // Draw step text
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  
  // Center text
  int textWidth = strlen(text) * 6;  // 6px per char
  int textX = (320 - textWidth) / 2;
  gfx->setCursor(textX, STARTUP_BAR_Y + 35);
  gfx->print(text);
  
  lcdDrawing = false;
  
  // Small delay for visual effect
  delay(200);
}

// ============================================
// ANIMATION FUNCTIONS
// ============================================

// Updates alarm pulse state and redraws tiles with alarm if needed
void updateAlarmPulse() {
  if (!lcd_ok) return;
  if (currentScreen != SCREEN_HOME) return;
  
  // Check if any device has PM2.5 > 75
  bool hasAlarm = false;
  for (uint8_t i = 0; i < deviceCount; i++) {
    if (devices[i].active && devices[i].pm2_5 > 75) {
      hasAlarm = true;
      break;
    }
  }
  
  if (!hasAlarm) return;
  
  // Check if 500ms passed
  if (millis() - lastAlarmPulseMs < ALARM_PULSE_INTERVAL_MS) return;
  lastAlarmPulseMs = millis();
  
  // Toggle pulse state
  alarmPulseState = !alarmPulseState;
  
  // Redraw only alarm tiles
  updateAlarmTiles();
}

// Redraws ONLY status bars of tiles with PM2.5 > 75 (alarm)
void updateAlarmTiles() {
  if (!lcd_ok) return;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  for (uint8_t i = 0; i < deviceCount; i++) {
    if (!devices[i].active || devices[i].pm2_5 <= 75) continue;
    
    int x, y;
    getTilePosition(i, &x, &y);
    
    // Determine color based on pulse state
    uint16_t barColor = alarmPulseState ? 
                        lightenColor(COLOR_STATUS_DANGER) : 
                        COLOR_STATUS_DANGER;
    
    // Redraw only status bar (5px height at top of tile)
    gfx->fillRoundRect(x, y, TILE_WIDTH, 5, 3, barColor);
  }
  
  lcdDrawing = false;
}

// Draws tile with highlighted background (touch feedback)
void drawTileFeedback(uint8_t tileIndex) {
  if (!lcd_ok) return;
  if (tileIndex >= MAX_DEVICES) return;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  int x, y;
  getTilePosition(tileIndex, &x, &y);
  
  // Draw highlighted tile background
  uint16_t highlightColor = lightenColor(COLOR_BG_TILE);
  gfx->fillRoundRect(x, y, TILE_WIDTH, TILE_HEIGHT, 6, highlightColor);
  
  // Draw border for extra visibility
  gfx->drawRoundRect(x, y, TILE_WIDTH, TILE_HEIGHT, 6, COLOR_ACCENT);
  
  lcdDrawing = false;
}

// Draws button with highlighted background (touch feedback)
void drawButtonFeedback(int x, int y, int w, int h) {
  if (!lcd_ok) return;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  // Draw highlighted rectangle
  uint16_t highlightColor = lightenColor(COLOR_ACCENT);
  gfx->fillRoundRect(x, y, w, h, 4, highlightColor);
  
  lcdDrawing = false;
}

// Updates "Scanning..." animation on WiFi Config screen
void updateScanAnimation() {
  if (!lcd_ok) return;
  if (!wifiScanning) return;
  if (currentScreen != SCREEN_WIFI_CONFIG) return;
  
  // Check if 300ms passed
  if (millis() - lastScanAnimMs < SCAN_ANIM_INTERVAL_MS) return;
  lastScanAnimMs = millis();
  
  // Cycle animation state: 0 -> 1 -> 2 -> 0
  scanAnimState = (scanAnimState + 1) % 3;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  // Clear scanning text area (y=114, where network list starts)
  gfx->fillRect(10, 112, 180, 20, COLOR_BG_MAIN);
  
  // Draw animated text
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  gfx->setCursor(10, 117);
  
  switch (scanAnimState) {
    case 0: gfx->print("Scanning."); break;
    case 1: gfx->print("Scanning.."); break;
    case 2: gfx->print("Scanning..."); break;
  }
  
  lcdDrawing = false;
}

// ============================================
// TILE DRAWING
// ============================================

// Draws empty tile (waiting for device)
void drawEmptyTile(uint8_t tilePos) {
  int x, y;
  getTilePosition(tilePos, &x, &y);
  
  // Tile background
  gfx->fillRoundRect(x, y, TILE_WIDTH, TILE_HEIGHT, 6, COLOR_BG_TILE);
  
  // "Waiting..." text
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  gfx->setCursor(x + 30, y + 35);
  gfx->print("Waiting");
  gfx->setCursor(x + 20, y + 50);
  gfx->print("for device...");
}

// Draws device tile
void drawDeviceTile(int8_t deviceIndex, uint8_t tilePos) {
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) {
    drawEmptyTile(tilePos);
    return;
  }
  
  DeviceData* dev = &devices[deviceIndex];
  
  int x, y;
  getTilePosition(tilePos, &x, &y);
  
  // Tile background
  gfx->fillRoundRect(x, y, TILE_WIDTH, TILE_HEIGHT, 6, COLOR_BG_TILE);
  
  // PM2.5 status bar (top of tile) - with alarm pulse support
  uint16_t pm25Color;
  if (!dev->active) {
    pm25Color = COLOR_TEXT_SEC;
  } else if (dev->pm2_5 > 75) {
    // Alarm! Use pulse color
    pm25Color = alarmPulseState ? lightenColor(COLOR_STATUS_DANGER) : COLOR_STATUS_DANGER;
  } else {
    pm25Color = getPM25StatusColor(dev->pm2_5);
  }
  uint16_t pm10Color = dev->active ? getPM10StatusColor(dev->pm10) : COLOR_TEXT_SEC;
  gfx->fillRoundRect(x, y, TILE_WIDTH, 5, 3, pm25Color);
  
  // Inactive device - gray overlay
  if (!dev->active) {
    gfx->setTextColor(COLOR_TEXT_SEC);
    gfx->setTextSize(1);
    gfx->setCursor(x + 5, y + 10);
    gfx->print(dev->name);
    gfx->setCursor(x + 30, y + 45);
    gfx->print("OFFLINE");
    return;
  }
  
  // Device name
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  gfx->setCursor(x + 5, y + 10);
  gfx->print(dev->name);
  
  // Time since last packet (top right corner)
  uint32_t secAgo = (millis() - dev->lastSeenMs) / 1000;
  gfx->setTextColor(COLOR_TEXT_SEC);
  char timeStr[8];
  if (secAgo < 60) {
    sprintf(timeStr, "%ds", (int)secAgo);
  } else if (secAgo < 3600) {
    sprintf(timeStr, "%dm", (int)(secAgo / 60));
  } else {
    sprintf(timeStr, "%dh", (int)(secAgo / 3600));
  }
  gfx->setCursor(x + TILE_WIDTH - 25, y + 10);
  gfx->print(timeStr);
  
  // Temperature and humidity (large font)
  gfx->setTextColor(COLOR_TEXT_VALUE);
  gfx->setTextSize(2);
  gfx->setCursor(x + 5, y + 25);
  gfx->print(dev->temperature, 1);
  gfx->setTextSize(1);
  gfx->print("C");
  
  gfx->setTextSize(2);
  gfx->setCursor(x + 85, y + 25);
  gfx->print((int)round(dev->humidity));
  gfx->setTextSize(1);
  gfx->print("%");
  
  // === PM2.5: value + mini bar ===
  int pmY = y + 48;
  int barW = 60;
  int barH = 6;
  int barX = x + 85;
  
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  gfx->setCursor(x + 5, pmY);
  gfx->print("PM2.5:");
  
  gfx->setTextColor(pm25Color);
  gfx->setCursor(x + 45, pmY);
  gfx->print(dev->pm2_5);
  
  // Mini PM2.5 bar
  gfx->drawRect(barX, pmY, barW, barH, COLOR_TEXT_SEC);
  int pm25_constrained = dev->pm2_5 > 100 ? 100 : dev->pm2_5;
  int fillW25 = (barW - 2) * pm25_constrained / 100;
  if (fillW25 > 0) {
    gfx->fillRect(barX + 1, pmY + 1, fillW25, barH - 2, pm25Color);
  }
  
  // === PM10: value + mini bar ===
  pmY += 14;
  
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(x + 5, pmY);
  gfx->print("PM10:");
  
  gfx->setTextColor(pm10Color);
  gfx->setCursor(x + 45, pmY);
  gfx->print(dev->pm10);
  
  // Mini PM10 bar
  gfx->drawRect(barX, pmY, barW, barH, COLOR_TEXT_SEC);
  int pm10_constrained = dev->pm10 > 150 ? 150 : dev->pm10;
  int fillW10 = (barW - 2) * pm10_constrained / 150;
  if (fillW10 > 0) {
    gfx->fillRect(barX + 1, pmY + 1, fillW10, barH - 2, pm10Color);
  }
  
  // === Pressure at bottom ===
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  gfx->setCursor(x + 5, y + 82);
  gfx->print(dev->pressure);
  gfx->print(" hPa");
}

// ============================================
// HEADER / FOOTER
// ============================================

// Draws header
void drawHeader() {
  gfx->fillRect(0, 0, 320, HEADER_HEIGHT, COLOR_BG_HEADER);
  
  // Title
  gfx->setTextColor(COLOR_ACCENT2);
  gfx->setTextSize(2);
  gfx->setCursor(10, 8);
  gfx->print("AIR QUALITY");
  
  // WiFi icon (before time, x=220)
  uint16_t wifiColor;
  const uint16_t* wifiIcon;
  
  if (!wifiConnected) {
    wifiIcon = ICON_WIFI_OFF;
    wifiColor = COLOR_TEXT_SEC;  // gray when disconnected
  } else {
    int8_t rssi = WiFi.RSSI();
    if (rssi > -60) {
      wifiIcon = ICON_WIFI_STRONG;
      wifiColor = COLOR_STATUS_OK;  // green - strong signal
    } else if (rssi > -75) {
      wifiIcon = ICON_WIFI_MEDIUM;
      wifiColor = COLOR_STATUS_WARN;  // orange - medium
    } else {
      wifiIcon = ICON_WIFI_WEAK;
      wifiColor = COLOR_STATUS_DANGER;  // red - weak
    }
  }
  safeDrawIcon(222, 8, wifiIcon, 16, 16, wifiColor);
  
  // Time (4px before settings icon) - 8 chars × 6px = 48px
  updateTimeString();
  gfx->setTextColor(ntpSynced ? COLOR_TEXT : COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  gfx->setCursor(244, 12);  // 296 - 4 - 48 = 244
  gfx->print(currentTimeStr);
  
  // Settings icon (x=296)
  safeDrawIcon(296, 8, ICON_SETTINGS, 16, 16, COLOR_TEXT);
}

// Refreshes only time in header (without redrawing entire screen)
void updateHeaderTime() {
  if (!lcd_ok) return;
  
  // Remember previous time
  char prevTime[12];
  strcpy(prevTime, currentTimeStr);
  
  // Update time string
  updateTimeString();
  
  // If unchanged, don't draw
  if (strcmp(prevTime, currentTimeStr) == 0) return;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  // Clear time area (48px width, 8px height)
  gfx->fillRect(244, 10, 50, 12, COLOR_BG_HEADER);
  
  // Draw new time
  gfx->setTextColor(ntpSynced ? COLOR_TEXT : COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  gfx->setCursor(244, 12);
  gfx->print(currentTimeStr);
  
  lcdDrawing = false;
}

// Refreshes only WiFi icon in header (when connection state changes)
void updateHeaderWifiIcon() {
  if (!lcd_ok) return;
  
  // Check if WiFi state changed
  if (wifiConnected == lastWifiIconState) return;
  
  // Remember new state
  lastWifiIconState = wifiConnected;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  // Clear WiFi icon area (x=222, y=8, 16x16)
  gfx->fillRect(220, 6, 20, 20, COLOR_BG_HEADER);
  
  // Draw WiFi icon with current state
  uint16_t wifiColor;
  const uint16_t* wifiIcon;
  
  if (!wifiConnected) {
    wifiIcon = ICON_WIFI_OFF;
    wifiColor = COLOR_TEXT_SEC;
  } else {
    int8_t rssi = WiFi.RSSI();
    if (rssi > -60) {
      wifiIcon = ICON_WIFI_STRONG;
      wifiColor = COLOR_STATUS_OK;
    } else if (rssi > -75) {
      wifiIcon = ICON_WIFI_MEDIUM;
      wifiColor = COLOR_STATUS_WARN;
    } else {
      wifiIcon = ICON_WIFI_WEAK;
      wifiColor = COLOR_STATUS_DANGER;
    }
  }
  safeDrawIcon(222, 8, wifiIcon, 16, 16, wifiColor);
  
  lcdDrawing = false;
}

// Draws footer
void drawFooter() {
  int y = 240 - FOOTER_HEIGHT;
  gfx->fillRect(0, y, 320, FOOTER_HEIGHT, COLOR_BG_HEADER);
  
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  gfx->setCursor(5, y + 4);
  gfx->print("RX:");
  gfx->print(packetsReceived);
  gfx->print("  Devices:");
  gfx->print(getActiveDeviceCount());
  gfx->print("/");
  gfx->print(deviceCount);
}

// ============================================
// HOME SCREEN
// ============================================

void drawMainScreen() {
  // Legacy - redirect to new system
  drawHomeScreen();
}

// Main Home Screen drawing function
void drawHomeScreen() {
  if (!lcd_ok) return;
  
  lcdDrawing = true;   // SPI lock
  prepareLcdSpi();     // Make sure SPI is ready for LCD
  
  gfx->fillScreen(COLOR_BG_MAIN);
  
  drawHeader();
  
  // Draw 4 tiles
  for (uint8_t i = 0; i < MAX_DEVICES; i++) {
    if (i < deviceCount) {
      drawDeviceTile(i, i);
    } else {
      drawEmptyTile(i);
    }
  }
  
  // Footer removed - unnecessary
  
  lcdDrawing = false;  // Release SPI
  screenNeedsRedraw = false;
}

// ============================================
// PM CHART
// ============================================

// Draws PM history chart with pending bar
// pmType: 0=PM1.0, 1=PM2.5, 2=PM10
void drawPMChart(int8_t deviceIndex, int chartX, int chartY, int chartW, int chartH, uint8_t pmType) {
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) return;
  
  DeviceData* dev = &devices[deviceIndex];
  
  // Parameters depending on PM type
  uint16_t* historyArray;
  uint16_t maxValue;
  uint16_t threshold1, threshold2;
  uint16_t currentPending;
  
  switch (pmType) {
    case 0:  // PM1.0
      historyArray = dev->pm1History;
      maxValue = 75;
      threshold1 = 25;
      threshold2 = 50;
      currentPending = (dev->pmSampleCount > 0) ? (dev->pm1Sum / dev->pmSampleCount) : 0;
      break;
    case 2:  // PM10
      historyArray = dev->pm10History;
      maxValue = 150;
      threshold1 = 50;
      threshold2 = 100;
      currentPending = (dev->pmSampleCount > 0) ? (dev->pm10Sum / dev->pmSampleCount) : 0;
      break;
    default:  // PM2.5
      historyArray = dev->pm25History;
      maxValue = 100;
      threshold1 = 35;
      threshold2 = 75;
      currentPending = (dev->pmSampleCount > 0) ? (dev->pm25Sum / dev->pmSampleCount) : 0;
      break;
  }
  
  // Chart frame
  gfx->drawRect(chartX, chartY, chartW, chartH, COLOR_TEXT_SEC);
  
  // Y-axis labels
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  
  char buf[6];
  sprintf(buf, "%d", maxValue);
  gfx->setCursor(chartX - 25, chartY);
  gfx->print(buf);
  sprintf(buf, "%d", maxValue/2);
  gfx->setCursor(chartX - 19, chartY + chartH/2 - 4);
  gfx->print(buf);
  gfx->setCursor(chartX - 13, chartY + chartH - 8);
  gfx->print("0");
  
  // Horizontal grid lines
  for (int i = 1; i < 4; i++) {
    int lineY = chartY + (chartH * i / 4);
    for (int x = chartX + 2; x < chartX + chartW - 2; x += 4) {
      gfx->drawPixel(x, lineY, COLOR_TEXT_SEC);
    }
  }
  
  // Thresholds (horizontal lines)
  int thresholdY1 = chartY + chartH - (chartH * threshold1 / maxValue);
  int thresholdY2 = chartY + chartH - (chartH * threshold2 / maxValue);
  
  gfx->drawFastHLine(chartX + 1, thresholdY1, chartW - 2, COLOR_STATUS_WARN);
  gfx->drawFastHLine(chartX + 1, thresholdY2, chartW - 2, COLOR_STATUS_DANGER);
  
  // Calculate number of bars (history + pending)
  uint8_t totalBars = dev->historyCount;
  bool showPending = (dev->pmSampleCount > 0);
  if (showPending) totalBars++;
  
  if (totalBars == 0) {
    gfx->setCursor(chartX + chartW/2 - 30, chartY + chartH/2 - 4);
    gfx->setTextColor(COLOR_TEXT_SEC);
    gfx->print("No data");
    return;
  }
  
  // Sanity check - historyCount may be corrupted (uninitialized)
  // If exceeds HISTORY_SIZE, reset to safe values
  if (dev->historyCount > HISTORY_SIZE) {
    dev->historyCount = 0;
    dev->historyIndex = 0;
    return;
  }
  
  // Bar width
  int barWidth = (chartW - 4) / (HISTORY_SIZE + 1);
  if (barWidth < 2) barWidth = 2;
  
  // Draw history bars (oldest on left)
  for (uint8_t i = 0; i < dev->historyCount; i++) {
    uint8_t idx = (dev->historyIndex - dev->historyCount + i + HISTORY_SIZE) % HISTORY_SIZE;
    uint16_t value = historyArray[idx];
    
    if (value > maxValue) value = maxValue;
    
    int barH = (chartH - 2) * value / maxValue;
    int barX = chartX + 2 + i * barWidth;
    int barY = chartY + chartH - 1 - barH;
    
    uint16_t barColor;
    uint16_t rawValue = historyArray[idx];
    switch (pmType) {
      case 0:  barColor = getPM1StatusColor(rawValue); break;
      case 2:  barColor = getPM10StatusColor(rawValue); break;
      default: barColor = getPM25StatusColor(rawValue); break;
    }
    
    if (barH > 0) {
      gfx->fillRect(barX, barY, barWidth - 1, barH, barColor);
    }
  }
  
  // Draw pending bar (current value - lightened color)
  if (showPending) {
    uint16_t value = currentPending;
    if (value > maxValue) value = maxValue;
    
    int barH = (chartH - 2) * value / maxValue;
    int barX = chartX + 2 + dev->historyCount * barWidth;
    int barY = chartY + chartH - 1 - barH;
    
    uint16_t barColor;
    switch (pmType) {
      case 0:  barColor = lightenColor(getPM1StatusColor(currentPending)); break;
      case 2:  barColor = lightenColor(getPM10StatusColor(currentPending)); break;
      default: barColor = lightenColor(getPM25StatusColor(currentPending)); break;
    }
    
    if (barH > 0) {
      gfx->fillRect(barX, barY, barWidth - 1, barH, barColor);
    }
  }
}

// ============================================
// DETAIL SCREEN
// ============================================

// Draws device detail screen
void drawDetailScreen(int8_t deviceIndex) {
  if (!lcd_ok) return;
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) {
    currentScreen = SCREEN_HOME;
    drawHomeScreen();
    return;
  }
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  DeviceData* dev = &devices[deviceIndex];
  
  // Check if device is active - if not, return to Home
  // (may happen when device becomes inactive while viewing)
  if (!dev->active) {
    lcdDrawing = false;
    currentScreen = SCREEN_HOME;
    screenNeedsRedraw = true;
    return;
  }
  
  // Reset chart rotation timer on entry
  lastChartSwitch = millis();
  
  gfx->fillScreen(COLOR_BG_MAIN);
  
  // ========== HEADER (y=0, h=30) ==========
  gfx->fillRect(0, 0, 320, 30, COLOR_BG_HEADER);
  
  // Back icon (left corner)
  safeDrawIcon(8, 7, ICON_BACK, 16, 16, COLOR_TEXT);
  
  // Device name (center)
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(2);
  gfx->setCursor(35, 7);
  gfx->print(dev->name);
  
  // Edit icon (next to name) - position depends on name length
  int nameWidth = strlen(dev->name) * 12;  // 12px per char in size 2
  safeDrawIcon(40 + nameWidth, 7, ICON_EDIT, 16, 16, COLOR_ACCENT2);
  
  // Settings icon (right corner)
  safeDrawIcon(296, 7, ICON_SETTINGS, 16, 16, COLOR_TEXT);
  
  // ========== COMPACT LINE: Temp/Hum/Press (y=32, h=20) ==========
  int y = 34;
  
  // FIRST DRAW ALL ICONS (before text!)
  // This prevents artifacts caused by GFX state after print()
  safeDrawIcon(5, y, ICON_THERMOMETER, 16, 16, COLOR_TEXT_VALUE);
  safeDrawIcon(105, y, ICON_HUMIDITY, 16, 16, COLOR_ACCENT2);
  safeDrawIcon(185, y, ICON_PRESSURE, 16, 16, COLOR_PRESSURE);
  
  // THEN DRAW TEXTS
  // Temperature
  gfx->setTextColor(COLOR_TEXT_VALUE);
  gfx->setTextSize(2);
  gfx->setCursor(23, y);
  gfx->print(dev->temperature, 1);
  gfx->print("C");
  
  // Humidity
  gfx->setTextColor(COLOR_ACCENT2);
  gfx->setCursor(123, y);
  gfx->print((int)round(dev->humidity));
  gfx->print("%");
  
  // Pressure
  gfx->setTextColor(COLOR_PRESSURE);
  gfx->setCursor(203, y);
  gfx->print(dev->pressure);
  gfx->print(" hPa");
  
  // ========== PM SECTION with 3 bars (y=55, h=66) ==========
  y = 58;
  int barX = 70;
  int barW = 180;
  int barH = 10;
  int rowH = 22;
  
  // --- PM1.0 ---
  uint16_t pm1Color = getPM1StatusColor(dev->pm1_0);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  gfx->setCursor(10, y + 1);
  gfx->print("PM1.0:");
  
  gfx->setTextColor(pm1Color);
  gfx->setCursor(48, y + 1);
  char pmBuf[8];  // Increased buffer (was 6) - safe for values up to 65535
  snprintf(pmBuf, sizeof(pmBuf), "%3d", dev->pm1_0);
  gfx->print(pmBuf);
  
  // PM1.0 bar (max 75)
  gfx->drawRect(barX, y, barW, barH, COLOR_TEXT_SEC);
  int pm1_constrained = dev->pm1_0 > 75 ? 75 : dev->pm1_0;
  int fillW1 = (barW - 2) * pm1_constrained / 75;
  if (fillW1 > 0) {
    gfx->fillRect(barX + 1, y + 1, fillW1, barH - 2, pm1Color);
  }
  
  // PM1.0 status
  gfx->setCursor(barX + barW + 5, y + 1);
  gfx->setTextColor(pm1Color);
  if (dev->pm1_0 > 50) gfx->print("BAD");
  else if (dev->pm1_0 > 25) gfx->print("WARN");
  else gfx->print("OK");
  
  // --- PM2.5 ---
  y += rowH;
  uint16_t pm25Color = getPM25StatusColor(dev->pm2_5);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(10, y + 1);
  gfx->print("PM2.5:");
  
  gfx->setTextColor(pm25Color);
  gfx->setCursor(48, y + 1);
  snprintf(pmBuf, sizeof(pmBuf), "%3d", dev->pm2_5);
  gfx->print(pmBuf);
  
  // PM2.5 bar (max 100)
  gfx->drawRect(barX, y, barW, barH, COLOR_TEXT_SEC);
  int pm25_constrained = dev->pm2_5 > 100 ? 100 : dev->pm2_5;
  int fillW25 = (barW - 2) * pm25_constrained / 100;
  if (fillW25 > 0) {
    gfx->fillRect(barX + 1, y + 1, fillW25, barH - 2, pm25Color);
  }
  
  // PM2.5 status
  gfx->setCursor(barX + barW + 5, y + 1);
  gfx->setTextColor(pm25Color);
  if (dev->pm2_5 > 75) gfx->print("BAD");
  else if (dev->pm2_5 > 35) gfx->print("WARN");
  else gfx->print("OK");
  
  // --- PM10 ---
  y += rowH;
  uint16_t pm10Color = getPM10StatusColor(dev->pm10);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(10, y + 1);
  gfx->print("PM10:");
  
  gfx->setTextColor(pm10Color);
  gfx->setCursor(48, y + 1);
  snprintf(pmBuf, sizeof(pmBuf), "%3d", dev->pm10);
  gfx->print(pmBuf);
  
  // PM10 bar (max 150)
  gfx->drawRect(barX, y, barW, barH, COLOR_TEXT_SEC);
  int pm10_constrained = dev->pm10 > 150 ? 150 : dev->pm10;
  int fillW10 = (barW - 2) * pm10_constrained / 150;
  if (fillW10 > 0) {
    gfx->fillRect(barX + 1, y + 1, fillW10, barH - 2, pm10Color);
  }
  
  // PM10 status
  gfx->setCursor(barX + barW + 5, y + 1);
  gfx->setTextColor(pm10Color);
  if (dev->pm10 > 100) gfx->print("BAD");
  else if (dev->pm10 > 50) gfx->print("WARN");
  else gfx->print("OK");
  
  // ========== PM CHART with rotation (y=124, h=90) ==========
  y += rowH + 5;
  
  // Chart label
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  gfx->setCursor(10, y);
  gfx->print("History ");
  switch (chartPMType) {
    case 0: gfx->print("PM1.0"); break;
    case 2: gfx->print("PM10"); break;
    default: gfx->print("PM2.5"); break;
  }
  gfx->print(" (24h):");
  
  // Chart
  y += 12;
  drawPMChart(deviceIndex, 35, y, 280, 70, chartPMType);
  
  // ========== STATUS BAR (y=216, h=24) ==========
  y = 220;
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  
  // Time since last packet
  uint32_t secAgo = (millis() - dev->lastSeenMs) / 1000;
  gfx->setCursor(200, y);
  gfx->print("Last: ");
  if (secAgo < 60) {
    gfx->print(secAgo);
    gfx->print("s");
  } else if (secAgo < 3600) {
    gfx->print(secAgo / 60);
    gfx->print("m");
  } else {
    gfx->print(secAgo / 3600);
    gfx->print("h");
  }
  
  lcdDrawing = false;
  screenNeedsRedraw = false;
}

// Updates ONLY chart on Detail Screen (without flashing)
void updateDetailChartOnly(int8_t deviceIndex) {
  if (!lcd_ok) return;
  if (deviceIndex < 0 || deviceIndex >= MAX_DEVICES) return;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  // Rotate chart type
  chartPMType = (chartPMType + 1) % 3;
  lastChartSwitch = millis();
  
  // Area to clear
  int clearY = 120;
  int clearH = 95;
  
  gfx->fillRect(0, clearY, 320, clearH, COLOR_BG_MAIN);
  
  // Draw chart title
  int y = 124;
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  gfx->setCursor(10, y);
  gfx->print("History ");
  switch (chartPMType) {
    case 0: gfx->print("PM1.0"); break;
    case 2: gfx->print("PM10"); break;
    default: gfx->print("PM2.5"); break;
  }
  gfx->print(" (24h):");
  
  // Draw chart
  y += 12;
  drawPMChart(deviceIndex, 35, y, 280, 70, chartPMType);
  
  lcdDrawing = false;
}

// ============================================
// SETTINGS SCREEN
// ============================================

// Draws settings screen with system statistics
void drawSettingsScreen() {
  if (!lcd_ok) return;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  gfx->fillScreen(COLOR_BG_MAIN);
  
  // ========== HEADER ==========
  gfx->fillRect(0, 0, 320, 30, COLOR_BG_HEADER);
  
  // Back icon
  safeDrawIcon(8, 7, ICON_BACK, 16, 16, COLOR_TEXT);
  
  // Title
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(2);
  gfx->setCursor(35, 7);
  gfx->print("SETTINGS");
  
  int y = SETTINGS_START_Y;
  int rowH = SETTINGS_ROW_HEIGHT;
  
  // ========== SECTION: WiFi Status ==========
  gfx->setTextColor(COLOR_ACCENT2);
  gfx->setTextSize(1);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("--- WiFi ---");
  y += 14;
  
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("Status:");
  gfx->setTextColor(wifiConnected ? COLOR_STATUS_OK : COLOR_STATUS_DANGER);
  gfx->setCursor(SETTINGS_VALUE_X, y);
  gfx->print(wifiConnected ? "Connected" : "Disconnected");
  y += 12;
  
  if (wifiConnected) {
    gfx->setTextColor(COLOR_TEXT_SEC);
    gfx->setCursor(SETTINGS_LABEL_X, y);
    gfx->print("SSID:");
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(SETTINGS_VALUE_X, y);
    gfx->print(wifiSSID);
    y += 12;
    
    gfx->setTextColor(COLOR_TEXT_SEC);
    gfx->setCursor(SETTINGS_LABEL_X, y);
    gfx->print("IP:");
    gfx->setTextColor(COLOR_TEXT);
    gfx->setCursor(SETTINGS_VALUE_X, y);
    gfx->print(WiFi.localIP());
    y += 12;
    
    gfx->setTextColor(COLOR_TEXT_SEC);
    gfx->setCursor(SETTINGS_LABEL_X, y);
    gfx->print("RSSI:");
    int8_t rssi = WiFi.RSSI();
    gfx->setTextColor(rssi > -60 ? COLOR_STATUS_OK : (rssi > -80 ? COLOR_STATUS_WARN : COLOR_STATUS_DANGER));
    gfx->setCursor(SETTINGS_VALUE_X, y);
    gfx->print(rssi);
    gfx->print(" dBm");
    y += 12;
  } else {
    y += 36;  // Skip rows when not connected
  }
  
  // ========== SECTION: LoRa Status ==========
  y += 4;
  gfx->setTextColor(COLOR_ACCENT2);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("--- LoRa ---");
  y += 14;
  
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("Packets RX:");
  gfx->setTextColor(COLOR_TEXT_VALUE);
  gfx->setCursor(SETTINGS_VALUE_X, y);
  gfx->print(packetsReceived);
  y += 12;
  
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("CRC Errors:");
  gfx->setTextColor(packetsCrcError > 0 ? COLOR_STATUS_WARN : COLOR_TEXT);
  gfx->setCursor(SETTINGS_VALUE_X, y);
  gfx->print(packetsCrcError);
  y += 12;
  
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("Devices:");
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(SETTINGS_VALUE_X, y);
  gfx->print(getActiveDeviceCount());
  gfx->print("/");
  gfx->print(deviceCount);
  gfx->print(" active");
  y += 12;
  
  // ========== SECTION: System ==========
  y += 4;
  gfx->setTextColor(COLOR_ACCENT2);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("--- System ---");
  y += 14;
  
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("Uptime:");
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(SETTINGS_VALUE_X, y);
  gfx->print(formatUptime(millis()));
  y += 12;
  
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("Free Heap:");
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(SETTINGS_VALUE_X, y);
  gfx->print(ESP.getFreeHeap() / 1024);
  gfx->print(" KB");
  y += 12;
  
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setCursor(SETTINGS_LABEL_X, y);
  gfx->print("Time:");
  gfx->setTextColor(ntpSynced ? COLOR_TEXT_VALUE : COLOR_TEXT_SEC);
  gfx->setCursor(SETTINGS_VALUE_X, y);
  updateTimeString();
  gfx->print(currentTimeStr);
  if (!ntpSynced) {
    gfx->print(" (no sync)");
  }
  
  // ========== BUTTON: WiFi Config (at bottom) ==========
  int btnY = 195;
  int btnW = 300;
  int btnH = 32;
  int btnX = (320 - btnW) / 2;
  
  gfx->fillRoundRect(btnX, btnY, btnW, btnH, 5, COLOR_ACCENT);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(2);
  gfx->setCursor(btnX + 60, btnY + 8);
  gfx->print("Configure WiFi");
  
  lcdDrawing = false;
  screenNeedsRedraw = false;
}

// ============================================
// WIFI CONFIG SCREEN
// ============================================

void drawWifiConfigScreen() {
  if (!lcd_ok) return;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  gfx->fillScreen(COLOR_BG_MAIN);
  
  // ========== HEADER ==========
  gfx->fillRect(0, 0, 320, 30, COLOR_BG_HEADER);
  safeDrawIcon(8, 7, ICON_BACK, 16, 16, COLOR_TEXT);
  
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(2);
  gfx->setCursor(35, 7);
  gfx->print("WiFi Config");
  
  int y = 35;
  
  // ========== CONNECTION STATUS ==========
  gfx->setTextColor(COLOR_TEXT_SEC);
  gfx->setTextSize(1);
  gfx->setCursor(10, y);
  gfx->print("Status: ");
  gfx->setTextColor(wifiConnected ? COLOR_STATUS_OK : COLOR_TEXT_SEC);
  gfx->print(wifiConnected ? wifiSSID : "Not connected");
  y += 14;
  
  // ========== SSID AND PASSWORD FIELDS ==========
  // SSID field
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(10, y);
  gfx->print("SSID:");
  gfx->drawRect(60, y - 2, 200, 16, COLOR_TEXT_SEC);
  gfx->fillRect(61, y - 1, 198, 14, COLOR_BG_TILE);
  gfx->setCursor(65, y);
  gfx->setTextColor(COLOR_TEXT_VALUE);
  gfx->print(wifiSSID);
  
  // Edit SSID button
  gfx->fillRoundRect(265, y - 3, 50, 18, 3, COLOR_ACCENT);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(275, y);
  gfx->print("Edit");
  y += 22;
  
  // Password field
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(10, y);
  gfx->print("Pass:");
  gfx->drawRect(60, y - 2, 200, 16, COLOR_TEXT_SEC);
  gfx->fillRect(61, y - 1, 198, 14, COLOR_BG_TILE);
  gfx->setCursor(65, y);
  gfx->setTextColor(COLOR_TEXT_VALUE);
  // Show password (visible, not asterisks)
  gfx->print(wifiPassword);
  
  // Edit Password button
  gfx->fillRoundRect(265, y - 3, 50, 18, 3, COLOR_ACCENT);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(275, y);
  gfx->print("Edit");
  y += 25;
  
  // ========== NETWORK LIST ==========
  gfx->setTextColor(COLOR_ACCENT2);
  gfx->setCursor(10, y);
  gfx->print("Available Networks:");
  
  // Scan button
  gfx->fillRoundRect(200, y - 3, 55, 16, 3, COLOR_ACCENT);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(210, y);
  gfx->print("Scan");
  y += 16;
  
  // Network list (max 5)
  if (wifiScanning) {
    gfx->setTextColor(COLOR_TEXT_SEC);
    gfx->setCursor(10, y + 20);
    gfx->print("Scanning...");
  } else if (wifiNetworkCount == 0) {
    gfx->setTextColor(COLOR_TEXT_SEC);
    gfx->setCursor(10, y + 20);
    gfx->print("No networks. Press Scan.");
  } else {
    for (int i = 0; i < wifiNetworkCount && i < WIFI_MAX_NETWORKS; i++) {
      int rowY = y + 2 + i * 20;
      
      // Highlight selected network
      if (i == wifiSelectedNetwork) {
        gfx->fillRect(5, rowY - 2, 250, 18, COLOR_ACCENT);
      }
      
      // SSID
      gfx->setTextColor(i == wifiSelectedNetwork ? COLOR_TEXT : COLOR_TEXT);
      gfx->setCursor(10, rowY);
      
      // Truncate SSID if too long
      char ssidDisplay[20];
      strncpy(ssidDisplay, wifiNetworks[i].ssid, 18);
      ssidDisplay[18] = '\0';
      gfx->print(ssidDisplay);
      
      // Lock icon if encrypted
      if (wifiNetworks[i].encrypted) {
        gfx->setCursor(180, rowY);
        gfx->setTextColor(COLOR_TEXT_SEC);
        gfx->print("*");
      }
      
      // RSSI
      int8_t rssi = wifiNetworks[i].rssi;
      gfx->setCursor(200, rowY);
      gfx->setTextColor(rssi > -60 ? COLOR_STATUS_OK : (rssi > -80 ? COLOR_STATUS_WARN : COLOR_STATUS_DANGER));
      gfx->print(rssi);
      gfx->print("dB");
    }
  }
  
  // ========== BUTTONS AT BOTTOM ==========
  int btnY = 215;
  int btnH = 22;
  
  // Connect button
  gfx->fillRoundRect(10, btnY, 95, btnH, 4, COLOR_STATUS_OK);
  gfx->setTextColor(COLOR_BLACK);
  gfx->setTextSize(1);
  gfx->setCursor(28, btnY + 7);
  gfx->print("Connect");
  
  // Disconnect button
  gfx->fillRoundRect(115, btnY, 95, btnH, 4, wifiConnected ? COLOR_STATUS_WARN : COLOR_TEXT_SEC);
  gfx->setTextColor(COLOR_BLACK);
  gfx->setCursor(123, btnY + 7);
  gfx->print("Disconnect");
  
  lcdDrawing = false;
  screenNeedsRedraw = false;
}

// ============================================
// KEYBOARD SCREEN
// ============================================

void drawKeyboardScreen() {
  if (!lcd_ok) return;
  
  lcdDrawing = true;
  prepareLcdSpi();
  
  gfx->fillScreen(COLOR_BG_MAIN);
  
  // ========== HEADER ==========
  gfx->fillRect(0, 0, 320, 28, COLOR_BG_HEADER);
  
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  gfx->setCursor(10, 10);
  if (kbCtx.target == KB_TARGET_SSID) {
    gfx->print("Enter SSID:");
  } else if (kbCtx.target == KB_TARGET_PASSWORD) {
    gfx->print("Enter Password:");
  } else if (kbCtx.target == KB_TARGET_DEVICE_NAME) {
    gfx->print("Enter Device Name:");
  }
  
  // ========== TEXT FIELD ==========
  int fieldY = 32;
  gfx->drawRect(8, fieldY, 304, 24, COLOR_TEXT_SEC);
  gfx->fillRect(9, fieldY + 1, 302, 22, COLOR_BG_TILE);
  
  gfx->setTextColor(COLOR_TEXT_VALUE);
  gfx->setTextSize(2);
  gfx->setCursor(12, fieldY + 4);
  
  // Display text with cursor
  char displayBuf[WIFI_PASS_MAX_LEN + 2];
  strncpy(displayBuf, kbCtx.buffer, kbCtx.cursorPos);
  displayBuf[kbCtx.cursorPos] = '_';  // Cursor
  strcpy(displayBuf + kbCtx.cursorPos + 1, kbCtx.buffer + kbCtx.cursorPos);
  
  // Limit display to 24 characters
  if (strlen(displayBuf) > 24) {
    displayBuf[24] = '\0';
  }
  gfx->print(displayBuf);
  
  // ========== KEYBOARD ==========
  const char** rows = kbCtx.shifted ? kbRowsUpper : kbRowsLower;
  
  int keyW = 28;
  int keyH = 28;
  int startY = 60;
  int rowOffsets[] = {10, 20, 35, 50};  // Offsets for each row
  
  for (int row = 0; row < 4; row++) {
    int y = startY + row * (keyH + 4);
    int len = strlen(rows[row]);
    int startX = rowOffsets[row];
    
    for (int col = 0; col < len; col++) {
      int x = startX + col * (keyW + 2);
      
      // Draw key
      gfx->fillRoundRect(x, y, keyW, keyH, 3, COLOR_BG_TILE);
      gfx->drawRoundRect(x, y, keyW, keyH, 3, COLOR_TEXT_SEC);
      
      // Letter
      gfx->setTextColor(COLOR_TEXT);
      gfx->setTextSize(2);
      gfx->setCursor(x + 8, y + 6);
      gfx->print(rows[row][col]);
    }
  }
  
  // ========== SPECIAL KEYS ==========
  int specialY = startY + 4 * (keyH + 4);
  
  // Shift
  gfx->fillRoundRect(10, specialY, 45, keyH, 3, kbCtx.shifted ? COLOR_ACCENT : COLOR_BG_TILE);
  gfx->drawRoundRect(10, specialY, 45, keyH, 3, COLOR_TEXT_SEC);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  gfx->setCursor(18, specialY + 10);
  gfx->print("Shift");
  
  // Space
  gfx->fillRoundRect(60, specialY, 120, keyH, 3, COLOR_BG_TILE);
  gfx->drawRoundRect(60, specialY, 120, keyH, 3, COLOR_TEXT_SEC);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(100, specialY + 10);
  gfx->print("Space");
  
  // Backspace
  gfx->fillRoundRect(185, specialY, 45, keyH, 3, COLOR_BG_TILE);
  gfx->drawRoundRect(185, specialY, 45, keyH, 3, COLOR_TEXT_SEC);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setCursor(193, specialY + 10);
  gfx->print("<--");
  
  // ========== OK / CANCEL (in same row as special keys) ==========
  // Cancel (right corner, next to Backspace)
  gfx->fillRoundRect(235, specialY, 36, keyH, 3, COLOR_STATUS_DANGER);
  gfx->setTextColor(COLOR_TEXT);
  gfx->setTextSize(1);
  gfx->setCursor(243, specialY + 10);
  gfx->print("ESC");
  
  // OK (far right)
  gfx->fillRoundRect(276, specialY, 36, keyH, 3, COLOR_STATUS_OK);
  gfx->setTextColor(COLOR_BLACK);
  gfx->setCursor(288, specialY + 10);
  gfx->print("OK");
  
  // ========== TOUCH FEEDBACK DOT ==========
  // Draw last click dot (if < 2s ago)
  if (lastKbTouchX >= 0 && millis() - lastKbTouchTime < 2000) {
    gfx->fillCircle(lastKbTouchX, lastKbTouchY, 4, COLOR_RED);
  }
  
  lcdDrawing = false;
  screenNeedsRedraw = false;
}

// ============================================
// TOUCH DOT (DEBUG)
// ============================================

void drawTouchDot(int16_t x, int16_t y) {
  if (!lcd_ok) return;
  
  gfx->fillCircle(x, y, 5, COLOR_ACCENT2);
  
  // Coordinates at bottom
  gfx->fillRect(200, 220, 120, 20, COLOR_BG_MAIN);
  gfx->setTextColor(COLOR_TEXT_VALUE);
  gfx->setTextSize(1);
  gfx->setCursor(200, 225);
  gfx->print("Touch: ");
  gfx->print(x);
  gfx->print(",");
  gfx->print(y);
}

#endif // ENABLE_LCD
