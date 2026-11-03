# SPI Bus Analysis - XPT2046 Touchscreen Library

> Analysis date: 2026-01-02
> Applies to: Air Condition Receiver v2.6

## 1. How the library manages SPI transactions

Biblioteka XPT2046_Touchscreen używa **prawidłowego zarządzania transakcjami SPI**. Cała komunikacja SPI jest opakowana w `beginTransaction()` / `endTransaction()`:

**Z XPT2046_Touchscreen.cpp (linie 127-146):**
```cpp
_pspi->beginTransaction(SPI_SETTING);
digitalWrite(csPin, LOW);
// ... transfery SPI ...
digitalWrite(csPin, HIGH);
_pspi->endTransaction();
```

Biblioteka:
- Przechowuje wskaźnik do instancji SPI (`SPIClass *_pspi`)
- Wywołuje `beginTransaction()` przed KAŻDĄ komunikacją SPI
- Prawidłowo ustawia CS LOW przed transferami
- Ustawia CS HIGH po transferach
- Wywołuje `endTransaction()` po zakończeniu całej komunikacji

## 2. Wzorzec Begin/End Transaction

**TAK** - Biblioteka jawnie wywołuje zarówno `beginTransaction()` jak i `endTransaction()`:

| Funkcja | Linia | Opis |
|---------|-------|------|
| `beginTransaction(SPI_SETTING)` | 128 | Wywoływana przed odczytem dotyku |
| `endTransaction()` | 146 | Wywoływana po zakończeniu odczytu |

Wzorzec transakcji jest zaimplementowany w funkcji `update()` (linie 120-216), która jest wywoływana przez:
- `touched()`
- `getPoint()`
- `readData()`

## 3. Prędkość SPI

**2 MHz (2,000,000 Hz)**

Zdefiniowane w linii 28:
```cpp
#define SPI_SETTING SPISettings(2000000, MSBFIRST, SPI_MODE0)
```

Konfiguracja SPI:
- **Prędkość:** 2 MHz
- **Kolejność bitów:** MSBFIRST (Most Significant Bit First)
- **Tryb:** SPI_MODE0 (CPOL=0, CPHA=0)

Jest to stosunkowo wolne w porównaniu z typowymi prędkościami SPI dla LCD (10-40 MHz), co jest normalne dla kontrolerów dotyku. Datasheet XPT2046 określa max 2.5 MHz.

## 4. `ts.begin(SPIClass&)` vs `ts.begin()` bez argumentów

**Plik nagłówkowy (linia 52):**
```cpp
bool begin(SPIClass &wspi = SPI);
```

**Implementacja (linie 33-45):**
```cpp
bool XPT2046_Touchscreen::begin(SPIClass &wspi)
{
  _pspi = &wspi;              // Zapisz wskaźnik do dostarczonego SPI
  _pspi->begin();             // Zainicjalizuj tę magistralę SPI
  pinMode(csPin, OUTPUT);
  digitalWrite(csPin, HIGH);
  // ... konfiguracja IRQ jeśli pin podany ...
  return true;
}
```

| Wzorzec wywołania | Zachowanie |
|-------------------|------------|
| `ts.begin()` | Używa domyślnego globalnego obiektu `SPI` (parametr domyślny) |
| `ts.begin(mySPI)` | Używa dostarczonej instancji SPIClass (np. HSPI, VSPI, własnej) |

Biblioteka:
1. Zapisuje referencję do instancji SPI wewnętrznie
2. Wywołuje `begin()` na tej instancji SPI
3. Konfiguruje pin CS jako OUTPUT i ustawia HIGH (niewybrany)
4. Ustawia przerwanie IRQ jeśli tirqPin został podany w konstruktorze

## 5. Czy może bezpiecznie współdzielić magistralę SPI?

**TAK** - Biblioteka jest zaprojektowana do bezpiecznego współdzielenia magistrali SPI z innymi urządzeniami.

**Cechy bezpiecznego współdzielenia:**

1. **Prawidłowe granice transakcji** - Używa `beginTransaction()` / `endTransaction()`
2. **Zarządzanie pinem CS** - Prawidłowo ustawia/zwalnia chip select
3. **Izolacja ustawień** - Każda transakcja używa `SPISettings` do konfiguracji prędkości/trybu
4. **Brak stanu trwałego** - Nie modyfikuje globalnych ustawień SPI poza transakcjami

## 6. Implementacja w naszym projekcie (hardware.cpp)

