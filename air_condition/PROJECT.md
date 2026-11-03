# Projekt: Air Condition - System Monitorowania Jakości Powietrza

## Opis Ogólny

System monitorowania jakości powietrza i warunków atmosferycznych z komunikacją radiową LoRa (SX1262). Urządzenie mierzy temperaturę, wilgotność, ciśnienie atmosferyczne oraz stężenie pyłów PM1.0/PM2.5/PM10. Dane wyświetlane są na ekranie OLED i mogą być przesyłane bezprzewodowo.

---

## Lista Komponentów (BOM)

| # | Element | Model | Interfejs | Napięcie | Uwagi |
|---|---------|-------|-----------|----------|-------|
| 1 | MCU | XIAO ESP32-S3 Sense | - | 3.3V/5V USB | Główny kontroler |
| 2 | Moduł LoRa | Wio-SX1262 | SPI (B2B) | 3.3V | Komunikacja radiowa |
| 3 | Czujnik temp./wilg. | SHTC3 (DFRobot SEN0414) | I2C | 3.3V | Adres: 0x70 |
| 4 | Czujnik ciśnienia | BMP280 (DFRobot SEN0372) | I2C | 3.3V | Adres: 0x77 lub 0x76 |
| 5 | Wyświetlacz | OLED 0.96" 128x64 niebieski | I2C | 3.3V-5V | Adres: 0x3C |
| 6 | Czujnik pyłu | PMS7003 + adapter IDC-microUSB (CH340G) | UART | 5V | Logika 3.3V TTL, goldpiny: 5V/GND/RX/TX |
| 7 | Buzzer | Moduł z buzzerem aktywnym (zielony) | GPIO | 3.3V | Sygnalizacja alarmów, bezpośrednio do GPIO |
| 8 | Zasilanie | Akumulator Li-Po 4000mAh | - | 3.7V | Ładowanie przez USB |
| 9 | Przetwornica | Pololu S7V7F5 (step-up/step-down) | - | IN: 2.7-11.8V -> OUT: 5V 1A | Zasilanie 5V dla PMS7003 |
| 10 | MOSFET N-channel | IRLZ44N (TO-220) | GPIO | V_DS: 55V, I_D: 47A, V_GS(th): 1-2V | Low-side switching zasilania peryferiów |
| 11 | Rezystory | 10kΩ (THT) | - | - | Pull-down dla bramek MOSFET |

### Linki do Zakupu (Botland)

| Element | Link |
|---------|------|
| Pololu S7V7F5 | https://botland.com.pl/przetwornice-step-updown-pololu/1696-pololu-s7v7f5-przetwornica-step-upstep-down-5v-1a-5904422359898.html |
| IRLZ44N (5szt) | https://botland.com.pl/n-mosfet/1950-n-mosfet-irlz44n-55v47a-tht-5szt-5904422373276.html |
| Adapter PMS7003 IDC-microUSB | https://botland.com.pl/czujniki-czystosci-powietrza/13445-adapter-idc-10pin-127mm-microusb-dla-czujnika-pms7003-5904422372293.html |
| Moduł z buzzerem aktywnym (zielony) | https://botland.com.pl/buzzery-generatory-dzwieku/4526-modul-z-buzzerem-aktywnym-z-generatorem-zielony-5904422300654.html |

### Ilości Elementów do Zakupu

| Element | Ilość | Przeznaczenie | Status |
|---------|-------|---------------|--------|
| IRLZ44N N-MOSFET | 2 szt | Q1: low-side switch 5V (PMS7003+Pololu), Q2: low-side switch 3.3V (czujniki I2C+OLED) | **KUPIONY** |
| Rezystor 10kΩ | 2 szt | R1, R2: pull-down bramek (domyślnie OFF) | DO ZAKUPU |

---

## Analiza Kompatybilności

### Zgodność Napięć

| Komponent | Zasilanie | Logika I/O | Kompatybilność z ESP32 (3.3V) |
|-----------|-----------|------------|-------------------------------|
| XIAO ESP32-S3 | 3.7V bat / 5V USB | 3.3V | - (główny kontroler) |
| Wio-SX1262 | 3.3V | 3.3V | **OK** - natywne 3.3V |
| SHTC3 | 1.62V-3.6V | 3.3V | **OK** - natywne 3.3V |
| BMP280 | 1.71V-3.6V | 3.3V | **OK** - natywne 3.3V |
| OLED 128x64 | 3.3V-5V | 3.3V | **OK** - toleruje 3.3V |
| PMS7003 | 5V | 3.3V TTL | **WYMAGA 5V** - przez przetwornicę |
| Buzzer aktywny | 3.3V | - | **OK** - bezpośrednio do GPIO (<20mA) |
| Pololu S7V7F5 | 2.7-11.8V | - | **OK** - zasila PMS7003 |
| IRLZ44N N-MOSFET | - | 3.3V bramka | **OK** - logic-level, V_GS(th)=1-2V, pełne otwarcie przy 3.3V |

### Wynik Analizy

**Wszystkie komponenty są kompatybilne** z ESP32-S3 (3.3V logika).

### WAŻNE: Problem Zasilania 5V przy Pracy Bateryjnej

> **Pin 5V na XIAO ESP32-S3 NIE DZIAŁA przy zasilaniu z baterii!**
>
> XIAO nie posiada wbudowanej przetwornicy boost (step-up). Pin 5V jest bezpośrednio 
> połączony z USB VBUS - dostarcza 5V tylko gdy podłączone jest USB.
>
> | Źródło zasilania | Pin 5V | Pin 3V3 |
> |------------------|--------|---------|
> | USB-C | **5V** | 3.3V |
> | Bateria 3.7V | **0V (brak!)** | 3.3V |

### Rozwiązanie: Przetwornica Pololu S7V7F5

Przetwornica step-up/step-down konwertuje napięcie z baterii (3.7V) na stabilne 5V dla PMS7003.

**Specyfikacja Pololu S7V7F5:**
- Napięcie wejściowe: 2.7V - 11.8V
- Napięcie wyjściowe: 5V (stałe)
- Prąd wyjściowy: do 1A (wystarczający dla PMS7003 ~100mA)
- Sprawność: ~90%
- Wymiary: 9.7mm x 10.7mm x 3mm

**Uwagi:**
- PMS7003 wymaga 5V zasilania - **MUSI być zasilany z przetwornicy Pololu**
- Moduł buzzera aktywnego (zielony) - **bezpośrednio do GPIO** (pobór ~10-15mA)
- OLED może pracować na 3.3V (z modułu DFRobot ma wbudowany regulator)

---

## Schemat Połączeń

### Pinout XIAO ESP32-S3 Sense

```
             XIAO ESP32-S3 Sense
             ┌───────────────────┐
       D0/A0 │ 1              14 │ 5V
       D1/A1 │ 2              13 │ GND
       D2/A2 │ 3              12 │ 3V3
       D3/A3 │ 4              11 │ D10/MOSI
       D4/A4 │ 5              10 │ D9/MISO
       D5/A5 │ 6               9 │ D8/SCK
   D6/TX/A6  │ 7               8 │ D7/RX/A7
             └───────────────────┘
                   [USB-C]
```

### Tabela Połączeń

| XIAO GPIO | Funkcja | Podłączony do | Kolor przewodu (sugestia) |
|-----------|---------|---------------|---------------------------|
| **I2C (wspólna magistrala)** ||||
| D4 (GPIO5) | SDA | SHTC3, BMP280, OLED | Niebieski |
| D5 (GPIO6) | SCL | SHTC3, BMP280, OLED | Żółty |
| **UART (PMS7003)** ||||
| D6 (GPIO43) | TX | PMS7003 RX | Zielony |
| D7 (GPIO44) | RX | PMS7003 TX | Biały |
| **Sterowanie zasilaniem (MOSFETy)** ||||
| D1 (GPIO2) | PWR_5V_EN | Q1 bramka (przez R1) - zasilanie 5V | Fioletowy |
| D2 (GPIO3) | PWR_3V3_EN | Q2 bramka (przez R2) - zasilanie 3.3V peryferiów | Szary |
| **Buzzer** ||||
| D0 (GPIO1) | BUZZER_OUT | Buzzer S (bezpośrednio) | Pomarańczowy |
| **Przycisk wybudzenia** ||||
| D3 (GPIO4) | BUTTON_WAKE | Przycisk NO do GND (wbudowany pull-up) | Brązowy |
| **Zasilanie stałe (zawsze włączone)** ||||
| 3V3 | 3.3V | Rezystory pull-up MOSFETów | Czerwony |
| GND | GND | Wszystkie moduły | Czarny |
| **SX1262 (B2B - wbudowane)** ||||
| GPIO41 | NSS/CS | SX1262 | (B2B) |
| GPIO7 | SCK | SX1262 | (B2B) |
| GPIO9 | MOSI | SX1262 | (B2B) |
| GPIO8 | MISO | SX1262 | (B2B) |
| GPIO42 | RST | SX1262 | (B2B) |
| GPIO40 | BUSY | SX1262 | (B2B) |
| GPIO39 | DIO1/IRQ | SX1262 | (B2B) |
| GPIO38 | ANTSW | SX1262 | (B2B) |
| **Akumulator** ||||
| BAT+ | + | Akumulator 3.7V (+) | Czerwony |
| BAT- | - | Akumulator 3.7V (-) | Czarny |

