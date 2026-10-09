# Architecture

`esp32_escaperoom` is layered so that a puzzle author only ever touches the top layer and
the `hw` singleton. This document describes the layers, the boot sequence, the
orchestration model and the two ways of writing a puzzle.

---

## 1. Layers

```
┌───────────────────────────────────────────────────────────────────────────┐
│  SKETCH  —  your puzzle(s)                                                │
│    class MyPuzzle : public PuzzleBase { setupHardware(); update(); }      │
│    (optional) class MyManager : public LogicManager { onGameLevelEnter… }│
├───────────────────────────────────────────────────────────────────────────┤
│  ORCHESTRATION  —  LogicManager                                            │
│    up to 8 puzzles · up to 8 game levels · PARALLEL / SEQUENCE · timer    │
│    reacts to PuzzleState::Solved through a callback + a poll fallback    │
├───────────────────────────────────────────────────────────────────────────┤
│  PUZZLE FRAMEWORK  —  PuzzleBase · PuzzleState · PuzzlePixelView          │
│    the state machine, per-puzzle LED ranges, input helpers, math helpers  │
├───────────────────────────────────────────────────────────────────────────┤
│  HARDWARE ABSTRACTION  —  Hardware singleton (`hw`)                       │
│    boot I2C/SPI scan, detection flags, one uniform API per peripheral    │
│    AudioPlayer · Display14Seg · NeoPixel (int/ext) · Servo · ADS1115 ·    │
│    PCA9685 · LIS3DH · PN532 · PCF857x · RotaryDialReader · Encoder ·      │
│    MicInput · battery/low-power                                            │
├───────────────────────────────────────────────────────────────────────────┤
│  BOARD PIN MAP  —  PlatformPins.h → pins/<board>.h                        │
│    compile-time selection by board macro; one file per supported board    │
├───────────────────────────────────────────────────────────────────────────┤
│  CONFIGURATION  —  ConfigManager                                          │
│    SD → LittleFS → AP-mode captive portal; JSON puzzle configs            │
├───────────────────────────────────────────────────────────────────────────┤
│  ESP32 Arduino core  (WiFi, MQTT via ArduinoHA in the sketch)             │
└───────────────────────────────────────────────────────────────────────────┘
```

The single public include is `#include <ESP32EscapeRoom.h>`, which pulls in the whole
public surface. Internal headers live under `src/esp32_escaperoom/`.

---

## 2. The `hw` singleton

```cpp
#define hw esp32_escaperoom::Hardware::instance()
```

`hw` is a **global macro**, not a member of `PuzzleBase` and not a global variable — it
expands to the call `Hardware::instance()`, a function-local static. That means:

- it is available anywhere, including free functions and global constructors;
- there is exactly one instance for the whole program;
- it is created on first use, after the Arduino runtime is up.

### Boot sequence (`hw.begin()`)

```
hw.begin()
  ├─ Serial banner + engine name
  ├─ Wire.begin() ; scanI2C()                       ← detection
  │     ├─ 0x18-0x19  LIS3DH accelerometer      → begin(), setRange(4G), setClick()
  │     ├─ 0x20-0x27  PCF857x                   → DETECTED (added later, explicitly)
  │     ├─ 0x38-0x3F  PCF857xA                 → DETECTED
  │     ├─ 0x40-0x47 / 0x4C-0x4F  PCA9685       → setupSingleDevice(), 60 Hz
  │     ├─ 0x48-0x4B  ADS1115                   → init(), range 6144 mV, 128 SPS
  │     ├─ 0x51       PCF8563 RTC              → DETECTED
  │     ├─ 0x70-0x77  HT16K33 display          → begin(), brightness 15
  │     └─ 0x24       PN532 NFC                → DETECTED
  ├─ SPI.begin()
  ├─ PIN_EXTERNAL_POWER HIGH (5 V rail for audio/servo/NeoPixel)
  ├─ LittleFS.begin() → mount + directory listing
  ├─ SD.begin(PIN_SD_CS) → mount + directory listing
  ├─ audio_.begin()  (I2S out, Helix MP3 decoder)
  ├─ NeoPixel internal + external .begin(), clear, brightness 50
  ├─ servo_.attach(PIN_SERVO_PWM)
  ├─ boardInit()     (board-specific vendor init, e.g. M5.begin())
  └─ print the detection summary (FOUND / NOT FOUND per device)
```