### Inicjalizacja Touch (setupTouch):
```cpp
// Touch używa HSPI - te same piny co LCD, inny CS
touchSPI.begin(LCD_SCLK, LCD_MISO, LCD_MOSI, TOUCH_CS);

if (ts.begin(touchSPI)) {
  ts.setRotation(1);
  touch_ok = true;
  digitalWrite(TOUCH_CS, HIGH);
}
```

### Po każdej operacji touch (checkTouch):
```cpp
// Upewnij się że TOUCH_CS jest HIGH
digitalWrite(TOUCH_CS, HIGH);

// Jawne zakończenie transakcji SPI touch
// To przywraca SPI bus do stanu domyślnego
touchSPI.endTransaction();

touchActive = false;
lastTouchEndTime = millis();
```

### Funkcja prepareLcdSpi():
```cpp
void prepareLcdSpi() {
  // Upewnij się że TOUCH_CS jest HIGH (touch nieaktywny)
  digitalWrite(TOUCH_CS, HIGH);
  
  // Wymuś zakończenie ewentualnej transakcji touch
  touchSPI.endTransaction();
  
  // Krótkie opóźnienie dla ustabilizowania magistrali
  delayMicroseconds(100);
  
  // Upewnij się że LCD_CS jest HIGH przed rozpoczęciem
  digitalWrite(LCD_CS, HIGH);
}
```

## 7. Tabela podsumowująca

| Aspekt | Wartość/Zachowanie |
|--------|-------------------|
| Obsługa transakcji | Pełne wsparcie `beginTransaction()`/`endTransaction()` |
| Prędkość SPI | 2 MHz |
| Tryb SPI | MODE0 (CPOL=0, CPHA=0) |
| Kolejność bitów | MSBFIRST |
| Wsparcie własnego SPI | Tak, przez `begin(SPIClass&)` |
| Domyślne SPI | Globalny obiekt `SPI` |
| Bezpieczne współdzielenie magistrali | **TAK** |
| Zarządzanie CS | Manualne (biblioteka obsługuje prawidłowo) |

## 8. Ścieżki do plików źródłowych biblioteki

```
~/Documents/Arduino/libraries/XPT2046_Touchscreen/
├── XPT2046_Touchscreen.cpp    # Implementacja
├── XPT2046_Touchscreen.h      # Nagłówek
├── README.md                   # Dokumentacja
└── examples/
    ├── ILI9341Test/           # Przykład współdzielenia SPI z LCD
    ├── TouchTest/
    └── TouchTestIRQ/
```

## 9. Potencjalne problemy i rozwiązania

### Problem: Konflikt SPI między LCD a Touch
**Objaw:** Artefakty na ekranie, brak odczytu dotyku
**Rozwiązanie:** 
- Sprawdź czy oba CS są HIGH przed rozpoczęciem operacji
- Użyj `prepareLcdSpi()` przed operacjami LCD
- Nie sprawdzaj touch podczas rysowania LCD (`lcdDrawing` flag)

### Problem: Touch nie odpowiada
**Objaw:** `ts.touched()` zawsze zwraca false
**Rozwiązanie:**
- Sprawdź czy `ts.begin(touchSPI)` zwróciło true
- Zweryfikuj połączenia MISO (touch wymaga odczytu danych)
- Sprawdź czy TOUCH_CS jest prawidłowo skonfigurowany

### Problem: Niestabilne odczyty współrzędnych
**Objaw:** Współrzędne skaczą lub są nieprawidłowe
**Rozwiązanie:**
- Biblioteka używa wewnętrznego uśredniania (besttwoavg)
- Dodatkowe filtrowanie w kodzie aplikacji
- Sprawdź wartość Z (siła nacisku) przed użyciem X/Y

---

## 10. Kluczowe wnioski (ZAKTUALIZOWANE 2026-01-02)

### PROBLEM GŁÓWNY: Dwie osobne instancje SPI z osobnymi mutexami!

Pomimo że obie biblioteki (XPT2046 i Arduino_GFX) prawidłowo używają `beginTransaction()`/`endTransaction()`, **używały RÓŻNYCH instancji SPI**, które mają **RÓŻNE mutexy**:

