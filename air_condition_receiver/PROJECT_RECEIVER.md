# Projekt: Air Condition Receiver - Odbiornik z Ekranem Dotykowym

## Opis Ogólny

Odbiornik danych z czujników jakości powietrza z komunikacją LoRa (SX1262). Urządzenie odbiera dane z wielu nadajników (max 4), wyświetla je na kolorowym ekranie dotykowym LCD 2.8" i rozsyła przez WiFi (UDP) do innych urządzeń (Raspberry Pi, iOS).

**Kluczowe funkcje:**
- Odbiór danych LoRa z wielu nadajników (identyfikacja po MAC)
- Kolorowy interfejs dotykowy 320×240px (landscape)
- Kafelki z podglądem danych każdego nadajnika
- Szczegółowy widok z wykresem historii PM (24h, rotacja PM1.0/PM2.5/PM10)
- Konfiguracja WiFi przez dotyk z klawiaturą ekranową QWERTY
- Skanowanie i wybór sieci WiFi z listy
- Wyświetlanie aktualnego czasu (NTP) w headerze z auto-odświeżaniem
- Non-blocking WiFi/NTP - urządzenie startuje natychmiast
- Zapis credentials WiFi w NVS (Preferences)

---

## Lista Komponentów (BOM)

| # | Element | Model | Status | Uwagi |
|---|---------|-------|--------|-------|
| 1 | MCU | XIAO ESP32-S3 | **KUPIONY** | Główny kontroler |
| 2 | Moduł LoRa | Wio-SX1262 (B2B) | **KUPIONY** | Komunikacja radiowa 868MHz |
| 3 | Wyświetlacz | Waveshare Pico-ResTouch-LCD-2.8" | **KUPIONY** | 320×240, ST7789V, XPT2046 |
| 4 | Przewody | Dupont F-F lub płytka uniwersalna | DO PRZYGOTOWANIA | 10 połączeń |

### Link do Wyświetlacza

**Waveshare Pico-ResTouch-LCD-2.8"**
- Botland: https://botland.com.pl/raspberry-pi-pico-hat-klawiatury-i-wyswietlacze/19718-ekran-dotykowy-rezystancyjny-lcd-ips-28-320x240px-spi-65k-rgb-do-raspberry-pi-pico-waveshare-19804-5904422347246.html
- Wiki: https://www.waveshare.com/wiki/Pico-ResTouch-LCD-2.8

### Specyfikacja Wyświetlacza

| Parametr | Wartość |
|----------|---------|
| Rozdzielczość | 320 × 240 px |
| Typ panelu | IPS |
| Kolory | 262K (RGB565) |
| Kontroler LCD | ST7789V |
| Kontroler dotyku | XPT2046 (rezystancyjny) |
| Interfejs | SPI |
| Napięcie | 3.3V / 5V |
| Wymiary | 70.2 × 50.2 mm |

### Wymagania Zasilania

| Parametr | Wartość | Uwagi |
|----------|---------|-------|
| Napięcie robocze | **5V** | Pin VSYS |
| Pobór prądu | ~140mA max | LCD + backlight |
| Źródło zasilania | **USB-C** | XIAO musi być podłączony do USB |

**UWAGA:** Wyświetlacz wymaga zasilania **5V** z pinu VSYS. XIAO ESP32-S3 dostarcza 5V na pinie 14 **tylko przy zasilaniu przez USB-C**. 

**Odbiornik NIE działa na baterii** bez dodatkowej przetwornicy step-up 3.3V→5V.

---

## Schemat Połączeń

### Mapowanie Pinów: Waveshare LCD (Pico) → XIAO ESP32-S3

LCD Waveshare używa pinów Raspberry Pi Pico. Poniżej mapowanie na XIAO ESP32-S3:

| LCD Pin (PCB) | Pico GPIO | Funkcja | XIAO Pin | XIAO GPIO | Kolor przewodu |
|---------------|-----------|---------|----------|-----------|----------------|
| VCC (VSYS) | - | Zasilanie 5V | **5V** | - | Czerwony |
| GND | - | Masa | **GND** | - | Czarny |
| LCD_DC | GP8 | Data/Command | **D1** | GPIO2 | Żółty |
| LCD_CS | GP9 | LCD Chip Select | **D0** | GPIO1 | Pomarańczowy |
| LCD_CLK | GP10 | SPI Clock | **D4** | GPIO5 | Niebieski |
| MOSI | GP11 | SPI MOSI | **D5** | GPIO6 | Zielony |
| MISO | GP12 | SPI MISO | **D7** | GPIO44 | Biały |
| LCD_BL | GP13 | Backlight PWM | **D3** | GPIO4 | Fioletowy |
| LCD_RST | GP15 | LCD Reset | **D2** | GPIO3 | Szary |
| TP_CS | GP16 | Touch Chip Select | **D6** | GPIO43 | Brązowy |

### Piny NIE Podłączane (SD Card - nieużywane)

| LCD Pin | Pico GPIO | Powód |
|---------|-----------|-------|
| TP_IRQ | GP17 | Używamy polling zamiast interrupt |
| SDIO_CLK | GP5 | Karta SD nieużywana |
| SDIO_CMD | GP18 | Karta SD nieużywana |
| D0-D3 | GP19-22 | Karta SD nieużywana |

### Podsumowanie - Lista Połączeń (10 przewodów)

```
WAVESHARE LCD              XIAO ESP32-S3
(pin na płytce)            (pin na płytce)
───────────────────────────────────────────
VCC  (VSYS, pin 2)    ───►  5V  (pin 14)
GND  (pin 3)          ───►  GND (pin 13)
LCD_DC  (GP8)         ───►  D1  (pin 2)
LCD_CS  (GP9)         ───►  D0  (pin 1)
LCD_CLK (GP10)        ───►  D4  (pin 5)
MOSI    (GP11)        ───►  D5  (pin 6)
MISO    (GP12)        ───►  D7  (pin 8)
LCD_BL  (GP13)        ───►  D3  (pin 4)
LCD_RST (GP15)        ───►  D2  (pin 3)
TP_CS   (GP16)        ───►  D6  (pin 7)
```

### Wizualizacja Pinout - XIAO ESP32-S3

```
              XIAO ESP32-S3 (widok z góry)
              ┌───────────────────┐
   D0 (GPIO1) │ 1  ●         ● 14 │ 5V ◄── LCD VCC
   D1 (GPIO2) │ 2  ●         ● 13 │ GND ◄── LCD GND
   D2 (GPIO3) │ 3  ●         ● 12 │ 3V3 (nie używane)
   D3 (GPIO4) │ 4  ●         ● 11 │ D10 ─┐
   D4 (GPIO5) │ 5  ●         ● 10 │ D9  ├─ B2B SX1262
   D5 (GPIO6) │ 6  ●         ●  9 │ D8  ─┘
  D6 (GPIO43) │ 7  ●         ●  8 │ D7 (GPIO44)
              └───────────────────┘
                    [USB-C]

PRZYPISANIE PINÓW LCD:
Pin 1  (D0) ◄── LCD_CS      (Chip Select LCD)
Pin 2  (D1) ◄── LCD_DC      (Data/Command)
Pin 3  (D2) ◄── LCD_RST     (Reset)
Pin 4  (D3) ◄── LCD_BL      (Backlight PWM)
Pin 5  (D4) ◄── LCD_CLK     (SPI Clock)
Pin 6  (D5) ◄── MOSI        (SPI Data Out)
Pin 7  (D6) ◄── TP_CS       (Chip Select Touch)
Pin 8  (D7) ◄── MISO        (SPI Data In - Touch)
Pin 13 (GND) ◄── GND        (Masa)
Pin 14 (5V)  ◄── VCC        (Zasilanie 5V - wymaga USB!)

PINY B2B (Wio-SX1262 - zajęte):
GPIO7  = LORA_SCK
GPIO8  = LORA_MISO
GPIO9  = LORA_MOSI
GPIO38 = LORA_ANTSW
GPIO39 = LORA_DIO1/IRQ
GPIO40 = LORA_BUSY
GPIO41 = LORA_NSS/CS
GPIO42 = LORA_RST
```

### Wizualizacja Pinout - Waveshare LCD 2.8"

