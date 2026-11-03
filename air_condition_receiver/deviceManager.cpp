/*
 * deviceManager.cpp - Zarządzanie urządzeniami i odbiór LoRa
 * Air Condition Receiver v2.6
 */

#include "types.h"

// ============================================
// DEVICE MANAGEMENT
// ============================================

// Generuje domyślną nazwę z ostatnich 2 bajtów MAC
void generateDefaultName(uint8_t* mac, char* name) {
  sprintf(name, "Device %02X%02X", mac[4], mac[5]);
}

// Inicjalizuje nowe urządzenie
void initDevice(int8_t index, uint8_t* mac) {
  DeviceData* dev = &devices[index];
  
  memcpy(dev->mac, mac, 6);
  generateDefaultName(mac, dev->name);
  
  // Próbuj wczytać zapisaną nazwę z NVS
  loadDeviceName(index);
  
  dev->active = true;
  
  // Reset danych sensorów
  dev->temperature = 0;
  dev->humidity = 0;
  dev->pressure = 0;
  dev->pm1_0 = 0;
  dev->pm2_5 = 0;
  dev->pm10 = 0;
  dev->status = 0;
  
  // Reset danych LoRa
  dev->rssi = 0;
  dev->snr = 0;
  dev->lastSeenMs = millis();
  dev->lastSeqNum = 0;
  dev->packetsReceived = 0;  // NOWE
  
  // Reset historii - wszystkie typy PM
  memset(dev->pm1History, 0, sizeof(dev->pm1History));
  memset(dev->pm25History, 0, sizeof(dev->pm25History));
  memset(dev->pm10History, 0, sizeof(dev->pm10History));
  dev->historyIndex = 0;
  dev->historyCount = 0;
  dev->lastHistoryUpdateMs = millis();
  
  // Reset akumulacji - wszystkie typy PM
  dev->pm1Sum = 0;
  dev->pm25Sum = 0;
  dev->pm10Sum = 0;
  dev->pmSampleCount = 0;
  
  DEBUG_PRINT("[DEV] New device: ");
  DEBUG_PRINTLN(dev->name);
}

// Szuka urządzenia po MAC lub dodaje nowe
// Zwraca indeks urządzenia lub -1 jeśli brak miejsca
int8_t findOrAddDevice(uint8_t* mac) {
  // Szukaj istniejącego
  for (uint8_t i = 0; i < deviceCount; i++) {
    if (memcmp(devices[i].mac, mac, 6) == 0) {
      return i;
    }
  }
  
  // Dodaj nowe jeśli jest miejsce
  if (deviceCount < MAX_DEVICES) {
    int8_t newIndex = deviceCount;
    deviceCount++;
    initDevice(newIndex, mac);
    return newIndex;
  }
  
  // Brak miejsca - szukaj nieaktywnego do zastąpienia
  uint32_t oldestTime = millis();
  int8_t oldestIndex = -1;
  
  for (uint8_t i = 0; i < MAX_DEVICES; i++) {
    if (!devices[i].active || devices[i].lastSeenMs < oldestTime) {
      oldestTime = devices[i].lastSeenMs;
      oldestIndex = i;
    }
  }
  
  if (oldestIndex >= 0) {
    DEBUG_PRINT("[DEV] Replacing device at slot ");
    DEBUG_PRINTLN(oldestIndex);
    initDevice(oldestIndex, mac);
    return oldestIndex;
  }
  
  return -1;
}

// Aktualizuje dane urządzenia z pakietu LoRa
void updateDeviceData(int8_t index, SensorPacket* pkt, float rssi, float snr) {
  if (index < 0 || index >= MAX_DEVICES) return;
  
  DeviceData* dev = &devices[index];
  
  // Dane sensorów
  dev->temperature = pkt->temperature / 100.0;
  dev->humidity = pkt->humidity / 100.0;
  dev->pressure = pkt->pressure;
  dev->pm1_0 = pkt->pm1_0;
  dev->pm2_5 = pkt->pm2_5;
  dev->pm10 = pkt->pm10;
  dev->status = pkt->status;
  
  // Dane LoRa
  dev->rssi = rssi;
  dev->snr = snr;
  dev->lastSeenMs = millis();
  dev->lastSeqNum = pkt->seqNum;
  dev->active = true;
  dev->packetsReceived++;  // NOWE - inkrementuj licznik
  
  // Akumulacja do historii - wszystkie typy PM
  dev->pm1Sum += pkt->pm1_0;
  dev->pm25Sum += pkt->pm2_5;
  dev->pm10Sum += pkt->pm10;
  dev->pmSampleCount++;
  
  // Sprawdź czy czas na zapis do historii
  updateHistoryIfNeeded(index);
}