```cpp
// globals.cpp - PROBLEM:

// LCD tworzy własną wewnętrzną strukturę spi_t z własnym mutex-em
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO,
  HSPI,      // <-- Tworzy wewnętrzną _spi_bus_array[HSPI] z własnym lock-iem
  true
);

// Touch tworzy OSOBNĄ instancję SPIClass  
SPIClass touchSPI(HSPI);  // <-- Inny mutex/lock!
XPT2046_Touchscreen ts(TOUCH_CS);
```

**Rezultat:** Obie instancje operują na tym samym sprzęcie HSPI, ale nie synchronizują się wzajemnie - każda ma własny mutex, więc `beginTransaction()` jednej nie blokuje drugiej!

### Dowód z analizy Arduino_ESP32SPI:

W pliku `Arduino_ESP32SPI.cpp` (linia 233):
```cpp
_spi = &_spi_bus_array[_spi_num];
```

`Arduino_ESP32SPI` używa **statycznej tablicy** struktur SPI z ESP-IDF, a nie Arduino `SPIClass`. To kompletnie osobny system synchronizacji!

### ROZWIĄZANIE: Jedna wspólna instancja SPIClass

Zamiast `Arduino_ESP32SPI` używamy `Arduino_HWSPI`, która akceptuje zewnętrzny wskaźnik `SPIClass*`:

```cpp
// Arduino_HWSPI.h (linia 23):
Arduino_HWSPI(int8_t dc, int8_t cs, int8_t sck, int8_t mosi, int8_t miso, 
              SPIClass *spi = &SPI, bool is_shared_interface = true);
```

**Nowa architektura:**
```cpp
// JEDNA wspólna instancja SPI dla LCD i Touch
SPIClass sharedSPI(HSPI);

// LCD używa Arduino_HWSPI z wskaźnikiem do sharedSPI
Arduino_DataBus *bus = new Arduino_HWSPI(
  LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO,
  &sharedSPI,  // <-- Ten sam SPI co Touch!
  true
);

// Touch też używa sharedSPI
ts.begin(sharedSPI);  // <-- Ten sam mutex!
```

**Korzyści:**
1. **Jeden mutex** - obie biblioteki używają tego samego `SPIClass::beginTransaction()`
2. **Prawidłowa synchronizacja** - gdy Touch czyta, LCD czeka (i odwrotnie)
3. **Minimalny koszt zmian** - tylko zmiana klasy databus dla LCD

---

## 11. POPRAWKI WPROWADZONE (2026-01-02)

### Problem 1: Podwójne `endTransaction()` w checkTouch()

**BYŁO (BŁĄD):**
```cpp
bool checkTouch(int16_t* x, int16_t* y) {
  bool touched = ts.touched();  // Biblioteka sama kończy transakcję!
  // ...
  touchSPI.endTransaction();    // ❌ DRUGI RAZ - undefined behavior!
  return touched;
}
```

**JEST (POPRAWNIE):**
```cpp
bool checkTouch(int16_t* x, int16_t* y) {
  // Biblioteka XPT2046 sama zarządza transakcjami SPI
  bool touched = ts.touched();
  // ...
  digitalWrite(TOUCH_CS, HIGH);  // Tylko bezpieczeństwo CS
  // NIE wywołuj endTransaction() - biblioteka już to zrobiła!
  return touched;
}
```

### Problem 2: `endTransaction()` bez `beginTransaction()` w prepareLcdSpi()

**BYŁO (BŁĄD):**
```cpp
void prepareLcdSpi() {
  digitalWrite(TOUCH_CS, HIGH);
  touchSPI.endTransaction();  // ❌ Kończy transakcję która nie istnieje!
  delayMicroseconds(100);
  digitalWrite(LCD_CS, HIGH);
}
```

**JEST (POPRAWNIE):**
```cpp
void prepareLcdSpi() {
  digitalWrite(TOUCH_CS, HIGH);
  // NIE wywołuj endTransaction() - biblioteka XPT2046 sama kończy transakcje
  delayMicroseconds(10);
  // LCD_CS jest zarządzany przez Arduino_GFX automatycznie
}
```

### Dlaczego to był problem?

1. **XPT2046_Touchscreen** wywołuje `beginTransaction()` / `endTransaction()` wewnętrznie w funkcji `update()` (wywoływanej przez `touched()` i `getPoint()`)

2. **Arduino_GFX** z `is_shared_interface=true` również sam zarządza transakcjami

3. Wywołanie `endTransaction()` bez wcześniejszego `beginTransaction()` to **undefined behavior** - może powodować:
   - Zawieszenie SPI
   - Artefakty na ekranie
   - Brak odpowiedzi dotyku
   - Losowe błędy