```
Waveshare Pico-ResTouch-LCD-2.8" (widok od tyłu)
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│   ┌─────────────┐                                               │
│   │  SD Card    │  ◄── NIE UŻYWAMY                              │
│   │   Slot      │                                               │
│   └─────────────┘                                               │
│                                                                 │
│   40-pin Header (numeracja jak Raspberry Pi Pico)               │
│                                                                 │
│   Pin 1  ●────── VBUS (5V)         Pin 2  ●────── VSYS ◄── VCC  │
│   Pin 3  ●────── GND ◄── GND       Pin 4  ●────── 3V3_EN        │
│   Pin 5  ●────── GP2               Pin 6  ●────── GP3           │
│   Pin 7  ●────── GP4               Pin 8  ●────── GP5 (SDIO)    │
│   Pin 9  ●────── GND               Pin 10 ●────── GP6           │
│   Pin 11 ●────── GP7               Pin 12 ●────── GP8 ◄── DC    │
│   Pin 13 ●────── GND               Pin 14 ●────── GP9 ◄── CS    │
│   Pin 15 ●────── GP10 ◄── CLK      Pin 16 ●────── GP11 ◄── MOSI │
│   Pin 17 ●────── GP12 ◄── MISO     Pin 18 ●────── GND           │
│   Pin 19 ●────── GP13 ◄── BL       Pin 20 ●────── GP14          │
│   Pin 21 ●────── GP15 ◄── RST      Pin 22 ●────── GP16 ◄── TP_CS│
│   Pin 23 ●────── GND               Pin 24 ●────── GP17 (TP_IRQ) │
│   ...                                                           │
│   Pin 36 ●────── 3V3 OUT           Pin 38 ●────── GND           │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

UŻYWANE PINY (10 połączeń):
Pin 2  (VSYS)  = VCC      → XIAO 3V3
Pin 3  (GND)   = GND      → XIAO GND
Pin 12 (GP8)   = LCD_DC   → XIAO D1
Pin 14 (GP9)   = LCD_CS   → XIAO D0
Pin 15 (GP10)  = LCD_CLK  → XIAO D4
Pin 16 (GP11)  = MOSI     → XIAO D5
Pin 17 (GP12)  = MISO     → XIAO D7
Pin 19 (GP13)  = LCD_BL   → XIAO D3
Pin 21 (GP15)  = LCD_RST  → XIAO D2
Pin 22 (GP16)  = TP_CS    → XIAO D6
```

### Schemat Blokowy Połączeń

```
┌─────────────────────┐                    ┌─────────────────────┐
│  XIAO ESP32-S3      │                    │  Waveshare LCD 2.8" │
│  + Wio-SX1262 (B2B) │                    │  ST7789V + XPT2046  │
│                     │                    │                     │
│              3V3 ───┼────── czerwony ────┼─── VCC (VSYS)       │
│              GND ───┼────── czarny ──────┼─── GND              │
│                     │                    │                     │
│   D0 (GPIO1)    ────┼────── pomarańcz ───┼─── LCD_CS (GP9)     │
│   D1 (GPIO2)    ────┼────── żółty ───────┼─── LCD_DC (GP8)     │
│   D2 (GPIO3)    ────┼────── szary ───────┼─── LCD_RST (GP15)   │
│   D3 (GPIO4)    ────┼────── fioletowy ───┼─── LCD_BL (GP13)    │
│   D4 (GPIO5)    ────┼────── niebieski ───┼─── LCD_CLK (GP10)   │
│   D5 (GPIO6)    ────┼────── zielony ─────┼─── MOSI (GP11)      │
│   D6 (GPIO43)   ────┼────── brązowy ─────┼─── TP_CS (GP16)     │
│   D7 (GPIO44)   ────┼────── biały ───────┼─── MISO (GP12)      │
│                     │                    │                     │
│  ┌───────────────┐  │                    │  ┌───────────────┐  │
│  │ Wio-SX1262    │  │                    │  │   SD Card     │  │
│  │ (B2B connector)│  │                    │  │ (nie używana) │  │
│  │               │  │                    │  │               │  │
│  │ GPIO41 = NSS  │  │                    │  └───────────────┘  │
│  │ GPIO7  = SCK  │  │                    │                     │
│  │ GPIO9  = MOSI │  │                    │                     │
│  │ GPIO8  = MISO │  │                    │                     │
│  │ GPIO42 = RST  │  │                    │                     │
│  │ GPIO40 = BUSY │  │                    │                     │
│  │ GPIO39 = DIO1 │  │                    │                     │
│  └───────────────┘  │                    │                     │
└─────────────────────┘                    └─────────────────────┘
```

---

## Płytka Prototypowa Pico jako Adapter

### Problem Kompatybilności

Wyświetlacz Waveshare Pico-ResTouch-LCD-2.8" ma złącze 40-pin zgodne z Raspberry Pi Pico. XIAO ESP32-S3 ma tylko 14 pinów w zupełnie innym układzie. **Nie można bezpośrednio włożyć XIAO w gniazdo Pico**.

### Rozwiązania Montażowe

#### Opcja 1: Przewody Dupont (najprostsza, do testów)
Użyj 10 przewodów żeńsko-żeńskich:
- Jeden koniec do pinów XIAO
- Drugi koniec do odpowiednich pinów na wyświetlaczu

#### Opcja 2: Płytka prototypowa Pico jako adapter (trwałe)
Przylutuj XIAO na płytce i połącz przewodami z odpowiednimi pinami Pico.

#### Opcja 3: Uniwersalna płytka prototypowa
Użyj standardowej płytki perfboard zamiast dedykowanej dla Pico.

### Mapowanie Pinów: Pico → XIAO ESP32-S3

Tabela konwersji numerów pinów płytki Pico na piny XIAO:

| Pico Pin # | Pico GPIO | Funkcja LCD | → | XIAO Pin | XIAO GPIO |
|------------|-----------|-------------|---|----------|-----------|
| **2** (VSYS) | - | VCC (5V) | → | **5V** (pin 14) | - |
| **3** (GND) | - | GND | → | **GND** (pin 13) | - |
| **12** | GP8 | LCD_DC | → | **D1** (pin 2) | GPIO2 |
| **14** | GP9 | LCD_CS | → | **D0** (pin 1) | GPIO1 |
| **15** | GP10 | LCD_CLK | → | **D4** (pin 5) | GPIO5 |
| **16** | GP11 | MOSI | → | **D5** (pin 6) | GPIO6 |
| **17** | GP12 | MISO | → | **D7** (pin 8) | GPIO44 |
| **19** | GP13 | LCD_BL | → | **D3** (pin 4) | GPIO4 |
| **21** | GP15 | LCD_RST | → | **D2** (pin 3) | GPIO3 |
| **22** | GP16 | TP_CS | → | **D6** (pin 7) | GPIO43 |

### Płytka Prototypowa Pico - Wizualizacja

```
    PŁYTKA PROTOTYPOWA PICO (widok z góry - strona z numeracją)
    ═══════════════════════════════════════════════════════════
    
    Lewa strona                              Prawa strona
    (piny 0-15)                              (piny VB-16)
    
     0 ●●──────────────────────────────●● VB (VBUS)
     1 ●●──────────────────────────────●● VS (VSYS) ◄── VCC
    ═══════════════════════════════════════════════════════════
     2 ●●──────────────────────────────●● 3E (3V3_EN)
     3 ●●──────────────────────────────●● + (3V3 OUT)
     4 ●●──────────────────────────────●● VA (VREF)
     5 ●●──────────────────────────────●● 28 (GP28)
    ═══════════════════════════════════════════════════════════
     6 ●●──────────────────────────────●● 27 (GP27)
     7 ●●──────────────────────────────●● 26 (GP26)
     8 ●●──────────────────────────────●● RU (RUN)
     9 ●●──────────────────────────────●● 22 (GP22)
    ═══════════════════════════════════════════════════════════
    10 ●●──────────────────────────────●● 21 (GP15) ◄── LCD_RST
    11 ●●──────────────────────────────●● 20 (GP14)
    12 ●●──────────────────────────────●● 19 (GP13) ◄── LCD_BL
    13 ●●──────────────────────────────●● 18 (GND)
    ═══════════════════════════════════════════════════════════
    14 ●●──────────────────────────────●● 17 (GP12) ◄── MISO
    15 ●●──────────────────────────────●● 16 (GP11) ◄── MOSI

    (Ciąg dalszy - dolna część płytki)

    15 (GP10) ◄── LCD_CLK                16 (GP11) ◄── MOSI
    14 (GP9)  ◄── LCD_CS                 17 (GP12) ◄── MISO
    13 (GND)                             18 (GND)
    12 (GP8)  ◄── LCD_DC                 19 (GP13) ◄── LCD_BL
    11 (GP7)                             20 (GP14)
    10 (GP6)                             21 (GP15) ◄── LCD_RST
     9 (GND)                             22 (GP16) ◄── TP_CS
     ...
     3 (GND) ◄── GND
     2 (VSYS) ◄── VCC
```