// Aktualizuje historię PM (wszystkie typy) jeśli minęła godzina
void updateHistoryIfNeeded(int8_t index) {
  if (index < 0 || index >= MAX_DEVICES) return;
  
  DeviceData* dev = &devices[index];
  
  if (dev->pmSampleCount == 0) return;
  
  uint32_t elapsed = millis() - dev->lastHistoryUpdateMs;
  
  if (elapsed >= HISTORY_INTERVAL_MS) {
    // Oblicz średnie dla wszystkich typów PM
    uint16_t avg1 = dev->pm1Sum / dev->pmSampleCount;
    uint16_t avg25 = dev->pm25Sum / dev->pmSampleCount;
    uint16_t avg10 = dev->pm10Sum / dev->pmSampleCount;
    
    // Zapisz do circular buffer - wszystkie typy
    dev->pm1History[dev->historyIndex] = avg1;
    dev->pm25History[dev->historyIndex] = avg25;
    dev->pm10History[dev->historyIndex] = avg10;
    
    dev->historyIndex = (dev->historyIndex + 1) % HISTORY_SIZE;
    if (dev->historyCount < HISTORY_SIZE) {
      dev->historyCount++;
    }
    
    DEBUG_PRINT("[DEV] ");
    DEBUG_PRINT(dev->name);
    DEBUG_PRINT(" PM avg: 1.0=");
    DEBUG_PRINT(avg1);
    DEBUG_PRINT(" 2.5=");
    DEBUG_PRINT(avg25);
    DEBUG_PRINT(" 10=");
    DEBUG_PRINT(avg10);
    DEBUG_PRINT(" (");
    DEBUG_PRINT(dev->pmSampleCount);
    DEBUG_PRINTLN(" samples)");
    
    // Reset akumulacji - wszystkie typy
    dev->pm1Sum = 0;
    dev->pm25Sum = 0;
    dev->pm10Sum = 0;
    dev->pmSampleCount = 0;
    dev->lastHistoryUpdateMs = millis();
  }
}

// Sprawdza timeout urządzeń (oznacza nieaktywne)
void checkDeviceTimeouts() {
  uint32_t now = millis();
  
  for (uint8_t i = 0; i < deviceCount; i++) {
    if (devices[i].active) {
      uint32_t elapsed = now - devices[i].lastSeenMs;
      if (elapsed > DEVICE_TIMEOUT_MS) {
        devices[i].active = false;
        DEBUG_PRINT("[DEV] Timeout: ");
        DEBUG_PRINTLN(devices[i].name);
        screenNeedsRedraw = true;
      }
    }
  }
}

// Zwraca liczbę aktywnych urządzeń
uint8_t getActiveDeviceCount() {
  uint8_t count = 0;
  for (uint8_t i = 0; i < deviceCount; i++) {
    if (devices[i].active) count++;
  }
  return count;
}

// ============================================
// LORA PACKET RECEPTION (Interrupt-based, non-blocking)
// ============================================
#if ENABLE_LORA

bool receivePacket() {
  if (!lora_ok || radio == nullptr) return false;
  
  // Sprawdź flagę przerwania (non-blocking)
  if (!loraPacketReceived) return false;
  
  // Reset flagi PRZED przetwarzaniem (żeby nie stracić kolejnego pakietu)
  loraPacketReceived = false;
  
  // Oznacz że LoRa używa SPI
  loraActive = true;
  
  uint8_t buffer[64];
  int state = radio->readData(buffer, sizeof(SensorPacket));
  
  // Pobierz RSSI i SNR zaraz po readData
  float rssi = radio->getRSSI();
  float snr = radio->getSNR();
  
  // W trybie continuous RX (RX_TIMEOUT_INF) radio automatycznie
  // kontynuuje nasłuchiwanie po readData - NIE wywołujemy startReceive()!
  
  // Zwolnij SPI
  loraActive = false;
  
  if (state == RADIOLIB_ERR_NONE) {
    SensorPacket* pkt = (SensorPacket*)buffer;
    uint16_t calculatedCRC = calculateCRC16(buffer, sizeof(SensorPacket) - 2);
    
    if (calculatedCRC != pkt->crc) {
      packetsCrcError++;
      DEBUG_LORA_PRINTLN("[LORA] CRC ERROR!");
      return false;
    }
    
    if (pkt->msgType != MSG_TYPE_DATA && pkt->msgType != MSG_TYPE_STATUS) {
      return false;
    }
    
    // === MULTI-DEVICE SYSTEM ===
    int8_t devIndex = findOrAddDevice(pkt->mac);
    if (devIndex >= 0) {
      updateDeviceData(devIndex, pkt, rssi, snr);
      loraDataUpdated = true;
    }
    
    // Legacy - zachowaj dla kompatybilności
    memcpy(&lastPacket, pkt, sizeof(SensorPacket));
    lastRssi = rssi;
    lastSnr = snr;
    lastPacketTime = millis();
    hasData = true;
    packetsReceived++;
    
    // === WYŚLIJ ACK ===
    sendAckPacket(pkt->mac, pkt->seqNum);
    
    char macStr[18];
    formatMAC(pkt->mac, macStr);
    DEBUG_LORA_PRINT("[LORA] RX ");
    DEBUG_LORA_PRINT(macStr);
    DEBUG_LORA_PRINT(" seq=");
    DEBUG_LORA_PRINT(pkt->seqNum);
    DEBUG_LORA_PRINT(" T=");
    DEBUG_LORA_PRINT(pkt->temperature / 100.0);
    DEBUG_LORA_PRINT("C PM2.5=");
    DEBUG_LORA_PRINT(pkt->pm2_5);
    DEBUG_LORA_PRINT(" -> device[");
    DEBUG_LORA_PRINT(devIndex);
    DEBUG_LORA_PRINTLN("]");
    
    return true;
    
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    packetsCrcError++;
    DEBUG_LORA_PRINTLN("[LORA] RadioLib CRC mismatch");
  } else {
    DEBUG_LORA_PRINT("[LORA] readData error: ");
    DEBUG_LORA_PRINTLN(state);
  }
  
  return false;
}

#endif // ENABLE_LORA
