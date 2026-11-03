/*
 * Air Condition - System Monitorowania Jakości Powietrza
 * ======================================================
 * 
 * Hardware:
 *   - XIAO ESP32-S3 Sense + Wio-SX1262 (B2B)
 *   - SHTC3 (temp/wilg) - I2C 0x70
 *   - BMP280 (ciśnienie) - I2C 0x77
 *   - OLED SSD1306 128x64 - I2C 0x3C
 *   - PMS7003 (pył) - UART
 *   - Buzzer aktywny - GPIO1
 *   - MOSFET Q1 (5V line) - GPIO2
 *   - MOSFET Q2 (3.3V I2C) - GPIO3
 *   - Przycisk wybudzenia - GPIO4
 * 
 * Wersja: 1.4 - Adaptacyjny Deep Sleep
 *   - Stały interwał wybudzenia: 15 minut
 *   - Adaptacyjny pomiar PMS zależny od jakości powietrza:
 *     * PM<15 (dobra):     co 8 cykli = 2h
 *     * PM 15-35 (umiark): co 4 cykle = 1h
 *     * PM 35-75 (niedrowa): co 2 cykle = 30min
 *     * PM>75 (zła):       co każdy cykl = 15min
 *   - Wybudzenie TIMER: Quick (I2C) lub Full (I2C+PMS)
 *   - Wybudzenie BUTTON: Quick + restart cyklu 15min
 *   - Zmienne RTC_DATA_ATTR przeżywają deep sleep
 *   - Optymalizacja zapisu NVS (tylko przy zmianie historii)
 * 
 * Wersja: 1.3 - Deep Sleep
 *   - Deep sleep z wybudzaniem timer (60s) + przycisk GPIO4
 *   - Pauza przed snem (2 cykle ekranów = 22s)
 *   - Zapis historii i wysyłanie LoRa przed snem
 *   - Wykrywanie przyczyny wybudzenia
 * 
 * Wersja: 1.2 - Protokół LoRa binarny
 *   - Nowy protokół transmisji (23B pakiet binarny)
 *   - Identyfikacja urządzenia po MAC WiFi
 *   - CRC-16/CCITT dla integralności danych
 *   - Wysyłanie przed deep sleep
 * 
 * Wersja: 1.1.1 - Poprawki OLED i NVS
 *   - Poprawiony layout PM10 (nie nachodzi na separator)
 *   - Animowany wentylator 16x16 na ekranie głównym
 *   - Logarytmiczna skala w wykresach historii
 *   - Zapis/odczyt historii do NVS (pamięć nieulotna)
 *   - Przygotowanie do deep sleep (prepareForDeepSleep)
 * 
 * Wersja: 1.1 - Ulepszone wyświetlanie OLED
 *   - 3 ekrany (Main/Details/History) z auto-przełączaniem
 *   - Animacja wentylatora podczas rozgrzewki PMS7003
 *   - Paski postępu PM2.5/PM10 ze skalą logarytmiczną
 *   - Historia pomiarów z wykresami słupkowymi
 *   - Obsługa przycisku do przełączania ekranów
 * 
 * Data: 2025-12-31
 */

// ============================================
// FEATURE FLAGS - włącz/wyłącz moduły do testów
// ============================================
#define TEST_MODE                0  // 1=test (30s cykl), 0=produkcja (15min cykl)
#define ENABLE_POWER_MANAGEMENT  1
#define ENABLE_I2C_SCAN          1
#define ENABLE_SHTC3             1
#define ENABLE_BMP280            1
#define ENABLE_OLED              1
#define ENABLE_PMS7003           1
#define ENABLE_BUZZER            1
#define ENABLE_LORA              1
#define ENABLE_DEEP_SLEEP        1  // Deep sleep z wybudzaniem timer + button

// ============================================
// DEEP SLEEP - Konfiguracja
// ============================================
#if ENABLE_DEEP_SLEEP

#if TEST_MODE
// TRYB TESTOWY - krótkie interwały do obserwacji algorytmu
#define WAKEUP_INTERVAL_SEC      30   // 30 sekund
#define PMS_SKIP_GOOD            4    // PM<15:  co 4 cykle = 2min
#define PMS_SKIP_MODERATE        3    // PM<35:  co 3 cykle = 1.5min
#define PMS_SKIP_UNHEALTHY       2    // PM<75:  co 2 cykle = 1min
#define PMS_SKIP_BAD             1    // PM>=75: każdy cykl = 30s
#else
// TRYB PRODUKCYJNY - 15 minut interwał
#define WAKEUP_INTERVAL_SEC      900  // 15 minut
#define PMS_SKIP_GOOD            8    // PM<15:  co 8 cykli = 2h
#define PMS_SKIP_MODERATE        4    // PM<35:  co 4 cykle = 1h
#define PMS_SKIP_UNHEALTHY       2    // PM<75:  co 2 cykle = 30min
#define PMS_SKIP_BAD             1    // PM>=75: każdy cykl = 15min
#endif

// Wspólne dla obu trybów
#define WAKEUP_INTERVAL_US       (WAKEUP_INTERVAL_SEC * 1000000ULL)
#define WAKEUP_BUTTON_PIN        GPIO_NUM_4                   // Przycisk wybudzenia
#define PRE_SLEEP_DISPLAY_CYCLES 2                            // Ile cykli ekranów przed snem
#endif

// ============================================
// LORA ACK - Konfiguracja retransmisji
// ============================================
#if ENABLE_LORA
#define ACK_TIMEOUT_MS     300   // Czas oczekiwania na ACK (ms)
#define MAX_RETRIES        2     // Maksymalnie 2 powtórzenia (3 próby łącznie)
#endif

// ============================================
// INCLUDES
// ============================================
#include <SPI.h>
#include <Wire.h>
#include <math.h>  // dla log10() w logScale()
#include <Preferences.h>  // NVS dla zachowania historii
#include <esp_mac.h>  // dla ESP32 MAC address
#include <esp_sleep.h> // dla konfiguracji power domains
#include <driver/rtc_io.h> // dla rtc_gpio_pullup_en

#if ENABLE_LORA
#include <RadioLib.h>
#endif

#if ENABLE_SHTC3
#include "Adafruit_SHTC3.h"
#endif

#if ENABLE_BMP280
#include <Adafruit_BMP280.h>
#endif

#if ENABLE_OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#endif

// ============================================
// DEFINICJE PINÓW
// ============================================

// Power Management (MOSFETy N-channel, LOW-SIDE switching)
#define PWR_5V_EN_PIN   2   // D1 - Q1 (Pololu + PMS7003) - HIGH=ON
#define PWR_3V3_EN_PIN  3   // D2 - Q2 (I2C + OLED) - HIGH=ON
#define BUZZER_PIN      1   // D0 - Buzzer aktywny
#define BUTTON_PIN      4   // D3 - Przycisk wybudzenia

// I2C
#define I2C_SDA         5   // D4
#define I2C_SCL         6   // D5

// UART dla PMS7003
#define PMS_RX         44   // D7 - odbiera z TX czujnika
#define PMS_TX         43   // D6 - wysyła do RX czujnika

// LoRa SX1262 (B2B connector)
#define LORA_NSS       41   // CS
#define LORA_SCK        7   // SCK
#define LORA_MOSI       9   // MOSI
#define LORA_MISO       8   // MISO
#define LORA_RST       42   // RESET
#define LORA_BUSY      40   // BUSY
#define LORA_DIO1      39   // IRQ
#define LORA_ANTSW     38   // Antenna Switch

// ============================================
// STAŁE KONFIGURACYJNE
// ============================================

// I2C Adresy
#define SHTC3_ADDR     0x70
#define BMP280_ADDR    0x77
#define OLED_ADDR      0x3C

// OLED
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64

// Progi PM2.5 (ug/m3)
#define PM25_GOOD           15
#define PM25_MODERATE       35
#define PM25_UNHEALTHY      75

// PMS7003
#define PMS_WARMUP_TIME     30000  // 30 sekund rozgrzewki

// ============================================
// PROTOKÓŁ LORA - Struktura pakietu v1.0
// ============================================
#if ENABLE_LORA