### Schemat Połączeń na Płytce Uniwersalnej (Low-Side Switching)

**UWAGA:** GND Pololu i GND PMS7003 idą RAZEM do Q1 Drain!

```
SEKCJA ZASILANIA 5V (Q1)
========================

  BAT+ 3.7V ──────┬────────────────> XIAO BAT+
                  │
                  v
            ┌──────────┐
            │  POLOLU  │
            │  S7V7F5  │
            │          │
            │ VIN  GND │──┐
            │ VOUT     │  │
            └────┬─────┘  │
                 │        │
                 v        │
           ┌──────────┐   │
           │ PMS7003  │   │
           │ VCC  GND │───┤
           └──────────┘   │
                          │
                     Q1 DRAIN
                          │
    D1 (GPIO2) ───────────┼───> Q1 Gate
          │               │
        [10k]        ┌────┴────┐
          │          │ IRLZ44N │
         GND         │   Q1    │
                     └────┬────┘
                          │
                     Q1 SOURCE
                          │
                     GND GŁÓWNA
```

```
SEKCJA I2C 3.3V (Q2)
====================

  XIAO 3V3 ───┬────────┬────────┐
              │        │        │
              v        v        v
           ┌─────┐  ┌─────┐  ┌─────┐
           │SHTC3│  │BMP28│  │OLED │
           │ VCC │  │ VCC │  │ VCC │
           │ GND │  │ GND │  │ GND │
           └──┬──┘  └──┬──┘  └──┬──┘
              │        │        │
              └────────┴────────┤
                                │
                           Q2 DRAIN
                                │
    D2 (GPIO3) ─────────────────┼───> Q2 Gate
          │                     │
        [10k]              ┌────┴────┐
          │                │ IRLZ44N │
         GND               │   Q2    │
                           └────┬────┘
                                │
                           Q2 SOURCE
                                │
                           GND GŁÓWNA
```

```
BUZZER (bezpośrednio do GPIO)
=============================

    XIAO 3V3 ───> Buzzer (+)
    D0 (GPIO1) ──> Buzzer (S)
    GND główna ──> Buzzer (-)
```

### Schemat Zasilania z MOSFETami N-channel (LOW-SIDE SWITCHING)

```
SCHEMAT ZASILANIA - LOW-SIDE SWITCHING (N-MOSFET IRLZ44N)
═════════════════════════════════════════════════════════

WAŻNE: N-MOSFET odcina MASĘ (GND), nie zasilanie VCC!
       Q1 odcina GND dla CAŁEJ LINII 5V (Pololu + PMS7003)
       Q2 odcina GND dla czujników I2C + OLED


                        AKUMULATOR Li-Po 3.7V
                             (+)     (-)
                              │       │
              ┌───────────────┤       │
              │               │       │
              ▼               ▼       ▼
          XIAO BAT+      Pololu VIN   GND główna
                              │           │
                         [przetwornica]   │
                              │           │
                         VOUT (5V)        │
                              │           │
                              ▼           │
                      ┌───────┴───────┐   │
                      │               │   │
                      ▼               ▼   │
                 PMS7003 VCC    Pololu GND│
                      │               │   │
                 PMS7003 GND          │   │
                      │               │   │
                      └───────┬───────┘   │
                              │           │
                         Q1 DRAIN         │
                              │           │
                D1 (GPIO2) ───┼─── Gate   │
                      │       │           │
                    [10k]   Q1 IRLZ44N    │
                      │       │           │
                     GND  SOURCE ─────────┤
                 (pull-down)              │
                                          │
                      ┌───────────────────┘
                      │
                      ▼
      ┌───────────────┼───────────────┐
      │               │               │
      ▼               ▼               ▼
 SHTC3 GND      BMP280 GND       OLED GND
      │               │               │
      └───────┬───────┴───────┬───────┘
              │               │
         Q2 DRAIN             │
              │               │
D2 (GPIO3) ───┼─── Gate       │
      │       │               │
    [10k]   Q2 IRLZ44N        │
      │       │               │
     GND  SOURCE ─────────────┴──→ GND główna
 (pull-down)
```

### Schemat Blokowy Zasilania

```
┌─────────────┐     ┌─────────────┐     ┌───────────┐
│ AKUMULATOR  │     │    XIAO     │     │ CZUJNIKI  │
│   3.7V      │────▶│   BAT+      │     │   I2C     │
│   Li-Po     │     │             │     │ SHTC3     │
└──────┬──────┘     │     3V3 ────┼────▶│ BMP280    │
       │            │             │     │ OLED      │
       │            │   D1 (GPIO2)│──┐  └───────────┘
       │            │   D2 (GPIO3)│──┼───▶ Q2 Gate    GND przez Q2
       │            └─────────────┘  │
       │                             │
       ▼                             │
┌──────┴──────┐                      │
│   POLOLU    │                      │
│   S7V7F5    │                      │
│             │                      │
│ VIN  = BAT+ │                      │
│ VOUT = 5V   │───▶ PMS7003 VCC      │
│ GND ────────┼─┐                    │
└─────────────┘ │                    │
                │                    │
                ▼                    │
          ┌─────┴─────┐              │
          │  PMS7003  │              │
          │   GND     │              │
          └─────┬─────┘              │
                │                    │
                └───────▶ Q1 Drain ◀─┼── D1 (GPIO2)
                               │
                          Q1 Source ──▶ GND główna
```

### Szczegółowy Schemat Q1 (Linia 5V)

```
                    BAT+ (3.7V, z akumulatora)
                         │
         ┌───────────────┼───────────────┐
         │                               │
         ▼                               ▼
    XIAO BAT+                       Pololu VIN
                                         │
                                    [przetwornica]
                                         │
                                    VOUT = 5V
                                         │
                 ┌───────────────────────┤
                 │                       │
                 ▼                       ▼
          PMS7003 VCC              Pololu GND
                 │                       │
          PMS7003 GND                    │
                 │                       │
                 └───────────┬───────────┘
                             │
                             │  (wspólna masa Pololu + PMS7003)
                             │
                        Q1 DRAIN
                             │
       GPIO2 (D1) ───────────┼──────┐
             │                      │
             │                 ┌────┴────┐
             │                 │ IRLZ44N │
             │                 │   Q1    │
           [10k]               │  N-CH   │
             │                 └────┬────┘
             │                      │
            GND                Q1 SOURCE
        (pull-down)                 │
                                    ▼
                              GND GŁÓWNA
                         (masa akumulatora)


DZIAŁANIE:
  GPIO2 = HIGH (3.3V) ──▶ Q1 ON  ──▶ GND połączone ──▶ Pololu+PMS WŁĄCZONE
  GPIO2 = LOW  (0V)   ──▶ Q1 OFF ──▶ GND odcięte   ──▶ Pololu+PMS WYŁĄCZONE

KLUCZOWA ZMIANA: GND Pololu idzie przez Q1 (nie bezpośrednio do masy!)
                 Dzięki temu wyłączenie Q1 odcina prąd jałowy (~2mA)
```

### Szczegółowy Schemat Q2 (Linia 3.3V - I2C)

```
                    3V3 (z regulatora XIAO, stałe)
                         │
         ┌───────────────┼───────────────┐
         │               │               │
         ▼               ▼               ▼
    SHTC3 VCC      BMP280 VCC       OLED VCC
         │               │               │
    SHTC3 GND      BMP280 GND       OLED GND
         │               │               │
         └───────┬───────┴───────┬───────┘
                 │               │
                 │  (wspólna masa czujników I2C)
                 │
            Q2 DRAIN
                 │
   GPIO3 (D2) ───┼───────────────┐
         │                       │
         │                  ┌────┴────┐
         │                  │ IRLZ44N │
         │                  │   Q2    │
       [10k]                │  N-CH   │
         │                  └────┬────┘
         │                       │
        GND                 Q2 SOURCE
    (pull-down)                  │
                                 ▼
                           GND GŁÓWNA


DZIAŁANIE:
  GPIO3 = HIGH (3.3V) ──▶ Q2 ON  ──▶ GND połączone ──▶ Czujniki WŁĄCZONE
  GPIO3 = LOW  (0V)   ──▶ Q2 OFF ──▶ GND odcięte   ──▶ Czujniki WYŁĄCZONE
```

### Logika Sterowania

