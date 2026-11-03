/*
 * touchHandler.cpp - Obsługa dotyku i nawigacja
 * Air Condition Receiver v2.7
 */

#include "types.h"

#if ENABLE_TOUCH

// ============================================
// TOUCH HELPERS
// ============================================

// Sprawdza czy punkt (x,y) jest w prostokącie
bool pointInRect(int16_t x, int16_t y, int rx, int ry, int rw, int rh) {
  return (x >= rx && x < rx + rw && y >= ry && y < ry + rh);
}

// ============================================
// TOUCH ACTIONS
// ============================================

// Rozpoznaje akcję dotyku na Home Screen
uint8_t getTouchActionHome(int16_t x, int16_t y) {
  // Przycisk ustawień [*] w header
  if (pointInRect(x, y, 250, 0, 70, HEADER_HEIGHT)) {
    return ACTION_SETTINGS;
  }
  
  // Sprawdź kafelki
  for (uint8_t i = 0; i < MAX_DEVICES; i++) {
    int tileX, tileY;
    getTilePosition(i, &tileX, &tileY);
    
    if (pointInRect(x, y, tileX, tileY, TILE_WIDTH, TILE_HEIGHT)) {
      return ACTION_TILE_0 + i;
    }
  }
  
  return ACTION_NONE;
}

// Rozpoznaje akcję dotyku na Detail Screen
uint8_t getTouchActionDetail(int16_t x, int16_t y) {
  // Oblicz pozycję ikony Edit na podstawie długości nazwy
  int nameWidth = 0;
  if (selectedDeviceIndex >= 0 && selectedDeviceIndex < MAX_DEVICES) {
    nameWidth = strlen(devices[selectedDeviceIndex].name) * 12;
  }
  int editIconX = 40 + nameWidth;
  
  // Back: ikona < plus nazwa urządzenia (x: 0 do editIconX)
  if (pointInRect(x, y, 0, 0, editIconX, 30)) {
    return ACTION_BACK;
  }
  
  // Edit Name: ikona pencil i obszar na prawo (50px szerokości)
  if (pointInRect(x, y, editIconX, 0, 50, 30)) {
    return ACTION_EDIT_NAME;
  }
  
  // Przycisk Settings (ikona w prawym rogu)
  if (pointInRect(x, y, 280, 0, 40, 30)) {
    return ACTION_SETTINGS;
  }
  
  return ACTION_NONE;
}

// Rozpoznaje akcję dotyku na Settings Screen
uint8_t getTouchActionSettings(int16_t x, int16_t y) {
  // Przycisk Back (ikona < plus tytuł "SETTINGS")
  // "SETTINGS" = 8 znaków × 12px = 96px, zaczyna się od x=35
  if (pointInRect(x, y, 0, 0, 135, 30)) {
    return ACTION_BACK;
  }
  
  // Przycisk WiFi Config (dolna część ekranu)
  if (pointInRect(x, y, 10, 195, 300, 35)) {
    return ACTION_WIFI_CONFIG;
  }
  
  return ACTION_NONE;
}

// Rozpoznaje akcję dotyku na WiFi Config Screen
uint8_t getTouchActionWifiConfig(int16_t x, int16_t y) {
  // Przycisk Back (ikona < plus tytuł "WiFi Config")
  // "WiFi Config" = 11 znaków × 12px = 132px, zaczyna się od x=35
  if (pointInRect(x, y, 0, 0, 170, 30)) {
    return ACTION_BACK;
  }
  
  // Edit SSID (y=49)
  if (pointInRect(x, y, 265, 46, 50, 18)) {
    return ACTION_EDIT;  // Użyjemy ACTION_EDIT z kontekstem SSID
  }
  
  // Edit Password (y=71)
  if (pointInRect(x, y, 265, 68, 50, 18)) {
    return ACTION_KEY_PRESS;  // Użyjemy do rozróżnienia od SSID
  }
  
  // Scan button
  if (pointInRect(x, y, 200, 92, 55, 16)) {
    return ACTION_WIFI_SCAN;
  }
  
  // Lista sieci (5 pozycji, każda 20px wysokości, startuje od y=114)
  for (int i = 0; i < wifiNetworkCount && i < WIFI_MAX_NETWORKS; i++) {
    int rowY = 114 + i * 20;
    if (pointInRect(x, y, 5, rowY - 2, 250, 18)) {
      wifiSelectedNetwork = i;
      return ACTION_SELECT_NETWORK;  // Zaznaczenie sieci z listy
    }
  }
  
  // Przycisk Connect (y=215)
  if (pointInRect(x, y, 10, 215, 95, 22)) {
    return ACTION_WIFI_CONNECT;
  }
  
  // Przycisk Disconnect (y=215)
  if (pointInRect(x, y, 115, 215, 95, 22)) {
    return ACTION_WIFI_DISCONNECT;
  }
  
  return ACTION_NONE;
}