// Struktura pakietu (23 bajty)
struct __attribute__((packed)) SensorPacket {
  uint8_t mac[6];        // 0-5:   MAC WiFi ESP32 (unikalny ID)
  uint8_t msgType;       // 6:     Typ wiadomości
  uint8_t seqNum;        // 7:     Numer sekwencji (0-255)
  int16_t temperature;   // 8-9:   Temperatura × 100 (np. 2650 = 26.50°C)
  uint16_t humidity;     // 10-11: Wilgotność × 100 (np. 4520 = 45.20%)
  uint16_t pressure;     // 12-13: Ciśnienie hPa
  uint16_t pm1_0;        // 14-15: PM1.0 µg/m³
  uint16_t pm2_5;        // 16-17: PM2.5 µg/m³
  uint16_t pm10;         // 18-19: PM10 µg/m³
  uint8_t status;        // 20:    Flagi statusu
  uint16_t crc;          // 21-22: CRC-16/CCITT
};

// Typy wiadomości
#define MSG_TYPE_DATA    0x01  // Dane sensorów
#define MSG_TYPE_STATUS  0x02  // Tylko status (heartbeat)
#define MSG_TYPE_ACK     0x03  // Potwierdzenie odbioru
#define MSG_TYPE_ERROR   0xFF  // Błąd

// Struktura pakietu ACK (8 bajtów) - wysyłana przez odbiornik
struct __attribute__((packed)) AckPacket {
  uint8_t mac[6];        // MAC urządzenia którego pakiet potwierdzamy
  uint8_t msgType;       // MSG_TYPE_ACK
  uint8_t ackedSeqNum;   // Numer sekwencji potwierdzanego pakietu
};

// Flagi statusu (byte 20)
#define STATUS_BAT_LOW   (1 << 0)  // Niski poziom baterii
#define STATUS_PMS_ERR   (1 << 1)  // Błąd PMS7003
#define STATUS_I2C_ERR   (1 << 2)  // Błąd I2C (SHTC3/BMP280/OLED)
#define STATUS_LORA_ERR  (1 << 3)  // Błąd LoRa TX

#endif // ENABLE_LORA

// ============================================
// OLED - Konfiguracja ekranów i animacje
// ============================================
#if ENABLE_OLED

// Definicje ekranów
#define SCREEN_MAIN      0
#define SCREEN_DETAILS   1  
#define SCREEN_HISTORY   2
#define NUM_SCREENS      3

// Czasy wyświetlania ekranów (ms)
#define SCREEN_MAIN_TIME     5000
#define SCREEN_DETAILS_TIME  3000
#define SCREEN_HISTORY_TIME  3000
#define PAUSE_TIMEOUT       10000  // 10s bezczynności = wznowienie auto

// Skala PM (dla pasków postępu)
#define PM_SCALE_MAX        100

// Historia pomiarów
#define HISTORY_SIZE        24

// Animacja wentylatora 3-łopatkowego - 4 klatki 32x32 px
// Przekonwertowane z plików PNG fan_32x32_2color_frame_X.png
// Klatka 1 (frame 0) - △
static const unsigned char PROGMEM fan_frame1[] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x03, 0xc0, 0x00,
  0x00, 0x03, 0xc0, 0x00,
  0x00, 0x03, 0xc0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x0f, 0xf0, 0x00,
  0x00, 0x0f, 0xf0, 0x00,
  0x00, 0x0f, 0xf0, 0x00,
  0x00, 0x16, 0x38, 0x00,
  0x00, 0x3c, 0x1c, 0x00,
  0x00, 0xfc, 0x1f, 0x00,
  0x01, 0xfc, 0x1f, 0x80,
  0x03, 0xfe, 0x3f, 0xc0,
  0x0f, 0xff, 0xff, 0xf0,
  0x1f, 0xff, 0xff, 0xf8,
  0x0f, 0xf8, 0x1f, 0xf0,
  0x0f, 0xe0, 0x07, 0xf0,
  0x07, 0x00, 0x00, 0xe0,
  0x04, 0x00, 0x00, 0x20,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Klatka 2 (frame 1) - ▷
static const unsigned char PROGMEM fan_frame2[] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x10, 0x00, 0x00,
  0x00, 0x78, 0x00, 0x00,
  0x01, 0xf8, 0x00, 0x00,
  0x01, 0xfc, 0x00, 0x00,
  0x00, 0xfe, 0x00, 0x00,
  0x00, 0xff, 0x00, 0x00,
  0x00, 0x7f, 0x00, 0x00,
  0x00, 0x7f, 0x80, 0x00,
  0x00, 0x7f, 0xc0, 0x00,
  0x00, 0x3f, 0xd0, 0x00,
  0x00, 0x3f, 0xff, 0x80,
  0x00, 0x1e, 0x3f, 0xfc,
  0x00, 0x1c, 0x1f, 0xfc,
  0x00, 0x1c, 0x1f, 0xfc,
  0x00, 0x1c, 0x1f, 0xfc,
  0x00, 0x3e, 0x3f, 0x80,
  0x00, 0x3f, 0xf0, 0x00,
  0x00, 0x7f, 0xc0, 0x00,
  0x00, 0x7f, 0x80, 0x00,
  0x00, 0x7f, 0x00, 0x00,
  0x00, 0xff, 0x00, 0x00,
  0x00, 0xfe, 0x00, 0x00,
  0x01, 0xfc, 0x00, 0x00,
  0x01, 0xf8, 0x00, 0x00,
  0x00, 0x78, 0x00, 0x00,
  0x00, 0x10, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Klatka 3 (frame 2) - ▽
static const unsigned char PROGMEM fan_frame3[] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x04, 0x00, 0x00, 0x20,
  0x07, 0x00, 0x00, 0xe0,
  0x0f, 0xe0, 0x07, 0xf0,
  0x0f, 0xf8, 0x1f, 0xf0,
  0x1f, 0xff, 0xff, 0xf8,
  0x0f, 0xff, 0xff, 0xf0,
  0x03, 0xff, 0xff, 0xc0,
  0x01, 0xfe, 0x3f, 0x80,
  0x00, 0xfc, 0x1f, 0x00,
  0x00, 0x3c, 0x1c, 0x00,
  0x00, 0x1c, 0x18, 0x00,
  0x00, 0x06, 0x30, 0x00,
  0x00, 0x0f, 0xf0, 0x00,
  0x00, 0x0f, 0xf0, 0x00,
  0x00, 0x0f, 0xf0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x07, 0xe0, 0x00,
  0x00, 0x03, 0xc0, 0x00,
  0x00, 0x03, 0xc0, 0x00,
  0x00, 0x03, 0xc0, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Klatka 4 (frame 3) - ◁
static const unsigned char PROGMEM fan_frame4[] = {
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x1e, 0x00,
  0x00, 0x00, 0x1f, 0x80,
  0x00, 0x00, 0x3f, 0x80,
  0x00, 0x00, 0x7f, 0x00,
  0x00, 0x00, 0xff, 0x00,
  0x00, 0x00, 0xfe, 0x00,
  0x00, 0x01, 0xfe, 0x00,
  0x00, 0x03, 0xfe, 0x00,
  0x00, 0x05, 0xfc, 0x00,
  0x00, 0xff, 0xfc, 0x00,
  0x1f, 0xfe, 0x38, 0x00,
  0x1f, 0xfc, 0x18, 0x00,
  0x1f, 0xfc, 0x18, 0x00,
  0x1f, 0xfc, 0x18, 0x00,
  0x00, 0xfe, 0x3c, 0x00,
  0x00, 0x07, 0xfc, 0x00,
  0x00, 0x03, 0xfe, 0x00,
  0x00, 0x01, 0xfe, 0x00,
  0x00, 0x00, 0xfe, 0x00,
  0x00, 0x00, 0xff, 0x00,
  0x00, 0x00, 0x7f, 0x00,
  0x00, 0x00, 0x3f, 0x80,
  0x00, 0x00, 0x1f, 0x80,
  0x00, 0x00, 0x1e, 0x00,
  0x00, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00
};

