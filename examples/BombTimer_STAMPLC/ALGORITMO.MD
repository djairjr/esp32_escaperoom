# BombTimer STAMPLC — Algorithm and Complete Structure (M5Stack StamPLC)

## Overview

Port of the original GameTimerPro (RP2040/PropMaker) and BombTimer (ESP32-S3/XIAO) to **M5Stack StamPLC** (ESP32-S3).

**Hardware:**
- TFT Display ST7789v2 240×135 (rotation 1)
- PWM Buzzer (M5StamPLC.tone)
- 3 buttons A/B/C (PI4IOE5V6408) + BOOT (GPIO0 = START)
- 4×3 Keypad via PCF8574 (I2C 0x20-0x27, auto-detect)
- 4 relays AW9523B (DETONATE: RLY1/RLY2, DEFUSE: RLY3/RLY4)
- 4 opto-inputs AW9523B (wires WIRE1-4)
- 1 opto-input AW9523B (external DET)
- RFID MFRC522/WS1850S (I2C 0x28 via PORT.A, MFRC522_I2C library)
- RTC RX8130CE (I2C 0x32)
- WiFi + ArduinoHA (MQTT)
- RGB LED (PI4IOE5V6408, via M5StamPLC.setStatusLight)
- RF433 SYN531R (PORT.C white = GPIO4, rc-switch)

**I2C Buses:**
- **Internal (Wire):** G13(SDA) / G15(SCL) — PI4IOE5V6408(0x43), AW9523B(0x59), RX8130CE(0x32), LM75B(0x48), INA226(0x40)
- PORT.A (PortA): G2(SDA) / G1(SCL) — PCF8574(0x20-0x27), MFRC522(0x28)

---

## Project Files

| File | Description | Location |
|---------|-----------|-------------|
| BombTimer_STAMPLC.ino | Main: setup() + loop() + state machine + menu | Libraries + 2 destinations |
| GameLogic.h | Timer, wires, detonation, defusal, menu, NVS persistence | Same |
| DisplayManager.h | TFT ST7789v2, DSEG14 20pt, circles, status bar | Same |
| InputManager.h | Buttons, RF433, PCF8574 keypad, MFRC522 RFID, DET opto | Same |
| ConfigHA.h | AP mode + webserver + WiFi + ArduinoHA (MQTT) | Same |
| Pins.h | I2C, AW9523B, PI4IOE5V6408, keypad pin mapping | Same |
| docs/USER_MANUAL.md | Puzzle user manual | docs/ |

**Destinations (always copy to all 3):**
1. `~/Arduino/libraries/ESP32EscapeRoom/examples/BombTimer_STAMPLC/` (dev+lib)
2. Radxa: `~/shared_files/Code/ESP32_Projects/BombTimer_STAMPLC/`
3. Windows: `<Arduino>\ESP32_Projects\BombTimer_STAMPLC\`
4. Windows lib: `<Arduino>\libraries\esp32_escaperoom\examples\BombTimer_STAMPLC\`

---

## State Machine (6 states)

```
[IDLE] --START--> [RUNNING]
  |                   |
  +--MENU--> [MENU]   +--time=0-->   [DETONATED]
  |                   +--DET wire--> [DETONATED]
  |                   +--START------>[DETONATED]
  |                   +--ORDER err-->[DETONATED]
  |                   +--CODE ok---->[DEFUSED]
  |                   +--ORDER ok--->[DEFUSED]
  |                   +--WIRE ok---->[DEFUSED]
  |                   +--RFID DEFU-->[DEFUSED]
  |                   +--RFID DETO-->[DETONATED]
  |                   +--CODE 4x err>[DETONATED]
  |
  [DETONATED] --START--> [IDLE]
  [DEFUSED]   --START--> [IDLE]
  [MENU]      --START--> [IDLE]  (or 15s timeout)
  [ALARM]     --START--> [IDLE]