// Rozpoznaje akcję dotyku na Keyboard Screen
uint8_t getTouchActionKeyboard(int16_t x, int16_t y) {
  // ========== KLAWISZE QWERTY ==========
  const char** rows = kbCtx.shifted ? kbRowsUpper : kbRowsLower;
  
  int keyW = 28;
  int keyH = 28;
  int startY = 60;
  int rowOffsets[] = {10, 20, 35, 50};
  
  for (int row = 0; row < 4; row++) {
    int ky = startY + row * (keyH + 4);
    int len = strlen(rows[row]);
    int startX = rowOffsets[row];
    
    for (int col = 0; col < len; col++) {
      int kx = startX + col * (keyW + 2);
      
      if (pointInRect(x, y, kx, ky, keyW, keyH)) {
        kbPressedKey = rows[row][col];
        return ACTION_KEY_PRESS;
      }
    }
  }
  
  // ========== SPECJALNE KLAWISZE ==========
  int specialY = startY + 4 * (keyH + 4);
  
  // Shift
  if (pointInRect(x, y, 10, specialY, 45, keyH)) {
    return ACTION_KEY_SHIFT;
  }
  
  // Spacja
  if (pointInRect(x, y, 60, specialY, 120, keyH)) {
    kbPressedKey = ' ';
    return ACTION_KEY_PRESS;
  }
  
  // Backspace
  if (pointInRect(x, y, 185, specialY, 45, keyH)) {
    return ACTION_KEY_BACKSPACE;
  }
  
  // ========== OK / CANCEL (w rzędzie specjalnych klawiszy) ==========
  // specialY = 60 + 4 * 32 = 188
  
  // ESC (Cancel)
  if (pointInRect(x, y, 235, specialY, 36, keyH)) {
    return ACTION_KEY_CANCEL;
  }
  
  // OK
  if (pointInRect(x, y, 276, specialY, 36, keyH)) {
    return ACTION_KEY_OK;
  }
  
  return ACTION_NONE;
}

// ============================================
// MAIN TOUCH HANDLER
// ============================================