// Animacja wentylatora 16x16 - 4 klatki (przeskalowane z 32x32)
// Klatka 1 - △
static const unsigned char PROGMEM fan_small_frame1[] = {
  0x00, 0x00, 0x00, 0x80, 0x01, 0x80, 0x03, 0x80, 0x01, 0xC0, 0x03, 0xC0,
  0x03, 0xC0, 0x06, 0x40, 0x0E, 0x70, 0x3F, 0x7C, 0x3E, 0xFC, 0x30, 0x18,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Klatka 2 - ▷
static const unsigned char PROGMEM fan_small_frame2[] = {
  0x00, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0F, 0x00, 0x07, 0x80,
  0x07, 0xD0, 0x06, 0x7E, 0x02, 0x3E, 0x07, 0xD0, 0x0F, 0x00, 0x0F, 0x00,
  0x0E, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Klatka 3 - ▽
static const unsigned char PROGMEM fan_small_frame3[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x38, 0x1C, 0x3E, 0xFC,
  0x3F, 0xF8, 0x0E, 0x70, 0x06, 0x20, 0x03, 0x40, 0x03, 0xC0, 0x01, 0xC0,
  0x03, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00
};

// Klatka 4 - ◁
static const unsigned char PROGMEM fan_small_frame4[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x70, 0x00, 0x70, 0x00, 0xF0, 0x01, 0xE0,
  0x0B, 0xE0, 0x3E, 0x40, 0x7E, 0x20, 0x05, 0xE0, 0x01, 0xE0, 0x00, 0xF0,
  0x00, 0x70, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00
};

// Tablica wskaźników do klatek 16x16
static const unsigned char* const fan_small_frames[] PROGMEM = {
  fan_small_frame1, fan_small_frame2, fan_small_frame3, fan_small_frame4
};

#define FAN_FRAME_COUNT 4

#endif // ENABLE_OLED

// ============================================
// OBIEKTY GLOBALNE
// ============================================

#if ENABLE_LORA
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);
#endif

#if ENABLE_SHTC3
Adafruit_SHTC3 shtc3 = Adafruit_SHTC3();
#endif

#if ENABLE_BMP280
Adafruit_BMP280 bmp;
#endif

#if ENABLE_OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Preferences prefs;  // NVS dla zachowania historii między restartami
#endif

// ============================================
// ZMIENNE GLOBALNE - Odczyty czujników
// ============================================
float temperature = 0;
float humidity = 0;
float pressure = 0;
uint16_t pm1_0 = 0;
uint16_t pm2_5 = 0;
uint16_t pm10 = 0;

// Flagi statusu
bool shtc3_ok = false;
bool bmp280_ok = false;
bool oled_ok = false;
bool pms_ok = false;
bool lora_ok = false;

// ============================================
// ZMIENNE GLOBALNE - Ekrany OLED
// ============================================
#if ENABLE_OLED
// Stan ekranów
uint8_t currentScreen = SCREEN_MAIN;
uint32_t lastScreenChange = 0;
bool screenPaused = false;
uint32_t pauseStartTime = 0;

// Historia pomiarów (circular buffer)
uint16_t pm25History[HISTORY_SIZE] = {0};
uint16_t pm10History[HISTORY_SIZE] = {0};
uint8_t historyIndex = 0;
uint8_t historyCount = 0;

// Animacja wentylatora
uint8_t fanAnimFrame = 0;
uint32_t lastFanAnim = 0;
#define FAN_ANIM_INTERVAL 250  // ms między klatkami animacji
#endif // ENABLE_OLED

// ============================================
// ZMIENNE GLOBALNE - LoRa
// ============================================
#if ENABLE_LORA
uint8_t deviceMAC[6] = {0};          // MAC WiFi jako unikalny ID
RTC_DATA_ATTR uint8_t loraSeqNum = 0; // Numer sekwencji pakietów (przeżywa deep sleep)
#endif // ENABLE_LORA

// ============================================
// ZMIENNE RTC_DATA_ATTR - Przeżywają deep sleep
// ============================================
#if ENABLE_DEEP_SLEEP
RTC_DATA_ATTR uint16_t lastPM1_0 = 0;       // Ostatni pomiar PM1.0
RTC_DATA_ATTR uint16_t lastPM2_5 = 0;       // Ostatni pomiar PM2.5
RTC_DATA_ATTR uint16_t lastPM10 = 0;        // Ostatni pomiar PM10
RTC_DATA_ATTR uint8_t pmsSkipCounter = 0;   // Licznik cykli do następnego pomiaru PMS
RTC_DATA_ATTR bool historyChanged = false;  // Flaga zmiany historii (dla optymalizacji NVS)
#endif // ENABLE_DEEP_SLEEP

// ============================================
// POWER MANAGEMENT
// ============================================
#if ENABLE_POWER_MANAGEMENT

void setupPowerControl() {
  pinMode(PWR_5V_EN_PIN, OUTPUT);
  pinMode(PWR_3V3_EN_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Domyślnie wyłączone
  digitalWrite(PWR_5V_EN_PIN, LOW);
  digitalWrite(PWR_3V3_EN_PIN, LOW);
  
  Serial.println("[PWR] Power control initialized");
}

void enable5V() {
  digitalWrite(PWR_5V_EN_PIN, HIGH);
  delay(100);  // stabilizacja
  Serial.println("[PWR] 5V line ENABLED (Q1 ON)");
}

void disable5V() {
  digitalWrite(PWR_5V_EN_PIN, LOW);
  Serial.println("[PWR] 5V line DISABLED (Q1 OFF)");
}

void enable3V3() {
  digitalWrite(PWR_3V3_EN_PIN, HIGH);
  delay(50);  // stabilizacja
  Serial.println("[PWR] 3.3V I2C line ENABLED (Q2 ON)");
}

void disable3V3() {
  digitalWrite(PWR_3V3_EN_PIN, LOW);
  Serial.println("[PWR] 3.3V I2C line DISABLED (Q2 OFF)");
}

bool isButtonPressed() {
  return digitalRead(BUTTON_PIN) == LOW;
}

#endif // ENABLE_POWER_MANAGEMENT

// ============================================
// ADAPTACYJNY POMIAR PMS - Funkcje pomocnicze
// ============================================
#if ENABLE_DEEP_SLEEP

// Zwraca co ile cykli 15min należy uruchomić PMS (zależnie od jakości powietrza)
uint8_t getPmsSkipCount(uint16_t pm25) {
  if (pm25 < PM25_GOOD)       return PMS_SKIP_GOOD;       // 8 cykli = 2h
  if (pm25 < PM25_MODERATE)   return PMS_SKIP_MODERATE;   // 4 cykle = 1h
  if (pm25 < PM25_UNHEALTHY)  return PMS_SKIP_UNHEALTHY;  // 2 cykle = 30min
  return PMS_SKIP_BAD;                                     // 1 cykl = 15min
}

// Sprawdza czy czas na pomiar PMS (dekrementuje licznik)
bool shouldMeasurePMS() {
  // Przy pierwszym uruchomieniu (brak danych PM) - zawsze mierz
  if (lastPM2_5 == 0 && lastPM10 == 0) {
    Serial.println("[PMS] First boot - forcing PMS measurement");
    return true;
  }
  
  // Dekrementuj licznik
  if (pmsSkipCounter > 0) {
    pmsSkipCounter--;
    Serial.printf("[PMS] Skip counter: %d cycles remaining\n", pmsSkipCounter);
  }
  
  // Jeśli licznik = 0, czas na pomiar PMS
  if (pmsSkipCounter == 0) {
    Serial.println("[PMS] Counter reached 0 - time for PMS measurement");
    return true;
  }
  
  Serial.println("[PMS] Skipping PMS measurement this cycle");
  return false;
}

// Zwraca efektywny interwał PMS w minutach (dla logów)
uint16_t getEffectivePmsIntervalMinutes(uint16_t pm25) {
  return getPmsSkipCount(pm25) * (WAKEUP_INTERVAL_SEC / 60);
}

#endif // ENABLE_DEEP_SLEEP

// ============================================
// BUZZER
// ============================================
#if ENABLE_BUZZER

void setupBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  Serial.println("[BUZ] Buzzer initialized");
}

void buzzerBeep(int times, int onMs = 100, int offMs = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(onMs);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < times - 1) delay(offMs);
  }
}

void buzzerAlarm() {
  // 3 długie piknięcia - alarm PM2.5
  buzzerBeep(3, 300, 200);
}

void buzzerOK() {
  // 2 krótkie piknięcia - potwierdzenie
  buzzerBeep(2, 50, 50);
}

#endif // ENABLE_BUZZER

// ============================================
// I2C SCAN
// ============================================
#if ENABLE_I2C_SCAN

void scanI2C() {
  Serial.println("[I2C] Scanning...");
  
  int found = 0;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("[I2C] Found device at 0x");
      if (addr < 16) Serial.print("0");
      Serial.print(addr, HEX);
      
      // Identyfikacja znanego urządzenia
      if (addr == SHTC3_ADDR) Serial.print(" (SHTC3)");
      else if (addr == BMP280_ADDR) Serial.print(" (BMP280)");
      else if (addr == 0x76) Serial.print(" (BMP280 alt)");
      else if (addr == OLED_ADDR) Serial.print(" (OLED SSD1306)");
      
      Serial.println();
      found++;
    }
  }
  
  Serial.print("[I2C] Scan complete. Found ");
  Serial.print(found);
  Serial.println(" device(s)");
}

