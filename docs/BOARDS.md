# Boards and pin mapping

The library targets **ESP32** boards. Each supported board has one pin file under
`src/esp32_escaperoom/pins/`; `PlatformPins.h` is the dispatcher that picks one at compile
time and exposes the legacy global aliases.

---

## 1. How the board is selected

`src/esp32_escaperoom/PlatformPins.h` is a cascade of `#if defined(...)` branches, evaluated
in this order:

| Order | Condition | Pin file | Detection |
|------:|-----------|----------|-----------|
| 1 | `ARDUINO_M5STACK_CARDPUTER` or `ARDUINO_M5Stack_Cardputer` or `BOARD_CARDPUTER` | `pins/cardputer.h` | automatic |
| 2 | `ARDUINO_M5STACK_STAMPLC` or `BOARD_STAMPLC` | `pins/stamplc.h` | semi (M5Stack core defines it; else manual flag) |
| 3 | `ARDUINO_XIAO_ESP32S3` or `SEEED_XIAO_ESP32S3` | `pins/xiao_esp32s3.h` | automatic |
| 4 | `ARDUINO_XIAO_ESP32C3` | `pins/xiao_esp32c3.h` | automatic |
| 5 | `ARDUINO_ADAFRUIT_MATRIXPORTAL_ESP32S3` or `BOARD_MATRIXPORTAL` | `pins/matrixportal.h` | semi |
| 6 | `ARDUINO_ESP32_A1S_AUDIO_KIT` or `BOARD_AUDIO_KIT` | `pins/audio_kit.h` | semi |
| 7 | `BOARD_YD_ESP32S3` | `pins/yd_esp32s3.h` | manual only |
| — | otherwise | `pins/xiao_esp32s3.h` (fallback) | prints `#pragma message` |

To force a board, add the corresponding `-DBOARD_xxx` to the build flags (PlatformIO
`build_flags`, or Arduino IDE *Tools → Compile → Build flags*). The full flag names:
`BOARD_CARDPUTER`, `BOARD_STAMPLC`, `BOARD_MATRIXPORTAL`, `BOARD_AUDIO_KIT`,
`BOARD_YD_ESP32S3`.

`PlatformPins.h` also defines the **legacy global aliases** (`PUZZLE_PIN_SPI_MISO`,
`PUZZLE_PIN_SD_CS`, `PUZZLE_PIN_RELAY`, `PUZZLE_PIN_I2S_*`, `PUZZLE_PIN_BTN`,
`PUZZLE_PIN_SERVO_PWM`, `PUZZLE_PIN_NEOPIXEL`, `PUZZLE_PIN_EXTERNAL_NEOPIXEL`,
`PUZZLE_PIN_EXTERNAL_POWER`, `PUZZLE_PIN_BATTERY_SENSE`) drawn from whichever pin file was
selected, for sketches written against the RP2040 names. The `PUZZLE_RS485_*` macros are
**legacy leftovers from the RP2040 original** — RS485 is not implemented on ESP32 (the
inter-puzzle link is MQTT).

---

## 2. The pin contract

Every `pins/<board>.h` is a single `#pragma once` header included **inside**
`namespace esp32_escaperoom`, and must define:

### Identity

| Symbol | Type | Meaning |
|--------|------|---------|
| `BOARD_NAME` | `const char*` | Human-readable board name (printed at boot). |
| `BOARD_MANUFACTURER` | `const char*` | Vendor. |
| `BOARD_CHIP` | `const char*` | Chip variant. |
| `BOARD_FLASH_MB`, `BOARD_PSRAM_MB` | `#define` int | Memory sizes. |

### Peripherals