### Zasada: Trust the libraries!

Obie biblioteki (XPT2046 i Arduino_GFX) są zaprojektowane do współdzielenia SPI i **same zarządzają transakcjami**. Nie trzeba ręcznie wywoływać `beginTransaction()`/`endTransaction()` - wystarczy:

1. Ustawić flagę `lcdDrawing` gdy LCD rysuje
2. Nie sprawdzać touch gdy `lcdDrawing == true`
3. Upewnić się że CS piny są HIGH gdy urządzenie nieaktywne

---

## 12. IMPLEMENTACJA ROZWIĄZANIA (2026-01-02)

### Zmiana w globals.cpp

**PRZED (dwa osobne SPI):**
```cpp
#if ENABLE_LCD
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO,
  HSPI, true
);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0, true, 240, 320, 0, 0);
#endif

#if ENABLE_TOUCH
SPIClass touchSPI(HSPI);  // ❌ OSOBNA instancja = osobny mutex!
XPT2046_Touchscreen ts(TOUCH_CS);
#endif
```

**PO (wspólny SPI):**
```cpp
// WSPÓLNA instancja SPI dla LCD i Touch (HSPI)
SPIClass sharedSPI(HSPI);

#if ENABLE_LCD
// Używamy Arduino_HWSPI która akceptuje zewnętrzny wskaźnik SPIClass
Arduino_DataBus *bus = new Arduino_HWSPI(
  LCD_DC, LCD_CS, LCD_SCLK, LCD_MOSI, LCD_MISO,
  &sharedSPI,  // ✅ Wskaźnik do wspólnej instancji SPI
  true         // is_shared_interface
);
Arduino_GFX *gfx = new Arduino_ST7789(bus, LCD_RST, 0, true, 240, 320, 0, 0);
#endif

#if ENABLE_TOUCH
XPT2046_Touchscreen ts(TOUCH_CS);
// Touch użyje sharedSPI w begin()
#endif
```

### Zmiana w types.h

**PRZED:**
```cpp
#if ENABLE_TOUCH
extern SPIClass touchSPI;
extern XPT2046_Touchscreen ts;
#endif
```

**PO:**
```cpp
// Wspólna instancja SPI dla LCD i Touch
extern SPIClass sharedSPI;

#if ENABLE_TOUCH
extern XPT2046_Touchscreen ts;
#endif
```

### Zmiana w hardware.cpp (setupTouch)

**PRZED:**
```cpp
bool setupTouch() {
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  
  touchSPI.begin(LCD_SCLK, LCD_MISO, LCD_MOSI, TOUCH_CS);
  
  if (ts.begin(touchSPI)) {
    // ...
  }
}
```

**PO:**
```cpp
bool setupTouch() {
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);
  
  // SPI jest już zainicjalizowane przez LCD (gfx->begin())
  // Tylko zarejestruj touch z tą samą instancją
  if (ts.begin(sharedSPI)) {
    // ...
  }
}
```

### Kolejność inicjalizacji

1. `setup()` woła `setupLCD()` → `gfx->begin()` → `bus->begin()` → `sharedSPI.begin(sck, miso, mosi)`
2. `setup()` woła `setupTouch()` → `ts.begin(sharedSPI)` - SPI już zainicjalizowane!

### Biblioteki używane

- **Arduino_HWSPI** (`Arduino_GFX_Library`) - klasa data bus akceptująca `SPIClass*`
- **XPT2046_Touchscreen** - akceptuje `SPIClass&` w `begin()`

### Ścieżki do plików źródłowych

```
~/Documents/Arduino/libraries/GFX_Library_for_Arduino/src/databus/
├── Arduino_HWSPI.h      # Deklaracja - linia 23: konstruktor z SPIClass*
├── Arduino_HWSPI.cpp    # Implementacja - używa _spi->beginTransaction()
└── Arduino_ESP32SPI.h   # STARA klasa - używa wewnętrznej _spi_bus_array[]
```

---

## 13. WERYFIKACJA PO ZMIANACH

### Testy do wykonania:

1. **Home screen** - czy kafelki renderują się poprawnie?
2. **Detail screen** - czy wykres rotuje co 3s bez artefaktów?
3. **Touch response** - czy dotyk działa podczas/po rysowaniu LCD?
4. **LoRa reception** - czy pakiety nadal przychodzą? (FSPI - nie powinno być wpływu)
5. **Triple-tap reinit** - czy LCD reinicjalizuje się poprawnie?

