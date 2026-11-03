/*
 * Air Condition Receiver v2.7
 * ===========================
 * 
 * Odbiornik LoRa z ekranem dotykowym LCD 2.8" (ST7789V + XPT2046)
 * Multi-Device support + Touch navigation + PM History Charts
 * WiFi Config UI + QWERTY Keyboard + Non-blocking WiFi/NTP
 * Animations: Startup loader, Alarm pulse, Touch feedback, WiFi scan
 * 
 * Hardware:
 *   - XIAO ESP32-S3 + Wio-SX1262 (B2B)
 *   - Waveshare Pico-ResTouch-LCD-2.8" (320×240, ST7789V, XPT2046)
 *   - Zasilanie: 5V przez USB-C
 * 
 * Mapowanie pinów LCD → XIAO:
 *   LCD_CS  (GP9)  → D0 (GPIO1)
 *   LCD_DC  (GP8)  → D1 (GPIO2)
 *   LCD_RST (GP15) → D2 (GPIO3)
 *   LCD_BL  (GP13) → D3 (GPIO4)
 *   LCD_CLK (GP10) → D4 (GPIO5)
 *   MOSI    (GP11) → D5 (GPIO6)
 *   TP_CS   (GP16) → D6 (GPIO43)
 *   MISO    (GP12) → D7 (GPIO44)
 *   VCC     (VSYS) → 5V (pin 14)
 *   GND            → GND (pin 13)
 * 
 * Biblioteki:
 *   - Arduino_GFX (zamiast TFT_eSPI - lepsza kompatybilność z ESP32-S3)
 *   - XPT2046_Touchscreen
 *   - RadioLib
 * 
 * Struktura plików (9 modułów):
 *   - air_condition_receiver.ino - główny plik (setup + loop)
 *   - config.h                   - konfiguracja pinów i stałych
 *   - types.h                    - struktury danych i deklaracje
 *   - icons.h                    - bitmapy ikon 16x16
 *   - globals.cpp                - definicje zmiennych globalnych
 *   - hardware.cpp               - funkcje inicjalizacji sprzętu + WiFi/NTP
 *   - deviceManager.cpp          - zarządzanie urządzeniami i LoRa
 *   - touchHandler.cpp           - obsługa dotyku i nawigacji
 *   - uiScreens.cpp              - funkcje rysowania UI
 * 
 * Data: 2026-01-02
 */

#include "types.h"

// ============================================
// SETUP
// ============================================
void setup() {
  // NAJPIERW: Inicjalizacja wszystkich struktur danych do znanego stanu
  // To zapobiega artefaktom LCD spowodowanym przez niezainicjalizowane wartości
  // (np. historyCount = śmieciowa wartość mogłaby spowodować rysowanie 255 słupków)
  memset(devices, 0, sizeof(devices));
  deviceCount = 0;
  
  // Debug Serial - tylko gdy włączone w config.h
  DEBUG_INIT();
  DEBUG_PRINTLN("=== AIR CONDITION RECEIVER v2.7 ===");
  
  #if ENABLE_LCD
  setupLCD();
  drawStartupProgress(0, "Initializing LCD...");
  #endif
  
  #if ENABLE_TOUCH
  drawStartupProgress(1, "Initializing Touch...");
  setupTouch();
  lastTouchEndTime = millis();
  #endif
  
  #if ENABLE_LORA
  drawStartupProgress(2, "Initializing LoRa...");
  setupLoRa();
  #endif
  
  drawStartupProgress(3, "Connecting WiFi...");
  setupWiFi();
  setupNTP();
  
  delay(500);
  
  // Reset alarm timer
  lastAlarmPulseMs = millis();
  alarmPulseState = false;
  
  // UWAGA: memset(devices) przeniesiony na początek setup() - patrz wyżej
  // Tutaj tylko upewniamy się że deviceCount jest 0 (redundantne, ale bezpieczne)
  deviceCount = 0;
  
  #if ENABLE_LCD
  drawHomeScreen();
  #endif
  
  DEBUG_PRINTLN("[SYS] Ready. Waiting for LoRa packets...");
}

