# Air Condition ePaper Display

## Opis Projektu

Wyświetlacz e-paper odbierający dane jakości powietrza przez UDP broadcast z urządzenia `air_condition_receiver`. Wyświetla dane z maksymalnie 4 czujników w trybie niskiego poboru energii.

---

## Hardware

### Lista Komponentów

| # | Element | Model | Status |
|---|---------|-------|--------|
| 1 | MCU | XIAO ESP32-S3 | KUPIONY |
| 2 | ePaper Breakout Board | Seeed Studio 105990172 | KUPIONY |
| 3 | Wyświetlacz | 2.9" Monochrome ePaper 296x128px (SSD1680) | KUPIONY |

### Specyfikacja Wyświetlacza

| Parametr | Wartość |
|----------|---------|
| Rozdzielczość | 296 x 128 px |
| Kolory | Czarno-biały (monochromatyczny) |
| Sterownik | SSD1680 |
| Interfejs | SPI |
| Rozmiar | 2.9" (przekątna) |
| Czas odświeżania | ~2s (pełne), ~0.3s (częściowe) |

### Pinout: ePaper Breakout Board -> XIAO ESP32-S3

| Funkcja ePaper | Pin XIAO | GPIO ESP32-S3 |
|----------------|----------|---------------|
| RST | D0 | GPIO1 |
| CS | D1 | GPIO2 |
| DC | D3 | GPIO4 |
| BUSY | D5 | GPIO6 |
| SCK | D8 | GPIO7 |
| MOSI | D10 | GPIO9 |

### Schemat Połączeń

```
+-------------------------------------------------------------+
|  ePaper Breakout Board (Seeed Studio 105990172)             |
|                                                             |
|  +-----------------+     +-----------------------------+    |
|  |  XIAO ESP32-S3  |     |  2.9" ePaper Display        |    |
|  |                 |     |  296x128 Monochrome         |    |
|  |  Wkladane w     |     |                             |    |
|  |  gniazdo XIAO   |     |  Podlaczone przez           |    |
|  |  na plytce      |     |  24-pin FPC connector       |    |
|  +-----------------+     +-----------------------------+    |
|                                                             |
|  USB-C ----> Zasilanie + programowanie                      |
+-------------------------------------------------------------+
```

---

## Funkcjonalności (v0.8)

### Ekrany

1. **Splash Screen** - ikona pluc + "AIR QUALITY MONITOR" (2s)
2. **Connecting** - splash + "Connecting..." + SSID
3. **Connected** - splash + "Connected!" + IP
4. **Waiting** - oczekiwanie na dane UDP
5. **Main Screen** - wyswietlanie danych

### Layout Main Screen

```
AIR Q.              1003hPa OK 12:34:56
-------------------------------------------
OUTSIDE  -1.7C 70% P:6/10
SALON     23C 45% P:12/18
BEDROOM   21C 55% P:8/15
KITCHEN   25C 40% P:5/12
```

- **Naglowek**: nazwa, cisnienie, status WiFi, czas NTP
- **Wiersz**: nazwa (7 znakow), temp, wilgotnosc%, PM2.5/PM10

### Odswiezanie

- **Przy zmianie danych**: temperatura >=0.5C, wilgotnosc >=2%, PM >=3
- **Co 2 minuty**: aktualizacja zegara (jesli zmienila sie minuta)
- **Minimalne opoznienie**: 30s miedzy odswiezeniami

---

## Protokol Komunikacji

### Zrodlo Danych

Dane odbierane przez UDP broadcast z urzadzenia `air_condition_receiver`.

| Parametr | Wartosc |
|----------|---------|
| Port UDP | 5000 |
| Interwal | 10 sekund |
| Format | JSON |
| Broadcast | Subnet broadcast (np. 192.168.1.255) |

### Format JSON