#endif // ENABLE_I2C_SCAN

// ============================================
// SHTC3 - Czujnik temperatury i wilgotności
// ============================================
#if ENABLE_SHTC3

bool setupSHTC3() {
  Serial.print("[SHTC3] Initializing... ");
  
  if (shtc3.begin()) {
    Serial.println("OK");
    shtc3_ok = true;
    return true;
  } else {
    Serial.println("FAILED!");
    shtc3_ok = false;
    return false;
  }
}

bool readSHTC3() {
  if (!shtc3_ok) return false;
  
  sensors_event_t hum_event, temp_event;
  shtc3.getEvent(&hum_event, &temp_event);
  
  temperature = temp_event.temperature;
  humidity = hum_event.relative_humidity;
  
  Serial.print("[SHTC3] Temp: ");
  Serial.print(temperature, 1);
  Serial.print(" C, Hum: ");
  Serial.print(humidity, 1);
  Serial.println(" %");
  
  return true;
}

#endif // ENABLE_SHTC3

// ============================================
// BMP280 - Czujnik ciśnienia
// ============================================
#if ENABLE_BMP280

bool setupBMP280() {
  Serial.print("[BMP280] Initializing... ");
  
  // Próba na adresie 0x77, potem 0x76
  if (bmp.begin(BMP280_ADDR)) {
    Serial.println("OK (0x77)");
    bmp280_ok = true;
    return true;
  } else if (bmp.begin(0x76)) {
    Serial.println("OK (0x76)");
    bmp280_ok = true;
    return true;
  } else {
    Serial.println("FAILED!");
    bmp280_ok = false;
    return false;
  }
}

bool readBMP280() {
  if (!bmp280_ok) return false;
  
  pressure = bmp.readPressure() / 100.0F;  // hPa
  float bmp_temp = bmp.readTemperature();
  
  Serial.print("[BMP280] Pressure: ");
  Serial.print(pressure, 1);
  Serial.print(" hPa, Temp: ");
  Serial.print(bmp_temp, 1);
  Serial.println(" C");
  
  return true;
}

#endif // ENABLE_BMP280

// ============================================
// OLED SSD1306 - Ulepszone wyświetlanie
// ============================================
#if ENABLE_OLED

bool setupOLED() {
  Serial.print("[OLED] Initializing... ");
  
  if (display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OK");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Air Condition");
    display.println("System Ready");
    display.display();
    oled_ok = true;
    lastScreenChange = millis();
    return true;
  } else {
    Serial.println("FAILED!");
    oled_ok = false;
    return false;
  }
}

// --------------------------------------------
// Funkcje pomocnicze
// --------------------------------------------

// Skalowanie logarytmiczne (dla lepszej widoczności niskich wartości)
// Wartości: 1 → ~0%, 10 → ~50%, 100 → 100%
int logScale(int value, int maxVal) {
  if (value <= 1) return 0;
  if (value >= maxVal) return 100;
  // log10(1)=0, log10(10)=1, log10(100)=2
  float logVal = log10((float)value);
  float logMax = log10((float)maxVal);
  return (int)((logVal / logMax) * 100.0f);
}

// Zwraca etykietę jakości powietrza
const char* getAirQualityLabel(uint16_t pm25) {
  if (pm25 < PM25_GOOD) return "GOOD";
  if (pm25 < PM25_MODERATE) return "MODERATE";
  if (pm25 < PM25_UNHEALTHY) return "UNHEALTHY";
  return "HAZARD";
}

// Dodaje pomiar do historii (circular buffer)
void addToHistory(uint16_t pm25, uint16_t pm10_val) {
  pm25History[historyIndex] = pm25;
  pm10History[historyIndex] = pm10_val;
  
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) {
    historyCount++;
  }
  
  // Ustaw flagę zmiany historii (dla optymalizacji zapisu NVS)
  #if ENABLE_DEEP_SLEEP
  historyChanged = true;
  #endif
  
  Serial.print("[OLED] History updated. Count: ");
  Serial.println(historyCount);
}

// --------------------------------------------
// NVS - Zapis/odczyt historii do pamięci nieulotnej
// --------------------------------------------

// Zapis historii do NVS (wywoływany przed deep sleep)
void saveHistoryToNVS() {
  prefs.begin("airquality", false);  // read-write mode
  prefs.putBytes("pm25hist", pm25History, sizeof(pm25History));
  prefs.putBytes("pm10hist", pm10History, sizeof(pm10History));
  prefs.putUChar("histIdx", historyIndex);
  prefs.putUChar("histCnt", historyCount);
  prefs.end();
  Serial.println("[NVS] History saved");
}

// Odczyt historii z NVS (wywoływany przy starcie)
void loadHistoryFromNVS() {
  prefs.begin("airquality", true);  // read-only mode
  size_t len25 = prefs.getBytes("pm25hist", pm25History, sizeof(pm25History));
  size_t len10 = prefs.getBytes("pm10hist", pm10History, sizeof(pm10History));
  historyIndex = prefs.getUChar("histIdx", 0);
  historyCount = prefs.getUChar("histCnt", 0);
  prefs.end();
  
  if (len25 > 0 && len10 > 0 && historyCount > 0) {
    Serial.print("[NVS] History loaded. Count: ");
    Serial.println(historyCount);
  } else {
    Serial.println("[NVS] No saved history found");
    // Reset na wypadek uszkodzonych danych
    historyIndex = 0;
    historyCount = 0;
  }
}

// --------------------------------------------
// Pauza wyświetlania przed sleep (2 cykle ekranów = 22s)
// --------------------------------------------
#if ENABLE_OLED
void displayPauseLoop() {
  Serial.printf("[OLED] Displaying results for %d screen cycles before sleep...\n", PRE_SLEEP_DISPLAY_CYCLES);
  
  // Oblicz czas pauzy: 2 cykle × (5s MAIN + 3s DETAILS + 3s HISTORY) = 22s
  uint32_t totalDisplayTime = PRE_SLEEP_DISPLAY_CYCLES * 
    (SCREEN_MAIN_TIME + SCREEN_DETAILS_TIME + SCREEN_HISTORY_TIME);
  uint32_t displayStart = millis();
  
  while (millis() - displayStart < totalDisplayTime) {
    updateScreenAuto();
    handleButtonOLED();
    delay(50);
  }
  
  Serial.println("[OLED] Display pause complete");
}
#endif // ENABLE_OLED

// Przygotowanie do deep sleep - zapisz dane i wyłącz peryferia
void prepareForDeepSleep(bool saveHistory) {
  Serial.println("[PWR] Preparing for deep sleep...");
  
  // 1. Zapisz historię do NVS (tylko jeśli się zmieniła)
  #if ENABLE_DEEP_SLEEP
  if (saveHistory && historyChanged) {
    saveHistoryToNVS();
    historyChanged = false;
  } else if (saveHistory) {
    Serial.println("[NVS] History unchanged - skipping save");
  } else {
    Serial.println("[NVS] Quick measurement - no history save");
  }
  #endif
  
  // 2. Wyślij dane przez LoRa przed snem (z retransmisją)
  #if ENABLE_LORA
  sendSensorPacketWithRetry();
  delay(100);  // Poczekaj na zakończenie transmisji
  #endif
  
  // 3. Wyczyść ekran OLED
  #if ENABLE_OLED
  display.clearDisplay();
  display.display();
  #endif
  
  // 4. Wyłącz zasilanie peryferiów
  #if ENABLE_POWER_MANAGEMENT
  disable5V();
  disable3V3();
  #endif
  
  #if ENABLE_DEEP_SLEEP
  // 5. Wymuś pull-up na GPIO4 (przycisk) — RTC domena przetrwa deep sleep
  rtc_gpio_pullup_en(WAKEUP_BUTTON_PIN);
  rtc_gpio_pulldown_dis(WAKEUP_BUTTON_PIN);
  
  // 6. Czekaj aż przycisk zostanie zwolniony (zapobiega natychmiastowemu wybudzeniu)
  while (digitalRead(BUTTON_PIN) == LOW) {
    delay(10);
  }
  
  // 7. Konfiguracja wakeup sources
  
  // Timer wakeup - stały interwał 15 minut
  esp_sleep_enable_timer_wakeup(WAKEUP_INTERVAL_US);
  Serial.printf("[PWR] Timer wakeup: %d sec\n", WAKEUP_INTERVAL_SEC);
  
  // Button wakeup (GPIO4, LOW = wake)
  esp_sleep_enable_ext0_wakeup(WAKEUP_BUTTON_PIN, 0);
  Serial.println("[PWR] Button wakeup: GPIO4 (LOW)");
  
  // 8. Wejdź w deep sleep
  Serial.println("[PWR] Entering deep sleep NOW...");
  Serial.flush();  // Upewnij się, że Serial jest opróżniony
  delay(10);
  
  esp_deep_sleep_start();
  // Kod poniżej nigdy się nie wykona
  
  #else
  Serial.println("[PWR] Deep sleep disabled (ENABLE_DEEP_SLEEP=0)");
  #endif
}

