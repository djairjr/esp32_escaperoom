# Dependencies

Everything the library and its examples need. Core dependencies are declared in
`library.properties` so the Arduino Library Manager installs them automatically.

---

## 1. Required by the library

| Library | Library Manager name | Used by |
|---------|----------------------|---------|
| ArduinoJson | `ArduinoJson` | `ConfigManager`, all examples |
| IoAbstraction | `IoAbstraction` | `Pcf857x`, expanders, keypad scanning |
| TaskManagerIO | `TaskManagerIO` | IoAbstraction's scheduler |
| audio-tools | `arduino-audio-tools` | `AudioPlayer` (I²S, Helix MP3, tone) |
| Adafruit NeoPixel | `Adafruit NeoPixel` | LED strips |
| Adafruit LED Backpack Library | `Adafruit LED Backpack Library` | HT16K33 14-segment display |
| Adafruit LIS3DH | `Adafruit LIS3DH` | accelerometer |
| Adafruit BusIO | `Adafruit BusIO` | I²C helper used by the Adafruit drivers |
| Adafruit PN532 | `Adafruit PN532` | NFC reader |
| ADS1115_WE | `ADS1115_WE` | ADC |
| PCA9685 | `PCA9685` | PWM driver |
| ESP32Servo | `ESP32Servo` | servo |
| RotaryDial | `RotaryDial` | phone pulse dial |
| M5Unified | `M5Unified` | M5Stack and other M5 boards (also pulled by `pins/cardputer.h` / `pins/stamplc.h`) |

> **Note on the port.** The library was ported from `propmaker_escaperoom` (RP2040). The
> RP2040 dependency on the Arduino `Servo` library becomes **`ESP32Servo`** here, and the
> RS485 stack (`ArduinoRS485`/`ArduinoModbus`) is **no longer needed** — inter-puzzle
> communication is MQTT.

---

## 2. Manual dependency (not in the Library Manager)

### `arduino-libhelix` — MP3 decoder

The audio pipeline relies on the Helix MP3 decoder, which is **not published in the Arduino
Library Manager** and must be installed by hand:

1. Arduino IDE → *Sketch → Include Library → Add .ZIP Library…* with the archive from
   <https://github.com/pschatzmann/arduino-libhelix/archive/refs/heads/main.zip>, or
2. clone it into your `libraries/` folder:

   ```bash
   cd <arduino>/libraries
   git clone https://github.com/pschatzmann/arduino-libhelix.git
   ```

Without it the library will not compile (the audio header includes
`AudioTools/AudioCodecs/CodecMP3Helix.h`).

---

## 3. Needed only by some examples

| Library | Manager name | Needed by |
|---------|--------------|-----------|
| ArduinoHA | `ArduinoHA` | every example that talks to Home Assistant (most of them) — it also pulls `PubSubClient` |
| lvgl | `lvgl` | `Cardputer_H4CK-OS` (LVGL UI) |
| M5Unified / M5Cardputer / M5StamPLC | `M5Unified` / `M5Cardputer` / `M5StamPLC` | the M5Stack examples and the M5Stack pin files |

Install these only for the sketches you build; the library core does not need them.

---

## 4. Verifying the install

Compile and flash the smoke test:

```
File → Examples → esp32_escaperoom → Hardware_Tests → LibrarySmokeTest
```

Its boot report lists the detected I²C/SPI devices and the mounted filesystems; if it
compiles and reports your hardware, the dependency set is complete.