| Symbol | Meaning |
|--------|---------|
| `PIN_I2C_SDA`, `PIN_I2C_SCL` | I²C bus for expanders, display, NFC, ADC, PWM, RTC, OLED. |
| `PIN_I2S_BCLK`, `PIN_I2S_WS`, `PIN_I2S_DATA` | I²S output to the amplifier/ codec. |
| `PIN_I2S_SDIN` | I²S input (codec/mic), where present. |
| `HAS_I2S` | `1`/`0` — whether the board can do I²S output. |
| `PIN_SPI_SCK`, `PIN_SPI_MISO`, `PIN_SPI_MOSI`, `PIN_SD_CS` | SPI bus + SD card chip-select. |
| `PIN_NEOPIXEL_INTERNAL` | On-board RGB LED pixel (`255` = none). |
| `PIN_EXTERNAL_NEOPIXEL`, `PIN_NEOPIXEL` | External strip data pin (`-1`/`255` = none). |
| `PIN_RELAY` | Lock/relay output. |
| `PIN_SERVO_PWM` | Servo signal. |
| `PIN_BTN` | Boot button. |
| `PIN_EXPANSION_BUTTON` | Secondary/user button (`-1` = none). |
| `PIN_EXTERNAL_POWER` | 5 V rail enable (`-1` = none). |
| `PIN_BATTERY_SENSE` | Battery divider ADC (`-1` = none). |
| `PIN_BUZZER` | Passive buzzer (`-1` = none; use I²S speaker instead). |
| `OLED_I2C_ADDRESS`, `RTC_I2C_ADDRESS` | Addresses of the expansion-board OLED/RTC (`0x00`/`0x51`). |
| `I2C_DEFAULT_ADDRESS` | Default expander address (`0x20`). |
| `DOOR_OPEN_MS` | Default lock-open pulse (`5000`). |
| `PIN_IO0` … `PIN_IOn` | Named GPIOs available to the sketch. |

### Capability flags

`HAS_ONBOARD_*` and `HAS_*` (`1`/`0`): `HAS_ONBOARD_NEOPIXEL`, `HAS_ONBOARD_LEDS`,
`HAS_ONBOARD_BUZZER`, `HAS_ONBOARD_DISPLAY`, `HAS_ONBOARD_TFT`, `HAS_ONBOARD_RTC`,
`HAS_ONBOARD_SD`, `HAS_ONBOARD_MIC`, `HAS_ONBOARD_ACCEL`, `HAS_ONBOARD_CODEC`,
`HAS_ONBOARD_I2S_AMP`, `HAS_ONBOARD_OPTO_IN`, `HAS_ONBOARD_RELAY`, `HAS_ONBOARD_PWM_OUT`,
`HAS_ONBOARD_GROVE`, `HAS_ONBOARD_KEYBOARD`, `HAS_ONBOARD_IR`, `HAS_ONBOARD_LED_MATRIX`,
`HAS_ONBOARD_USB_OTG`, `HAS_ONBOARD_BATTERY`, `HAS_PSRAM`, `HAS_WIFI`, `HAS_BLUETOOTH`,
`HAS_M5UNIFIED`. `Hardware::begin()` prints a report for whichever are set.

### Board init hook

```cpp
inline void boardInit() { /* vendor init, e.g. M5.begin() */ }
```

Called by `Hardware::begin()` after the generic peripherals are up. The XIAO boards leave
it empty; the M5Stack boards (Cardputer, StampPLC) use it to bring up M5Unified.

---

## 3. Board-by-board

### Seeed XIAO ESP32-S3 / Sense / Plus — `pins/xiao_esp32s3.h`
ESP32-S3R8, 8 MB flash, 8 MB PSRAM. The reference board.
I²C SDA 5 / SCL 6 · I²S 43/44/4 · SPI 7/8/9, SD_CS 3 · NeoPixel ext. 3 · relay 1 · servo 2 ·
ext power 21 · battery 10. Expansion Board adds OLED (0x3C), RTC PCF8563 (0x51), microSD.
The **Sense** variant adds a PDM microphone (CLK 42 / DATA 41) read via PDM mode.

### Seeed XIAO ESP32-C3 — `pins/xiao_esp32c3.h`
ESP32-C3 (RISC-V), 4 MB flash, no PSRAM, 12 free GPIOs.
I²C SDA 6 / SCL 7. The C3 has **no DAC**, so IoAbstraction does not compile: the library
auto-defines `ESP32_ESCAPEROOM_NO_IOABSTRACTION` for this board (no PCF expanders, no keypad
scanner) — pins are used directly. I²S is wired through the GPIO matrix
(BCLK 4 / WS 5 / DATA 6). Note the strapping pins (GPIO2 D0, GPIO8 D8, GPIO9 D9).

### M5Stack Cardputer — `pins/cardputer.h`
ESP32-S3FN8, 8 MB flash, **no PSRAM**. M5Unified board: TFT 240×135, NS4168 I²S amp, SPM1423
mic, 56-key keyboard, IR emitter, microSD, Grove, RGB LED on GPIO4, dual battery.
I²C SDA 2 / SCL 1 (Grove) · I²S 41/43/42 (+SDIN 46) · RGB 4. **No external NeoPixel and no
5 V rail** — the strip and relay pins are `-1`; use the on-board keyboard/screen instead of
wiring buttons. Requires `HAS_M5UNIFIED`.

