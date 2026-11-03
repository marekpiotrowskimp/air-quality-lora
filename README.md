# air-quality-lora

System monitorowania jakości powietrza na **Seeed XIAO ESP32-S3** z łącznością **LoRa (SX1262)**.

| Katalog | Urządzenie |
|---|---|
| [`air_condition/`](air_condition/) | Nadajnik: PMS7003 (PM1.0/2.5/10), temperatura, wilgotność, ciśnienie, ekran OLED, obudowa 3D |
| [`air_condition_receiver/`](air_condition_receiver/) | Odbiornik: do 4 nadajników, kolorowy ekran dotykowy 2,8", rozsyłanie danych po Wi-Fi (UDP), obudowa 3D |
| [`air_condition_epaper/`](air_condition_epaper/) | Wariant nadajnika z wyświetlaczem e-paper |

Pełna dokumentacja (BOM, schematy połączeń, zarządzanie energią, protokół):
[air_condition/PROJECT.md](air_condition/PROJECT.md), [air_condition_receiver/PROJECT_RECEIVER.md](air_condition_receiver/PROJECT_RECEIVER.md).

## Build

Każdy katalog to szkic Arduino (nazwa katalogu = nazwa pliku `.ino`). Arduino IDE / `arduino-cli`, płytka `esp32:esp32:XIAO_ESP32S3`, biblioteki wymienione w dokumentacji.

```sh
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 air_condition
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 air_condition_receiver
```

Wariant z Wi-Fi (`air_condition_epaper`): skopiuj `secrets.h.example` jako `secrets.h` i wpisz dane sieci (plik jest w `.gitignore`).

Obudowy: pliki `.scad` / `.stl` / `.gcode` w katalogach `enclosure/`.
