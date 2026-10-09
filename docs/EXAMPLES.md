# Examples

26 sketches under `examples/`. The default target is the **Seeed XIAO ESP32-S3 + Expansion
Board**; a few are board-specific (M5Stack StamPLC, M5Stack Cardputer) and say so. Several
were ported from the RP2040 `propmaker_escaperoom` library and may still mention "PropMaker"
in their headers — the code targets ESP32.

Every example has a `data/` folder with its JSON configuration. **Binary media (MP3, SPIFFS
images, animations) and the BombTimer KiCad project are not versioned**; each `data/README.txt`
lists the files to add locally.

---

## Hardware tests — start here

| Sketch | What it does |
|--------|--------------|
| `Hardware_Tests/LibrarySmokeTest` | The bring-up sketch. Runs `hw.begin()` (full I²C/SPI/filesystem/audio report) and exercises every device that was detected. **Compile this first** for a new board. |
| `Hardware_Tests/KeypadTester` | Discovers the wiring of an unknown matrix keypad on a PCF8574/8575: sweeps the pins and prints which are rows and which are columns, so you can fill `rowPins[]` / `colPins[]`. |
| `Hardware_Tests/ButtonWireTest` | Isolated test of four buttons and four wires on a PCF8575 through `SwitchInput` — the smallest possible input-side test. |
| `Hardware_Tests/MultiPuzzlePixels` | Demonstrates several puzzles sharing one NeoPixel strip, each with its own `setPixelRange()` window. |
| `Hardware_Tests/BatteryLowPower` | Battery monitoring (`readBatteryVoltage/Percent`, `isBatteryConnected`) and low-power mode (proportional dimming, rail cut at the critical threshold). |

---

## Core puzzles

Each of these is a self-contained puzzle with its own JSON configuration and sound set.

| Sketch | Mechanic | Touches |
|--------|----------|---------|
| `KeypadCodeEntry` | Enter one numeric code on a 4×3 I²C keypad. | keypad, audio, JSON |
| `KeypadCodeMultiple` | Several valid codes; any of them solves the puzzle. | keypad, audio, JSON |
| `KeypadRfidLock` | Two-factor: keypad **and** an RFID card (PN532) release the lock. | keypad, PN532, relay |
| `ConnectWires` | Connect the right wires; an ADS1115 reads a voltage to confirm. | PCF8575, ADS1115 |
| `Cyclone` | Timing game: stop a chasing light in the green sector. | NeoPixel ring, button |
| `Roulette` | Light spins around a ring and decelerates to a sector. | NeoPixel ring, button |
| `SimonSays` | Repeat the light sequence; four buttons with NeoPixel rings. | 4 buttons, NeoPixel, audio |
| `Mastermind` | Guess a hidden colour sequence; keypad entry + display feedback. | keypad, display, NeoPixel |
| `RadioClock` | Tune a dial to the right frequency; static vs. clear signal. | display, audio, button |
| `ReactorControl` | Analog balancing: potentiometers keep gauges in range. | ADS1115 pots, PCA9685 gauges, display |
| `BallMaze` | Tilt the box to steer a virtual ball through a maze. | LIS3DH accelerometer, NeoPixel, relay |
| `VintageRadio` | Shortwave radio with a rotary encoder and RFID. | encoder, PN532, NeoPixel, relay |
| `EscapeRoomPhone` | Dial a number on a phone rotary dial (or keypad) to hear a message. | rotary dial, SD audio, relay |

---

## Orchestrated multi-puzzle

| Sketch | What it shows |
|--------|---------------|
| `MultiPuzzlesGame` | **The `LogicManager` showcase.** Nine puzzles across **five game levels** (parallel access phase, positioning, reactor, lid, sequential interior) in a nuclear-reactor theme; up to five PCF8575 expanders, PN532, ADS1115, PCA9685, a 48-LED strip and a global timer. |

---

## Data-driven puzzles (the `UniversalController` family)

These three share an example-local engine (`PuzzleEngine.h`/`.cpp`) that interprets a JSON
file at runtime: inputs, mode, timeout, PWM, audio, LED colours. The firmware is generic;
the puzzle is data.

| Sketch | What it shows |
|--------|---------------|
| `UniversalController` | One configurable puzzle: `CUSTOM_SEQUENCE`, `LINEAR_SEQUENCE`, `ALL_CONNECTED`, `RANDOM`, `MUSICAL_SEQUENCE`. Several ready-made configs in `data/`. |
| `UniversalControllerMulti` | Two `UniversalController` instances on one board, each with its own config and expander. |
| `UniversalUniqueRFID` | The same engine driven by an RFID reader (one reader, configurable tags). |

---

## Product puzzles

| Sketch | What it shows |
|--------|---------------|
| `BombTimer` | A finished product: countdown bomb with keypad, wire-order and RFID defuse modes, MP3 cues, 14-segment display, NeoPixel, and a full Home Assistant entity set (sensors, lock, buttons, select, numbers, switch). The reference for a shipping puzzle. |

---

## Board-specific builds

| Sketch | Board | Notes |
|--------|-------|-------|
| `BombTimer_STAMPLC` | M5Stack StamPLC | The BombTimer rebuilt for a relay board with TFT, opto-isolated inputs and a buzzer (no I²S). Split into `ConfigHA.h`, `GameLogic.h`, `InputManager.h`, `DisplayManager.h`, `Pins.h`. |
| `Cardputer_H4CK-OS` | M5Stack Cardputer | A miniature "hacker terminal" puzzle: boot animation → login → shell with simulated pentest tools, LVGL UI, I²C-tap and RFID. Uses the on-board keyboard and screen instead of external wiring. |

---

## Notes on portability

- A sketch that mentions "RP2040 PropMaker" in its header is usually an ESP32 port of that
  example; the pin mapping comes from the board file (see [`BOARDS.md`](BOARDS.md)), not from
  the header text.
- Sketches that use the audio API rely on `hw.sound().loop()` being called in `loop()`;
  without it the MP3 decoder stalls.
- `MasterController` dates from the RS485 era: on ESP32 the inter-puzzle link is MQTT, so
  treat its RS485 parts as reference.