```
┌──────────────────────────────────────────────────────────────────┐
│  STEROWANIE N-MOSFET (LOW-SIDE)                                  │
├──────────────────────────────────────────────────────────────────┤
│  GPIO = HIGH (3.3V) ──▶ MOSFET ON  ──▶ GND podłączone ──▶ ON     │
│  GPIO = LOW  (0V)   ──▶ MOSFET OFF ──▶ GND odcięte    ──▶ OFF    │
└──────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────┐
│  STANY PRZY STARCIE/RESECIE ESP32                                │
├──────────────────────────────────────────────────────────────────┤
│  GPIO w stanie Hi-Z (wysoka impedancja)                          │
│  Rezystor 10k pull-down ściąga Gate do GND                       │
│  MOSFET OFF ──▶ peryferia WYŁĄCZONE ──▶ ochrona baterii          │
└──────────────────────────────────────────────────────────────────┘
```

### Podsumowanie Połączeń Zasilania

```
AKUMULATOR 3.7V
    │
    ├────────▶ XIAO BAT+ (zasilanie mikrokontrolera)
    │
    ├────────▶ Pololu VIN (wejście przetwornicy)
    │
    └────────▶ GND główna
                   │
                   ├────────▶ XIAO GND
                   │
                   ├────────▶ Q1 Source
                   │
                   ├────────▶ Q2 Source
                   │
                   └────────▶ Buzzer (-)

Q1 Drain ◀──────── Pololu GND + PMS7003 GND (wspólnie!)

Q2 Drain ◀──────── SHTC3 GND + BMP280 GND + OLED GND (wspólnie!)
```

### Schemat Pojedynczego Switcha N-MOSFET (Low-Side)

```
ZASADA DZIAŁANIA N-MOSFET LOW-SIDE SWITCHING
═════════════════════════════════════════════

           VCC (3.3V lub 5V - stałe zasilanie)
            │
            │
       ┌────┴────┐
       │URZĄDZENIE│  (czujnik, przetwornica, itp.)
       └────┬────┘
            │
            │ GND urządzenia
            │
            ▼
       ┌─────────┐
       │         │
       │  DRAIN  │ ◀── tu podłączasz GND urządzenia
       │         │
       │ IRLZ44N │
       │  N-CH   │
       │         │
       │  GATE ──┼─────┐
       │         │     │
       │ SOURCE  │   [10k] pull-down
       │         │     │
       └────┬────┘    GND
            │
            │
            ▼
       GND GŁÓWNA (masa akumulatora)


STEROWANIE Z GPIO:

    GPIO (D1 lub D2)
            │
            └──────────▶ GATE
            │
          [10k]
            │
           GND


LOGIKA:
  GPIO = HIGH (3.3V)  ──▶  MOSFET ON   ──▶  GND połączone  ──▶  DZIAŁA
  GPIO = LOW  (0V)    ──▶  MOSFET OFF  ──▶  GND odcięte    ──▶  WYŁĄCZONE
  GPIO = Hi-Z (reset) ──▶  pull-down   ──▶  MOSFET OFF     ──▶  WYŁĄCZONE


PINOUT IRLZ44N (TO-220, widok od przodu - strona z napisem):

       ┌─────────────┐
       │   IRLZ44N   │
       │             │
       │  ┌───────┐  │
       │  │       │  │
       │  │ metal │  │  ◀── radiator (połączony z Drain)
       │  │       │  │
       │  └───────┘  │
       └──┬──┬──┬────┘
          │  │  │
          │  │  │
          G  D  S
         (1)(2)(3)

    Pin 1 (G) - Gate   - sterowanie z GPIO + 10k do GND
    Pin 2 (D) - Drain  - GND urządzenia (urządzeń)
    Pin 3 (S) - Source - GND główna (masa akumulatora)


PARAMETRY IRLZ44N (logic-level MOSFET):
  • V_GS(th) = 1-2V      (włącza się przy 3.3V z GPIO - OK!)
  • R_DS(on) = 0.065 Ω   (bardzo niskie straty)
  • I_D = 47A            (dużo więcej niż potrzeba)
  • V_DS = 55V           (dużo więcej niż potrzeba)
  • Obudowa TO-220       (duża, łatwa do lutowania)
```

### Schemat Sterowania Buzzerem (bezpośrednio do GPIO)

```
BUZZER AKTYWNY - PODŁĄCZENIE
════════════════════════════

    3.3V (z XIAO, zasilanie stałe)
     │
     │ (+)
     ▼
┌────┴────┐
│  BUZZER │
│ aktywny │
│(zielony)│
└────┬────┘
     │
     │ (S) sygnał
     ▼
  D0 (GPIO1)  ◀── sterowanie ON/OFF
     
     │ (-)
     ▼
GND główna


PINY MODUŁU BUZZERA:
  (+) = VCC  ──▶ 3.3V z XIAO
  (-) = GND  ──▶ GND główna (bezpośrednio, nie przez MOSFET!)
  (S) = SIG  ──▶ D0 (GPIO1)

STEROWANIE:
  GPIO1 = HIGH ──▶ buzzer włączony (piszczy)
  GPIO1 = LOW  ──▶ buzzer wyłączony (cicho)

POBÓR PRĄDU: ~10-15mA (bezpiecznie dla GPIO ESP32, max 40mA)
```

**Moduł z buzzerem aktywnym (zielony)** - Botland 4526:
- Pobór prądu ~10-15mA - bezpiecznie w zakresie GPIO ESP32 (max 40mA)
- Piny: S (sygnał), - (GND), + (VCC 3.3V)
- GPIO HIGH = buzzer włączony, GPIO LOW = buzzer wyłączony
- **Nie wymaga tranzystora** - można podłączyć bezpośrednio

---

## Zarządzanie Energią (Power Management)

### Strategia Oszczędzania Energii

System wykorzystuje dwa MOSFETy N-channel (IRLZ44N) do odcinania masy peryferiów (low-side switching):

| MOSFET | GPIO | Funkcja | Kontrolowane urządzenia | Logika |
|--------|------|---------|------------------------|--------|
| Q1 | D1 (GPIO2) | PWR_5V_EN | GND dla Pololu + PMS7003 | HIGH=ON |
| Q2 | D2 (GPIO3) | PWR_3V3_EN | GND dla SHTC3, BMP280, OLED | HIGH=ON |

### Tryby Pracy Systemu

| Tryb | Q1 (5V) | Q2 (3.3V) | ESP32 | Opis |
|------|---------|-----------|-------|------|
| **FULL_MEASUREMENT** | ON | ON | Active | Pełny pomiar (PM + I2C) - co N cykli |
| **QUICK_MEASUREMENT** | OFF | ON | Active | Tylko I2C + ostatni PM - co 15 min |
| **DEEP_SLEEP** | OFF | OFF | Deep Sleep | Minimalne zużycie |

### Adaptacyjny Pomiar PMS (v1.4)

**NOWA ARCHITEKTURA:** Stały interwał wybudzenia 15 minut, adaptacyjny pomiar PMS.

```
INTERWAŁ WYBUDZENIA: STAŁY 15 MINUT
===================================

Co 15 minut ESP32 się budzi i wykonuje:
- ZAWSZE: Odczyt I2C (temp, wilg, ciśnienie) + Wyświetlanie + LoRa TX
- ADAPTACYJNIE: Co N-ty cykl włączamy PMS7003 (30s rozgrzewki)

PM2.5 [µg/m³]     Jakość           Co ile cykli PMS    Efektywny interwał PM
--------------------------------------------------------------------------------
0 - 15            DOBRA            co 8 cykli          8 × 15min = 2h
15 - 35           UMIARKOWANA      co 4 cykle          4 × 15min = 1h
35 - 75           NIEZDROWA        co 2 cykle          2 × 15min = 30min
> 75              ZŁA              co każdy cykl       1 × 15min = 15min + ALARM!
```

### Typy Wybudzeń

| Wybudzenie | Działanie | Q1 (5V) | Q2 (3.3V) | Czas |
|------------|-----------|---------|-----------|------|
| **TIMER** (co 15min) | Quick lub Full (adaptacyjnie) | OFF/ON | ON | 5s / 35s |
| **BUTTON** | Quick + restart cyklu 15min | OFF | ON | ~5s |
| **POWER_ON** | Full (zawsze) | ON | ON | ~35s |

### Zmienne RTC (przeżywają deep sleep)

```cpp
RTC_DATA_ATTR uint16_t lastPM1_0 = 0;       // Ostatni pomiar PM1.0
RTC_DATA_ATTR uint16_t lastPM2_5 = 0;       // Ostatni pomiar PM2.5
RTC_DATA_ATTR uint16_t lastPM10 = 0;        // Ostatni pomiar PM10
RTC_DATA_ATTR uint8_t pmsSkipCounter = 0;   // Licznik do następnego PMS
RTC_DATA_ATTR uint8_t loraSeqNum = 0;       // Numer sekwencji LoRa
RTC_DATA_ATTR bool historyChanged = false;  // Flaga zmiany historii
```

### Algorytm Adaptacyjny v1.4 - Schemat