### M5Stack StampPLC — `pins/stamplc.h`
ESP32-S3, 8 MB flash, no PSRAM. M5Unified board: TFT, **4 relays** (via PCF8574), opto-isolated
inputs, PWM outputs, buzzer (GPIO17), Grove.
I²C SDA 2 / SCL 1 · SPI 12/13/11, SD_CS 10 · servo 14 · aux relay GPIO21 · ext power 23.
**No I²S** (`HAS_I2S = false`): use the on-board buzzer or M5.Speaker. This is the board for
relay-heavy puzzles (BombTimer_STAMPLC).

### Adafruit MatrixPortal S3 — `pins/matrixportal.h`
ESP32-S3, 8 MB flash, 2 MB PSRAM. 64×64 HUB75 RGB matrix, LIS3DH accelerometer (0x18) on
board, NeoPixel GPIO2, IR, Grove.
I²C SDA 14 / SCL 15 · user button 6 · relay 16 · servo 17 · buzzer 18. **SPI and SD pins are
consumed by the LED matrix** (`-1`) and there is no I²S. Best for the accelerometer-based
puzzle (BallMaze).

### Ai-Thinker ESP32-A1S Audio Kit v2.2 — `pins/audio_kit.h`
ESP32 (Xtensa), 4 MB flash, 4 MB PSRAM. ES8388 codec (0x10) with I²S in+out, headphone amp
(jack detect 19) and microSD (SD_CS 15).
I²C SDA 18 / SCL 23 · I²S BCLK 25 / WS 26 / DOUT 22 / SDIN 21. `PIN_SERVO_PWM = 0` collides
with BOOT — avoid. The audio-centric board (EscapeRoomPhone).

### VCC-GND Studio YD-ESP32-S3 — `pins/yd_esp32s3.h`
ESP32-S3 N16R8, 16 MB flash, 8 MB PSRAM, 25 exposed GPIOs — the board for puzzles that need
many pins.
I²C SDA 4 / SCL 5 · I²S 6/7/15 · SPI 12/13/11, SD_CS 10 · RGB 48 · relay 21 · servo 14 ·
ext power 23. Requires `-DBOARD_YD_ESP32S3`.

---

## 4. Adding a new board

1. **Copy** the closest file in `pins/` to `pins/<your_board>.h` and adjust the values.
2. **Fill the contract** (§2). Use `-1` for an unavailable pin and `255` for an unavailable
   NeoPixel index; the library checks these.
3. **Add a branch** to the cascade in `PlatformPins.h`. Prefer an automatic board macro; fall
   back to a `BOARD_<NAME>` flag. Keep the most specific boards first.
4. **Implement `boardInit()`** if the vendor stack needs setup (M5Unified boards call
   `M5.begin()` there).
5. **Update `library.properties`** if the board adds a dependency (e.g. M5Unified).
6. **Compile** at least `Hardware_Tests/LibrarySmokeTest` for the new board, on the bench.

---

## 5. Pitfalls

- **I²C address collisions.** The default board files all reserve `0x20` for the first
  expander. Plan a per-board address map before wiring several expanders; `0x20–0x27` and
  `0x38–0x3F` are the PCF ranges, `0x40–0x4F` PCA9685 (minus `0x48–0x4B` for the ADS1115),
  `0x48–0x4B` ADS1115, `0x51` PCF8563, `0x70–0x77` HT16K33.
- **Shared Grove bus.** On M5Stack boards, Grove I²C shares the internal I²C bus: an
  external module must not answer at an address already used internally.
- **Boards without I²S.** StampPLC and MatrixPortal S3 have `HAS_I2S = false`. Code that
  calls `hw.sound().play…` there will not produce audio — use the buzzer or M5.Speaker.
  Guard such code with `#if esp32_escaperoom::HAS_I2S`.
- **The C3 drops IoAbstraction.** No expanders, no keypad scanner; read pins directly.
- **Strapping pins.** The XIAO C3 exposes GPIO2/8/9 as D0/D8/D9; they affect boot mode. Do
  not pull them at reset.
- **The vendor macro may be missing.** Some toolchains do not define a board macro
  (`ARDUINO_M5STACK_STAMPLC`, etc.). Then either pass `-DBOARD_xxx` or let the fallback map
  apply — but verify the pinout, because the fallback is a XIAO S3.