```

### States

| State | Function | Description |
|--------|--------|-----------|
| IDLE | updateIdle() | Adjusts time (PREV/NEXT ±1s, long press ±5s/150ms). BTN_START → RUNNING. BTN_MENU → MENU. |
| MENU | updateMenu() | 18 navigable items. PREV/NEXT navigates, MENU enters edit, MENU confirms. Long press speeds up adjustment. START or 15s timeout → IDLE. |
| RUNNING | updateRunning() | Timer, tick (via tickDue()), scanWires, processKeypad, RFID check, HA publish. Pulsing red RGB LED (500ms, 250ms last 10s). |
| DETONATED | updateDetonated() | "DETONATED! BOOM!" screen, solid red RGB. Relays active for detTriggerSec. START → IDLE. |
| DEFUSED | updateDefused() | "DEFUSED! SUCCESS!" screen, green RGB. Relays active for defuseTriggerSec. START → IDLE. |
| ALARM | updateAlarm() | 3500Hz intermittent beep 500ms + orange RGB. START → IDLE. |

---

## Display (DisplayManager.h)

- **Rotation:** 1 (USB on the left, w=240 h=135)
- **DSEG14 Timer:** `TIMER_Y = 24`, y = 28 (centered), 5 chars + ':', ~128px
- **Seconds:** `SEC_Y = 78` — small "180s" text
- **Status bar (top right corner):** `[W]`(60) `[M]`(45) `[R]`(30) `[T]`(15) — X positions from the right
- **Wire circles (bottom left corner):** YEL(8) RED(26) BLU(44) GRN(62) — X every 18px
- **Status text (bottom right corner):** IDLE/RUN/OK/BOOM
- **Menu:** NEVER calls `display.begin()` — uses `M5.Lcd.print()` directly with partial `fillRect`
- **Full screen:** `drawFullScreenMsg(line1, line2, color)` — fillScreen + centered text
- **Code:** `drawCodeEntry(digits, currentDigit)` — orange DSEG14, 4 digits + blinking cursor

---

## Input (InputManager.h)

### Buttons
| Button | Method | Function |
|-------|--------|--------|
| BtnA | wasPressed(BTN_PREV) | Decrease / navigate back |
| BtnB | wasPressed(BTN_MENU) | Menu (IDLE) / Confirm (MENU edit) |
| BtnC | wasPressed(BTN_NEXT) | Increase / navigate forward |
| BOOT (GPIO0) | wasPressed(BTN_START) | Start (IDLE) / Reset (DETONATED/DEFUSED/ALARM) / Detonate (RUNNING) |

### isHeld(btn, ms)
Used for long press: 600ms to trigger acceleration, 50ms to detect release.

### RF433 (rc-switch)
SYN531R on PORT.C (GPIO4). 8 commands: PREV, NEXT, MENU, START, DETONATE, DEFUSE, SPEED, PAUSE.

### Keypad (PCF8574)
- Address auto-detected (0x20-0x27) in i2cScan()
- Pin mapping configurable in Pins.h: `KEYPAD_ROW_PINS[4]` and `KEYPAD_COL_PINS[3]`
- Fixed character matrix: `123 456 789 *0#`
- Only polls I2C if `pcfOk_ == true` and the current mode is CODE

### RFID (MFRC522_I2C)
- I2C 0x28 via PORT.A
- Self-test at boot to confirm the chip's presence
- `readRFID(uidHex, &uidLen)` returns the UID in hex
- UID converted to uint32_t (first 4 bytes) for matching

---

## Game Logic (GameLogic.h)

### Config (NVS namespace "bomb")
17 parameters saved via Preferences:
- `countdownSeconds` (5-5999)
- `defuseMode`: CODE(0), WIRE(1), ORDER(2)
- `defuseCode` (0000-9999)
- `defuseOrder` (1111-4444)
- `detWire`, `speedupWire`, `pauseWire` (0=RAND, 1-4=wire, 5=NONE)
- `brightness` (0-15)
- `tickEnabled`, `rfidEnabled`
- `alarmHour`, `alarmMin`, `alarmEnabled`
- `maxCodeAttempts` (default 3)
- `detTriggerSec`, `defuseTriggerSec` (1-10s)
- `defuseTags[4]`, `detonateTags[4]` (RFID UIDs)

### Wires (Wires)
| Wire | Config | Effect |
|-----|--------|--------|
| DET | detWire (RAND/1-4/NONE) | Cut → detonates immediately |
| DEFUSE | defuseMode=WIRE, defuseWire | Cut → defuses |
| ORDER | defuseMode=ORDER, defuseOrder | Correct sequence → defuses, wrong → detonates |
| SPEEDUP | speedupWire | Cut → toggle doubleSpeed (high beep) |
| PAUSE | pauseWire | Cut → pauses 2s (low beep) |

### Code (CODE mode)
- 3 attempts (`maxCodeAttempts`, configurable)
- On each error: "ERROR / Try again" screen for 1s (fullScreenUntil, NOT delay)
- 4th attempt: detonates
- `*`/`#` or START: cancels entry

### Menu (18 items)
COUNTDOWN → MODE → CODE → ORDER → DET WIRE → SPD WIRE → PAUSE WIRE → BRIGHT → TICK → RFID ENABLE → RFID DEFU (learn) → RFID DETO (learn) → TDET → TDEF → ALARM H → ALARM M → CLR DEFU → CLR DETO

### Timed Relay Release
- `triggerDetonate()`: RLY1+2 ON, auto-release after detTriggerSec (updateRelays() in loop)
- `triggerDefuse()`: RLY3+4 ON, auto-release after defuseTriggerSec
- `clearAllRelays()`: turns all off immediately

---

## ConfigHA (AP Mode + WiFi + HA MQTT)

### Boot sequence
1. SPIFFS mount → load `/wifi.json`
2. If SSID saved: `WiFi.begin(ssid, pass)`, 20s timeout
3. If connected: `beginHA()` (ArduinoHA + MQTT)
4. If failed: AP mode `BombTimer-STAMPLC` + webserver on port 80
5. Automatic publication: `loopHA()` detects the disconnected→connected transition in MQTT and calls `publishAll()`