```
CYKL PRACY Z ADAPTACYJNYM PMS
═════════════════════════════

                    [WAKEUP co 15 min]
                           │
           ┌───────────────┼───────────────┐
           │               │               │
       BUTTON          TIMER           POWER_ON
           │               │               │
           ▼               ▼               ▼
    ┌────────────┐   shouldMeasurePMS()?   ┌────────────┐
    │   QUICK    │         │               │   FULL     │
    │ I2C only   │    ┌────┴────┐          │ I2C + PMS  │
    └─────┬──────┘   TAK       NIE         └─────┬──────┘
          │           │         │                │
          │           ▼         ▼                │
          │      ┌────────┐ ┌────────┐           │
          │      │  FULL  │ │ QUICK  │           │
          │      │I2C+PMS │ │I2C only│           │
          │      └───┬────┘ └───┬────┘           │
          │          │          │                │
          └──────────┴────┬─────┴────────────────┘
                          │
                          ▼
                  Wyświetl na OLED
                  (2 cykle = 22s)
                          │
                          ▼
                     LoRa TX
                  (świeże I2C +
                   ostatni/nowy PM)
                          │
                          ▼
                  ┌───────────────┐
                  │  DEEP SLEEP   │
                  │   15 minut    │
                  │  Q1=OFF       │
                  │  Q2=OFF       │
                  └───────────────┘
```

### Quick Measurement (bez PMS)

```
performQuickMeasurement()
═════════════════════════
1. Q2=ON (3.3V dla I2C)
2. Init I2C + czujniki
3. Odczyt SHTC3 (temp, wilg)
4. Odczyt BMP280 (ciśnienie)
5. PM = wartości z RTC (lastPM2_5, lastPM10)
6. Wyświetl na OLED (2 cykle = 22s)
7. LoRa TX (świeże I2C + ostatni PM)
8. Q2=OFF
9. Deep sleep 15 min

Czas: ~27s (bez 30s rozgrzewki PMS!)
```

### Full Measurement (z PMS)

```
performFullMeasurement()
════════════════════════
1. Q1=ON, Q2=ON (5V + 3.3V)
2. Init I2C + czujniki
3. Init PMS7003
4. Rozgrzewka PMS 30s (animacja wentylatora)
5. Odczyt SHTC3, BMP280, PMS7003
6. Zapisz PM do RTC
7. Ustaw pmsSkipCounter (adaptacyjnie)
8. Dodaj do historii
9. Alarm jeśli PM2.5 > 75
10. Wyświetl na OLED (2 cykle = 22s)
11. LoRa TX
12. Zapisz historię do NVS (jeśli się zmieniła)
13. Q1=OFF, Q2=OFF
14. Deep sleep 15 min

Czas: ~57s (30s rozgrzewka + 27s)
```

### Kod Adaptacyjnego Zarządzania Energią (v1.4)

```cpp
// ============================================
// DEEP SLEEP - Konfiguracja
// ============================================
#define WAKEUP_INTERVAL_SEC      900       // 15 minut - stały interwał wybudzenia
#define WAKEUP_INTERVAL_US       (WAKEUP_INTERVAL_SEC * 1000000ULL)
#define WAKEUP_BUTTON_PIN        GPIO_NUM_4
#define PRE_SLEEP_DISPLAY_CYCLES 2         // 2 cykle ekranów = 22s

// Adaptacyjny pomiar PMS - co ile cykli 15min włączać PMS7003
#define PMS_SKIP_GOOD            8   // PM<15:  co 8 cykli = 2h
#define PMS_SKIP_MODERATE        4   // PM<35:  co 4 cykle = 1h
#define PMS_SKIP_UNHEALTHY       2   // PM<75:  co 2 cykle = 30min
#define PMS_SKIP_BAD             1   // PM>=75: każdy cykl = 15min

// Progi PM2.5
#define PM25_GOOD           15      // µg/m³
#define PM25_MODERATE       35      // µg/m³
#define PM25_UNHEALTHY      75      // µg/m³

// ============================================
// ZMIENNE RTC (przeżywają deep sleep)
// ============================================
RTC_DATA_ATTR uint16_t lastPM1_0 = 0;
RTC_DATA_ATTR uint16_t lastPM2_5 = 0;
RTC_DATA_ATTR uint16_t lastPM10 = 0;
RTC_DATA_ATTR uint8_t pmsSkipCounter = 0;
RTC_DATA_ATTR uint8_t loraSeqNum = 0;
RTC_DATA_ATTR bool historyChanged = false;

// ============================================
// FUNKCJE ADAPTACYJNEGO PMS
// ============================================
uint8_t getPmsSkipCount(uint16_t pm25) {
    if (pm25 < PM25_GOOD)       return PMS_SKIP_GOOD;       // 8
    if (pm25 < PM25_MODERATE)   return PMS_SKIP_MODERATE;   // 4
    if (pm25 < PM25_UNHEALTHY)  return PMS_SKIP_UNHEALTHY;  // 2
    return PMS_SKIP_BAD;                                     // 1
}

bool shouldMeasurePMS() {
    if (lastPM2_5 == 0 && lastPM10 == 0) return true;  // Pierwszy boot
    if (pmsSkipCounter > 0) pmsSkipCounter--;
    return (pmsSkipCounter == 0);
}

// ============================================
// SETUP - Rozgałęzienie wg przyczyny wybudzenia
// ============================================
void setup() {
    setupPowerControl();
    setupBuzzer();
    setupLoRa();
    
    esp_sleep_wakeup_cause_t wakeup = esp_sleep_get_wakeup_cause();
    
    switch (wakeup) {
        case ESP_SLEEP_WAKEUP_EXT0:   // BUTTON
            handleButtonWakeup();
            break;
        case ESP_SLEEP_WAKEUP_TIMER:  // TIMER (co 15 min)
            if (shouldMeasurePMS()) {
                performFullMeasurement();
            } else {
                performQuickMeasurement();
            }
            break;
        default:                       // POWER ON
            pmsSkipCounter = 0;
            performFullMeasurement();
            break;
    }
}
```

### Pobór Prądu - Szczegółowa Analiza (v1.4 z optymalizacją CPU)

| Komponent | Tryb aktywny | Odcięty (Q OFF) | Uwagi |
|-----------|--------------|-----------------|-------|
| ESP32-S3 @ 80MHz | **10mA** | - | Zoptymalizowane (było 25mA @ 240MHz) |
| ESP32-S3 deep sleep | 10μA | - | RTC Fast Mem OFF |
| PMS7003 + Pololu | 102mA | **0mA** | Q1 odcina GND |
| SHTC3 + BMP280 | 1mA | **0mA** | Q2 odcina GND |
| OLED SSD1306 | 20mA | **0mA** | Q2 odcina GND |
| SX1262 TX | 120mA | 1μA | Krótki burst |
| SX1262 standby | 2mA | - | Podczas pracy |

### Zużycie Energii - Quick vs Full Measurement (v1.4 + CPU 80MHz)

```
FULL MEASUREMENT (z PMS7003):
=============================
Prąd podczas rozgrzewki: ESP(10) + PMS+Pololu(102) + I2C(1) + OLED(20) + LoRa_stby(2) = 135mA
Prąd podczas wyświetlania: ESP(10) + I2C(1) + OLED(20) + LoRa_stby(2) = 33mA
Prąd podczas TX LoRa: ESP(10) + OLED(20) + LoRa_TX(120) = 150mA

1. Startup + init (2s):          2s × 12mA  = 0.007 mAh
2. Rozgrzewka PMS (30s):        30s × 135mA = 1.125 mAh
3. Pomiar I2C + PMS (1s):        1s × 135mA = 0.038 mAh
4. Wyświetlanie OLED (22s):     22s × 33mA  = 0.202 mAh
5. Transmisja LoRa (0.1s):     0.1s × 150mA = 0.004 mAh
6. Shutdown (0.5s):            0.5s × 10mA  = 0.001 mAh
                                -----------------------
                          SUMA FULL: ~1.38 mAh (~56s)

QUICK MEASUREMENT (bez PMS7003):
================================
Prąd podczas pomiaru: ESP(10) + I2C(1) + OLED(20) + LoRa_stby(2) = 33mA

1. Startup + init (2s):          2s × 12mA  = 0.007 mAh
2. Pomiar I2C (1s):              1s × 33mA  = 0.009 mAh
3. Wyświetlanie OLED (22s):     22s × 33mA  = 0.202 mAh
4. Transmisja LoRa (0.1s):     0.1s × 150mA = 0.004 mAh
5. Shutdown (0.5s):            0.5s × 10mA  = 0.001 mAh
                                -----------------------
                          SUMA QUICK: ~0.22 mAh (~26s)

PORÓWNANIE Z POPRZEDNIĄ WERSJĄ (CPU 240MHz):
- Full:  było 1.68 mAh → jest 1.38 mAh (oszczędność 18%)
- Quick: było 0.34 mAh → jest 0.22 mAh (oszczędność 35%)
```