### Lista Pinów do Połączenia na Płytce Pico

```
UŻYWANE PINY NA PŁYTCE PICO (10 połączeń):
══════════════════════════════════════════

Zasilanie:
  Pin "VS" (VSYS, prawa strona u góry) ──► XIAO 5V (pin 14)
  Pin "3" (GND, lewa strona)           ──► XIAO GND (pin 13)

SPI LCD:
  Pin "12" (GP8)  ──► XIAO D1 (LCD_DC)
  Pin "14" (GP9)  ──► XIAO D0 (LCD_CS)
  Pin "15" (GP10) ──► XIAO D4 (LCD_CLK)
  Pin "16" (GP11) ──► XIAO D5 (MOSI)
  Pin "17" (GP12) ──► XIAO D7 (MISO)

LCD Control:
  Pin "19" (GP13) ──► XIAO D3 (LCD_BL - podświetlenie)
  Pin "21" (GP15) ──► XIAO D2 (LCD_RST - reset)

Touch:
  Pin "22" (GP16) ──► XIAO D6 (TP_CS - touch chip select)
```

### Instrukcja Montażu z Płytką Pico

1. **Przygotowanie płytki:**
   - Przylutuj piny męskie (goldpin) od SPODU płytki (strona bez numeracji)
   - Piny powinny wystawać w dół, aby wejść w gniazda wyświetlacza

2. **Montaż XIAO:**
   - Przylutuj gniazdo żeńskie 2×7 pin na środku płytki dla XIAO
   - Lub przylutuj XIAO bezpośrednio (mniej elastyczne)

3. **Połączenia:**
   - Użyj krótkich przewodów lub ścieżek lutowniczych
   - Połącz według tabeli mapowania powyżej

4. **Weryfikacja:**
   - Sprawdź ciągłość multimetrem przed włączeniem zasilania
   - Upewnij się, że nie ma zwarć między sąsiednimi pinami

---

## Zajętość Pinów XIAO ESP32-S3

| Pin | GPIO | Funkcja | Moduł | Uwagi |
|-----|------|---------|-------|-------|
| D0 | GPIO1 | LCD_CS | LCD | LOW active |
| D1 | GPIO2 | LCD_DC | LCD | HIGH=data, LOW=cmd |
| D2 | GPIO3 | LCD_RST | LCD | LOW active |
| D3 | GPIO4 | LCD_BL | LCD | PWM 0-255 |
| D4 | GPIO5 | SPI_CLK | LCD+Touch | Software SPI |
| D5 | GPIO6 | SPI_MOSI | LCD+Touch | Software SPI |
| D6 | GPIO43 | TP_CS | Touch | LOW active |
| D7 | GPIO44 | SPI_MISO | Touch | Dane z dotyku |
| (B2B) | GPIO7 | LORA_SCK | SX1262 | Hardware SPI |
| (B2B) | GPIO8 | LORA_MISO | SX1262 | Hardware SPI |
| (B2B) | GPIO9 | LORA_MOSI | SX1262 | Hardware SPI |
| (B2B) | GPIO38 | LORA_ANTSW | SX1262 | Przełącznik anteny |
| (B2B) | GPIO39 | LORA_DIO1 | SX1262 | Przerwanie |
| (B2B) | GPIO40 | LORA_BUSY | SX1262 | Status |
| (B2B) | GPIO41 | LORA_NSS | SX1262 | Chip Select |
| (B2B) | GPIO42 | LORA_RST | SX1262 | Reset |

**Wolne piny:** Brak (wszystkie wykorzystane)

---

## Parametry LoRa (identyczne z nadajnikiem)

| Parametr | Wartość |
|----------|---------|
| Częstotliwość | 868.0 MHz |
| Szerokość pasma (BW) | 125.0 kHz |
| Spreading Factor (SF) | 7 |
| Coding Rate (CR) | 5 (4/5) |
| Sync Word | 0x34 |
| Moc nadawania | 14 dBm |
| Preambuła | 8 symboli |

---

## Protokół Pakietu LoRa (z nadajnika)

```cpp
struct __attribute__((packed)) SensorPacket {
  uint8_t mac[6];        // 0-5:   MAC WiFi ESP32 (unikalny ID)
  uint8_t msgType;       // 6:     Typ wiadomości (0x01 = DATA)
  uint8_t seqNum;        // 7:     Numer sekwencji (0-255)
  int16_t temperature;   // 8-9:   Temperatura × 100
  uint16_t humidity;     // 10-11: Wilgotność × 100
  uint16_t pressure;     // 12-13: Ciśnienie hPa
  uint16_t pm1_0;        // 14-15: PM1.0 µg/m³
  uint16_t pm2_5;        // 16-17: PM2.5 µg/m³
  uint16_t pm10;         // 18-19: PM10 µg/m³
  uint8_t status;        // 20:    Flagi statusu
  uint16_t crc;          // 21-22: CRC-16/CCITT
};
// Rozmiar: 23 bajty
```

---

## Architektura Multi-Device

### Struktura Danych Urządzenia

```cpp
#define MAX_DEVICES 4
#define DEVICE_NAME_LEN 16
#define HISTORY_SIZE 24       // 24 punkty = 24h przy 1h/punkt

struct DeviceData {
  // Identyfikacja
  uint8_t mac[6];             // MAC jako unikalny ID
  char name[DEVICE_NAME_LEN]; // Nazwa edytowalna (np. "Salon")
  bool active;                // Czy urządzenie aktywne
  
  // Ostatnie dane pomiarowe
  float temperature;          // °C
  float humidity;             // %
  uint16_t pressure;          // hPa
  uint16_t pm1_0;             // µg/m³
  uint16_t pm2_5;             // µg/m³
  uint16_t pm10;              // µg/m³
  uint8_t status;             // Flagi błędów
  
  // Metadane odbioru
  float rssi;                 // dBm
  float snr;                  // dB
  uint32_t lastSeenMs;        // millis() ostatniego pakietu
  uint8_t lastSeqNum;         // Wykrywanie duplikatów
  
  // Historia PM (circular buffer) - dla wszystkich typów
  uint16_t pm1History[HISTORY_SIZE];   // PM1.0 uśrednione
  uint16_t pm25History[HISTORY_SIZE];  // PM2.5 uśrednione
  uint16_t pm10History[HISTORY_SIZE];  // PM10 uśrednione
  uint8_t historyIndex;                // Wspólny indeks zapisu
  uint8_t historyCount;                // Liczba zapisanych punktów
  uint32_t lastHistoryUpdateMs;        // Czas ostatniego zapisu
  
  // Akumulacja do uśredniania - wszystkie typy PM
  uint32_t pm1Sum;            // Suma wartości PM1.0
  uint32_t pm25Sum;           // Suma wartości PM2.5
  uint32_t pm10Sum;           // Suma wartości PM10
  uint16_t pmSampleCount;     // Wspólna liczba próbek
};

DeviceData devices[MAX_DEVICES];
uint8_t activeDeviceCount = 0;
```

### Timeout Urządzenia

| Parametr | Wartość | Opis |
|----------|---------|------|
| DEVICE_TIMEOUT_MS | 30 minut | 2 cykle × 15 min |
| HISTORY_INTERVAL_MS | 1 godzina | Interwał aktualizacji historii |

Urządzenie jest oznaczane jako **offline** jeśli nie otrzymaliśmy danych przez 30 minut (2 cykle nadajnika).

---

## Design Interfejsu Użytkownika (UI)

### Paleta Kolorów