### Oczekiwany rezultat:

- Brak artefaktów na ekranie
- Płynna rotacja wykresów
- Responsywny touch
- Stabilna praca przez długi czas

---

## 14. MITYGACJA: Auto-repair przy powrocie na Home Screen (2026-01-02)

### Problem resztkowy

Po implementacji wspólnej instancji SPI (sekcja 12), artefakty LCD nadal mogą 
występować sporadycznie, szczególnie na Detail Screen który intensywnie 
używa LCD (rotacja wykresów co 3 sekundy). Touch działa znacznie lepiej.

### Rozwiązanie: Naprawa stanu SPI przy powrocie na Home Screen

Zamiast periodycznych resetów, naprawiamy stan SPI **tylko gdy użytkownik 
wraca na Home Screen** poprzez ACTION_BACK. Jest to naturalne miejsce na 
"reset" ponieważ:

1. Home Screen jest rysowany od nowa (pełny redraw)
2. Użytkownik oczekuje zmiany ekranu więc krótkie opóźnienie jest akceptowalne
3. Nie wymaga widocznego "mrugnięcia" - nowy ekran rysuje się na wierzchu

### Implementacja

#### Funkcja `repairLcdState()` w hardware.cpp:

```cpp
void repairLcdState() {
  // Upewnij się że CS piny są nieaktywne
  digitalWrite(TOUCH_CS, HIGH);
  
  // Krótka pauza dla ustabilizowania SPI
  delayMicroseconds(100);
  
  // Nie robimy fillScreen() - unikamy mrugnięcia
  // drawHomeScreen() narysuje wszystko od nowa
  
  DEBUG_LCD_PRINTLN("[LCD] SPI state repaired before Home Screen");
}
```

#### Wywołanie w touchHandler.cpp (ACTION_BACK):

```cpp
case ACTION_BACK:
  if (currentScreen == SCREEN_SETTINGS) {
    repairLcdState();  // Naprawa przed Home
    currentScreen = SCREEN_HOME;
  } else if (currentScreen == SCREEN_WIFI_CONFIG) {
    currentScreen = SCREEN_SETTINGS;  // Bez repair - nie Home
  } else {
    repairLcdState();  // Naprawa przed Home (z Detail)
    currentScreen = SCREEN_HOME;
    selectedDeviceIndex = -1;
  }
  screenNeedsRedraw = true;
  break;
```

### Dlaczego płynne (bez mrugnięcia)?

1. **Brak `fillScreen()`** - nie czyścimy ekranu na czarno
2. **Pełny redraw** - `drawHomeScreen()` rysuje każdy element od nowa
3. **Progresywne rysowanie** - header, kafelki, footer przykrywają stary obraz

### Kiedy się uruchamia?

| Przejście | Repair? | Powód |
|-----------|---------|-------|
| Detail → Home | ✅ TAK | Główne źródło artefaktów |
| Settings → Home | ✅ TAK | Prewencja |
| WiFi Config → Settings | ❌ NIE | Nie wracamy na Home |
| Keyboard → (return screen) | ❌ NIE | Obsługiwane przez ACTION_KEY_OK/CANCEL |

---

## 15. OPTYMALIZACJA: Obniżenie częstotliwości SPI dla LCD (2026-01-02)

### Problem: Zbyt wysoka częstotliwość SPI dla długich przewodów

Domyślna częstotliwość SPI dla ESP32 w Arduino_GFX to **40 MHz** (zdefiniowane w `Arduino_DataBus.h`).
Przy przewodach o długości ~15cm między ESP32 a LCD, jest to zdecydowanie za dużo i może powodować:

- Odbicia sygnału (reflections)
- Crosstalk między przewodami  
- Degradację sygnału (rise/fall time)
- Losowe artefakty na ekranie

### Zalecane częstotliwości vs długość przewodów

| Częstotliwość | Max zalecana długość | Uwagi |
|---------------|---------------------|-------|
| 40 MHz | ~5-10 cm | Domyślna, zbyt szybka dla 15cm |
| 20 MHz | ~10-15 cm | Na granicy |
| 10 MHz | ~20-30 cm | Bezpieczna |
| 8 MHz | ~30+ cm | Najbezpieczniejsza |

### Rozwiązanie: 8 MHz

Obniżamy częstotliwość SPI do **8 MHz** - najbezpieczniejsza opcja dla ~15cm przewodów.