// Rysuje pasek postępu z logarytmiczną skalą
void drawProgressBar(int x, int y, int w, int h, int value, int maxVal) {
  // Ramka
  display.drawRect(x, y, w, h, SSD1306_WHITE);
  
  // Wypełnienie (logarytmiczna skala)
  int percent = logScale(value, maxVal);
  int fillWidth = (w - 2) * percent / 100;
  
  if (fillWidth > 0) {
    display.fillRect(x + 1, y + 1, fillWidth, h - 2, SSD1306_WHITE);
  }
}

// --------------------------------------------
// Ekran 1: GŁÓWNY - PM2.5/PM10 z paskami
// --------------------------------------------
void drawMainScreen() {
  if (!oled_ok) return;
  
  display.clearDisplay();
  
  // Ikona wentylatora w rogu (16x16) - animowana
  const unsigned char* fanFrame = (const unsigned char*)pgm_read_ptr(&fan_small_frames[fanAnimFrame]);
  display.drawBitmap(110, 0, fanFrame, 16, 16, SSD1306_WHITE);
  
  // PM2.5
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("PM2.5");
  
  display.setTextSize(2);
  display.setCursor(0, 10);
  if (pm2_5 < 1000) {
    display.print(pm2_5);
  } else {
    display.print("999+");
  }
  
  // Pasek PM2.5 (x=50, y=12, w=55, h=10)
  drawProgressBar(50, 12, 55, 10, pm2_5, PM_SCALE_MAX);
  
  // PM10
  display.setTextSize(1);
  display.setCursor(0, 26);
  display.print("PM10");
  
  display.setTextSize(2);
  display.setCursor(0, 34);
  if (pm10 < 1000) {
    display.print(pm10);
  } else {
    display.print("999+");
  }
  
  // Pasek PM10 (x=50, y=40, w=55, h=10)
  drawProgressBar(50, 40, 55, 10, pm10, PM_SCALE_MAX);
  
  // Linia separatora
  display.drawLine(0, 52, 127, 52, SSD1306_WHITE);
  
  // Status jakości powietrza (po prawej)
  display.setTextSize(1);
  display.setCursor(108, 28);
  
  // Krótka etykieta
  if (pm2_5 < PM25_GOOD) {
    display.setCursor(108, 28);
    display.print("OK");
  } else if (pm2_5 < PM25_MODERATE) {
    display.setCursor(102, 28);
    display.print("MOD");
  } else if (pm2_5 < PM25_UNHEALTHY) {
    display.setCursor(102, 28);
    display.print("BAD");
  } else {
    display.setCursor(102, 28);
    display.print("!!!");
  }
  
  // Dolna linia: T / H / P
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print(temperature, 1);
  display.print("C ");
  display.print((int)humidity);
  display.print("% ");
  display.print((int)pressure);
  display.print("hPa");
  
  display.display();
}

// --------------------------------------------
// Ekran 2: SZCZEGÓŁY - Wszystkie wartości
// --------------------------------------------
void drawDetailsScreen() {
  if (!oled_ok) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  
  // PM wartości
  display.setCursor(0, 0);
  display.print("PM1.0:  ");
  display.print(pm1_0);
  display.println(" ug/m3");
  
  display.setCursor(0, 10);
  display.print("PM2.5:  ");
  display.print(pm2_5);
  display.println(" ug/m3");
  
  display.setCursor(0, 20);
  display.print("PM10:   ");
  display.print(pm10);
  display.println(" ug/m3");
  
  // Separator
  display.drawLine(0, 29, 127, 29, SSD1306_WHITE);
  
  // Temperatura, wilgotność, ciśnienie
  display.setCursor(0, 33);
  display.print("Temp:   ");
  display.print(temperature, 1);
  display.println(" C");
  
  display.setCursor(0, 43);
  display.print("Humid:  ");
  display.print(humidity, 1);
  display.println(" %");
  
  display.setCursor(0, 53);
  display.print("Press:  ");
  display.print((int)pressure);
  display.println(" hPa");
  
  display.display();
}

// --------------------------------------------
// Ekran 3: HISTORIA - Wykresy słupkowe
// --------------------------------------------
void drawHistoryScreen() {
  if (!oled_ok) return;
  
  display.clearDisplay();
  display.setTextSize(1);
  
  // Sprawdź czy są dane
  if (historyCount == 0) {
    display.setCursor(20, 20);
    display.print("No data yet");
    display.setCursor(5, 35);
    display.print("Waiting for readings...");
    display.display();
    return;
  }
  
  // Tytuł PM2.5
  display.setCursor(0, 0);
  display.print("PM2.5 (");
  display.print(historyCount);
  display.print(")");
  
  // Wykres PM2.5 (y=9 do y=24, wysokość 16px)
  int barWidth = 5;  // 24 słupków * 5px = 120px
  int chartY1 = 9;
  int chartH = 16;
  
  // Znajdź maksymalną wartość w historii dla skalowania
  uint16_t maxPM25 = 1;
  uint16_t maxPM10 = 1;
  for (int i = 0; i < historyCount; i++) {
    if (pm25History[i] > maxPM25) maxPM25 = pm25History[i];
    if (pm10History[i] > maxPM10) maxPM10 = pm10History[i];
  }
  // Minimum skali to PM_SCALE_MAX
  if (maxPM25 < PM_SCALE_MAX) maxPM25 = PM_SCALE_MAX;
  if (maxPM10 < PM_SCALE_MAX) maxPM10 = PM_SCALE_MAX;
  
  // Rysuj słupki PM2.5
  for (int i = 0; i < historyCount; i++) {
    // Oblicz indeks (od najstarszego do najnowszego)
    int idx = (historyIndex - historyCount + i + HISTORY_SIZE) % HISTORY_SIZE;
    
    int barH = logScale(pm25History[idx], maxPM25) * chartH / 100;
    if (barH < 1 && pm25History[idx] > 0) barH = 1;
    
    int x = i * barWidth;
    display.fillRect(x, chartY1 + chartH - barH, barWidth - 1, barH, SSD1306_WHITE);
  }
  
  // Tytuł PM10
  display.setCursor(0, 27);
  display.print("PM10");
  
  // Wykres PM10 (y=36 do y=51, wysokość 16px)
  int chartY2 = 36;
  
  // Rysuj słupki PM10
  for (int i = 0; i < historyCount; i++) {
    int idx = (historyIndex - historyCount + i + HISTORY_SIZE) % HISTORY_SIZE;
    
    int barH = logScale(pm10History[idx], maxPM10) * chartH / 100;
    if (barH < 1 && pm10History[idx] > 0) barH = 1;
    
    int x = i * barWidth;
    display.fillRect(x, chartY2 + chartH - barH, barWidth - 1, barH, SSD1306_WHITE);
  }
  
  // Ostatni/Max pomiar - Y=55 (8px tekstu kończy się na Y=63)
  display.setCursor(0, 55);
  display.print("L:");
  display.print(pm2_5);
  display.print("/");
  display.print(pm10);
  display.print(" M:");
  
  // Znajdź max z historii
  uint16_t histMax25 = 0, histMax10 = 0;
  for (int i = 0; i < historyCount; i++) {
    if (pm25History[i] > histMax25) histMax25 = pm25History[i];
    if (pm10History[i] > histMax10) histMax10 = pm10History[i];
  }
  display.print(histMax25);
  display.print("/");
  display.print(histMax10);
  
  display.display();
}

