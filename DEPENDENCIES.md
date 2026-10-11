# Dependencies

Everything the library and its examples need. The core dependencies are declared in
`library.properties` so the Arduino Library Manager installs them automatically — **except
the two that are not in the Library Manager at all** (section 2), which have to be cloned.

---

## 1. Required by the library

| Library | Library Manager name | Version | Used by |
|---------|----------------------|---------|---------|
| ArduinoJson | `ArduinoJson` | | `ConfigManager`, all examples |
| IoAbstraction | `IoAbstraction` | `4.5.3` | `Pcf857x`, expanders, keypad scanning |
| TaskManagerIO | `TaskManagerIO` | `1.5.2` | IoAbstraction's scheduler |
| Adafruit NeoPixel | `Adafruit NeoPixel` | | LED strips |
| Adafruit LED Backpack Library | `Adafruit LED Backpack Library` | | HT16K33 14-segment display |
| Adafruit LIS3DH | `Adafruit LIS3DH` | | accelerometer |
| Adafruit BusIO | `Adafruit BusIO` | | I²C helper used by the Adafruit drivers |
| Adafruit PN532 | `Adafruit PN532` | | NFC reader |
| ADS1115_WE | `ADS1115_WE` | | ADC |
| PCA9685 | `PCA9685` | | PWM driver |
| ESP32Servo | `ESP32Servo` | | servo |
| RotaryDial | `RotaryDial` | | phone pulse dial |
| M5Unified | `M5Unified` | | M5Stack and other M5 boards (also pulled by `pins/cardputer.h` / `pins/stamplc.h`) |

The IoAbstraction and TaskManagerIO versions are pinned on purpose: the 5.x line of both does
not compile on the ESP32 core 3.x. These are the versions the CI installs.

> **Note on the port.** The library was ported from `propmaker_escaperoom` (RP2040). The
> RP2040 dependency on the Arduino `Servo` library becomes **`ESP32Servo`** here, and the
> RS485 stack (`ArduinoRS485`/`ArduinoModbus`) is **no longer needed** — inter-puzzle
> communication is MQTT.

---

## 2. Manual dependencies (published on GitHub, **not** in the Library Manager)

### `arduino-audio-tools` — the audio pipeline

`AudioPlayer` includes `AudioTools.h`, `AudioTools/AudioCodecs/CodecMP3Helix.h` and
`AudioTools/CoreAudio/AudioEffects/SoundGenerator.h`. The library is published on GitHub
only: it is **not** in the Arduino Library Manager index, so `arduino-cli lib install
arduino-audio-tools` fails with `Library 'arduino-audio-tools@latest' not found`.

1. Arduino IDE → *Sketch → Include Library → Add .ZIP Library…* with the archive from
   <https://github.com/pschatzmann/arduino-audio-tools/archive/refs/heads/main.zip>, or
2. clone it into your `libraries/` folder:

   ```bash
   cd <arduino>/libraries
   git clone https://github.com/pschatzmann/arduino-audio-tools.git
   ```

### `arduino-libhelix` — MP3 decoder

The Helix decoder behind that pipeline is published on GitHub only as well.

1. Arduino IDE → *Sketch → Include Library → Add .ZIP Library…* with the archive from
   <https://github.com/pschatzmann/arduino-libhelix/archive/refs/heads/main.zip>, or
2. clone it:

   ```bash
   cd <arduino>/libraries
   git clone https://github.com/pschatzmann/arduino-libhelix.git
   ```

Without either of them the library does not compile.

---

## 3. Needed only by some examples

| Library | Library Manager name | Needed by |
|---------|----------------------|-----------|
| Home Assistant (`ArduinoHA.h`) | `home-assistant-integration` | every example that talks to Home Assistant, the smoke test included — it also pulls `PubSubClient` |
| lvgl | `lvgl` | `Cardputer_H4CK-OS` (LVGL UI) |
| M5Unified / M5Cardputer / M5StamPLC | `M5Unified` / `M5Cardputer` / `M5StamPLC` | the M5Stack examples and the M5Stack pin files |

Install these only for the sketches you build; the library core does not need them. Note the
Manager name: the library is called **`home-assistant-integration`**, `ArduinoHA.h` is only
the header it provides.

---

## 4. Known build issue: IoAbstraction on ESP32 SoCs without a DAC

**Symptom** (compiling for the ESP32-S3, ESP32-C3 and other SoCs without a DAC):

```
IoAbstraction/src/esp32/ESP32AnalogDevice.cpp:69:15:
error: 'DAC1' was not declared in this scope
```

**Cause.** IoAbstraction gates its DAC code on `__has_include(<driver/dac.h>)`, which is a
check for the *header*, not for the *hardware*: the header is present on the S3/C3 even though
those chips have no DAC.

**Workaround.** Add the hardware capability to the gate in
`IoAbstraction/src/esp32/ESP32AnalogDevice.cpp`:

```cpp
#if __has_include(<driver/dac.h>) && defined(SOC_DAC_SUPPORTED)
```

The CI applies exactly this edit, with the guard failing loudly if IoAbstraction ever changes
that line — see `.github/workflows/arduino-library-ci.yml`. The permanent fix belongs
upstream in <https://github.com/TcMenu/IoAbstraction>.

---

## 5. Verifying the install

Compile and flash the smoke test:

```
File → Examples → esp32_escaperoom → Hardware_Tests → LibrarySmokeTest
```

Its boot report lists the detected I²C/SPI devices and the mounted filesystems; if it
compiles and reports your hardware, the dependency set is complete.