```cpp
// Kolory w formacie RGB565 (16-bit)
#define COLOR_BG_MAIN       0x1082  // #1a1a2e - ciemny granat
#define COLOR_BG_TILE       0x1124  // #16213e - granatowy
#define COLOR_BG_TILE_HOVER 0x2148  // #1f4068 - jaśniejszy granat
#define COLOR_BG_HEADER     0x0861  // #0d1b2a - bardzo ciemny

#define COLOR_ACCENT        0x04B4  // #0f94d2 - niebieski
#define COLOR_ACCENT2       0x06DF  // #00d9ff - cyjan

#define COLOR_TEXT          0xFFFF  // #ffffff - biały
#define COLOR_TEXT_SEC      0xA514  // #a0a0a0 - szary
#define COLOR_TEXT_VALUE    0x07F2  // #00ff88 - zielony neon

#define COLOR_STATUS_OK     0x07F2  // #00ff88 - zielony
#define COLOR_STATUS_WARN   0xFD40  // #ffaa00 - pomarańczowy (PM 35-75)
#define COLOR_STATUS_DANGER 0xF9A6  // #ff3366 - czerwony (PM >75)
#define COLOR_STATUS_OFF    0x6B4D  // #666666 - ciemnoszary

#define COLOR_BUTTON        0x2148  // #1f4068 - przycisk normalny
#define COLOR_BUTTON_PRESS  0x04B4  // #0f94d2 - przycisk naciśnięty
```

### Progi Alarmów PM2.5

| PM2.5 [µg/m³] | Status | Kolor kafelka |
|---------------|--------|---------------|
| 0 - 35 | OK | Zielony pasek |
| 35 - 75 | WARNING | Pomarańczowy pasek |
| > 75 | DANGER | Czerwony pasek + animacja |

### Ekrany UI

#### 1. Home Screen (320×240, Landscape) - Zaimplementowane v2.6

```
┌────────────────────────────────────────────────────────────────┐
│  ╔══════════════════════════════════════════════════════════╗  │
│  ║  AIR QUALITY              📶  12:45:32   ⚙️           ║  │
│  ╚══════════════════════════════════════════════════════════╝  │
│                                                                │
│  ┌──────────────────────┐  ┌──────────────────────┐           │
│  │▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│  │▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│           │
│  │ Device E8D4          │  │ Device A1B2          │           │
│  │                       │  │                       │           │
│  │  26.5°C      45%     │  │  24.2°C      52%     │           │
│  │                       │  │                       │           │
│  │  PM2.5: 12   PM10: 18│  │  PM2.5: 8    PM10: 12│           │
│  │  ████░░░░  OK        │  │  ██░░░░░░  OK        │           │
│  └──────────────────────┘  └──────────────────────┘           │
│                                                                │
│  ┌──────────────────────┐  ┌──────────────────────┐           │
│  │░░░░░░░░░░░░░░░░░░░░░░│  │░░░░░░░░░░░░░░░░░░░░░░│           │
│  │                       │  │                       │           │
│  │   Waiting for         │  │   Waiting for         │           │
│  │   device...           │  │   device...           │           │
│  │                       │  │                       │           │
│  └──────────────────────┘  └──────────────────────┘           │
│                                                                │
└────────────────────────────────────────────────────────────────┘

Elementy v2.6:
- Header: tytuł "AIR QUALITY", ikona WiFi (📶), czas HH:MM:SS (auto-refresh 1s), ikona ⚙️
- Ikona WiFi: 3 poziomy (weak/medium/strong) + off, kolor: zielony/pomarańczowy/czerwony/szary
- Czas biały gdy NTP zsync, szary gdy nie
- 4 kafelki (2×2 grid) z danymi nadajników
- Kafelek: pasek statusu (kolor wg PM2.5), nazwa, temp, wilg, PM2.5/PM10, mini-bar
- Puste kafelki: "Waiting for device..."
- Dotknięcie kafelka → Detail Screen
- Dotknięcie ⚙️ → Settings Screen
```

#### 2. Detail Screen (Zaimplementowane v2.6)

```
┌────────────────────────────────────────────────────────────────┐
│  ┌────┐                                                        │
│  │ ◄  │  Device E8D4  ✏️                              ⚙️    │
│  └────┘                                                        │
│  ══════════════════════════════════════════════════════════    │
│                                                                │
│   🌡️ Temperature          26.52 °C                             │
│   💧 Humidity             45.2 %                               │
│   🌀 Pressure             1013 hPa                             │
│                                                                │
│  ──────────────────────────────────────────────────────────    │
│                                                                │
│   PM1.0   ████████░░░░       8 µg/m³                          │
│   PM2.5   ████████████░░░   12 µg/m³                          │
│   PM10    ████████████████  18 µg/m³                          │
│                                                                │
│  ──────────────────────────────────────────────────────────    │
│                                                                │
│   📶 RSSI: -65 dBm       SNR: 8.5 dB                          │
│   🕐 Last seen: 12s ago                                       │
│                                                                │
│  ──────────────────────────────────────────────────────────    │
│   PM2.5 History (24h):     ◄── rotuje: PM1.0 → PM2.5 → PM10   │
│                                                                │
│   50│          ▄▄                                              │
│   40│    ▄▄   ████  ▄▄                                        │
│   30│ ▄▄████▄▄████████▄▄    ▄▄                                │
│   20│████████████████████▄▄████▄▄▄▄                           │
│   10│                                                          │
│    0└──────────────────────────────                            │
│      -24h            -12h            now                       │
│                                                                │
└────────────────────────────────────────────────────────────────┘

Elementy v2.6:
- Przycisk Back (◄) → Home Screen
- Nazwa urządzenia (edytowalna) + ikona ✏️ (Edit)
- Dotknięcie ✏️ → otwiera klawiaturę do edycji nazwy
- Nazwa zapisywana do NVS (trwale)
- Przycisk Settings (⚙️) → Settings Screen
- Pełne dane: temp, wilg, ciśnienie z ikonami 16×16
- 3 paski PM (PM1.0/PM2.5/PM10) - kolorowe wg wartości
- RSSI, SNR, czas od ostatniego pakietu
- Wykres słupkowy historii PM (24h, circular buffer)
- Auto-rotacja wykresu co 3s: PM1.0 → PM2.5 → PM10
- "Pending" bar dla aktualnej godziny (akumulacja próbek)
```

#### 2a. Settings Screen (Nowy w v2.4)

```
┌────────────────────────────────────────────────────────────────┐
│  ┌────┐                                                        │
│  │ ◄  │  Settings                                              │
│  └────┘                                                        │
│                                                                │
│   --- WiFi ---                                                 │
│   Status:        Connected                                     │
│   SSID:          HomeNetwork                                   │
│   IP:            192.168.1.105                                 │
│   RSSI:          -55 dBm                                       │
│                                                                │
│   --- LoRa ---                                                 │
│   Status:        OK                                            │
│   Packets RX:    1234                                          │
│   CRC Errors:    2                                             │
│                                                                │
│   --- System ---                                               │
│   Uptime:        2h 15m 30s                                    │
│   Devices:       2 / 4                                         │
│   NTP:           Synced                                        │
│                                                                │
│   ┌────────────────────────────────────────────┐               │
│   │           Configure WiFi                   │               │
│   └────────────────────────────────────────────┘               │
│                                                                │
└────────────────────────────────────────────────────────────────┘

Elementy:
- Przycisk Back → Home Screen
- Sekcja WiFi: status, SSID, IP, RSSI
- Sekcja LoRa: status, pakiety odebrane, błędy CRC
- Sekcja System: uptime, aktywne urządzenia, status NTP
- Przycisk "Configure WiFi" → WiFi Config Screen
```

#### 3. Keyboard Screen (Zaimplementowane v2.4)