Two consequences worth knowing:

- **Expanders are not auto-added.** The scan only *detects* PCF857x and prints the exact
  `hw.addPcf8574(0x..)` / `addPcf8575(0x..)` call to make. You add them explicitly in
  `setupHardware()`. This avoids guessing 8-bit vs 16-bit.
- **Detection flags gate the API.** `hw.hasAdc()`, `hasPwmDriver()`, `hasDisplay()`, … are
  set by the boot scan; accessors check them before use and log a clear message instead of
  crashing.

### Detection flags

| Flag | Set when |
|------|----------|
| `hasPortExpander()` | an expander was added or detected |
| `hasAccelerometer()` | LIS3DH answered and initialized |
| `hasPwmDriver()` | PCA9685 initialized |
| `hasDisplay()` | HT16K33 initialized |
| `hasRtc()` | PCF8563 detected |
| `hasOled()` | SSD1306 detected |
| `hasAdc()` | ADS1115 initialized |
| `hasNfc()` | PN532 reader created |
| `isSystemHealthy()` | port expander **and** accelerometer **and** display present |

---

## 3. The two ways to write a puzzle

### 3.1 Direct — one puzzle, no orchestrator

```cpp
MyPuzzle puzzle;

void setup() {
  hw.begin();
  puzzle.begin();
  puzzle.run();
}

void loop() {
  hw.sound().loop();
  puzzle.update();
  delay(1);
}
```

Use this for a standalone prop: one board, one puzzle, no levels.

### 3.2 Orchestrated — several puzzles, game levels

```cpp
class MyManager : public LogicManager {
public:
  MyManager() : LogicManager(0, false) {}
  void onGameComplete() override { /* unlock the room */ }
};

MyManager manager;
PuzzleA a;  PuzzleB b;

void setup() {
  hw.begin();
  manager.addPuzzle(&a);
  manager.addPuzzle(&b);
  manager.begin();          // calls begin() on each puzzle + installs the callback
  manager.setGameLevel(levels, levelCount);
  manager.startGame();
}

void loop() {
  hw.sound().loop();
  manager.update();         // ticks only the puzzles the active level allows
  delay(1);
}
```

With levels, `LogicManager` decides *when* each puzzle runs; the puzzle never decides that
itself. See [`PUZZLE_STATE_MACHINE.md`](PUZZLE_STATE_MACHINE.md) for the full contract.

### 3.3 Data-driven (examples)

The `UniversalController` family goes one step further: the puzzle's behaviour — inputs,
mode, timeout, PWM, audio, LED colours — is described by a JSON file and interpreted at
runtime by the example-local `PuzzleEngine`. One firmware, many puzzles. It is a good
pattern for *generic* props; it is example code, not library API.

```
config.json ──► ConfigManager.loadPuzzleConfig() ──► PuzzleEngine
   inputs.pcfAddress / pinMapping
   logic.mode = CUSTOM_SEQUENCE | LINEAR_SEQUENCE | ALL_CONNECTED | RANDOM | MUSICAL_SEQUENCE
   logic.sequence / timeoutSeconds / resetOnError / allowRepeat
   pwm / audio / neopixel / output
```

---

## 4. Shared hardware, isolated puzzles

A single board runs several puzzles; the library provides two mechanisms to keep them from
fighting over hardware:

- **I/O isolation.** Each puzzle owns one I²C expander (PCF8574/PCF8575). The puzzle
  receives the expander index and reads/writes only its own pins.
- **LED ranges.** `PuzzleBase::setPixelRange(offset, count)` gives a puzzle a window of the
  shared NeoPixel strip, addressed by local index through `PuzzlePixelView`.

Shared, non-isolated resources (audio, display, servo) are arbitrated by whoever calls
them: `hw.sound()` is a single player, `hw.display()` a single display. A multi-puzzle game
sequences audio cues through `LogicManager` level hooks rather than playing from several
puzzles at once.

The address convention used across the examples:

| I²C address | Typical role |
|-------------|--------------|
| `0x20` | primary puzzle expander |
| `0x21` | second puzzle |
| `0x22` | third puzzle |
| `0x23`–`0x27` | further puzzles / RFID readers |
| `0x24` | PN532 NFC |
| `0x40` | PCA9685 PWM (servos, dimmers) |
| `0x48` | ADS1115 ADC (potentiometers) |
| `0x51` | PCF8563 RTC |
| `0x70` | HT16K33 display (also the 8-channel I²C hub in some builds) |

---

## 5. Audio pipeline

```
MP3 file (SD or LittleFS)
   → FileStream → EncodedAudioStream(MP3DecoderHelix) → VolumeStream → I2SStream → MAX98357A/…
```

- The decoder is fed by `hw.sound().loop()`, which the **sketch** must call every
  iteration. Nothing in the library calls it for you.
- `playOnce()` is non-blocking and is the default for cues; `playOnceBlocking()` blocks
  until the file ends (use sparingly, e.g. a diegetic countdown); `playLoop()` loops.
- `tone(frequency, volume)` stops any MP3 and emits a sine wave — used for Morse, beeps
  and calibration. `noTone()` stops it.
- Preferred example format: 128 kbps, 44.1 kHz stereo, no ID3 tags.

---

## 6. Microphone and pitch detection (`MicInput`)

`MicInput` reads an **INMP441** MEMS microphone over I2S (RX, 48 kHz, 32-bit stereo) and
detects pitch. It exists because the `MUSICAL_SEQUENCE` mode needs a *sensor*, not a
display:

- filtering — two cascaded 2nd-order Butterworth IIR filters (high-pass 60 Hz, low-pass
  6 kHz);
- pitch — **YIN** autocorrelation (coarse pass + refined pass with a window), harmonic
  validation (2nd/3rd/4th), parabolic interpolation for sub-sample accuracy, EMA smoothing
  with note-change detection, and a silence gate on RMS;
- why not FFT — a 512-point FFT at 16 kHz is 31.25 Hz/bin, ≈35 cents per bin at C4, too
  coarse for musical sequences; time-domain autocorrelation finds the period of the
  fundamental;
- the I²S read must be stereo with slot selection — mono 32-bit returns a zero-stuffed
  buffer (`[d,0,d,0,…]`) that the DC-removal turns into a square wave at fs/2, making every
  note read as B8.

`MicInput::setYieldPump()` lets the sketch pump its own cooperative work (e.g. an HTTP
handler) during the YIN analysis.

---

## 7. Repository layout

```
esp32_escaperoom/
├── src/
│   ├── ESP32EscapeRoom.h          ← the single public include
│   ├── ESP32EscapeRoom.cpp        ← includes the above
│   ├── AudioPlayer.h              ← forwarder
│   ├── PlatformPins.h             ← forwarder
│   ├── RfidBus.h                  ← forwarder
│   └── esp32_escaperoom/
│       ├── Hardware.h / .cpp      ← the hw singleton
│       ├── PuzzleBase.h / .cpp    ← the puzzle state machine
│       ├── PuzzleState.h          ← the enum
│       ├── LogicManager.h / .cpp  ← orchestration + game levels + timer
│       ├── GameLevelConfig.h      ← the level struct/mode
│       ├── PuzzlePixelView.h      ← per-puzzle LED windows
│       ├── AudioPlayer.h          ← I2S + Helix MP3 + tone
│       ├── Display14Seg.h / .cpp  ← HT16K33
│       ├── NfcReader.h            ← PN532
│       ├── RfidBus.h              ← multiple MFRC522 on one SPI bus
│       ├── Pcf857x.h              ← PCF8574/8575 wrapper
│       ├── RotaryDialReader.h/.cpp← phone pulse dial
│       ├── RotaryEncoder.h        ← native quadrature encoder
│       ├── MicInput.h / .cpp      ← INMP441 + YIN
│       ├── ConfigManager.h        ← SD/LittleFS/AP + JSON
│       ├── SerialDebug.h          ← serial command console
│       ├── PlatformPins.h         ← board dispatch
│       └── pins/                  ← one .h per board
├── examples/                      ← 26 sketches
├── docs/                          ← this documentation
├── library.properties · keywords.txt · LICENSE · .gitignore
```