// --------------------------------------------
// Ekran WARMUP - Animacja rozgrzewki
// --------------------------------------------
void drawWarmupScreen(int secondsLeft, int totalSeconds) {
  if (!oled_ok) return;
  
  display.clearDisplay();
  
  // Tytuł
  display.setTextSize(1);
  display.setCursor(10, 0);
  display.print("AIR CONDITION v1.4");
  
  // Animacja wentylatora (32x32) - pozycja: x=4, y=12
  const unsigned char* frames[] = {fan_frame1, fan_frame2, fan_frame3, fan_frame4};
  display.drawBitmap(4, 12, frames[fanAnimFrame], 32, 32, SSD1306_WHITE);
  fanAnimFrame = (fanAnimFrame + 1) % FAN_FRAME_COUNT;
  
  // Status czujników (po prawej od animacji)
  display.setTextSize(1);
  display.setCursor(44, 12);
  display.print("SHTC3: ");
  display.println(shtc3_ok ? "OK" : "--");
  
  display.setCursor(44, 22);
  display.print("BMP:   ");
  display.println(bmp280_ok ? "OK" : "--");
  
  display.setCursor(44, 32);
  display.print("LoRa:  ");
  display.println(lora_ok ? "OK" : "--");
  
  // Tekst "Warming up"
  display.setCursor(0, 48);
  display.print("Warming up PMS7003...");
  
  // Pasek postępu (x=0, y=56, w=100, h=8)
  int elapsed = totalSeconds - secondsLeft;
  int progressWidth = 100;
  int fillWidth = (progressWidth - 2) * elapsed / totalSeconds;
  
  display.drawRect(0, 56, progressWidth, 8, SSD1306_WHITE);
  if (fillWidth > 0) {
    display.fillRect(1, 57, fillWidth, 6, SSD1306_WHITE);
  }
  
  // Procent i czas
  int percent = (elapsed * 100) / totalSeconds;
  display.setCursor(104, 56);
  display.print(percent);
  display.print("%");
  
  display.setCursor(104, 48);
  display.print(secondsLeft);
  display.print("s");
  
  display.display();
}

// --------------------------------------------
// Auto-przełączanie ekranów
// --------------------------------------------
void updateScreenAuto() {
  if (!oled_ok) return;
  
  uint32_t now = millis();
  
  // Animacja wentylatora na ekranie głównym (odświeżanie co 250ms)
  if (currentScreen == SCREEN_MAIN) {
    if (now - lastFanAnim >= FAN_ANIM_INTERVAL) {
      lastFanAnim = now;
      fanAnimFrame = (fanAnimFrame + 1) % FAN_FRAME_COUNT;
      
      // Odrysuj tylko ikonę wentylatora (bez pełnego redraw)
      const unsigned char* fanFrame = (const unsigned char*)pgm_read_ptr(&fan_small_frames[fanAnimFrame]);
      display.fillRect(110, 0, 16, 16, SSD1306_BLACK);  // wyczyść obszar
      display.drawBitmap(110, 0, fanFrame, 16, 16, SSD1306_WHITE);
      display.display();
    }
  }
  
  // Sprawdź czy pauza wygasła
  if (screenPaused) {
    if (now - pauseStartTime > PAUSE_TIMEOUT) {
      screenPaused = false;
      Serial.println("[OLED] Auto-switching resumed");
    } else {
      return;  // Nadal pauza
    }
  }
  
  // Pobierz czas dla aktualnego ekranu
  uint32_t screenTime;
  switch (currentScreen) {
    case SCREEN_MAIN:    screenTime = SCREEN_MAIN_TIME; break;
    case SCREEN_DETAILS: screenTime = SCREEN_DETAILS_TIME; break;
    case SCREEN_HISTORY: screenTime = SCREEN_HISTORY_TIME; break;
    default:             screenTime = SCREEN_MAIN_TIME; break;
  }
  
  // Sprawdź czy czas minął
  if (now - lastScreenChange >= screenTime) {
    currentScreen = (currentScreen + 1) % NUM_SCREENS;
    lastScreenChange = now;
    
    Serial.print("[OLED] Switching to screen ");
    Serial.println(currentScreen);
    
    // Odrysuj nowy ekran
    switch (currentScreen) {
      case SCREEN_MAIN:    drawMainScreen(); break;
      case SCREEN_DETAILS: drawDetailsScreen(); break;
      case SCREEN_HISTORY: drawHistoryScreen(); break;
    }
  }
}

// --------------------------------------------
// Obsługa przycisku - pauza/przełączanie
// --------------------------------------------
void handleButtonOLED() {
  #if ENABLE_POWER_MANAGEMENT
  if (!oled_ok) return;
  
  if (isButtonPressed()) {
    // Włącz pauzę
    screenPaused = true;
    pauseStartTime = millis();
    
    // Przełącz na następny ekran
    currentScreen = (currentScreen + 1) % NUM_SCREENS;
    lastScreenChange = millis();
    
    Serial.print("[OLED] Button pressed. Screen: ");
    Serial.print(currentScreen);
    Serial.println(" (paused)");
    
    // Odrysuj nowy ekran
    switch (currentScreen) {
      case SCREEN_MAIN:    drawMainScreen(); break;
      case SCREEN_DETAILS: drawDetailsScreen(); break;
      case SCREEN_HISTORY: drawHistoryScreen(); break;
    }
    
    // Czekaj na puszczenie przycisku (debounce)
    while (isButtonPressed()) {
      delay(50);
    }
    delay(100);  // Dodatkowy debounce
  }
  #endif
}

#endif // ENABLE_OLED

// ============================================
// PMS7003 - Czujnik pyłu (UART)
// ============================================
#if ENABLE_PMS7003

// Struktura ramki danych PMS7003
struct PMS7003Data {
  uint16_t pm1_0_std;
  uint16_t pm2_5_std;
  uint16_t pm10_std;
  uint16_t pm1_0_atm;
  uint16_t pm2_5_atm;
  uint16_t pm10_atm;
};

bool setupPMS7003() {
  Serial.print("[PMS7003] Initializing UART... ");
  Serial1.begin(9600, SERIAL_8N1, PMS_RX, PMS_TX);
  delay(100);
  Serial.println("OK");
  pms_ok = true;
  return true;
}

bool readPMS7003() {
  if (!pms_ok) return false;
  
  // Czekaj na dane (max 2 sekundy)
  unsigned long start = millis();
  while (Serial1.available() < 32) {
    if (millis() - start > 2000) {
      Serial.println("[PMS7003] Timeout waiting for data");
      return false;
    }
    delay(10);
  }
  
  // Szukaj nagłówka ramki (0x42 0x4D)
  while (Serial1.available() >= 32) {
    if (Serial1.read() == 0x42) {
      if (Serial1.peek() == 0x4D) {
        Serial1.read();  // consume 0x4D
        
        // Odczytaj resztę ramki (30 bajtów)
        uint8_t buffer[30];
        if (Serial1.readBytes(buffer, 30) != 30) {
          Serial.println("[PMS7003] Incomplete frame");
          return false;
        }
        
        // Sprawdź sumę kontrolną
        uint16_t checksum = 0x42 + 0x4D;
        for (int i = 0; i < 28; i++) {
          checksum += buffer[i];
        }
        uint16_t received_checksum = (buffer[28] << 8) | buffer[29];
        
        if (checksum != received_checksum) {
          Serial.println("[PMS7003] Checksum error");
          return false;
        }
        
        // Parsuj dane (atmospheric environment)
        pm1_0 = (buffer[8] << 8) | buffer[9];
        pm2_5 = (buffer[10] << 8) | buffer[11];
        pm10 = (buffer[12] << 8) | buffer[13];
        
        Serial.print("[PMS7003] PM1.0: ");
        Serial.print(pm1_0);
        Serial.print(" ug/m3, PM2.5: ");
        Serial.print(pm2_5);
        Serial.print(" ug/m3, PM10: ");
        Serial.print(pm10);
        Serial.println(" ug/m3");
        
        return true;
      }
    }
  }
  
  Serial.println("[PMS7003] No valid frame found");
  return false;
}

void clearPMSBuffer() {
  while (Serial1.available()) {
    Serial1.read();
  }
}

#endif // ENABLE_PMS7003

// ============================================
// LORA SX1262
// ============================================
#if ENABLE_LORA

bool setupLoRa() {
  Serial.print("[LORA] Initializing SX1262... ");
  
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  
  int state = radio.begin(
    868.0,    // freq MHz
    125.0,    // BW
    7,        // SF
    5,        // CR
    0x34,     // sync word
    14,       // power
    8         // preamble
  );
  
  if (state != RADIOLIB_ERR_NONE) {
    Serial.print("FAILED! Error: ");
    Serial.println(state);
    lora_ok = false;
    return false;
  }
  
  radio.setDio2AsRfSwitch(true);
  Serial.println("OK");
  lora_ok = true;
  return true;
}

