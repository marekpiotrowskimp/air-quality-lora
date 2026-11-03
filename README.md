# air-quality-lora

Air-quality monitoring with **Seeed XIAO ESP32-S3** boards and **LoRa (SX1262)** radio links.

| Directory | Device |
|---|---|
| [`air_condition/`](air_condition/) | Transmitter: PMS7003 (PM1.0/2.5/10), temperature, humidity, pressure, OLED display, 3D-printed case |
| [`air_condition_receiver/`](air_condition_receiver/) | Receiver: up to 4 transmitters, 2.8" color touch screen, forwards data over Wi-Fi (UDP) to other devices, 3D-printed case |
| [`air_condition_epaper/`](air_condition_epaper/) | Transmitter variant with an e-paper display |

Full documentation (in Polish — BOM, wiring, power management, protocol):
[air_condition/PROJECT.md](air_condition/PROJECT.md), [air_condition_receiver/PROJECT_RECEIVER.md](air_condition_receiver/PROJECT_RECEIVER.md).

## Build

Each directory is an Arduino sketch (directory name = `.ino` file name). Use Arduino IDE / `arduino-cli`, board `esp32:esp32:XIAO_ESP32S3`, libraries listed in the documentation.

```sh
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 air_condition
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32S3 air_condition_receiver
```

The Wi-Fi variant (`air_condition_epaper`): copy `secrets.h.example` to `secrets.h` and fill in your network credentials (the file is git-ignored).

`air_condition_receiver/udp_listener.py` is a terminal dashboard showing the UDP broadcasts from up to 4 transmitters.

Enclosures: `.scad` / `.stl` / `.gcode` files in the `enclosure/` directories.

## License

MIT — see [LICENSE](LICENSE).