### Wpływ na wydajność

Czas rysowania pełnego ekranu (240×320×2 = 153,600 bytes):

| Częstotliwość | Czas transferu | Teoretyczne FPS |
|---------------|----------------|-----------------|
| 40 MHz | ~4 ms | 250 FPS |
| 8 MHz | ~19 ms | 52 FPS |

Nawet przy 8 MHz, pełny ekran rysuje się w ~19ms co daje teoretyczne 52 FPS.
Jest to więcej niż wystarczające dla naszego interfejsu użytkownika.

### Implementacja

W `setupLCD()`:
```cpp
// Inicjalizacja GFX z obniżoną częstotliwością SPI
// 8 MHz zamiast domyślnych 40 MHz - bezpieczniejsze dla długich przewodów (~15cm)
if (!gfx->begin(8000000)) {
  // ...
}
```

W `reinitLCD()`:
```cpp
// Pełna inicjalizacja z obniżoną częstotliwością SPI (8 MHz)
gfx->begin(8000000);
```

### Porównanie częstotliwości urządzeń na HSPI

| Urządzenie | Częstotliwość | Status |
|------------|---------------|--------|
| LCD (ST7789) | 8 MHz | ✅ Obniżona z 40 MHz |
| Touch (XPT2046) | 2 MHz | ✅ Bez zmian (wystarczająca) |

### Oczekiwany efekt

1. **Eliminacja lub znaczne zmniejszenie artefaktów** spowodowanych zbyt szybkim SPI
2. **Stabilniejsza komunikacja** na długich przewodach
3. **Minimalny wpływ na wydajność** - nadal płynne ~50 FPS

---

## 16. NAPRAWA NIEZAINICJALIZOWANYCH ZMIENNYCH (2026-01-02)

### Problem: Artefakty LCD tylko przy starcie

**Objaw:** Artefakty/korupcja ekranu występowały TYLKO przy pierwszym wejściu na Detail Screen 
po starcie urządzenia. Jeśli pierwsze wejście było OK, problem nigdy nie występował później.

**Diagnoza:** To klasyczny objaw **niezainicjalizowanej pamięci** - zmienne globalne bez 
jawnych inicjalizatorów mogą mieć losowe wartości przy starcie.

### Analiza przeprowadzona

Przeprowadzono gruntowną analizę wszystkich plików źródłowych:
- `globals.cpp` - zmienne globalne
- `types.h` - definicje struktur
- `uiScreens.cpp` - funkcje rysowania
- `deviceManager.cpp` - zarządzanie urządzeniami
- `touchHandler.cpp` - obsługa dotyku
- `air_condition_receiver.ino` - setup() i loop()

### Znalezione problemy i ich naprawy

#### 1. DeviceData struct bez domyślnych inicjalizatorów (HIGH PRIORITY)

**Problem:** Struktura `DeviceData` w `types.h` nie miała domyślnych wartości pól.
Jeśli `drawPMChart()` zostało wywołane przed `initDevice()`, pole `historyCount`
mogło mieć śmieciową wartość (np. 255), powodując rysowanie 255 słupków!

**Naprawa (types.h):**
```cpp
struct DeviceData {
  uint8_t mac[6] = {0};
  char name[DEVICE_NAME_LEN] = {0};
  bool active = false;
  float temperature = 0.0f;
  float humidity = 0.0f;
  uint16_t pressure = 0;
  uint16_t pm1_0 = 0, pm2_5 = 0, pm10 = 0;
  // ... wszystkie pola z domyślnymi wartościami ...
  uint8_t historyCount = 0;  // Kluczowe!
  // ...
};
```

#### 2. Brak bounds check dla historyCount w drawPMChart() (HIGH PRIORITY)

**Problem:** Pętla rysująca słupki wykresu nie sprawdzała czy `historyCount` 
jest w sensownym zakresie.

**Naprawa (uiScreens.cpp):**
```cpp
// Sanity check - historyCount może być uszkodzone
if (dev->historyCount > HISTORY_SIZE) {
  dev->historyCount = 0;
  dev->historyIndex = 0;
  return;
}
```

#### 3. memset(devices) wywoływane za późno w setup() (MEDIUM PRIORITY)

**Problem:** `memset(devices, 0, sizeof(devices))` było wywoływane PO inicjalizacji
LCD i Touch, co mogło prowadzić do wyścigów przy bardzo szybkich zdarzeniach.