// Główna funkcja obsługi dotyku
void handleTouch(int16_t x, int16_t y) {
  uint8_t action = ACTION_NONE;
  
  // Zapisz pozycję kliknięcia dla wizualnego feedbacku na klawiaturze
  if (currentScreen == SCREEN_KEYBOARD) {
    lastKbTouchX = x;
    lastKbTouchY = y;
    lastKbTouchTime = millis();
  }
  
  // Rozpoznaj akcję w zależności od ekranu
  switch (currentScreen) {
    case SCREEN_HOME:
      action = getTouchActionHome(x, y);
      break;
    case SCREEN_DETAIL:
      action = getTouchActionDetail(x, y);
      break;
    case SCREEN_SETTINGS:
      action = getTouchActionSettings(x, y);
      break;
    case SCREEN_WIFI_CONFIG:
      action = getTouchActionWifiConfig(x, y);
      break;
    case SCREEN_KEYBOARD:
      action = getTouchActionKeyboard(x, y);
      break;
    default:
      return;
  }
  
  // Wykonaj akcję
  switch (action) {
    case ACTION_TILE_0:
    case ACTION_TILE_1:
    case ACTION_TILE_2:
    case ACTION_TILE_3:
      {
        int8_t tileIndex = action - ACTION_TILE_0;
        if (tileIndex < deviceCount && devices[tileIndex].active) {
          // Touch feedback
          drawTileFeedback(tileIndex);
          delay(TOUCH_FEEDBACK_MS);
          
          selectedDeviceIndex = tileIndex;
          currentScreen = SCREEN_DETAIL;
          DEBUG_TOUCH_PRINT("[TOUCH] Open device ");
          DEBUG_TOUCH_PRINTLN(devices[tileIndex].name);
          screenNeedsRedraw = true;
        }
      }
      break;
      
    case ACTION_BACK:
      // Touch feedback for back button
      if (currentScreen == SCREEN_DETAIL) {
        drawButtonFeedback(0, 0, 50, 30);
      } else {
        drawButtonFeedback(0, 0, 135, 30);
      }
      delay(TOUCH_FEEDBACK_MS);
      
      if (currentScreen == SCREEN_SETTINGS) {
        repairLcdState();  // Naprawa SPI przed powrotem na Home
        currentScreen = SCREEN_HOME;
        // Reset alarm pulse timer on return to Home
        lastAlarmPulseMs = millis();
        alarmPulseState = false;
        DEBUG_TOUCH_PRINTLN("[TOUCH] Settings -> Home");
      } else if (currentScreen == SCREEN_WIFI_CONFIG) {
        currentScreen = SCREEN_SETTINGS;
        DEBUG_TOUCH_PRINTLN("[TOUCH] WiFi Config -> Settings");
        // Bez repair - nie wracamy na Home
      } else {
        // Powrót z SCREEN_DETAIL na SCREEN_HOME
        repairLcdState();  // Naprawa SPI przed powrotem na Home
        currentScreen = SCREEN_HOME;
        selectedDeviceIndex = -1;
        // Reset alarm pulse timer on return to Home
        lastAlarmPulseMs = millis();
        alarmPulseState = false;
        DEBUG_TOUCH_PRINTLN("[TOUCH] Back to home");
      }
      screenNeedsRedraw = true;
      break;
      
    case ACTION_EDIT:
      // Edit SSID - touch feedback
      if (currentScreen == SCREEN_WIFI_CONFIG) {
        drawButtonFeedback(265, 46, 50, 18);
        delay(TOUCH_FEEDBACK_MS);
        openKeyboard(KB_TARGET_SSID, wifiSSID, WIFI_SSID_MAX_LEN - 1);
        DEBUG_TOUCH_PRINTLN("[TOUCH] Edit SSID");
      }
      break;
      
    case ACTION_EDIT_NAME:
      // Edit device name (from Detail Screen) - touch feedback
      if (currentScreen == SCREEN_DETAIL && selectedDeviceIndex >= 0 && selectedDeviceIndex < MAX_DEVICES) {
        // Calculate edit icon position based on name length
        int nameWidth = strlen(devices[selectedDeviceIndex].name) * 12;
        drawButtonFeedback(40 + nameWidth, 7, 20, 16);
        delay(TOUCH_FEEDBACK_MS);
        openKeyboardForDeviceName(selectedDeviceIndex);
        DEBUG_TOUCH_PRINTLN("[TOUCH] Edit device name");
      }
      break;
      
    case ACTION_SETTINGS:
      // Touch feedback for settings
      drawButtonFeedback(280, 0, 40, 30);
      delay(TOUCH_FEEDBACK_MS);
      currentScreen = SCREEN_SETTINGS;
      DEBUG_TOUCH_PRINTLN("[TOUCH] Open Settings");
      screenNeedsRedraw = true;
      break;
      
    case ACTION_WIFI_CONFIG:
      // Touch feedback for WiFi Config button
      drawButtonFeedback(10, 195, 300, 35);
      delay(TOUCH_FEEDBACK_MS);
      currentScreen = SCREEN_WIFI_CONFIG;
      DEBUG_TOUCH_PRINTLN("[TOUCH] Open WiFi Config");
      screenNeedsRedraw = true;
      break;
      
    case ACTION_WIFI_SCAN:
      // Touch feedback for Scan button
      drawButtonFeedback(200, 92, 55, 16);
      delay(TOUCH_FEEDBACK_MS);
      DEBUG_TOUCH_PRINTLN("[TOUCH] Scan WiFi");
      // Reset scan animation state
      scanAnimState = 0;
      lastScanAnimMs = millis();
      scanWiFiNetworks();
      screenNeedsRedraw = true;
      break;
      
    case ACTION_WIFI_CONNECT:
      if (strlen(wifiSSID) > 0) {
        // Touch feedback for Connect button
        drawButtonFeedback(10, 215, 95, 22);
        delay(TOUCH_FEEDBACK_MS);
        DEBUG_TOUCH_PRINTLN("[TOUCH] Connect WiFi");
        connectToWiFi(wifiSSID, wifiPassword);
        screenNeedsRedraw = true;
      }
      break;
      
    case ACTION_WIFI_DISCONNECT:
      // Touch feedback for Disconnect button
      drawButtonFeedback(115, 215, 95, 22);
      delay(TOUCH_FEEDBACK_MS);
      DEBUG_TOUCH_PRINTLN("[TOUCH] Disconnect WiFi");
      disconnectWiFi();
      screenNeedsRedraw = true;
      break;
      
    case ACTION_KEY_PRESS:
      // Kontekst: z WiFi Config = edit password, z Keyboard = znak
      if (currentScreen == SCREEN_WIFI_CONFIG) {
        // Edit Password - touch feedback
        drawButtonFeedback(265, 68, 50, 18);
        delay(TOUCH_FEEDBACK_MS);
        openKeyboard(KB_TARGET_PASSWORD, wifiPassword, WIFI_PASS_MAX_LEN - 1);
        DEBUG_TOUCH_PRINTLN("[TOUCH] Edit Password");
      } else if (currentScreen == SCREEN_KEYBOARD) {
        // Dodaj znak do bufora
        if (kbPressedKey != 0 && kbCtx.cursorPos < kbCtx.maxLen) {
          // Wstaw znak na pozycji kursora
          memmove(kbCtx.buffer + kbCtx.cursorPos + 1, 
                  kbCtx.buffer + kbCtx.cursorPos, 
                  strlen(kbCtx.buffer + kbCtx.cursorPos) + 1);
          kbCtx.buffer[kbCtx.cursorPos] = kbPressedKey;
          kbCtx.cursorPos++;
          kbPressedKey = 0;
          screenNeedsRedraw = true;
        }
      }
      break;
      
    case ACTION_KEY_BACKSPACE:
      if (kbCtx.cursorPos > 0) {
        memmove(kbCtx.buffer + kbCtx.cursorPos - 1,
                kbCtx.buffer + kbCtx.cursorPos,
                strlen(kbCtx.buffer + kbCtx.cursorPos) + 1);
        kbCtx.cursorPos--;
        screenNeedsRedraw = true;
      }
      break;
      
    case ACTION_KEY_SHIFT:
      kbCtx.shifted = !kbCtx.shifted;
      screenNeedsRedraw = true;
      break;
      
    case ACTION_KEY_OK:
      // Zapisz wynik i wróć
      if (kbCtx.target == KB_TARGET_SSID) {
        strncpy(wifiSSID, kbCtx.buffer, WIFI_SSID_MAX_LEN - 1);
        wifiSSID[WIFI_SSID_MAX_LEN - 1] = '\0';
        DEBUG_TOUCH_PRINT("[KB] SSID set to: ");
        DEBUG_TOUCH_PRINTLN(wifiSSID);
      } else if (kbCtx.target == KB_TARGET_PASSWORD) {
        strncpy(wifiPassword, kbCtx.buffer, WIFI_PASS_MAX_LEN - 1);
        wifiPassword[WIFI_PASS_MAX_LEN - 1] = '\0';
        DEBUG_TOUCH_PRINTLN("[KB] Password set");
      } else if (kbCtx.target == KB_TARGET_DEVICE_NAME) {
        // Zapisz nazwę urządzenia
        if (selectedDeviceIndex >= 0 && selectedDeviceIndex < MAX_DEVICES) {
          strncpy(devices[selectedDeviceIndex].name, kbCtx.buffer, DEVICE_NAME_LEN - 1);
          devices[selectedDeviceIndex].name[DEVICE_NAME_LEN - 1] = '\0';
          DEBUG_TOUCH_PRINT("[KB] Device name set to: ");
          DEBUG_TOUCH_PRINTLN(devices[selectedDeviceIndex].name);
          
          // Zapisz do NVS
          saveDeviceName(selectedDeviceIndex);
        }
      }
      currentScreen = kbCtx.returnScreen;
      screenNeedsRedraw = true;
      break;
      
    case ACTION_KEY_CANCEL:
      // Anuluj i wróć bez zapisywania
      DEBUG_TOUCH_PRINTLN("[KB] Cancelled");
      currentScreen = kbCtx.returnScreen;
      screenNeedsRedraw = true;
      break;
      
    case ACTION_SELECT_NETWORK:
      // Wybrano sieć z listy - skopiuj SSID
      if (wifiSelectedNetwork >= 0 && wifiSelectedNetwork < wifiNetworkCount) {
        // Touch feedback for network selection
        int rowY = 114 + wifiSelectedNetwork * 20;
        drawButtonFeedback(5, rowY - 2, 250, 18);
        delay(TOUCH_FEEDBACK_MS);
        
        strncpy(wifiSSID, wifiNetworks[wifiSelectedNetwork].ssid, WIFI_SSID_MAX_LEN - 1);
        wifiSSID[WIFI_SSID_MAX_LEN - 1] = '\0';
        DEBUG_WIFI_PRINT("[WIFI] Selected: ");
        DEBUG_WIFI_PRINTLN(wifiSSID);
        screenNeedsRedraw = true;
      }
      break;
      
    default:
      break;
  }
}

#endif // ENABLE_TOUCH