```
┌────────────────────────────────────────────────────────────────┐
│                                                                │
│   Edytuj: SSID                                                 │
│   ┌──────────────────────────────────────────────────────┐     │
│   │  HomeNetwork█                                        │     │
│   └──────────────────────────────────────────────────────┘     │
│                                                                │
│   ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐                   │
│   │ 1 │ 2 │ 3 │ 4 │ 5 │ 6 │ 7 │ 8 │ 9 │ 0 │  ◄── Rząd 1      │
│   └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘                   │
│   ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐                   │
│   │ q │ w │ e │ r │ t │ y │ u │ i │ o │ p │  ◄── Rząd 2      │
│   └───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘                   │
│    ┌───┬───┬───┬───┬───┬───┬───┬───┬───┐                      │
│    │ a │ s │ d │ f │ g │ h │ j │ k │ l │     ◄── Rząd 3      │
│    └───┴───┴───┴───┴───┴───┴───┴───┴───┘                      │
│      ┌───┬───┬───┬───┬───┬───┬───┐                            │
│      │ z │ x │ c │ v │ b │ n │ m │            ◄── Rząd 4      │
│      └───┴───┴───┴───┴───┴───┴───┘                            │
│                                                                │
│   ┌─────┬───────────────────────────┬────┬────┬────┐          │
│   │ ⇧  │         SPACJA            │ ⌫  │ESC │ OK │          │
│   └─────┴───────────────────────────┴────┴────┴────┘          │
│                                                                │
└────────────────────────────────────────────────────────────────┘

Tryb SHIFT (wielkie litery + symbole):
   Rząd 1: ! @ # $ % ^ & * ( )
   Rząd 2: Q W E R T Y U I O P
   Rząd 3: A S D F G H J K L
   Rząd 4: Z X C V B N M

Elementy:
- Pole tekstowe z aktualną wartością + kursor (█)
- Etykieta "Edytuj: SSID" lub "Edytuj: Password"
- Klawiatura QWERTY 4 rzędy (10+10+9+7 klawiszy)
- Rząd specjalny: Shift (⇧), Space, Backspace (⌫), ESC, OK
- ESC = anuluj i wróć, OK = zapisz i wróć
```

#### 4. WiFi Config Screen (Zaimplementowane v2.4)

```
┌────────────────────────────────────────────────────────────────┐
│  ┌────┐                                                        │
│  │ ◄  │  WiFi Configuration                                    │
│  └────┘                                                        │
│                                                                │
│   Networks:              ┌──────────┐                          │
│   ┌────────────────────┐ │   Scan   │                          │
│   │ > HomeNetwork -55  │ └──────────┘                          │
│   │   NeighborWiFi -68 │                                       │
│   │   Guest_5G     -72 │                                       │
│   │   IoT_Network  -81 │                                       │
│   │                    │                                       │
│   └────────────────────┘                                       │
│                                                                │
│   SSID:  [HomeNetwork      ] ◄── dotknij aby edytować         │
│   Pass:  [************     ] ◄── otwiera klawiaturę           │
│                                                                │
│   Status: Connected  IP: 192.168.1.105                        │
│                                                                │
│   ┌────────────────┐  ┌────────────────┐                       │
│   │   Disconnect   │  │    Connect     │                       │
│   └────────────────┘  └────────────────┘                       │
│                                                                │
└────────────────────────────────────────────────────────────────┘

Funkcje:
- Lista sieci (max 5, sortowane po RSSI)
- ">" przy wybranej sieci (kliknięcie kopiuje SSID)
- Przycisk Scan - skanuje sieci asynchronicznie
- Pola SSID/Password - dotknięcie otwiera klawiaturę
- Status: "Connected", "Disconnected", "Connecting..."
- IP adres gdy połączony
- Connect: łączy z podanym SSID/hasłem, zapisuje do NVS
- Disconnect: rozłącza WiFi
```

### Animacje

| Animacja | Opis | Czas | Status |
|----------|------|------|--------|
| Startup loader | Pasek postępu przy uruchamianiu (4 kroki) | 2-3s | ✅ |
| Alarm pulse | Pulsowanie paska statusu kafelka przy PM2.5>75 | 500ms | ✅ |
| Touch feedback | Podświetlenie przycisku/kafelka przy dotknięciu | 50ms | ✅ |
| WiFi scan | Animacja "Scanning." → ".." → "..." | 300ms | ✅ |
| Fade przejścia | Płynne przejście między ekranami | 200ms | ❌ opcjonalne |

### Ikony (System Icons → Bitmapy)

Ikony zostaną przekonwertowane z systemowych ikon macOS do bitmap 16×16 lub 24×24 px:

| Ikona | Użycie | Rozmiar |
|-------|--------|---------|
| 🏠 house | Nazwa domyślna | 16×16 |
| 🌡️ thermometer | Temperatura | 16×16 |
| 💧 droplet | Wilgotność | 16×16 |
| 🌀 wind | Ciśnienie | 16×16 |
| 📶 wifi | Status WiFi | 16×16 |
| ⚙️ gear | Ustawienia | 16×16 |
| ◄ back | Powrót | 24×24 |
| ✏️ edit | Edycja | 16×16 |
| ⌫ backspace | Kasowanie | 24×24 |
| 🔄 refresh | Odśwież | 16×16 |
| ⚠️ warning | Ostrzeżenie | 16×16 |
| 🛏️ bed | Sypialnia | 16×16 |
| 🍳 cooking | Kuchnia | 16×16 |

---

## Protokół WiFi UDP

### Konfiguracja

| Parametr | Wartość |
|----------|---------|
| Port UDP | 5000 |
| Interwał | 10 sekund |
| Format | JSON |
| Broadcast | 255.255.255.255 lub subnet broadcast |

### Format JSON

```json
{
  "type": "air_quality_data",
  "receiver_mac": "AA:BB:CC:DD:EE:FF",
  "timestamp": 1735650000,
  "uptime_sec": 3600,
  "wifi_rssi": -55,
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
      "lora_rssi": -65,
      "lora_snr": 8.5,
      "last_seen_sec": 12,
      "status": 0
    },
    {
      "mac": "11:22:33:44:55:02",
      "name": "Sypialnia",
      "active": true,
      "temperature": 24.2,
      "humidity": 52.0,
      "pressure": 1013,
      "pm1_0": 5,
      "pm2_5": 8,
      "pm10": 12,
      "lora_rssi": -72,
      "lora_snr": 6.2,
      "last_seen_sec": 45,
      "status": 0
    }
  ]
}
```

### Odbieranie na Raspberry Pi Zero (Python)

```python
import socket
import json

UDP_IP = "0.0.0.0"
UDP_PORT = 5000

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))

print(f"Listening on UDP port {UDP_PORT}...")

while True:
    data, addr = sock.recvfrom(1024)
    try:
        payload = json.loads(data.decode('utf-8'))
        print(f"Received from {addr}:")
        print(json.dumps(payload, indent=2))
    except json.JSONDecodeError:
        print(f"Invalid JSON from {addr}")
```

### Odbieranie na macOS (Python Dashboard)

W repozytorium znajduje się gotowy skrypt `udp_listener.py` - interaktywny dashboard do testowania UDP broadcast na macOS:

```bash
# Uruchomienie
python3 udp_listener.py

# Lub z uprawnieniami (jeśli port wymaga)
sudo python3 udp_listener.py
```

**Funkcje dashboardu:**
- Wyświetla wszystkie 4 urządzenia w tabeli
- Kolorowane wartości PM (zielony/żółty/czerwony według progów)
- Kolumna ciśnienia (Press) w hPa
- Statystyki: liczba pakietów, uptime
- Auto-odświeżanie przy każdym pakiecie
- Tryb dashboard (czyści ekran, aktualizuje w miejscu)

**Wygląd:**
```
╔══════════════════════════════════════════════════════════════════════════════╗
║  AIR QUALITY UDP LISTENER                            Packets:      0         ║
║  Listening on port 5000                              Uptime: 00:00:00        ║
╠════╤══════════════╤═══════╤══════╤═══════╤═══════╤═══════╤═══════╤═══════════╣
║ #  │ Name         │ Temp  │ Hum  │ Press │ PM1.0 │ PM2.5 │ PM10  │ Last      ║
╠════╪══════════════╪═══════╪══════╪═══════╪═══════╪═══════╪═══════╪═══════════╣
║ *1 │ Salon        │  23.5 │  45% │  1013 │    12 │    18 │    25 │       2s  ║
║ *2 │ Sypialnia    │  21.2 │  52% │  1013 │     8 │    14 │    20 │       5s  ║
║ .3 │ --           │    -- │  --  │    -- │    -- │    -- │    -- │       --  ║
║ .4 │ --           │    -- │  --  │    -- │    -- │    -- │    -- │       --  ║
╚══════════════════════════════════════════════════════════════════════════════╝
```

**Alternatywne metody testowania na macOS:**

```bash
# Netcat - prosty odbiór (bez formatowania)
nc -u -l 5000

# Socat - z wyświetlaniem adresu źródła
socat UDP-RECV:5000 STDOUT

# TCPdump - podgląd ruchu sieciowego
sudo tcpdump -i en0 udp port 5000 -A
```