### Szacowany Czas Pracy Baterii 4000mAh (v1.4 + optymalizacje)

```
ARCHITEKTURA v1.4: Stały interwał 15min, adaptacyjny pomiar PMS, CPU 80MHz
==========================================================================

Założenia:
- Bateria: 4000mAh (użyteczne ~3600mAh z 90% DOD)
- Full measurement: ~1.38 mAh (z PMS)
- Quick measurement: ~0.22 mAh (bez PMS)
- Deep sleep (15min): 0.0025 mAh (10μA × 0.25h)
- Wybudzenia dziennie: 96 (24h ÷ 15min)

POWIETRZE CZYSTE (PM < 15 µg/m³) - PMS co 8 cykli = 2h:
-------------------------------------------------------
Full: 12×/dzień, Quick: 84×/dzień
Zużycie: (12 × 1.38) + (84 × 0.22) + (96 × 0.0025) = 35.3 mAh/dzień
Czas pracy: 3600 / 35.3 = 102 dni = ~3.4 miesiąca

POWIETRZE UMIARKOWANE (PM 15-35 µg/m³) - PMS co 4 cykle = 1h:
-------------------------------------------------------------
Full: 24×/dzień, Quick: 72×/dzień
Zużycie: (24 × 1.38) + (72 × 0.22) + (96 × 0.0025) = 49.2 mAh/dzień
Czas pracy: 3600 / 49.2 = 73 dni = ~2.4 miesiące

POWIETRZE NIEZDROWE (PM 35-75 µg/m³) - PMS co 2 cykle = 30min:
--------------------------------------------------------------
Full: 48×/dzień, Quick: 48×/dzień
Zużycie: (48 × 1.38) + (48 × 0.22) + (96 × 0.0025) = 77.1 mAh/dzień
Czas pracy: 3600 / 77.1 = 47 dni = ~1.6 miesiąca

POWIETRZE ZŁE (PM > 75 µg/m³) - PMS każdy cykl = 15min:
-------------------------------------------------------
Full: 96×/dzień, Quick: 0×/dzień
Zużycie: (96 × 1.38) + (0 × 0.22) + (96 × 0.0025) = 132.7 mAh/dzień
Czas pracy: 3600 / 132.7 = 27 dni = ~0.9 miesiąca
```

### Podsumowanie Czasu Pracy Baterii (v1.4 + optymalizacje)

| Jakość powietrza | Pomiar PMS | Full/Quick dziennie | Czas pracy |
|------------------|------------|---------------------|------------|
| **Dobra** (PM<15) | co 2h | 12F + 84Q | **~3.4 miesiąca** |
| **Umiarkowana** (PM 15-35) | co 1h | 24F + 72Q | **~2.4 miesiące** |
| **Niezdrowa** (PM 35-75) | co 30min | 48F + 48Q | **~1.6 miesiąca** |
| **Zła** (PM>75) | co 15min | 96F + 0Q | **~1 miesiąc** |

> **UWAGA v1.4 z optymalizacjami:** 
> - CPU 80MHz (zamiast 240MHz) oszczędza ~15-35% energii
> - RTC Fast Memory OFF oszczędza ~4µA w deep sleep
> - W typowych warunkach domowych (czyste powietrze): **~3-4 miesiące**
> - Przy zanieczyszczonym powietrzu: **~1-2 miesiące**

---

## Adresy I2C

| Urządzenie | Adres domyślny | Adres alternatywny |
|------------|----------------|-------------------|
| SHTC3 | 0x70 | - (stały) |
| BMP280 | 0x77 | 0x76 (SDO->GND) |
| OLED SSD1306 | 0x3C | 0x3D (zależy od modułu) |

**Skan I2C** - kod do weryfikacji adresów znajduje się w sekcji oprogramowania.

---

## Połączenia PMS7003 (Adapter IDC-microUSB z CH340G)

### Opis Adaptera

Adapter IDC 10pin 1,27mm - microUSB dla czujnika PMS7003 (Botland DNG-13445):
- Konwertuje złącze IDC 10pin czujnika PMS7003 na wygodne piny i microUSB
- Wbudowany układ **CH340G** (konwerter UART<->USB)
- Gniazdo **microUSB** - może służyć do zasilania 5V lub komunikacji z PC
- Wyprowadzenia goldpin: **5V, GND, RX, TX**

**Link:** https://botland.com.pl/czujniki-czystosci-powietrza/13445-adapter-idc-10pin-127mm-microusb-dla-czujnika-pms7003-5904422372293.html

### Pinout Adaptera PMS7003 (goldpiny)

```
┌───────────────────────────────────────┐
│     ADAPTER PMS7003 (CH340G)          │
│                                       │
│  ┌───────────────────────────────┐    │
│  │      Gniazdo microUSB         │    │  ◀── Zasilanie 5V lub USB do PC
│  └───────────────────────────────┘    │
│                                       │
│        [5V] [GND] [RX] [TX]           │  ◀── Goldpiny do wlutowania
│                                       │
│  ┌───────────────────────────────┐    │
│  │   Złącze IDC 10pin 1,27mm     │    │  ◀── Do PMS7003
│  └───────────────────────────────┘    │
│                                       │
└───────────────────────────────────────┘
```

### Połączenia Adaptera z XIAO (przez goldpiny)

| Pin Adaptera | Funkcja | Podłączyć do | Uwagi |
|--------------|---------|--------------|-------|
| **5V** | Zasilanie 5V | Pololu VOUT (5V) | Zasilanie czujnika |
| **GND** | Masa | **Q1 Drain** | Przez MOSFET! (low-side) |
| **RX** | UART RX (wejście adaptera) | XIAO D6 (GPIO43/TX) | TX XIAO -> RX adaptera |
| **TX** | UART TX (wyjście adaptera) | XIAO D7 (GPIO44/RX) | TX adaptera -> RX XIAO |

### WAŻNE: Nie używać microUSB do zasilania!

> **W tym projekcie NIE używamy gniazda microUSB adaptera!**
>
> Zasilanie 5V dostarczamy przez pin **5V** z przetwornicy Pololu.
> Masę (GND) prowadzimy przez MOSFET Q1 (low-side switching).
>
> Gniazdo microUSB można użyć tylko do testowania z komputerem PC
> (wtedy adapter działa jako konwerter USB-UART).

### Stary pinout IDC (dla referencji)

```
Oryginalny connector IDC 10pin PMS7003 (1.27mm):
┌─────────────────────┐
│ 1  2  3  4  5       │
│ 6  7  8  9  10      │
└─────────────────────┘

Pin 1: VCC (5V)
Pin 2: GND
Pin 3: SET (sterowanie sleep, HIGH=praca, LOW=sleep)
Pin 4: RX (wejście UART, 3.3V TTL)
Pin 5: TX (wyjście UART, 3.3V TTL)
Pin 6: RESET (aktywny LOW)
Pin 7-10: NC/GND

Adapter wyprowadza tylko: 5V, GND, RX, TX (piny SET i RESET niedostępne)
```

### Połączenia Przetwornicy Pololu S7V7F5

| Pololu Pin | Funkcja | Podłączony do | Uwagi |
|------------|---------|---------------|-------|
| VIN | Wejście 2.7-11.8V | Akumulator BAT+ (3.7V) | Bezpośrednio do baterii |
| GND | Masa | **Q1 Drain** | **PRZEZ MOSFET!** |
| VOUT | Wyjście 5V | PMS7003 VCC (adapter 5V pin) | Zasilanie czujnika |

> **WAŻNA ZMIANA:** GND Pololu idzie przez Q1 (nie bezpośrednio do masy!)
> Dzięki temu wyłączenie Q1 całkowicie odcina przetwornicę i eliminuje
> pobór prądu jałowego (~2mA).

**Pinout Pololu S7V7F5:**
```
┌─────────────────────┐
│     POLOLU S7V7F5   │
│                     │
│  [VIN] [GND] [VOUT] │
│    │     │     │    │
│    │     │     └──▶ 5V do PMS7003 VCC
│    │     │
│    │     └────────▶ Q1 Drain (PRZEZ MOSFET!)
│    │
│    └──────────────▶ BAT+ (bezpośrednio)
└─────────────────────┘
```

---

## Lista Połączeń (Instrukcja Montażu)

### 1. ZASILANIE GŁÓWNE

| Z | Do | Uwagi |
|---|-----|-------|
| Akumulator (+) | XIAO BAT+ | Zasilanie główne |
| Akumulator (-) | GND główna | Masa główna |
| Akumulator (+) | Pololu VIN | Zasilanie przetwornicy |

> **UWAGA:** GND Pololu NIE idzie bezpośrednio do masy - idzie przez Q1!

### 2. MOSFET Q1 (IRLZ44N) - sterowanie linii 5V (Pololu + PMS7003)

