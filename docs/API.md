# API reference

Single include: `#include <ESP32EscapeRoom.h>`. Namespace: `esp32_escaperoom`.
Global macro: `#define hw esp32_escaperoom::Hardware::instance()`.

---

## `Hardware` — the `hw` singleton

`static Hardware& Hardware::instance()` — returned by the `hw` macro.

### Lifecycle & status

| Method | Description |
|--------|-------------|
| `void begin()` | Initialize the board: I²C scan, SPI, power rail, filesystems, audio, NeoPixel, servo, board init. Idempotent (guarded by `begun_`). |
| `bool isSystemHealthy()` | Port expander **and** accelerometer **and** display present. |
| `bool isLittleFSMounted()` / `bool isSDMounted()` | Filesystem status. |
| `void testMemory()` | Print free heap / PSRAM / FreeRTOS / mount status. |

### Detection flags

`hasPortExpander()`, `hasAccelerometer()`, `hasPwmDriver()`, `hasDisplay()`, `hasRtc()`,
`hasOled()`, `hasAdc()`, `hasNfc()`, `hasCodec()`, `hasExpander()`.

### Power rail

| Method | Description |
|--------|-------------|
| `void setPower5V(bool on)` | Drive `PIN_EXTERNAL_POWER` (5 V rail for audio/servo/NeoPixel). `begin()` turns it ON. |

### I/O expanders (PCF8574 / PCF8575)

| Method | Description |
|--------|-------------|
| `void addPcf8574(uint8_t addr)` | Register an 8-bit expander. |
| `void addPcf8575(uint8_t addr)` | Register a 16-bit expander. |
| `void addExpander(uint8_t addr, bool is16bit)` | Generic registration. |
| `PCF8574IoAbstraction* portExpander(uint8_t index = 0)` | Access; `nullptr` + log if absent. |
| `PCF8574IoAbstraction* io(uint8_t index = 0)` | Alias of `portExpander`. |
| `PCF8574IoAbstraction* inputOutput(uint8_t index = 0)` | Legacy alias. |
| `PCF8574IoAbstraction* getExpanderByAddress(uint8_t addr)` | Lookup by I²C address. |
| `int getExpanderIndexByAddress(uint8_t addr)` | Index or `-1`. |
| `int expanderCount()` | Number of registered expanders (max 8). |
| `bool hasPcf8575(uint8_t index = 0)` | Whether that expander is 16-bit. |
| `bool hasExpander(uint8_t index)`, `bool hasExpander()` | Presence checks. |

The scan detects PCF857x but does **not** register them; call `addPcf8574/addPcf8575` in
`setupHardware()`.

### Audio

| Method | Description |
|--------|-------------|
| `AudioPlayer& sound()` | The MP3/tone player. Call `hw.sound().loop()` every iteration. |