// Pobiera MAC adres WiFi jako unikalny identyfikator urządzenia
void getDeviceMAC(uint8_t* mac) {
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
}

// Wyświetla MAC na Serial
void printDeviceMAC() {
  Serial.print("[SYS] Device MAC: ");
  for (int i = 0; i < 6; i++) {
    if (deviceMAC[i] < 0x10) Serial.print("0");
    Serial.print(deviceMAC[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}

// CRC-16/CCITT
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

// Buduje byte statusu na podstawie flag globalnych
uint8_t buildStatusByte() {
  uint8_t status = 0;
  if (!pms_ok) status |= STATUS_PMS_ERR;
  if (!shtc3_ok || !bmp280_ok || !oled_ok) status |= STATUS_I2C_ERR;
  if (!lora_ok) status |= STATUS_LORA_ERR;
  // TODO: STATUS_BAT_LOW - dodać gdy będzie pomiar baterii
  return status;
}

// Oczekuje na pakiet ACK z odpowiednim numerem sekwencji
// Zwraca true jeśli ACK otrzymany, false jeśli timeout
bool waitForAck(uint8_t expectedSeqNum) {
  uint8_t buffer[16];
  uint32_t startTime = millis();
  
  // Pętla oczekiwania na ACK z blokującym receive()
  while (millis() - startTime < ACK_TIMEOUT_MS) {
    // receive() blokuje do odbioru pakietu lub wewnętrznego timeout
    // Dla krótkich pakietów ACK timeout będzie krótki
    int state = radio.receive(buffer, sizeof(AckPacket));
    
    if (state == RADIOLIB_ERR_NONE) {
      AckPacket* ack = (AckPacket*)buffer;
      
      // Sprawdź czy to ACK dla nas
      if (ack->msgType == MSG_TYPE_ACK &&
          memcmp(ack->mac, deviceMAC, 6) == 0 &&
          ack->ackedSeqNum == expectedSeqNum) {
        Serial.print("[LORA] ACK received for seq=");
        Serial.println(expectedSeqNum);
        return true;
      } else {
        Serial.println("[LORA] Received packet but not matching ACK");
      }
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
      // Wewnętrzny timeout - kontynuuj oczekiwanie jeśli nie minął ACK_TIMEOUT_MS
      continue;
    } else {
      // Inny błąd - wypisz i zakończ
      Serial.print("[LORA] RX error: ");
      Serial.println(state);
      break;
    }
  }
  
  Serial.println("[LORA] ACK timeout");
  return false;
}

// Buduje i wysyła pakiet z danymi sensorów z retransmisją
// Zwraca true jeśli ACK otrzymany, false jeśli wszystkie próby zawiodły
bool sendSensorPacketWithRetry() {
  if (!lora_ok) {
    Serial.println("[LORA] TX skipped - LoRa not initialized");
    return false;
  }
  
  SensorPacket pkt;
  
  // MAC urządzenia
  memcpy(pkt.mac, deviceMAC, 6);
  
  // Nagłówek
  pkt.msgType = MSG_TYPE_DATA;
  pkt.seqNum = loraSeqNum++;  // Inkrementuj PRZED wysyłaniem
  
  // Dane sensorów (skalowane do int)
  pkt.temperature = (int16_t)(temperature * 100);
  pkt.humidity = (uint16_t)(humidity * 100);
  pkt.pressure = (uint16_t)pressure;
  pkt.pm1_0 = pm1_0;
  pkt.pm2_5 = pm2_5;
  pkt.pm10 = pm10;
  
  // Status
  pkt.status = buildStatusByte();
  
  // CRC (oblicz bez pola CRC)
  pkt.crc = calculateCRC16((uint8_t*)&pkt, sizeof(pkt) - 2);
  
  // Pętla retransmisji
  for (int attempt = 0; attempt <= MAX_RETRIES; attempt++) {
    if (attempt > 0) {
      Serial.print("[LORA] Retry ");
      Serial.print(attempt);
      Serial.print("/");
      Serial.println(MAX_RETRIES);
    }
    
    // Wyślij pakiet
    int state = radio.transmit((uint8_t*)&pkt, sizeof(pkt));
    
    if (state == RADIOLIB_ERR_NONE) {
      Serial.print("[LORA] TX seq=");
      Serial.print(pkt.seqNum);
      Serial.print(", size=");
      Serial.print(sizeof(pkt));
      Serial.println("B - waiting for ACK...");
      
      // Czekaj na ACK
      if (waitForAck(pkt.seqNum)) {
        Serial.println("[LORA] Transmission confirmed!");
        return true;
      }
      // Timeout - kontynuuj pętlę (retry)
      
    } else {
      Serial.print("[LORA] TX FAILED! Error: ");
      Serial.println(state);
      // Błąd TX - spróbuj ponownie
    }
  }
  
  // Wszystkie próby zawiodły
  Serial.println("[LORA] All retries failed - giving up");
  return false;
}

#endif // ENABLE_LORA

// ============================================
// GŁÓWNE FUNKCJE
// ============================================

void initializeAllSensors() {
  Serial.println("\n=== Initializing sensors ===");
  
  #if ENABLE_SHTC3
  setupSHTC3();
  #endif
  
  #if ENABLE_BMP280
  setupBMP280();
  #endif
  
  #if ENABLE_OLED
  setupOLED();
  loadHistoryFromNVS();  // Odczytaj historię z pamięci nieulotnej
  #endif
  
  Serial.println("=== Sensors initialized ===\n");
}

void readAllSensors() {
  Serial.println("\n--- Reading sensors ---");
  
  #if ENABLE_SHTC3
  readSHTC3();
  #endif
  
  #if ENABLE_BMP280
  readBMP280();
  #endif
  
  #if ENABLE_PMS7003
  readPMS7003();
  #endif
  
  Serial.println("--- Sensors read complete ---\n");
}

void performMeasurementCycle() {
  Serial.println("\n========== MEASUREMENT CYCLE (LEGACY) ==========");
  // Ta funkcja jest zachowana dla kompatybilności wstecznej
  // W v1.4 używamy performQuickMeasurement() i performFullMeasurement()
  performFullMeasurement();
}

// ============================================
// NOWE FUNKCJE POMIAROWE v1.4
// ============================================

// Odczyt tylko I2C (bez PMS) - wywoływana gdy skipujemy pomiar PMS
void readI2CSensors() {
  Serial.println("\n--- Reading I2C sensors only ---");
  
  #if ENABLE_SHTC3
  readSHTC3();
  #endif
  
  #if ENABLE_BMP280
  readBMP280();
  #endif
  
  Serial.println("--- I2C read complete ---\n");
}

// --------------------------------------------
// QUICK MEASUREMENT - tylko I2C + ostatni PM
// Wywoływane przy: TIMER (gdy skip PMS) lub BUTTON
// --------------------------------------------
void performQuickMeasurement() {
  Serial.println("\n========== QUICK MEASUREMENT (I2C only) ==========");
  
  // 1. Włącz tylko 3.3V (I2C + OLED) - NIE włączamy 5V (PMS)!
  #if ENABLE_POWER_MANAGEMENT
  enable3V3();
  // NIE wywołujemy enable5V() - oszczędzamy energię
  #endif
  
  // 2. Inicjalizacja I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  #if ENABLE_I2C_SCAN
  scanI2C();
  #endif
  
  // 3. Inicjalizacja czujników I2C (bez PMS)
  initializeAllSensors();
  
  // 4. Odczyt tylko I2C (świeże temp/hum/pressure)
  readI2CSensors();
  
  // 5. PM wartości = ostatnie zapisane w RTC
  #if ENABLE_DEEP_SLEEP
  pm1_0 = lastPM1_0;
  pm2_5 = lastPM2_5;
  pm10 = lastPM10;
  Serial.printf("[PM] Using last RTC values: PM1.0=%d, PM2.5=%d, PM10=%d\n", pm1_0, pm2_5, pm10);
  #endif
  
  // 6. Wyświetl na OLED
  #if ENABLE_OLED
  drawMainScreen();
  lastScreenChange = millis();
  currentScreen = SCREEN_MAIN;
  
  // 7. Pauza wyświetlania (2 cykle ekranów)
  displayPauseLoop();
  #endif
  
  Serial.println("========== QUICK CYCLE COMPLETE ==========\n");
  
  // 8. Wejdź w deep sleep (bez zapisu historii - PM się nie zmienił)
  #if ENABLE_DEEP_SLEEP
  prepareForDeepSleep(false);  // false = nie zapisuj historii
  #endif
}

// --------------------------------------------
// FULL MEASUREMENT - I2C + PMS (30s rozgrzewki)
// Wywoływane przy: TIMER (gdy czas na PMS) lub POWER_ON
// --------------------------------------------
void performFullMeasurement() {
  Serial.println("\n========== FULL MEASUREMENT (with PMS) ==========");
  
  // 1. Włącz zasilanie (5V + 3.3V)
  #if ENABLE_POWER_MANAGEMENT
  enable3V3();
  enable5V();
  #endif
  
  // 2. Inicjalizacja I2C
  Wire.begin(I2C_SDA, I2C_SCL);
  
  #if ENABLE_I2C_SCAN
  scanI2C();
  #endif
  
  // 3. Inicjalizacja czujników I2C
  initializeAllSensors();
  
  // 4. PMS7003 - inicjalizacja + rozgrzewka z animacją
  #if ENABLE_PMS7003
  setupPMS7003();
  
  Serial.println("[PMS7003] Warming up (30s)...");
  int warmupSeconds = 30;
  for (int i = warmupSeconds; i > 0; i--) {
    #if ENABLE_OLED
    // 4 klatki animacji na sekundę (250ms każda)
    for (int frame = 0; frame < 4; frame++) {
      drawWarmupScreen(i, warmupSeconds);
      delay(250);
    }
    #else
    delay(1000);
    #endif
    
    Serial.print(".");
    if (i % 10 == 0) Serial.println();
  }
  Serial.println("\n[PMS7003] Warmup complete");
  clearPMSBuffer();
  #endif
  
  // 5. Odczyt wszystkich czujników (I2C + PMS)
  readI2CSensors();
  
  #if ENABLE_PMS7003
  readPMS7003();
  #endif
  
  // 6. Zapisz PM do RTC (przeżyje deep sleep)
  #if ENABLE_DEEP_SLEEP
  lastPM1_0 = pm1_0;
  lastPM2_5 = pm2_5;
  lastPM10 = pm10;
  Serial.printf("[RTC] Saved PM values: PM1.0=%d, PM2.5=%d, PM10=%d\n", lastPM1_0, lastPM2_5, lastPM10);
  
  // 7. Ustaw licznik do następnego PMS (adaptacyjny)
  pmsSkipCounter = getPmsSkipCount(pm2_5);
  uint16_t nextPmsMinutes = getEffectivePmsIntervalMinutes(pm2_5);
  Serial.printf("[PMS] Air quality: %s, next PMS in %d cycles (%d min)\n", 
                getAirQualityLabel(pm2_5), pmsSkipCounter, nextPmsMinutes);
  #endif
  
  // 8. Dodaj do historii
  #if ENABLE_OLED
  addToHistory(pm2_5, pm10);
  drawMainScreen();
  lastScreenChange = millis();
  currentScreen = SCREEN_MAIN;
  #endif
  
  // 9. Alarm jeśli PM2.5 > 75
  #if ENABLE_BUZZER
  if (pm2_5 > PM25_UNHEALTHY) {
    Serial.println("[ALARM] PM2.5 too high!");
    buzzerAlarm();
  }
  #endif
  
  // 10. Pauza wyświetlania (2 cykle ekranów)
  #if ENABLE_OLED
  displayPauseLoop();
  #endif
  
  Serial.println("========== FULL CYCLE COMPLETE ==========\n");
  
  // 11. Wejdź w deep sleep (z zapisem historii)
  #if ENABLE_DEEP_SLEEP
  prepareForDeepSleep(true);  // true = zapisz historię jeśli się zmieniła
  #endif
}

// --------------------------------------------
// BUTTON WAKEUP HANDLER
// Świeży pomiar I2C + ostatni PM + LoRa + restart cyklu 15min
// --------------------------------------------
void handleButtonWakeup() {
  Serial.println("\n========== BUTTON WAKEUP ==========");
  Serial.println("[BTN] User pressed button - quick I2C measurement");
  
  // Wykonaj szybki pomiar (bez PMS)
  // Po zakończeniu ESP wejdzie w sleep na nowe 15 minut
  performQuickMeasurement();
  
  // Uwaga: performQuickMeasurement() kończy się deep sleep,
  // więc poniższy kod nigdy się nie wykona
}

// ============================================
// SETUP
// ============================================
void setup() {
  // === OPTYMALIZACJA ENERGII ===
  // Obniż częstotliwość CPU (240MHz -> 80MHz) - oszczędność ~15-20mA
  setCpuFrequencyMhz(80);
  // WiFi/BT są domyślnie wyłączone w Arduino ESP32 dopóki nie wywołasz WiFi.begin()
  
  Serial.begin(115200);
  delay(2000);  // Czas na otwarcie Serial Monitor
  
  Serial.println("\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║   AIR CONDITION - System Monitor v1.4  ║");
  Serial.println("║   XIAO ESP32-S3 + Wio-SX1262           ║");
  Serial.println("║   Adaptive Deep Sleep                  ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.printf("[PWR] CPU: %d MHz (optimized)\n", getCpuFrequencyMhz());
  Serial.println();
  
  // 1. Power Management
  #if ENABLE_POWER_MANAGEMENT
  setupPowerControl();
  #endif
  
  // 2. Buzzer
  #if ENABLE_BUZZER
  setupBuzzer();
  buzzerOK();  // Potwierdzenie startu
  #endif
  
  // 3. LoRa (niezależne od power management)
  #if ENABLE_LORA
  setupLoRa();
  getDeviceMAC(deviceMAC);
  printDeviceMAC();
  #endif
  
  // 4. Sprawdź przyczynę wybudzenia i wykonaj odpowiedni cykl
  #if ENABLE_DEEP_SLEEP
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      // PRZYCISK - szybki pomiar I2C + restart cyklu 15min
      Serial.println("[PWR] Wakeup cause: BUTTON (GPIO4)");
      Serial.printf("[RTC] State: lastPM2.5=%d, pmsSkipCounter=%d\n", lastPM2_5, pmsSkipCounter);
      handleButtonWakeup();
      break;
      
    case ESP_SLEEP_WAKEUP_TIMER:
      // TIMER - sprawdź czy czas na PMS (adaptacyjnie)
      Serial.println("[PWR] Wakeup cause: TIMER (15 min)");
      Serial.printf("[RTC] State: lastPM2.5=%d, pmsSkipCounter=%d\n", lastPM2_5, pmsSkipCounter);
      
      if (shouldMeasurePMS()) {
        performFullMeasurement();
      } else {
        performQuickMeasurement();
      }
      break;
      
    default:
      // POWER ON / RESET - pierwszy start, zawsze pełny pomiar
      Serial.println("[PWR] Wakeup cause: POWER ON / RESET");
      Serial.println("[PWR] First boot - forcing full measurement with PMS");
      pmsSkipCounter = 0;  // Wymuś pomiar PMS
      performFullMeasurement();
      break;
  }
  
  #else
  // Tryb bez deep sleep - stary cykl pomiarowy
  performMeasurementCycle();
  #endif
  
  // Nigdy tu nie dojdziemy przy włączonym deep sleep
}

// ============================================
// LOOP
// ============================================
void loop() {
  #if ENABLE_DEEP_SLEEP
  // Przy włączonym deep sleep ta pętla nigdy się nie wykona
  // (ESP32 wchodzi w sleep na końcu setup())
  // Zostawione jako fallback gdyby coś poszło nie tak
  
  Serial.println("[LOOP] ERROR: Should be in deep sleep!");
  delay(5000);
  prepareForDeepSleep(false);  // Awaryjny powrót do sleep
  
  #else
  // Tryb bez deep sleep - normalna pętla z pomiarami co 60s
  static uint32_t lastMeasurement = 0;
  static const uint32_t MEASUREMENT_INTERVAL = 60000;  // 60 sekund
  
  uint32_t now = millis();
  
  // Obsługa auto-przełączania ekranów OLED
  #if ENABLE_OLED
  updateScreenAuto();
  handleButtonOLED();
  #endif
  
  // Sprawdź czy czas na nowy pomiar
  if (now - lastMeasurement >= MEASUREMENT_INTERVAL) {
    lastMeasurement = now;
    
    Serial.println("[LOOP] Starting new measurement cycle...");
    
    // Wykonaj pełny cykl pomiarowy
    performFullMeasurement();
  }
  
  // Krótka pauza żeby nie obciążać CPU
  delay(50);
  #endif
}