### Odbieranie na iOS (Swift) - Przyszłość

UDP można odbierać na iOS używając `Network.framework` (iOS 12+) lub `CocoaAsyncSocket`.
Wymaga bycia w tej samej sieci WiFi.

---

## Wymagane Biblioteki Arduino

```cpp
// LoRa
#include <SPI.h>
#include <RadioLib.h>

// LCD (Arduino_GFX - lepsza kompatybilność z ESP32-S3 niż TFT_eSPI)
#include <Arduino_GFX_Library.h>

// Touch (osobna biblioteka - współdzieli HSPI z LCD)
#include <XPT2046_Touchscreen.h>

// WiFi
#include <WiFi.h>
#include <WiFiUdp.h>

// JSON
#include <ArduinoJson.h>

// NTP (czas)
#include <time.h>

// Storage (nazwy, WiFi credentials)
#include <Preferences.h>
```

---

## Architektura SPI - Rozwiązanie Konfliktu

### Problem
LCD (ST7789V) i Touch (XPT2046) współdzielą magistralę **HSPI**. Przy dotknięciu ekranu następowała utrata synchronizacji LCD (biały ekran, artefakty).

### Rozwiązanie - Dual SPI Architecture
- **HSPI** - współdzielony przez LCD i Touch (te same piny SCK, MOSI, MISO, różne CS)
- **FSPI** - dedykowany dla LoRa SX1262 (piny B2B connector)

### Kluczowa funkcja: `prepareLcdSpi()`

```cpp
void prepareLcdSpi() {
  // Upewnij się że TOUCH_CS jest HIGH (touch nieaktywny)
  digitalWrite(TOUCH_CS, HIGH);
  // Upewnij się że LCD_CS jest HIGH przed rozpoczęciem
  digitalWrite(LCD_CS, HIGH);
  // Krótkie opóźnienie dla ustabilizowania magistrali
  delayMicroseconds(50);
}
```

Ta funkcja jest wywoływana na początku każdej funkcji rysującej:
- `drawHomeScreen()` 
- `drawDetailScreen()` 
- `updateDetailChartOnly()`

### Flagi synchronizacji SPI

```cpp
volatile bool lcdDrawing = false;   // true gdy LCD rysuje
volatile bool touchActive = false;  // true gdy touch komunikuje
bool loraDataUpdated = false;       // nowe dane z LoRa czekają
uint32_t lastTouchEndTime = 0;      // czas zakończenia touch

// W loop() - czekamy na stabilizację SPI:
bool spiReady = !lcdDrawing && !touchActive && (millis() - lastTouchEndTime >= 20);
```

### Co NIE zadziałało (dla przyszłej referencji)
1. Software SPI dla touch - psuło piny LCD (zmieniał pinMode)
2. `bus->beginWrite()`/`bus->endWrite()` - powodowało biały ekran
3. Samo opóźnienie 5-20ms po touch - niewystarczające
4. `touchSPI.endTransaction()` - nie pomagało

### Inicjalizacja SPI

```cpp
// LCD używa Arduino_GFX z własnym zarządzaniem SPI (HSPI)
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO,
  HSPI,    // SPI bus
  true     // is_shared_interface = TRUE (dzielimy z Touch)
);

// Touch używa tego samego HSPI
SPIClass touchSPI(HSPI);
XPT2046_Touchscreen ts(TOUCH_CS);
// Inicjalizacja: touchSPI.begin(LCD_SCLK, LCD_MISO, LCD_MOSI, TOUCH_CS);

// LoRa używa OSOBNYCH pinów na B2B - FSPI
SPIClass loraSPI(FSPI);
// Inicjalizacja: loraSPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
```

---

## Non-blocking WiFi/NTP (v2.4)

### Problem
Wcześniej urządzenie blokowało się na 10-15 sekund podczas startu, czekając na połączenie WiFi i synchronizację NTP.

### Rozwiązanie - Non-blocking startup

**1. setupWiFi() - tylko rozpoczyna połączenie:**
```cpp
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  
  // Wczytaj credentials z NVS
  Preferences prefs;
  prefs.begin("wifi", true);
  String savedSSID = prefs.getString("ssid", "");
  String savedPass = prefs.getString("pass", "");
  prefs.end();
  
  if (savedSSID.length() > 0) {
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    // NIE CZEKAJ - połączenie w tle
  }
}
```

**2. checkWiFiConnection() - wywoływana w loop():**
```cpp
void checkWiFiConnection() {
  static bool wasConnected = false;
  
  if (WiFi.status() == WL_CONNECTED) {
    if (!wasConnected) {
      wasConnected = true;
      wifiConnected = true;
      wifiRSSI = WiFi.RSSI();
      setupNTP();  // Rozpocznij sync NTP
    }
  } else {
    wasConnected = false;
    wifiConnected = false;
  }
}
```

**3. setupNTP() - tylko konfiguruje:**
```cpp
void setupNTP() {
  configTzTime(NTP_TIMEZONE, NTP_SERVER);
  // NIE CZEKAJ na sync - sprawdzamy w updateTimeString()
}
```

**4. updateTimeString() - auto-wykrywa sync:**
```cpp
void updateTimeString() {
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0) && timeinfo.tm_year > 124) {
    // tm_year > 124 oznacza rok > 2024 (czyli NTP zsynchronizował)
    if (!ntpSynced) ntpSynced = true;
    sprintf(currentTimeStr, "%02d:%02d:%02d", 
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  } else {
    strcpy(currentTimeStr, "--:--:--");
  }
}
```

### Przepływ w loop()

```cpp
void loop() {
  // 1. Sprawdź WiFi (non-blocking)
  checkWiFiConnection();
  
  // 2. Odbierz pakiety LoRa
  receivePacket();
  
  // 3. Obsłuż dotyk
  handleTouch();
  
  // 4. Odśwież czas co 1s (tylko na Home Screen)
  if (currentScreen == SCREEN_HOME && millis() - lastTimeUpdate > 1000) {
    updateTimeString();
    updateHeaderTime();  // Rysuje tylko czas, nie cały ekran
    lastTimeUpdate = millis();
  }
  
  // 5. Przerysuj ekran jeśli potrzeba
  if (screenNeedsRedraw) {
    drawCurrentScreen();
    screenNeedsRedraw = false;
  }
}
```

### Rezultat
- Urządzenie startuje natychmiast i jest responsywne
- WiFi łączy się w tle (2-5 sekund)
- NTP synchronizuje się automatycznie po połączeniu WiFi
- Czas w headerze zmienia kolor z szarego na biały po synchronizacji

---

## Struktura Kodu (v2.6 - Modularna)

Kod podzielony na **9 modułów** dla lepszej organizacji i czytelności:

```
air_condition_receiver/
├── air_condition_receiver.ino   # setup() + loop() (~150 linii)
├── config.h                     # Piny, stałe, kolory, ACTION_* (~140 linii)
├── types.h                      # Struktury, enum, extern, deklaracje funkcji (~300 linii)
├── icons.h                      # Bitmapy ikon 16×16 PROGMEM
├── globals.cpp                  # Definicje zmiennych globalnych + kbRows*
├── hardware.cpp                 # setupLCD, setupTouch, setupLoRa, WiFi, NTP
├── deviceManager.cpp            # Zarządzanie urządzeniami + receivePacket
├── touchHandler.cpp             # Obsługa dotyku + nawigacja
└── uiScreens.cpp                # Wszystkie ekrany UI
```

### Opis modułów

| Moduł | Zawartość | Kluczowe funkcje |
|-------|-----------|------------------|
| `config.h` | Stałe konfiguracyjne | Piny, kolory RGB565, ACTION_*, limity |
| `types.h` | Typy danych | SensorPacket, DeviceData, Screen enum, extern |
| `icons.h` | Bitmapy | ICON_THERMOMETER, ICON_HUMIDITY, ICON_WIFI_*, etc. |
| `globals.cpp` | Zmienne globalne | devices[], wifiSSID, kbRowsLower/Upper |
| `hardware.cpp` | Inicjalizacja HW + WiFi | setupLCD(), setupWiFi(), sendUdpBroadcast(), saveDeviceName() |
| `deviceManager.cpp` | Logika urządzeń | findOrAddDevice(), updateDeviceData(), receivePacket() |
| `touchHandler.cpp` | Touch handling | handleTouch(), getTouchActionKeyboard() |
| `uiScreens.cpp` | Ekrany UI | drawHomeScreen(), drawKeyboardScreen(), updateHeaderWifiIcon() |