See [`AudioPlayer`](#audioplayer).

### Display

| Method | Description |
|--------|-------------|
| `Display14Seg& display()` | HT16K33 4-digit 14-segment display. |

### NeoPixel

| Method | Description |
|--------|-------------|
| `Adafruit_NeoPixel& pixelsExternal()` | External strip (default 30 LEDs). |
| `Adafruit_NeoPixel& pixelsInternal()` | Internal strip (1 LED). |
| `void setLight(uint16_t n, uint32_t c)` | Set external pixel `n` + show. |
| `void setLightAll(uint32_t c)` | Fill external + show. |
| `void setLightBrightness(uint8_t b)` | External brightness + show. |
| `void setLightInternal(uint16_t n, uint32_t c)` / `setLightInternalAll(c)` | Internal equivalents. |

Puzzles should prefer `PuzzlePixelView` (per-puzzle windows) over raw indices.

### Actuators & analog

| Method | Description |
|--------|-------------|
| `void setActuator(int angle)` | Servo angle. |
| `void setAnalogOutput(uint8_t ch, float percent)` | PCA9685 channel duty 0–100 %. |
| `float readVoltage(uint8_t ch)` | ADS1115 channel in mV. |
| `uint16_t readAnalogRaw(uint8_t ch)` | ADS1115 raw counts. |
| `void setAdcChannel(uint8_t ch)` | Start a single-ended conversion (blocking until ready). |

### Motion (LIS3DH)

`bool isShaking(float threshold_ms2 = 15.0f)`, `uint8_t readTap()`,
`bool isSingleTapped()`, `bool isDoubleTapped()`, `Adafruit_LIS3DH& accelerometer()`.

### Factories

| Method | Returns |
|--------|---------|
| `EscapeRoomRotaryEncoder* createEncoder(uint8_t pinA, uint8_t pinB)` | Native quadrature encoder. |
| `RotaryDialUnifiedReader* createRotaryDialUnified(uint8_t pin)` | Mechanical pulse dial. |
| `RotaryDialUnifiedReader* createRotaryDialUnifiedOptical(uint8_t pin, debounce=50, maxgap=0, digitend=800)` | Optical dial with calibrated timings. |
| `NfcReader* createNfcReader(uint8_t irq = 255, uint8_t reset = 255)` | PN532 over I²C; also sets `hasNfc_`. |

Factories return `nullptr` on failure, so always check.

### Battery & low power

| Method | Description |
|--------|-------------|
| `void beginBatteryMonitor(uint8_t sensePin = PIN_BATTERY_SENSE)` | Start reading the divider. |
| `float readBatteryVoltage()` | Volts (cached, 1 s interval). |
| `float readBatteryPercent()` | 0–100 % over 3.3–4.2 V. |
| `bool isBatteryConnected()` | `> 3.0 V`. |
| `void setLowPowerMode(bool)` / `bool getLowPowerMode()` | Enable proportional dimming as the battery drains. |
| `void setPowerSaveThresholds(float warnV, float criticalV)` | Defaults 3.8 V / 3.5 V. |
| `void updatePowerSave()` | Apply dimming; cuts the 5 V rail at the critical threshold. |

### Native I/O & math

`void pinMode(uint8_t, uint8_t)`, `int digitalRead(uint8_t)`, `void digitalWrite(uint8_t, uint8_t)`,
`static uint8_t applyGamma(uint8_t)`, `static float sineWave(float periodSeconds, float min = 0, float max = 1)`.

---

## `PuzzleBase` — the puzzle

Abstract base class. Subclass it, and implement `setupHardware()` and `update()`.

| Member | Description |
|--------|-------------|
| `PuzzleBase(uint8_t id)` | Constructor; state starts at `Initialising`. |
| `virtual void setupHardware()` | **Pure virtual.** Declare the peripherals here. |
| `virtual void update()` | **Pure virtual.** One tick; returns quickly, never blocks. |
| `void begin()` | `setupHardware()` then `setState(Idle)`. |
| `void run()` | `setState(Running)` — start the puzzle. |
| `void setState(PuzzleState)` | Idempotent transition; logs + fires the callback. |
| `PuzzleState getState() const` | Current state. |
| `uint8_t getId() const` | Puzzle id. |
| `virtual uint8_t getProperty()` | Free-form value for orchestration/HA. |
| `static const char* stateName(PuzzleState)` | Mixed-case name. |
| `void setStateChangeCallback(cb, void* userData = nullptr)` | `void(*)(PuzzleBase*, PuzzleState, PuzzleState, void*)`. |

### Sequence helpers

`generateRandomSequence(uint8_t* seq, uint8_t count, uint8_t maxInput)`,
`generateRandomSequenceNoRepeat(...)` (Fisher–Yates, no repeats),
`generateRandomPairs(...)`, `areInputsActive(uint8_t* inputs, uint8_t count)` (AND),
`areNInputsActive(uint8_t n, uint8_t maxInput)`, `checkSequence(expected, len, current, curLen)`,
`resetSequence()`, `getSequenceIndex()`, `setSequenceIndex(i)`,
`advanceSequence(expectedInput, actualInput)`, `isSequenceComplete(len)`.

### Input helpers (on the PCF expander, `LOW` = active)

`bool readInputDebounce(uint8_t pin, uint16_t debounceMs = 50)`,
`bool detectRisingEdge(uint8_t pin)`, `bool detectFallingEdge(uint8_t pin)`.

> `int8_t waitForInput(uint8_t* inputs, uint8_t count, uint16_t timeoutMs = 0)` and
> `waitForAnyInput(uint8_t maxInput, uint16_t timeoutMs = 0)` **block** (spin on `millis()`).
> Use only in bring-up; prefer the edge helpers in a non-blocking `update()`.

### Progress helpers

`static uint8_t calculateProgressPercent(cur, total)`,
`static float calculateProgressFloat(cur, total)`,
`static uint16_t mapProgressToRange(cur, total, min, max)`.

### LED range

`void setPixelRange(uint8_t offset, uint8_t count)`, `PuzzlePixelView* pixels()`,
`bool hasPixels()`, `uint8_t pixelOffset()`, `uint8_t pixelCount()`.

### Effects

`float getSine(float period, float min = 0, float max = 255)`,
`uint8_t getFlicker(uint8_t baseVal, uint8_t amount)`.

---

## `LogicManager` — orchestration

| Method | Description |
|--------|-------------|
| `LogicManager(uint8_t hwId, bool isMaster = false)` | Constructor. |
| `void addPuzzle(PuzzleBase*)` | Register (max 8). |
| `void begin()` | `begin()` each puzzle, install the state callback, build the level map. |
| `void update()` | Tick the timer + the puzzles the active level allows. |
| `void setGameLevel(const GameLevelConfig*, uint8_t count)` | Define levels (max 8). |
| `void startGame()` | Activate level 0; fires `onGameLevelEnter(0)`. |
| `uint8_t getGameLevel() const` | Active level index. |
| `bool isGameComplete() const` | Whole game finished. |
| `void startTimer(uint16_t seconds)` / `stopTimer()` / `uint16_t getRemainingTime()` | Global countdown shown on the display. |

### Virtual hooks (override)

`onGameLevelEnter(uint8_t)`, `onGameLevelExit(uint8_t)`, `onGameComplete()`,
`onPuzzleSolved(uint8_t puzzleIndex)`.

### `GameLevelConfig` / `GameLevelMode`

```cpp
enum class GameLevelMode : uint8_t { PARALLEL = 0, SEQUENCE = 1 };
struct GameLevelConfig { const uint8_t* puzzleIds; uint8_t puzzleCount; GameLevelMode mode; };
```

---

## `PuzzlePixelView` — per-puzzle LED window

| Method | Description |
|--------|-------------|
| `PuzzlePixelView(Adafruit_NeoPixel*, uint8_t offset, uint8_t count)` | Window over a strip. |
| `bool setPixel(uint8_t localIndex, uint32_t color)` | Set a local LED. |
| `void setAll(uint32_t color)` / `void clear()` | Fill / clear the window. |
| `bool show()` | Push to the strip. |
| `bool setPixelAndShow(...)` / `void setAllAndShow(...)` | Set + show. |
| `uint8_t count()`, `uint8_t offset()`, `bool isValidIndex(i)`, `Adafruit_NeoPixel* strip()` | Introspection. |
| `void setBrightness(uint8_t)` | **Global** strip brightness (affects every puzzle). |
| `static uint32_t Color(uint8_t r, uint8_t g, uint8_t b)` | Pack an RGB colour. |

---

## `AudioPlayer` — reached via `hw.sound()`

| Method | Description |
|--------|-------------|
| `bool begin(bool sdAvailable, bool littleFsAvailable)` | Called by `hw.begin()`. |
| `void loop()` | **Call every iteration** — feeds the decoder. |
| `bool playOnce(const char* path, bool useSD = false)` | Non-blocking single play. |
| `bool playOnceBlocking(const char* path, bool useSD = false)` | Blocks until done. |
| `bool playLoop(const char* path, bool useSD = false)` | Loop. |
| `bool play(const char* path, bool useSD = false, bool blocking = false)` | Generic. |
| `void stop()` / `bool isPlaying()` | Control / status. |
| `void setVolume(uint8_t)` / `uint8_t getVolume()` | 0–255. |
| void tone(uint16_t hz, uint8_t volume = 128) / void noTone() | Sine generator (Morse, beeps). |
| `bool isPlayingTone()` / `uint16_t getToneFrequency()` | Tone status. |
| `bool mounted()` / `StorageBackend backend()` | Storage state (`SDCard` / `LittleFS`). |

---

## `Display14Seg` — `hw.display()`

`begin(uint8_t address = 0x70)`, `setBrightness(uint8_t)`, `setBlinkRate(uint8_t)`,
`clear()`, `setColon(bool)`, `printNum(uint16_t value, bool leadingZeros = false)`,
`printString(const char*)`, `printTime(uint16_t minutes, uint16_t seconds, bool blink = true)`,
`writeDisplay()`, `writeDigitRaw(digit, value)`, `writeDigitAscii(digit, char)`,
`bool isInitialized()`.

---

## `NfcReader` — PN532 (I²C)

| Member | Description |
|--------|-------------|
| `bool begin()` | Init + firmware check + `SAMConfig()`. |
| `bool isInitialized()` | Status. |
| `CardData readCard(uint16_t timeoutMs = 1000)` | Poll a card. `CardData{ bool present; uint8_t uid[7]; uint8_t uidLength; }`. |
| `uint32_t getFirmwareVersion()` | Raw version word. |

---

## `RfidBus` — several MFRC522 on one SPI bus

Constructor with explicit CS pins or through a `Pcf857x` expander:

```cpp
RfidBus(const uint8_t* csPins, uint8_t readerCount, uint8_t rstPin, uint8_t ssDummyPin, SPIClass& spi = SPI);
RfidBus(Pcf857x& expander, const uint8_t* csPins, uint8_t readerCount, uint8_t rstPin, uint8_t ssDummyPin, SPIClass& spi = SPI);
```

`bool begin(uint16_t expanderIdleMask = 0xFFFF)`, `bool beginReader(MFRC522&, index, dumpVersion = true)`,
`bool readCard(MFRC522&, index)`, `void finishCard(MFRC522&, index)`.
Available only when `MFRC522.h` is present (`__has_include`).

---

## `Pcf857x` — expander wrapper

`Pcf857x(uint8_t address, bool is16bit = true)`, `bool begin(uint16_t initialState = 0xFFFF)`,
`bool write(uint16_t)`, `uint16_t read()`, `void pinMode(uint8_t, uint8_t)`,
`void digitalWrite(uint8_t, uint8_t)`, `uint8_t digitalRead(uint8_t)`.

---

## `RotaryDialUnifiedReader` — phone pulse dial

`bool begin(uint8_t pin)` (mechanical), `bool beginOptical(uint8_t pin, unsigned long debounce, unsigned long maxgap, unsigned long digitend)`,
`bool available()`, `int read()` (0–9 or `-1`), `void reset()`, `bool isInitialized()`, `uint8_t getPin()`.

`maxgap = 0` means no ceiling (recommended for a clean optical dial).

---

## `EscapeRoomRotaryEncoder` — native quadrature encoder

`EscapeRoomRotaryEncoder(uint8_t pinA, uint8_t pinB)`, `void begin()`, `void update()`,
`int32_t getPosition()`, `int8_t getDirection()` (1 / −1 / 0), `void setPosition(int32_t)`.
Call `update()` every iteration.

---

## `ConfigManager` — SD → LittleFS → AP

| Member | Description |
|--------|-------------|
| `bool begin()` | Mount SD/SPIFFS, load `/wifi.json`, connect; else start the AP portal. |
| `bool loadPuzzleConfig(const char* path, JsonDocument&)` / `savePuzzleConfig(...)` | Read/write a puzzle JSON. |
| `const char* wifiSSID()`, `mqttBroker()`, `mqttUser()`, `mqttPass()`, `devName()`, `devRoom()`, `devPuzzleId()` | Accessors. |
| `bool isAPMode()`, `void handleAP()` | Captive-portal state and servicing. |

---

## `SerialDebug` — serial command console

`void begin(Stream& stream = Serial)`, `bool addCommand(const char* name, const char* desc, CommandHandler)`
(`bool(*)(const char* arg)`), `void update()`, `void printHelp()`. Up to 12 commands;
`help`/`?` is built in.

---

## Free helper

`esp32_escaperoom::stateName(PuzzleState)` → `const __FlashStringHelper*` (`"IDLE"`, …).