| Pin IRLZ44N | Podłączyć do | Uwagi |
|-------------|--------------|-------|
| G (Gate) | XIAO D1 (GPIO2) | Sterowanie |
| G (Gate) | Rezystor 10k -> GND | Pull-down (domyślnie OFF) |
| D (Drain) | Pololu GND + PMS7003 GND | **WSPÓLNIE!** |
| S (Source) | GND główna | Masa systemu |

```
Schemat połączeń Q1:

   Pololu GND ────┐
                  │
   PMS7003 GND ───┴───▶ Q1 Drain
                            │
          GPIO2 (D1) ───────┼───▶ Q1 Gate
                  │
                [10k]
                  │
                 GND

                            Q1 Source ───▶ GND główna
```

### 3. MOSFET Q2 (IRLZ44N) - sterowanie czujników I2C

| Pin IRLZ44N | Podłączyć do | Uwagi |
|-------------|--------------|-------|
| G (Gate) | XIAO D2 (GPIO3) | Sterowanie |
| G (Gate) | Rezystor 10k -> GND | Pull-down (domyślnie OFF) |
| D (Drain) | GND: SHTC3, BMP280, OLED | Wspólna masa czujników |
| S (Source) | GND główna | Masa systemu |

```
Schemat połączeń Q2:

   SHTC3 GND ─────┐
                  │
   BMP280 GND ────┼───▶ Q2 Drain
                  │
   OLED GND ──────┘
                            │
          GPIO3 (D2) ───────┼───▶ Q2 Gate
                  │
                [10k]
                  │
                 GND

                            Q2 Source ───▶ GND główna
```

### 4. PRZETWORNICA POLOLU S7V7F5

| Pin Pololu | Podłączyć do | Uwagi |
|------------|--------------|-------|
| VIN | Akumulator BAT+ | Bezpośrednio! |
| GND | **Q1 Drain** | **PRZEZ MOSFET!** |
| VOUT | PMS7003 VCC (adapter pin 5V) | Zasilanie 5V |

### 5. PMS7003 (czujnik pyłu) - przez adapter IDC-microUSB

| Pin Adaptera | Podłączyć do | Uwagi |
|--------------|--------------|-------|
| 5V | Pololu VOUT | Zasilanie 5V |
| GND | **Q1 Drain** | **PRZEZ MOSFET!** |
| RX | XIAO D6 (GPIO43/TX) | UART TX->RX |
| TX | XIAO D7 (GPIO44/RX) | UART RX<-TX |

### 6. MAGISTRALA I2C (wspólna dla 3 urządzeń)

| Sygnał | XIAO Pin | SHTC3 | BMP280 | OLED |
|--------|----------|-------|--------|------|
| SDA | D4 (GPIO5) | SDA | SDA | SDA |
| SCL | D5 (GPIO6) | SCL | SCL | SCL |
| VCC | 3V3 | VCC | VCC | VCC |
| GND | **Q2 Drain** | GND | GND | GND |

### 7. BUZZER

| Pin Buzzer | Podłączyć do | Uwagi |
|------------|--------------|-------|
| S (Signal) | XIAO D0 (GPIO1) | Sterowanie |
| - (GND) | GND główna | Masa (bezpośrednio!) |
| + (VCC) | XIAO 3V3 | Zasilanie |

### 8. PRZYCISK WYBUDZENIA

| Przycisk | Podłączyć do | Uwagi |
|----------|--------------|-------|
| Pin 1 | XIAO D3 (GPIO4) | Wbudowany pull-up ESP32 |
| Pin 2 | GND główna | Przycisk NO (normalnie otwarty) |

**Działanie:** Naciśnięcie przycisku łączy GPIO4 z GND -> wybudzenie z deep sleep -> wyświetlenie danych na OLED.

### 9. PODSUMOWANIE PINÓW XIAO

| Pin XIAO | GPIO | Funkcja | Podłączone do |
|----------|------|---------|---------------|
| D0 | GPIO1 | BUZZER | Buzzer S |
| D1 | GPIO2 | PWR_5V_EN | Q1 Gate + 10k->GND |
| D2 | GPIO3 | PWR_3V3_EN | Q2 Gate + 10k->GND |
| D3 | GPIO4 | BUTTON_WAKE | Przycisk NO do GND |
| D4 | GPIO5 | I2C SDA | SHTC3, BMP280, OLED |
| D5 | GPIO6 | I2C SCL | SHTC3, BMP280, OLED |
| D6 | GPIO43 | UART TX | PMS7003 RX |
| D7 | GPIO44 | UART RX | PMS7003 TX |
| 3V3 | - | 3.3V | Czujniki I2C VCC, Buzzer + |
| GND | - | GND | Q1 Source, Q2 Source, Buzzer -, Przycisk |
| BAT+ | - | Bateria + | Akumulator +, Pololu VIN |

### 10. DIAGRAM POŁĄCZEŃ - WIDOK CAŁOŚCIOWY

```
                        AKUMULATOR 3.7V Li-Po
                             (+)     (-)
                              │       │
              ┌───────────────┤       └────────────┐
              │               │                    │
              ▼               ▼                    ▼
          XIAO BAT+      Pololu VIN           GND główna
                              │                    │
                         [przetwornica]            │
                              │                    ├───▶ Q1 Source
                         VOUT (5V)                 │
                              │                    ├───▶ Q2 Source
              ┌───────────────┤                    │
              │               │                    ├───▶ Buzzer (-)
              ▼               ▼                    │
        PMS7003 VCC      Pololu GND                └───▶ XIAO GND
              │               │
        PMS7003 GND           │
              │               │
              └───────┬───────┘
                      │
                 Q1 DRAIN ◀──── D1 (GPIO2) + [10k pull-down]
                      │
                 Q1 SOURCE ───▶ GND główna


              XIAO 3V3
                  │
     ┌────────────┼────────────┐
     │            │            │
     ▼            ▼            ▼
 SHTC3 VCC   BMP280 VCC   OLED VCC
     │            │            │
 SHTC3 GND   BMP280 GND   OLED GND
     │            │            │
     └──────┬─────┴──────┬─────┘
            │
       Q2 DRAIN ◀──── D2 (GPIO3) + [10k pull-down]
            │
       Q2 SOURCE ───▶ GND główna
```

### 11. CHECKLIST PRZED WŁĄCZENIEM

- [ ] Brak zwarć między VCC a GND (multimetr - tryb ciągłości)
- [ ] Q1 Gate ma rezystor 10k do GND
- [ ] Q2 Gate ma rezystor 10k do GND
- [ ] Pololu VIN podłączony do BAT+ (bezpośrednio)
- [ ] **Pololu GND podłączony do Q1 Drain (NIE do GND głównej!)**
- [ ] **PMS7003 GND podłączony do Q1 Drain (NIE do GND głównej!)**
- [ ] Czujniki I2C GND do Q2 Drain (nie do GND głównej!)
- [ ] Q1 Source i Q2 Source do GND głównej
- [ ] Buzzer (-) bezpośrednio do GND głównej
- [ ] Przycisk podłączony między D3 (GPIO4) a GND
- [ ] Antena LoRa podłączona do SX1262

---

## Wymagane Biblioteki Arduino

```cpp
// Komunikacja LoRa
#include <RadioLib.h>

// Czujniki I2C
#include <Wire.h>
#include <Adafruit_SHTC3.h>        // lub SensirionI2CShtc3
#include <Adafruit_BMP280.h>

// Wyświetlacz OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Czujnik pyłu (UART)
// Biblioteka PMS lub ręczna obsługa protokołu
```

---

## Fazy Projektu

### Faza 1: Koncepcja i Planowanie (AKTUALNA) ✓

**Status:** Zakończona

- [x] Lista komponentów hardware
- [x] Analiza kompatybilności napięć
- [x] Schemat połączeń
- [x] Analiza poboru prądu

### Faza 2: Montaż Sprzętu

**Cel:** Połączenie wszystkich elementów na płytce uniwersalnej.

**Zadania:**
- [ ] Przylutowanie goldpinów do XIAO
- [ ] Montaż magistrali I2C (3.3V, GND, SDA, SCL)
- [ ] Podłączenie czujników I2C
- [ ] Podłączenie PMS7003 (UART + zasilanie 5V)
- [ ] Podłączenie OLED
- [ ] Podłączenie buzzera (bezpośrednio do GPIO)
- [ ] Podłączenie akumulatora
- [ ] Test ciągłości połączeń (multimetr)

### Faza 3: Implementacja Oprogramowania

**Baza kodu:** `air_condition.ino` - działająca komunikacja SX1262

**Zadania:**
- [ ] Test skanera I2C
- [ ] Odczyt SHTC3 (temperatura, wilgotność)
- [ ] Odczyt BMP280 (ciśnienie, temperatura)
- [ ] Odczyt PMS7003 (PM1.0, PM2.5, PM10)
- [ ] Wyświetlanie danych na OLED
- [ ] Integracja z komunikacją LoRa
- [ ] Alarmy (buzzer przy przekroczeniu progów)
- [ ] Tryby oszczędzania energii