### Kluczowe funkcje (v2.6)

| Funkcja | Plik | Opis |
|---------|------|------|
| `prepareLcdSpi()` | hardware.cpp | **KLUCZOWA** - synchronizacja SPI przed rysowaniem |
| `setupWiFi()` | hardware.cpp | Non-blocking start WiFi |
| `checkWiFiConnection()` | hardware.cpp | Sprawdzanie statusu w loop() |
| `updateTimeString()` | hardware.cpp | Auto-wykrywanie NTP sync |
| `sendUdpBroadcast()` | hardware.cpp | UDP broadcast JSON co 10s |
| `updateHeaderTime()` | uiScreens.cpp | Odświeża tylko czas w headerze |
| `updateHeaderWifiIcon()` | uiScreens.cpp | Odświeża ikonę WiFi w headerze |
| `getTouchActionKeyboard()` | touchHandler.cpp | Obsługa klawiatury QWERTY |
| `openKeyboard()` | hardware.cpp | Otwiera klawiaturę z kontekstem |
| `openKeyboardForDeviceName()` | hardware.cpp | Otwiera klawiaturę dla nazwy urządzenia |
| `saveDeviceName()` | hardware.cpp | Zapisuje nazwę urządzenia do NVS |
| `loadDeviceName()` | hardware.cpp | Ładuje nazwę urządzenia z NVS | |

---

## Fazy Implementacji

### Faza 1: Hardware + LCD Basic ✅ UKOŃCZONE
- [x] Podłączenie LCD do XIAO (10 przewodów)
- [x] Konfiguracja Arduino_GFX (zamiast TFT_eSPI)
- [x] Test wyświetlania (prostokąty, tekst, kolory)
- [x] Test dotyku XPT2046
- [x] Kalibracja dotyku

### Faza 2: Integracja LoRa + Multi-Device ✅ UKOŃCZONE
- [x] Połączenie z istniejącym kodem odbiornika
- [x] Struktura DeviceData z historią PM1.0/PM2.5/PM10
- [x] Dodawanie urządzeń po MAC
- [x] Timeout nieaktywnych (30 min)
- [x] Historia PM (24 punkty, circular buffer)

### Faza 3: UI Home Screen ✅ UKOŃCZONE
- [x] Tło i header
- [x] Kafelki 2×2
- [x] Wyświetlanie danych z devices[]
- [x] Pasek statusu PM (kolorowy gradient)
- [x] Dotykowa nawigacja

### Faza 4: UI Detail Screen ✅ UKOŃCZONE
- [x] Layout szczegółów z ikonami 16x16
- [x] Wykres historii PM z rotacją (PM1.0/PM2.5/PM10)
- [x] Przycisk Back (ikona)
- [x] Metadane (RSSI, SNR, czas)
- [x] Pending bar na wykresie

### Faza 5: Klawiatura Ekranowa ✅ UKOŃCZONE
- [x] Klawiatura QWERTY (małe/wielkie litery + cyfry/symbole)
- [x] Obsługa dotyku klawiszy
- [x] Pole tekstowe z kursorem
- [x] Shift, Space, Backspace, ESC, OK
- [x] Kontekst klawiatury (SSID/Password)

### Faza 6: WiFi + NTP ✅ UKOŃCZONE
- [x] Połączenie WiFi z NVS (Preferences)
- [x] Non-blocking startup (urządzenie startuje natychmiast)
- [x] NTP sync z polską strefą czasową
- [x] Czas w headerze z auto-odświeżaniem co 1s
- [x] Auto-wykrywanie synchronizacji NTP

### Faza 7: WiFi Config UI ✅ UKOŃCZONE
- [x] Ekran konfiguracji WiFi (Settings → Configure WiFi)
- [x] Skanowanie sieci (przycisk Scan)
- [x] Lista sieci (max 5, sortowane po RSSI)
- [x] Wybór sieci z listy (dotyk → kopiuje SSID)
- [x] Pola SSID/Password z edycją przez klawiaturę
- [x] Przyciski Connect/Disconnect
- [x] Zapis credentials do NVS

### Faza 8: Pozostałe funkcje ✅ UKOŃCZONE
- [x] Edycja nazw urządzeń (klawiatura + zapis do NVS)
- [x] UDP Broadcast JSON co 10s (port 5000)
- [x] Ikona WiFi w headerze (3 poziomy: weak/medium/strong + off)
- [x] Animacje: Startup loader, Alarm pulse, Touch feedback, WiFi scan

---

## Pliki Projektu

```
air_condition_receiver/
├── air_condition_receiver.ino   # Główny plik Arduino (setup/loop)
├── config.h                     # Konfiguracja pinów i stałych
├── types.h                      # Struktury danych i deklaracje
├── icons.h                      # Bitmapy ikon 16×16 PROGMEM
├── globals.cpp                  # Definicje zmiennych globalnych
├── hardware.cpp                 # Inicjalizacja sprzętu (LCD, Touch, LoRa, WiFi)
├── deviceManager.cpp            # Zarządzanie urządzeniami LoRa
├── touchHandler.cpp             # Obsługa dotyku i nawigacji
├── uiScreens.cpp                # Rysowanie ekranów UI
├── PROJECT_RECEIVER.md          # Ta dokumentacja
├── PROJECT_RECEIVER.pdf         # Export PDF (do aktualizacji)
└── pdf-style.css                # Style do eksportu PDF
```

---

## Historia Zmian

| Data | Wersja | Opis |
|------|--------|------|
| 2025-12-31 | 1.0 | Utworzenie dokumentacji projektu |
| 2025-12-31 | 2.0 | Rozszerzenie o pełny design UI, schematy połączeń, protokół UDP |
| 2026-01-01 | 2.2 | Implementacja: Arduino_GFX, multi-device, historia PM1.0/PM2.5/PM10, rozwiązanie konfliktu SPI |
| 2026-01-02 | 2.4 | **Modularna struktura** (9 plików), klawiatura QWERTY, WiFi config UI, non-blocking WiFi/NTP, czas w headerze |
| 2026-01-02 | 2.6 | **UDP Broadcast JSON** (co 10s), edycja nazw urządzeń z zapisem do NVS, ikona WiFi w headerze (3 poziomy sygnału) |
| 2026-01-02 | 2.7 | **Animacje UI**: Startup loader (4 kroki), Alarm pulse (PM2.5 > 75), Touch feedback (50ms), WiFi scan animation |
| 2026-01-03 | 3.0 | **Obudowa 3D**: Projekt obudowy do druku 3D (OpenSCAD), pliki STL/OBJ/G-code dla Ender-3 Pro |

---

## Kontekst dla Kolejnej Sesji

### Status Projektu (2026-01-02, v2.7)

| Faza | Status | Uwagi |
|------|--------|-------|
| Dokumentacja | **ZAKOŃCZONA** | PROJECT_RECEIVER.md (zaktualizowana) |
| Hardware | **ZAKOŃCZONY** | LCD + Touch + LoRa działają |
| Software v2.7 | **GOTOWY** | Modularna architektura (9 plików) + Animacje |
| Faza 1-4 | ✅ UKOŃCZONE | LCD, Touch, LoRa, Multi-device, Home/Detail UI |
| Faza 5 | ✅ UKOŃCZONE | Klawiatura QWERTY |
| Faza 6 | ✅ UKOŃCZONE | WiFi + NTP (non-blocking) |
| Faza 7 | ✅ UKOŃCZONE | WiFi Config UI |
| Faza 8 | ✅ UKOŃCZONE | UDP broadcast, edycja nazw, ikona WiFi, **animacje** |
| Faza 9 (Obudowa) | ✅ UKOŃCZONE | Projekt obudowy 3D, pliki STL + G-code |

### Co Jest Gotowe (v2.7)

1. **Hardware** - Wszystko podłączone i działające:
   - XIAO ESP32-S3 + Wio-SX1262 (B2B) - LoRa 868MHz
   - Waveshare LCD 2.8" (ST7789V + XPT2046)
   - Zasilanie przez USB-C