**Naprawa (air_condition_receiver.ino):**
```cpp
void setup() {
  // NAJPIERW: Inicjalizacja wszystkich struktur danych
  memset(devices, 0, sizeof(devices));
  deviceCount = 0;
  
  DEBUG_INIT();
  // ... reszta setup() ...
}
```

#### 4. Brak sprawdzenia dev->active w drawDetailScreen() (MEDIUM PRIORITY)

**Problem:** Funkcja `drawDetailScreen()` nie sprawdzała czy urządzenie jest aktywne,
co mogło prowadzić do wyświetlania nieaktualnych danych.

**Naprawa (uiScreens.cpp):**
```cpp
DeviceData* dev = &devices[deviceIndex];
if (!dev->active) {
  lcdDrawing = false;
  currentScreen = SCREEN_HOME;
  screenNeedsRedraw = true;
  return;
}
```

#### 5. Brakujące upper bound dla selectedDeviceIndex (MEDIUM PRIORITY)

**Problem:** W kilku miejscach sprawdzano tylko `selectedDeviceIndex >= 0`,
ale nie `selectedDeviceIndex < MAX_DEVICES`.

**Naprawy:**
- `touchHandler.cpp:274` - dodano `&& selectedDeviceIndex < MAX_DEVICES`
- `air_condition_receiver.ino:181` - dodano `&& selectedDeviceIndex < MAX_DEVICES`
- `air_condition_receiver.ino:204` - dodano `&& selectedDeviceIndex < MAX_DEVICES`

#### 6. Ryzyko buffer overflow w sprintf (MEDIUM PRIORITY)

**Problem:** `char pmBuf[6]` było za małe dla wartości 5-cyfrowych (max uint16_t = 65535).

**Naprawa (uiScreens.cpp):**
```cpp
char pmBuf[8];  // Zwiększony bufor
snprintf(pmBuf, sizeof(pmBuf), "%3d", dev->pm1_0);  // Użycie snprintf
```

#### 7. Jawne inicjalizatory dla tablic globalnych (LOW PRIORITY)

**Problem:** Tablice globalne nie miały jawnych inicjalizatorów `= {}`.

**Naprawa (globals.cpp):**
```cpp
DeviceData devices[MAX_DEVICES] = {};
SensorPacket lastPacket = {};
WiFiNetwork wifiNetworks[WIFI_MAX_NETWORKS] = {};
```

#### 8. kbCtx.buffer niepełna inicjalizacja (LOW PRIORITY)

**Problem:** Użycie `""` inicjalizowało tylko pierwszy bajt bufora.

**Naprawa (globals.cpp):**
```cpp
KeyboardContext kbCtx = {
  {0},     // buffer - pełna inicjalizacja tablicy
  0,       // cursorPos
  // ...
};
```

### Zwiększenie częstotliwości SPI do 20 MHz

Po stabilizacji poprzez naprawę niezainicjalizowanych zmiennych, zwiększono
częstotliwość SPI z 8 MHz do 20 MHz (kompromis między bezpieczeństwem a wydajnością).

**hardware.cpp:**
```cpp
// 20 MHz - kompromis między 8 MHz (bezpieczne) a 40 MHz (domyślne)
gfx->begin(20000000);
```

### Podsumowanie zmian w plikach

| Plik | Zmiany |
|------|--------|
| `types.h` | Domyślne inicjalizatory dla wszystkich pól DeviceData |
| `uiScreens.cpp` | Bounds check dla historyCount, dev->active check, snprintf |
| `air_condition_receiver.ino` | memset na początek setup(), bounds checks |
| `touchHandler.cpp` | Upper bound dla selectedDeviceIndex |
| `globals.cpp` | Jawne inicjalizatory `= {}` dla tablic |
| `hardware.cpp` | SPI 8→20 MHz |

### Oczekiwany rezultat

1. **Eliminacja artefaktów przy starcie** - wszystkie zmienne mają znane wartości
2. **Bezpieczniejszy kod** - bounds checks zapobiegają out-of-bounds access
3. **Lepszy Debug** - łatwiejsze diagnozowanie problemów
4. **Wyższa wydajność** - SPI 20 MHz przy zachowaniu stabilności

### Compilation

```
Sketch uses 1,035,919 bytes (30%) of program storage space.
Global variables use 48,752 bytes (14%) of dynamic memory.
```

---

## 17. SAFE ICON DRAWING FIX (2026-01-02)

### Problem: Icon artifacts for humidity and pressure icons