// ============================================
// LOOP
// ============================================
void loop() {
  static uint32_t lastTimeoutCheck = 0;
  static uint32_t lastTouchTime = 0;
  static bool touchWasPressed = false;
  static uint32_t lastDetailRefresh = 0;
  static uint32_t lastTimeUpdate = 0;
  static uint32_t lastLoopDebug = 0;
  
  // Debug: co 5s wypisz status loop
  if (millis() - lastLoopDebug > 5000) {
    lastLoopDebug = millis();
    DEBUG_PRINT("[LOOP] t=");
    DEBUG_PRINT(millis()/1000);
    DEBUG_PRINT("s irq=");
    DEBUG_PRINT(loraIrqCount);
    DEBUG_PRINT(" pkt=");
    DEBUG_PRINT(packetsReceived);
    DEBUG_PRINT(" lcd=");
    DEBUG_PRINT(lcdDrawing ? "1" : "0");
    DEBUG_PRINT(" touch=");
    DEBUG_PRINTLN(touchActive ? "1" : "0");
  }
  
  // Touch z nawigacją - TYLKO gdy LCD nie rysuje
  #if ENABLE_TOUCH && ENABLE_LCD
  if (!lcdDrawing) {
    int16_t touchX, touchY;
    if (checkTouch(&touchX, &touchY)) {
      // Wykryj tylko moment naciśnięcia (nie trzymanie)
      if (!touchWasPressed && millis() - lastTouchTime > 400) {
        touchWasPressed = true;
        lastTouchTime = millis();
        
        // Debug
        DEBUG_TOUCH_PRINT("[TOUCH] ");
        DEBUG_TOUCH_PRINT(touchX);
        DEBUG_TOUCH_PRINT(",");
        DEBUG_TOUCH_PRINTLN(touchY);
        
        // Obsługa nawigacji
        handleTouch(touchX, touchY);
      }
    } else {
      // Palec oderwany - reset flagi
      touchWasPressed = false;
    }
  }
  #endif
  
  // LoRa - odbiór pakietów TYLKO gdy LCD/Touch nie używają SPI
  #if ENABLE_LORA
  if (!lcdDrawing && !touchActive) {
    receivePacket();
  }
  #endif
  
  // Sprawdzanie timeout urządzeń co 10s
  if (millis() - lastTimeoutCheck > 10000) {
    lastTimeoutCheck = millis();
    checkDeviceTimeouts();
  }
  
  // Sprawdzanie połączenia WiFi (non-blocking)
  checkWiFiConnection();
  
  // UDP broadcast JSON co 10s (jeśli WiFi połączone)
  sendUdpBroadcast();
  
  // ========== OBSŁUGA WYŚWIETLACZA - SEKWENCYJNIE ==========
  #if ENABLE_LCD
  
  // WAŻNE: Czekaj na wolne SPI (LCD, Touch)
  // LoRa używa FSPI (osobna magistrala) - nie koliduje z LCD/Touch na HSPI
  bool spiReady = !lcdDrawing && !touchActive;
  
  // Konwertuj loraDataUpdated na screenNeedsRedraw TYLKO gdy SPI gotowe
  if (loraDataUpdated && spiReady) {
    loraDataUpdated = false;
    screenNeedsRedraw = true;
  }
  
  // Auto-refresh detail screen co 3s (dla rotacji wykresu)
  // WAŻNE: Nie wykonuj jeśli screenNeedsRedraw czeka - to ma priorytet
  if (currentScreen == SCREEN_DETAIL && 
      !screenNeedsRedraw && 
      spiReady &&
      millis() - lastDetailRefresh > 3000) {
    lastDetailRefresh = millis();
    if (selectedDeviceIndex >= 0 && selectedDeviceIndex < MAX_DEVICES) {
      updateDetailChartOnly(selectedDeviceIndex);
    }
  }
  
  // Odświeżanie czasu w headerze co 1s (tylko na Home Screen)
  if (currentScreen == SCREEN_HOME && 
      !screenNeedsRedraw && 
      spiReady &&
      millis() - lastTimeUpdate > 1000) {
    lastTimeUpdate = millis();
    updateHeaderTime();
    updateHeaderWifiIcon();  // Aktualizuj też ikonę WiFi
  }
  
  // Pełne odświeżanie ekranu - TYLKO gdy SPI gotowe
    if (screenNeedsRedraw && spiReady) {
      DEBUG_LCD_PRINTLN("[LCD] Redraw triggered");
      switch (currentScreen) {
        case SCREEN_HOME:
          drawHomeScreen();
          break;
        case SCREEN_DETAIL:
          if (selectedDeviceIndex >= 0 && selectedDeviceIndex < MAX_DEVICES) {
            drawDetailScreen(selectedDeviceIndex);
          } else {
            currentScreen = SCREEN_HOME;
            drawHomeScreen();
          }
          break;
        case SCREEN_SETTINGS:
          drawSettingsScreen();
          break;
        case SCREEN_WIFI_CONFIG:
          drawWifiConfigScreen();
          break;
        case SCREEN_KEYBOARD:
          drawKeyboardScreen();
          break;
        default:
          drawHomeScreen();
          break;
      }
      // Reset refresh timer po pełnym redraw
      lastDetailRefresh = millis();
    }
  
  // Alarm pulse animation (Home Screen only)
  if (currentScreen == SCREEN_HOME && spiReady) {
    updateAlarmPulse();
  }
  
  // WiFi scan animation (WiFi Config Screen only)
  if (currentScreen == SCREEN_WIFI_CONFIG && wifiScanning && spiReady) {
    updateScanAnimation();
  }
  
  #endif
  
  delay(10);
}