2. **Software v2.7** - Modularna implementacja (9 plików):
   - **LCD**: Arduino_GFX library (nie TFT_eSPI!)
   - **Touch**: XPT2046_Touchscreen z nawigacją
   - **LoRa**: RadioLib SX1262, odbiór pakietów
   - **Multi-device**: Max 4 urządzenia, auto-wykrywanie po MAC
   - **Home Screen**: Kafelki 2×2 z temp, wilg, PM2.5/PM10 + czas w headerze + ikona WiFi
   - **Detail Screen**: Pełne dane + wykres historii z rotacją PM1.0/PM2.5/PM10 + edycja nazwy
   - **Settings Screen**: Statystyki WiFi/LoRa/System + przycisk "Configure WiFi"
   - **WiFi Config Screen**: Skanowanie sieci, lista, SSID/Password, Connect/Disconnect
   - **Keyboard Screen**: QWERTY z Shift/Space/Backspace/ESC/OK
   - **WiFi non-blocking**: Urządzenie startuje natychmiast, łączy się w tle
   - **NTP sync**: Polska strefa czasowa, auto-odświeżanie czasu co 1s
   - **NVS storage**: Credentials WiFi + nazwy urządzeń zapisywane trwale
   - **SPI conflict**: Rozwiązany przez `prepareLcdSpi()` i flagi synchronizacji
   - **UDP Broadcast**: JSON co 10s na port 5000 z danymi wszystkich urządzeń
   - **Ikona WiFi**: W headerze, 3 poziomy sygnału (weak/medium/strong) + off
   - **Edycja nazw urządzeń**: Klawiatura ekranowa + zapis do NVS
   - **Animacje UI**:
     - Startup loader: 4-krokowy pasek postępu (LCD → Touch → LoRa → WiFi)
     - Alarm pulse: Pulsowanie paska statusu kafelka gdy PM2.5 > 75 (500ms)
     - Touch feedback: Podświetlenie przycisków/kafelków (50ms)
     - WiFi scan: "Scanning." → ".." → "..." animacja (300ms)

3. **Architektura SPI**:
   - **HSPI**: LCD + Touch (współdzielone, synchronizowane flagami)
   - **FSPI**: LoRa SX1262 (dedykowane, piny B2B)

### Schemat Połączeń - Szybka Ściąga

```
LCD → XIAO
──────────────
VCC  → 5V
GND  → GND
DC   → D1
CS   → D0
CLK  → D4
MOSI → D5
MISO → D7
BL   → D3
RST  → D2
TP_CS→ D6
```

### Parametry Kluczowe

| Parametr | Wartość |
|----------|---------|
| Max urządzeń | 4 |
| Timeout urządzenia | 30 min (2 cykle) |
| Historia PM | 24h (1 punkt/h) |
| Orientacja LCD | Landscape (320×240) |
| WiFi connect timeout | 10s |
| NTP timezone | CET-1CEST (Polska) |

---

## Obudowa 3D (v3.0)

### Sprzęt do produkcji
| Element | Model | Uwagi |
|---------|-------|-------|
| Drukarka 3D | **Creality Ender-3 Pro** | FDM |
| Filament | **Devil Design PLA 1.75mm** | Czarny, 1kg |
| Slicer | Cura / PrusaSlicer | Do wygenerowania G-code |
| Weryfikacja | **Blender 3.3.1** | Podgląd modeli STL/OBJ |

### Wymiary obudowy

**Kształt:** Prostopadłościan (klocek) - jedna prosta bryła bez wystających elementów.

**Orientacja:** Stoi PIONOWO na krawędzi 76×35mm (jak ramka na zdjęcie), LCD w orientacji LANDSCAPE.

| Parametr | Wartość | Źródło |
|----------|---------|--------|
| Szerokość zewn. | **76 mm** | 70.2 (LCD) + 2×2.5 (ścianki) + luz |
| Wysokość zewn. | **56 mm** | 50.2 (LCD) + 2×2.5 (ścianki) + luz |
| Głębokość zewn. | **37 mm** | 32 (elektronika) + 2.5 (tył) + 2.5 (ramka) |
| Grubość ścianki | **2.5 mm** | |
| Zaokrąglenie | **2 mm** | Promień krawędzi zewnętrznych |
| Tolerancja | **+0.3 mm** | Luz na wymiary wewnętrzne |

### Otwór LCD (flush mount)
| Parametr | Wartość |
|----------|---------|
| Widoczny otwór | 59 × 45 mm |
| Półka na PCB | 71 × 51 mm |
| Uskok (głębokość szkła) | 1.5 mm |
| Ekran RÓWNO z powierzchnią ramki (flush) | |

### Otwór USB-C (prawa strona obudowy)
| Parametr | Wartość |
|----------|---------|
| Wymiary otworu | 12 × 8 mm |
| Od górnej krawędzi | 8 mm |
| Od tylnej krawędzi | 7-13 mm (środek ~10 mm) |
| Kabel wychodzi | W PRAWO, równolegle do LCD |

### Widoki obudowy

```
WIDOK Z PRZODU:              WIDOK Z PRAWEJ:              WIDOK Z GÓRY:
(patrzę na LCD)              (patrzę na prawą ściankę)    (patrzę z góry, LCD na dole)

    ←── 76mm ──→              ←─ 35mm ─→                     ←── 76mm ──→
                              TYŁ   PRZÓD                  
  ↑ ┌────────────┐          ↑ ┌──────────┐                 ┌────────────┐
  │ │┌──────────┐│          │ │[USB]     │                 │            │
  │ ││          ││        56mm│          │               ↓ │            │ ↑
56mm││   LCD    ││          │ │          │          [USB]→ │            │ 35mm
  │ ││          ││          │ │          │                 │            │ ↓
  │ │└──────────┘│          ↓ └──────────┘                 │            │
  ↓ └────────────┘                     ↑                   └────────────┘
                                      LCD                  ↑
                                                          LCD (76mm krawędź)
```

### Konstrukcja dwuczęściowa

1. **Korpus (case_body):**
   - Pudełko otwarte od przodu
   - Otwór USB-C na prawej ściance
   - Gniazda na zatrzaski snap-fit (4 szt.)
   - Zaokrąglone krawędzie

2. **Ramka przednia (case_frame):**
   - Otwór flush na LCD (uskok 1.5mm)
   - Zatrzaski snap-fit (4 szt.)
   - Zaokrąglone krawędzie

### Montaż i wykończenie
- Części łączone zatrzaskami snap-fit (bez śrub)
- Gumowe nóżki naklejane na spód (krawędź 76×35mm) - dają lekkie nachylenie

### Pliki obudowy

```
enclosure/
├── air_quality_case.scad     # Źródło parametryczne OpenSCAD
├── case_body.stl             # Korpus - do druku i Blender
├── case_body.obj             # Korpus - dla Blender
├── case_frame.stl            # Ramka przednia - do druku i Blender
├── case_frame.obj            # Ramka przednia - dla Blender
├── case_body.gcode           # G-code dla Ender-3 Pro
├── case_frame.gcode          # G-code dla Ender-3 Pro
└── README.md                 # Instrukcja druku
```

### Parametry druku (Ender-3 Pro + Devil Design PLA)
| Parametr | Wartość |
|----------|---------|
| Wysokość warstwy | 0.2 mm |
| Ścianka (perimetry) | 3 |
| Wypełnienie | 20% |
| Wzór wypełnienia | Grid |
| Temperatura dyszy | 200-210°C |
| Temperatura stołu | 60°C |
| Prędkość druku | 50 mm/s |
| Supports | Tak (dla uskoku w ramce) |
| Orientacja druku | Korpus: tyłem do stołu, Ramka: przodem do stołu |

---

### Kolejne Kroki (Opcjonalne)

1. **Fade przejścia ekranów** (opcjonalne, niska priorytet):
   - Płynne przejście między ekranami (200ms fade)

2. **Dodatkowe ulepszenia**:
   - Zapisywanie historii PM do NVS (przetrwa restart)
   - Konfiguracja progów alarmu przez UI

### Kompilacja i Upload

```bash
cd air_condition_receiver

# Kompilacja
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 .

# Upload
arduino-cli upload -p /dev/cu.usbmodem1101 --fqbn esp32:esp32:XIAO_ESP32S3 .

# Serial Monitor
arduino-cli monitor -p /dev/cu.usbmodem1101 -c baudrate=115200
```

---

**Koniec dokumentacji. Powodzenia z implementacją!**