### Faza 4: Integracja i Testy

**Zadania:**
- [ ] Kalibracja czujników
- [ ] Testy zasięgu LoRa
- [ ] Optymalizacja zużycia energii
- [ ] Testy długotrwałe

---

## Specyfikacja Techniczna

### Parametry LoRa (aktualna konfiguracja)

| Parametr | Wartość |
|----------|---------|
| Częstotliwość | 868.0 MHz |
| Szerokość pasma (BW) | 125.0 kHz |
| Spreading Factor (SF) | 7 |
| Coding Rate (CR) | 5 (4/5) |
| Sync Word | 0x34 |
| Moc nadawania | 14 dBm |
| Preambuła | 8 symboli |

### Zakresy Pomiarowe Czujników

| Czujnik | Parametr | Zakres | Dokładność |
|---------|----------|--------|------------|
| SHTC3 | Temperatura | -40°C do +125°C | ±0.2°C |
| SHTC3 | Wilgotność | 0-100% RH | ±2% RH |
| BMP280 | Ciśnienie | 300-1100 hPa | ±1 hPa |
| BMP280 | Temperatura | -40°C do +85°C | ±1°C |
| PMS7003 | PM1.0/2.5/10 | 0-500 μg/m³ | ±10% |

---

## Uwagi i Ograniczenia

1. **PMS7003 - rozgrzewka**: Czujnik wymaga ~30 sekund rozgrzewki przed wiarygodnym pomiarem
2. **PMS7003 - żywotność**: Wentylator ma ograniczoną żywotność (~8000h), nie trzymać włączonego ciągle
3. **PMS7003 - zasilanie**: VCC z Pololu (5V), GND przez MOSFET Q1 (low-side switching)
4. **I2C pull-up**: Moduły DFRobot mają wbudowane rezystory pull-up, nie trzeba dodawać zewnętrznych
5. **Bateria**: XIAO ma wbudowany układ ładowania Li-Po (100mA), ładowanie przez USB-C
6. **Antena LoRa**: Niezbędna do prawidłowej pracy SX1262 - podłączyć przed włączeniem!
7. **Pololu S7V7F5**: VIN do BAT+ (bezpośrednio), **GND przez Q1** - wyłączenie Q1 całkowicie odcina przetwornicę
8. **MOSFETy IRLZ44N - logika**: N-channel low-side: **GPIO HIGH = urządzenie ON**, GPIO LOW = OFF
9. **Pull-down rezystory (10k)**: Zapewniają domyślnie WYŁĄCZONE peryferia przy starcie/resecie ESP32
10. **Deep Sleep**: Przy wybudzeniu GPIO wracają do stanu domyślnego - pull-down utrzymuje OFF
11. **"Pływająca masa"**: Gdy MOSFET OFF, peryferia mają VCC ale nie GND - bezpieczne, nie działa
12. **IRLZ44N obudowa TO-220**: Duża (wymaga miejsca), ale łatwa do lutowania na płytce uniwersalnej
13. **IRLZ44N parametry**: V_GS(th)=1-2V, R_DS(on)=0.065ohm, I_D=47A - nadmiarowe, ale pewne
14. **Eliminacja prądu jałowego Pololu**: GND Pololu przez Q1 = 0mA w trybie sleep (zamiast ~2mA)
15. **Przycisk wybudzenia (GPIO4)**: Używa wbudowanego pull-up ESP32, normalnie otwarty do GND
16. **Adaptacyjny pomiar PMS (v1.4)**: Stały interwał wybudzenia 15min, pomiar PMS co N cykli zależnie od PM2.5

---

## Następne Kroki

1. **Zakup brakujących elementów:**
   - Pololu S7V7F5 - przetwornica 5V
   - Rezystory 10kΩ (2 szt) - pull-down dla bramek MOSFET
   - Przycisk tact switch - wybudzenie z deep sleep
   - ~~IRLZ44N N-MOSFET~~ - **KUPIONY** (5 szt)
2. **Przygotowanie płytki uniwersalnej** - zaplanowanie układu z MOSFETami TO-220
3. **Lutowanie i montaż** - zgodnie ze schematem low-side switching
4. **Testy połączeń** - przed podłączeniem zasilania sprawdzić multimetrem
5. **Upload kodu testowego** - skan I2C, test power switching

---

## Historia Zmian

| Data | Wersja | Opis |
|------|--------|------|
| 2025-12-06 | 0.1 | Utworzenie dokumentu, wstępna specyfikacja |
| 2025-12-06 | 0.2 | Dodanie listy hardware, schematu połączeń, analizy kompatybilności i poboru prądu |
| 2025-12-06 | 0.3 | Dodanie przetwornicy Pololu S7V7F5 - rozwiązanie problemu zasilania 5V z baterii |
| 2025-12-06 | 0.4 | Dodanie MOSFETów AO3401 do sterowania zasilaniem peryferiów, sekcja Power Management |
| 2025-12-06 | 0.5 | **Zmiana MOSFET z AO3401 (P-ch) na IRLZ44N (N-ch logic-level)** - low-side switching, odwrócona logika sterowania (HIGH=ON), zaktualizowane schematy i kod |
| 2025-12-06 | 0.6 | Dodanie sekcji "Lista Połączeń (Instrukcja Montażu)" z checklistą przed włączeniem |
| 2025-12-06 | 0.7 | Aktualizacja o adapter PMS7003 IDC-microUSB (CH340G) - pinout adaptera (5V/GND/RX/TX), połączenia przez goldpiny |
| 2025-12-06 | 0.8 | **Buzzer: zmiana na moduł aktywny (zielony)** - bezpośrednio do GPIO bez tranzystora, dodano link Botland |
| 2025-12-06 | 0.9 | Dodanie sekcji "Kontekst dla Kolejnej Sesji" z pełnym podsumowaniem stanu projektu |
| 2025-12-30 | 1.0 | **WAŻNA ZMIANA: GND Pololu przez Q1** - eliminacja prądu jałowego przetwornicy (~2mA), zaktualizowane wszystkie schematy ASCII, poprawiona czytelność diagramów |
| 2025-12-30 | 1.1 | **Adaptacyjny algorytm pomiarów** - interwał zależny od PM2.5 (2h/1h/30min/15min), przycisk wybudzenia (GPIO4), usunięcie zduplikowanej sekcji poboru prądu, zaktualizowana sekcja kontekstu |
| 2025-12-30 | 1.2 | **Przywrócenie znaków Unicode** w schematach ASCII dla lepszej czytelności (┌┐└┘│─├┤┬┴▶◀▼) |
| 2025-12-31 | 1.3 | **Implementacja air_condition.ino v1.4** - Adaptacyjny Deep Sleep: stały interwał 15min, Quick vs Full measurement, zmienne RTC_DATA_ATTR, optymalizacja NVS |
| 2025-12-31 | 1.4 | **Optymalizacje energetyczne** - CPU 80MHz (oszczędność ~15-35%), RTC Fast Memory OFF w deep sleep, zaktualizowane obliczenia czasu pracy baterii (3.4 miesiąca przy czystym powietrzu) |

---

## Kontekst dla Kolejnej Sesji (Session Handoff)

> **CEL:** Ta sekcja zawiera wszystkie informacje niezbędne do kontynuacji pracy w nowej sesji AI.
> Przeczytaj tę sekcję na początku kolejnej sesji, aby szybko wdrożyć się w projekt.

### Status Projektu (2025-12-31)

| Faza | Status | Uwagi |
|------|--------|-------|
| Faza 1: Koncepcja | **ZAKOŃCZONA** | Dokumentacja kompletna |
| Faza 2: Hardware | **W TOKU** | Czeka na zakup + montaż |
| Faza 3: Software | **ZAKOŃCZONA** | `air_condition.ino` v1.4 - pełna implementacja |
| Faza 4: Testy | DO REALIZACJI | Wymaga zmontowanego hardware |

### Co Jest Gotowe

1. **Dokumentacja techniczna** - PROJECT.md v1.3 zawiera:
   - Kompletną listę BOM z linkami Botland
   - Schematy połączeń (ASCII art)
   - Architekturę zasilania z low-side switching (IRLZ44N)
   - **Adaptacyjny pomiar PMS v1.4** (stały interwał 15min, Quick vs Full)
   - **Optymalizacje energii:** CPU 80MHz, RTC Fast Mem OFF
   - Analizę poboru prądu i żywotności baterii (3-4 miesiące)
   - Instrukcję montażu z checklistą

2. **Kod kompletny** - `air_condition.ino` v1.4 (~1870 linii):
   - Wszystkie czujniki: SHTC3, BMP280, PMS7003
   - Wyświetlacz OLED (3 ekrany: Main/Details/History)
   - Komunikacja LoRa (protokół binarny 23B)
   - **Adaptacyjny Deep Sleep** - Quick vs Full measurement
   - **Optymalizacje energii:** CPU 80MHz, RTC Fast Mem OFF
   - Zapis historii do NVS (pamięć nieulotna)
   - Alarmy buzzer przy PM2.5 > 75 µg/m³
   - Kompiluje się bez błędów (~430 KB flash)