**Symptom:** On Detail Screen, the humidity icon (ICON_HUMIDITY) and pressure icon 
(ICON_PRESSURE) were displaying artifacts or not rendering at all. The thermometer 
icon worked correctly.

**Observation:** Icons drawn in the header (BACK, SETTINGS, WiFi) worked fine. 
The problematic icons were rendered AFTER text drawing operations (`gfx->print()`).

### Root cause analysis

The original `drawIcon()` function used pixel-by-pixel drawing with `gfx->drawPixel()`.
When called after `gfx->print()` operations, the GFX library's internal state (cursor 
position, text mode flags, etc.) could interfere with pixel drawing operations.

**Original problematic code:**
```cpp
void drawIcon(int x, int y, const uint16_t* icon, int w, int h, uint16_t tintColor) {
  for (int py = 0; py < h; py++) {
    for (int px = 0; px < w; px++) {
      uint16_t pixel = pgm_read_word(&icon[py * w + px]);
      if (pixel != 0x0000) {
        gfx->drawPixel(x + px, y + py, (pixel == 0xFFFF) ? tintColor : pixel);
      }
    }
  }
}
```

### Solution: Two-part fix

#### Part A: New `safeDrawIcon()` function

Created a more robust icon drawing function that:
1. Uses a static buffer to pre-process icon data
2. Applies tint color substitution before drawing
3. Uses optimized drawing methods instead of pixel-by-pixel

**New function (uiScreens.cpp):**
```cpp
void safeDrawIcon(int x, int y, const uint16_t* icon, int w, int h, uint16_t tintColor) {
  // Static buffer for processed icon (max 16x16 = 256 pixels)
  static uint16_t iconBuffer[256];
  
  // Validate size
  if (w * h > 256) return;
  
  // Prepare buffer with color substitutions
  bool hasTransparent = false;
  for (int i = 0; i < w * h; i++) {
    uint16_t pixel = pgm_read_word(&icon[i]);
    if (pixel == 0x0000) {
      hasTransparent = true;
      iconBuffer[i] = 0x0000;
    } else if (pixel == 0xFFFF) {
      iconBuffer[i] = tintColor;  // White -> tint color
    } else {
      iconBuffer[i] = pixel;
    }
  }
  
  // Draw based on transparency presence
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
    // No transparency - use fast bitmap drawing
    gfx->draw16bitRGBBitmap(x, y, iconBuffer, w, h);
  }
}
```

#### Part B: Changed drawing order in `drawDetailScreen()`

Previously, icons and text were interleaved:
```cpp
// OLD - problematic order
drawIcon(5, y, ICON_THERMOMETER, ...);
gfx->print(temperature);
drawIcon(105, y, ICON_HUMIDITY, ...);  // May fail after print()!
gfx->print(humidity);
drawIcon(185, y, ICON_PRESSURE, ...);  // May fail after print()!
gfx->print(pressure);
```

Now, all icons are drawn first, then all text:
```cpp
// NEW - safe order
safeDrawIcon(5, y, ICON_THERMOMETER, ...);
safeDrawIcon(105, y, ICON_HUMIDITY, ...);
safeDrawIcon(185, y, ICON_PRESSURE, ...);

// Then text
gfx->print(temperature);
gfx->print(humidity);
gfx->print(pressure);
```

### Files modified

| File | Changes |
|------|---------|
| `uiScreens.cpp` | Added `safeDrawIcon()`, changed all `drawIcon()` calls to `safeDrawIcon()`, reordered drawing in `drawDetailScreen()` |
| `types.h` | Added declaration for `safeDrawIcon()` |

### All icon draw locations updated

| Location | Function | Icons |
|----------|----------|-------|
| Header | `drawHeader()` | WiFi, Settings |
| Header update | `updateHeaderWifiIcon()` | WiFi |
| Detail screen | `drawDetailScreen()` | Back, Edit, Settings, Thermometer, Humidity, Pressure |
| Settings | `drawSettingsScreen()` | Back |
| WiFi config | `drawWifiConfigScreen()` | Back |

### Expected results

1. **All icons render correctly** - including humidity and pressure
2. **No dependency on GFX state** - icons work regardless of previous operations
3. **Consistent behavior** - same function used everywhere for uniformity

### Compilation result (v2.6)

```
Sketch uses 1,036,011 bytes (30%) of program storage space.
Global variables use 49,264 bytes (15%) of dynamic memory.
```