```json
{
  "type": "air_quality_data",
  "receiver_mac": "AA:BB:CC:DD:EE:FF",
  "timestamp": 1735650000,
  "devices": [
    {
      "mac": "11:22:33:44:55:01",
      "name": "Salon",
      "active": true,
      "temperature": 26.52,
      "humidity": 45.2,
      "pressure": 1013,
      "pm1_0": 8,
      "pm2_5": 12,
      "pm10": 18,
      "lora_rssi": -65.0,
      "lora_snr": 8.5,
      "last_seen_sec": 12
    }
  ]
}
```

### Pola Urzadzenia

| Pole | Typ | Opis |
|------|-----|------|
| mac | string | Adres MAC urzadzenia |
| name | string | Nazwa (max 15 znakow) |
| active | boolean | Czy urzadzenie aktywne |
| temperature | float | Temperatura C |
| humidity | float | Wilgotnosc % |
| pressure | int | Cisnienie hPa |
| pm1_0 | int | PM1.0 ug/m3 |
| pm2_5 | int | PM2.5 ug/m3 |
| pm10 | int | PM10 ug/m3 |
| lora_rssi | float | Sila sygnalu LoRa dBm |
| lora_snr | float | SNR LoRa dB |
| last_seen_sec | int | Sekundy od ostatniego pakietu |

---

## Biblioteki Arduino

| Biblioteka | Wersja | Uzycie |
|------------|--------|--------|
| GxEPD2 | 1.6.5 | Sterownik e-paper |
| ArduinoJson | 7.x | Parsowanie JSON |
| WiFi | wbudowana | Polaczenie WiFi |
| WiFiUdp | wbudowana | Odbieranie UDP |

---

## Struktura Plikow

```
air_condition_epaper/
├── air_condition_epaper.ino   # Glowny plik
├── config.h                   # Konfiguracja pinow, WiFi, stalych
├── splash_bitmap.h            # Bitmapa ikony pluc dla splash screen
└── PROJECT.md                 # Ta dokumentacja
```

---

## Fazy Implementacji

### Faza 1: Test Hardware - DONE
- [x] Dokumentacja projektu
- [x] Test wyswietlania na e-paper
- [x] Kompilacja i upload

### Faza 2: WiFi + UDP - DONE
- [x] Polaczenie WiFi
- [x] Odbieranie UDP broadcast
- [x] Parsowanie JSON
- [x] Synchronizacja czasu NTP

### Faza 3: UI - DONE
- [x] Splash screen z grafika
- [x] Layout dla danych urzadzen
- [x] Wyswietlanie pomiarow
- [x] Odswiezanie przy nowych danych
- [x] Automatyczne odswiezanie zegara co 2 min

### Faza 4: Optymalizacja (przyszla)
- [ ] Deep sleep miedzy odswiezeniami
- [ ] Partial refresh dla zegara
- [ ] Wskazniki jakosci powietrza (ikony)

---

## Kompilacja i Upload

```bash
# Kompilacja
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 .

# Upload (WAZNE: odlacz baterie z breakout board przed upload!)
arduino-cli upload -p /dev/cu.usbmodem101 --fqbn esp32:esp32:XIAO_ESP32S3 .

# Serial Monitor
arduino-cli monitor -p /dev/cu.usbmodem101 -c baudrate=115200
```

---

## Konfiguracja WiFi

Edytuj `config.h`:
```cpp
#define WIFI_SSID       "TwojaSSID"
#define WIFI_PASSWORD   "TwojeHaslo"
```

---

## Historia Zmian

| Data | Wersja | Opis |
|------|--------|------|
| 2026-01-10 | 0.1 | Utworzenie projektu, dokumentacja |
| 2026-01-10 | 0.2 | Test hardware, pierwszy upload |
| 2026-01-10 | 0.3 | WiFi, UDP, parsowanie JSON |
| 2026-01-10 | 0.4 | UI layout, wyswietlanie danych |
| 2026-01-10 | 0.5 | Poprawa layoutu, NTP |
| 2026-01-10 | 0.6 | Kompaktowy format, 7 znakow nazwy |
| 2026-01-10 | 0.7 | Splash screen, odswiezanie co 2 min |
| 2026-01-10 | 0.8 | Splash z ikona pluc, connecting na tle splash |