3. **Zakupione elementy:**
   - XIAO ESP32-S3 Sense + Wio-SX1262 (B2B)
   - SHTC3 (DFRobot SEN0414)
   - BMP280 (DFRobot SEN0372)
   - PMS7003 + adapter IDC-microUSB (CH340G)
   - OLED 0.96" 128x64 I2C
   - Akumulator Li-Po 4000mAh
   - Moduł buzzera aktywnego (zielony)
   - **IRLZ44N N-MOSFET** (5 szt)

### Do Zakupu (PILNE)

| Element | Ilość | Link | Cena ~|
|---------|-------|------|-------|
| Pololu S7V7F5 | 1 szt | https://botland.com.pl/przetwornice-step-updown-pololu/1696-pololu-s7v7f5-przetwornica-step-upstep-down-5v-1a-5904422359898.html | ~25 zł |
| Rezystor 10k THT | 2 szt | dowolny sklep | ~1 zł |
| Przycisk tact switch | 1 szt | dowolny sklep | ~1 zł |

### Architektura v1.4 - Adaptacyjny Deep Sleep

```
NOWA ARCHITEKTURA (v1.4):
=========================
- Stały interwał wybudzenia: 15 MINUT
- Adaptacyjny POMIAR PMS (nie interwał!) zależny od PM2.5:

PM2.5 [µg/m³]     Jakość           PMS co N cykli    Efektywny interwał PM
--------------------------------------------------------------------------------
0 - 15            DOBRA            co 8 cykli        8 × 15min = 2h
15 - 35           UMIARKOWANA      co 4 cykle        4 × 15min = 1h
35 - 75           NIEZDROWA        co 2 cykle        2 × 15min = 30min
> 75              ZŁA              co każdy cykl     1 × 15min = 15min + ALARM!

TYPY POMIARÓW:
- QUICK: Tylko I2C (temp/wilg/ciśn) + ostatni PM z RTC + LoRa TX (~25s)
- FULL:  I2C + PMS7003 (30s rozgrzewki) + LoRa TX + zapis historii (~57s)

WYBUDZENIA:
- TIMER (co 15min): Quick lub Full (adaptacyjnie wg shouldMeasurePMS())
- BUTTON: Quick + restart cyklu 15min
- POWER_ON: Full (zawsze)
```

### Kluczowe Funkcje w Kodzie v1.4

```cpp
// Zmienne RTC (przeżywają deep sleep)
RTC_DATA_ATTR uint16_t lastPM1_0, lastPM2_5, lastPM10;  // Ostatnie PM
RTC_DATA_ATTR uint8_t pmsSkipCounter;                   // Licznik cykli
RTC_DATA_ATTR uint8_t loraSeqNum;                       // Sekwencja LoRa
RTC_DATA_ATTR bool historyChanged;                      // Flaga NVS

// Główne funkcje
uint8_t getPmsSkipCount(uint16_t pm25);    // 8/4/2/1 zależnie od PM
bool shouldMeasurePMS();                    // Czy uruchomić PMS w tym cyklu
void performQuickMeasurement();             // Tylko I2C + LoRa
void performFullMeasurement();              // I2C + PMS + LoRa + historia
void handleButtonWakeup();                  // Quick + restart cyklu
void displayPauseLoop(int cycles);          // Wyświetlanie przed snem
void prepareForDeepSleep(bool saveHistory); // Optymalizacja NVS
```

### Szacowany Czas Pracy Baterii 4000mAh (v1.4)

| Jakość powietrza | Pomiar PMS | Full/Quick dziennie | Czas pracy |
|------------------|------------|---------------------|------------|
| **Dobra** (PM<15) | co 2h | 12F + 84Q | **~2.7 miesiąca** |
| **Umiarkowana** (PM 15-35) | co 1h | 24F + 72Q | **~2 miesiące** |
| **Niezdrowa** (PM 35-75) | co 30min | 48F + 48Q | **~1.4 miesiąca** |
| **Zła** (PM>75) | co 15min | 96F + 0Q | **~1 miesiąc** |

> **UWAGA:** Quick measurement zużywa ~80% mniej energii niż Full!

### Pinout XIAO - Szybka Ściąga

```
| Pin  | GPIO  | Funkcja      | Podłączone do              |
|------|-------|--------------|----------------------------|
| D0   | GPIO1 | BUZZER       | Buzzer S (bezpośrednio)    |
| D1   | GPIO2 | PWR_5V_EN    | Q1 Gate + 10k->GND         |
| D2   | GPIO3 | PWR_3V3_EN   | Q2 Gate + 10k->GND         |
| D3   | GPIO4 | BUTTON_WAKE  | Przycisk NO do GND         |
| D4   | GPIO5 | I2C SDA      | SHTC3, BMP280, OLED        |
| D5   | GPIO6 | I2C SCL      | SHTC3, BMP280, OLED        |
| D6   | GPIO43| UART TX      | PMS7003 RX (adapter)       |
| D7   | GPIO44| UART RX      | PMS7003 TX (adapter)       |
```

### Schemat Zasilania - Szybki Podgląd

```
AKUMULATOR 3.7V
    │
    ├───▶ XIAO BAT+
    │
    ├───▶ Pololu VIN (bezpośrednio!)
    │
    └───▶ GND główna ──┬───▶ XIAO GND
                       │
                       ├───▶ Q1 Source
                       │
                       ├───▶ Q2 Source
                       │
                       ├───▶ Buzzer (-)
                       │
                       └───▶ Przycisk (pin 2)

Q1 (GPIO2): Pololu GND + PMS7003 GND ───▶ Q1 Drain

Q2 (GPIO3): SHTC3 GND + BMP280 GND + OLED GND ───▶ Q2 Drain
```

### Adresy I2C

| Urządzenie | Adres |
|------------|-------|
| SHTC3 | 0x70 |
| BMP280 | 0x77 |
| OLED SSD1306 | 0x3C |

### Kolejne Kroki (w kolejności)

#### ETAP A: Zakup (przed montażem)
- [ ] Zamówić Pololu S7V7F5
- [ ] Zamówić rezystory 10k (2 szt, THT)
- [ ] Zamówić przycisk tact switch

#### ETAP B: Montaż Hardware
- [ ] Zaplanować układ na płytce uniwersalnej (MOSFETy TO-220 wymagają miejsca)
- [ ] Przylutować goldpiny do XIAO
- [ ] Zmontować sekcję zasilania (Pololu + Q1)
- [ ] **Pololu GND do Q1 Drain (NIE do masy głównej!)**
- [ ] Zmontować sekcję I2C (Q2 + czujniki + OLED)
- [ ] Podłączyć PMS7003 przez adapter (GND do Q1 Drain!)
- [ ] Podłączyć buzzer
- [ ] Podłączyć przycisk (D3/GPIO4 do GND)
- [ ] **TEST MULTIMETREM** przed włączeniem (checklist w sekcji 11)

#### ETAP C: Testy Hardware
- [ ] Upload `air_condition.ino` v1.4
- [ ] Weryfikacja odczytów czujników na Serial Monitor
- [ ] Test wyświetlania na OLED (3 ekrany)
- [ ] Test alarmu buzzer (PM2.5 > 75)
- [ ] Test power switching (Q1, Q2) - pomiar prądu
- [ ] Test deep sleep (obserwacja poboru prądu)
- [ ] Test wybudzenia przyciskiem
- [ ] Test komunikacji LoRa z odbiornikiem

### Kompilacja

```bash
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 air_condition
```

### Znane Problemy / Uwagi

1. **PMS7003 rozgrzewka** - 30 sekund przed wiarygodnym pomiarem
2. **PMS7003 żywotność** - ~8000h wentylatora, nie włączać ciągle
3. **Antena LoRa** - ZAWSZE podłączyć przed włączeniem zasilania!
4. **IRLZ44N TO-220** - duże, zaplanować miejsce na płytce
5. **Adapter PMS7003** - goldpiny mogą wymagać wlutowania
6. **Pololu GND** - MUSI iść przez Q1, nie bezpośrednio do masy!
7. **Przycisk** - używa wbudowanego pull-up ESP32, normalnie otwarty
8. **RTC memory** - zmienne RTC_DATA_ATTR przeżywają deep sleep, ale NIE reset/power cycle

### Pliki Projektu

```
air_condition/
├── PROJECT.md          # Ta dokumentacja (v1.3)
├── PROJECT.pdf         # Export PDF
└── air_condition.ino   # Kod v1.4 - pełna implementacja (~1850 linii)
```

---

**Koniec sekcji kontekstu. Powodzenia w kolejnej sesji!**