### AP mode
- SSID: `BombTimer-STAMPLC`
- IP: `192.168.4.1`
- Form: WiFi (SSID+Password), MQTT (Broker, Port, User, Pass), Device Name
- Saves → SPIFFS `/wifi.json` → `ESP.restart()`

### Home Assistant Entities
| Type | ID | Name | Description |
|------|----|------|-----------|
| Sensor | state | State | idle/running/defused/detonated/menu/alarm |
| Sensor | time | Remaining Time | remaining seconds |
| Sensor | wires_cut | Wires Cut | quantity |
| Sensor | code_attempts | Attempts | code attempts |
| Sensor | mode | Defuse Mode | CODE/WIRE/ORDER |
| Sensor | i2c_status | I2C Devices | detected devices |
| Sensor | wire1-4 | Wire 1-4 | 0=connected, 1=cut |
| Button | start | Start | starts countdown |
| Button | reset | Reset | resets to IDLE |
| Number | countdown | Time (seconds) | 5-5999 |
| Number | defuse_code | Defuse Code | 0-9999 |
| Number | brightness | Brightness | 0-15 |
| Select | mode | Defuse Mode | CODE/WIRE/ORDER |
| Select | det_wire | DET Wire | RAND/1/2/3/4/NONE |
| Select | defuse_wire | DEFUSE Wire | RAND/1/2/3/4/NONE |
| Select | speed_wire | SPEED Wire | RAND/1/2/3/4/NONE |
| Select | pause_wire | PAUSE Wire | RAND/1/2/3/4/NONE |
| Switch | tick_sound | Tick Sound | ON/OFF |

---

## RTC and Alarm

```cpp
struct tm t;
M5StamPLC.getRtcTime(&t);
game.checkAlarm(t.tm_hour, t.tm_min);  // match → STATE_ALARM
```

Alarm enabled automatically if alarmHour > 0 or alarmMin > 0.

---

## RGB LED

| State | Effect | Colors |
|--------|--------|-------|
| IDLE | Off | (0,0,0) |
| RUNNING | Pulsing red 500ms (250ms <10s) | (255,0,0) toggle |
| DETONATED | Solid red | (255,0,0) |
| DEFUSED | Solid green | (0,255,0) |
| ALARM | Intermittent orange 500ms | (255,128,0) toggle |
| MENU | Off | (0,0,0) |

---

## Sound Tick (important: intermittency)

### Root cause identified and fixed
The tick used `lastDisplayedRemaining` as a reference — a variable shared with `updateDisplay()`. If any function took long (keypad I2C polling, RFID, MQTT), the display did not update and the tick did not fire.

### Fixes applied
1. **tickDue()** — independent function in GameLogic.h with its own `lastTickSecond_`
2. **processKeypad()** — only calls `getKey()` if `mode == DEFUSE_CODE` **AND** `pcfOk_ == true`
3. **RFID** — only scans if `hasRFID()` is true (chip detected at boot)
4. **publishAll()** — every 2s with `lastHAPublish`, not `millis() % 1000`
5. **delay(5)** → **delay(1)** in the main loop
6. **Automatic HA publish** on MQTT connection (disconnected→connected transition)

---

## Compilation

```
FQBN: esp32:esp32:esp32s3:PartitionScheme=default_8MB,FlashMode=qio,PSRAM=enabled,UploadSpeed=921600
Partition: default_8MB (1.25MB APP)
Usage: ~38% flash (1.303.055 / 3.342.336 bytes)
RAM: ~16% (54.280 / 327.680 bytes)
```

---

## RFID Tags

- Tags stored as uint32_t (first 4 bytes of the UID)
- Up to 4 defuse tags + 4 detonation tags
- Menu: RF DEFU (learn defuse tag), RF DETO (learn detonation tag)
- Menu: CLR DEFU, CLR DETO (clear tags)
- During RUNNING with rfidEnabled: poll every 300ms, checks defuseTags and detonateTags

---

## Key Differences from the Original GameTimerPro

| Feature | Original RP2040 | StamPLC Port |
|---------|----------------|--------------|
| Display | HT16K33 4×14seg | TFT 240×135 ST7789v2 DSEG14 |
| Audio | I2S MP3 | M5StamPLC.tone (PWM) |
| Buttons | PCF8575 + IoAbstraction | M5.BtnA/B/C + BOOT |
| Neopixel | 48 WS2812 LEDs | PI4IOE5V6408 RGB LED |
| Config | LittleFS bomb.json | NVS (Preferences) |
| Network | RS485 | WiFi + ArduinoHA (MQTT) |
| Accelerometer | LIS3DH | Removed |
| Servo | PCA9685 | Removed |
| Partition | 8MB (4MB APP) | 8MB (1.25MB APP default) |
