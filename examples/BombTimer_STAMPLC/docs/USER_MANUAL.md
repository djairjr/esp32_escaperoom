# BombTimer STAMPLC — User Manual

> Escape room puzzle — countdown timer with wires, codes and RFID.
> Based on the M5Stack StamPLC (ESP32-S3).

---

## 1. Overview

BombTimer is a puzzle where players need to **defuse a virtual bomb** before time runs out. There are 3 different ways to do this, which can be configured by the game master.

- **CODE** — type a 4-digit numeric code on the keypad
- **WIRE** — cut the correct wire (DEFUSE) among 4 colored wires
- **ORDER** — cut the 4 wires in a specific sequence

The system has WiFi connectivity and integrates with **Home Assistant** via MQTT so the game master can monitor and control the puzzle remotely.

---

## 2. Hardware

### Front
```
┌──────────────────────────────────────┐
│                              [W][M][R][T]│  ← WiFi / MQTT / RFID / RTC
│                                      │
│          ┌─────────────┐             │
│          │   03:00     │             │  ← Timer (minutes:seconds)
│          └─────────────┘             │
│              180s                    │  ← Total in seconds
│                                      │
│  ●Y ●R ●B ●G            IDLE        │  ← Wires + Status
└──────────────────────────────────────┘
```

### Connections

| Port | Connector | Device |
|-------|----------|-------------|
| PORT.A (RED) | Grove 4-pin | 4×3 Keypad (via PCF8574 I2C) + RFID 2 Unit (MFRC522 I2C) |
| PORT.B (BLACK) | Grove 4-pin | (free) |
| PORT.C (BLUE) | Grove 4-pin | RF433 Receiver (SYN531R) — DATA on the white wire (G4) |
| USB-C | Rear | 5V Power + Upload |

### Relays (rear terminals)

| Relay | Function | Terminals |
|------|--------|--------|
| RLY1 | DETONATE 1 | NO/COM/NC |
| RLY2 | DETONATE 2 | NO/COM/NC |
| RLY3 | DEFUSE 1 | NO/COM/NC |
| RLY4 | DEFUSE 2 | NO/COM/NC |

### Opto-inputs (rear terminals)

| Input | Function | Signal |
|---------|--------|-------|
| WIRE1 (P0_5) | Yellow Wire | HIGH=connected, LOW=cut |
| WIRE2 (P0_6) | Red Wire | HIGH=connected, LOW=cut |
| WIRE3 (P0_7) | Blue Wire | HIGH=connected, LOW=cut |
| WIRE4 (P1_4) | Green Wire | HIGH=connected, LOW=cut |
| DET (P0_4) | External detonation | LOW→HIGH = detonate |

---

## 3. Buttons and Controls

| Button | Symbol | Function |
|-------|---------|--------|
| BtnA (left) | ◀ | Decrease value / Navigate back in the menu |
| BtnB (center) | ● | Enter menu / Confirm edit |
| BtnC (right) | ▶ | Increase value / Navigate forward |
| BOOT (rear) | ▶▶ | Start countdown / Reset / Detonate |

### 4×3 Keypad

```
┌───┬───┬───┐
│ 1 │ 2 │ 3 │
├───┼───┼───┤
│ 4 │ 5 │ 6 │
├───┼───┼───┤
│ 7 │ 8 │ 9 │
├───┼───┼───┤
│ * │ 0 │ # │
└───┴───┴───┘
```

- Used to type the defuse code (CODE mode)
- Used to configure the code and order in the menu
- `*` or `#` cancels the entry during typing

---

## 4. Puzzle States

### 4.1 IDLE (Rest)

Initial state. The display shows the adjustable timer and "IDLE".

**Controls in IDLE:**
- **BtnA/BtnC (short):** Adjusts the time by ±1 second
- **BtnA/BtnC (long >600ms):** Adjusts the time by ±5 seconds (speeds up every 150ms)
- **BtnB (center):** Enters the configuration menu
- **BOOT:** Starts the countdown → RUNNING
- **RF433 START:** Starts the countdown

### 4.2 RUNNING (Countdown)

The countdown is running. The timer decrements every second.

**What happens:**
- **Sound tick:** Beep every second (frequency increases in the last 10s)
- **RGB LED:** Pulsing red (500ms, 250ms in the last 10s)
- **Wires:** Cutting a wire can detonate, defuse, speed up or pause (depends on the configuration)
- **Keypad:** If CODE mode, you can type the defuse code
- **RFID:** If enabled, bringing a tag close can defuse or detonate

**Controls in RUNNING:**
- **BOOT:** Detonates immediately
- **Keypad (CODE mode):** Type the 4 code digits
- **RF433 DETONATE:** Detonates remotely
- **RF433 DEFUSE:** Defuses remotely
- **RF433 PAUSE:** Pauses for 2s
- **RF433 SPEED:** Speeds up (toggle)

### 4.3 DETONATED (Bomb exploded!)

The time ran out or a DET wire was cut.

**What appears:**
- Screen: "DETONATED! BOOM!" in red
- RGB LED: Solid red
- DETONATE relays activated for [TDET] seconds

**Controls:**
- **BOOT:** Returns to IDLE

### 4.4 DEFUSED (Bomb defused!)

The correct code was typed, the DEFUSE wire was cut, or the ORDER sequence was completed.

**What appears:**
- Screen: "DEFUSED! SUCCESS!" in green
- RGB LED: Solid green
- DEFUSE relays activated for [TDEF] seconds

**Controls:**
- **BOOT:** Returns to IDLE

### 4.5 ALARM (Alarm)

The alarm scheduled in the RTC triggered.

**What appears:**
- Screen: "ALARM! START to stop" in magenta
- RGB LED: Intermittent orange
- 3500Hz beep every 500ms

**Controls:**
- **BOOT:** Returns to IDLE

### 4.6 MENU (Configuration)

18 navigable configuration items.

**Navigation:**
- **BtnA/BtnC:** Navigate between items
- **BtnB (center):** Enters edit mode for the current item
- **BtnB (center) during edit:** Confirms and saves
- **BOOT:** Exits the menu (returns to IDLE)
- **15s Timeout:** Exits automatically

---

## 5. Configuration Menu (18 items)

| # | Item | Options | Description |
|---|------|--------|-----------|
| 0 | COUNTDOWN | 5-5999s | Countdown time |
| 1 | MODE | CODE / WIRE / ORDER | Defuse mode |
| 2 | CODE | 0000-9999 | Defuse code (CODE mode) |
| 3 | ORDER | Sequence 1-4 | Wire order (ORDER mode) |
| 4 | DET WIRE | RAND / 1-4 / NONE | Wire that detonates when cut |
| 5 | SPD WIRE | RAND / 1-4 / NONE | Wire that speeds up the timer |
| 6 | PAUSE WIRE | RAND / 1-4 / NONE | Wire that pauses 2s |
| 7 | BRIGHT | 0-15 | Display brightness |
| 8 | TICK | ON / OFF | Tick sound |
| 9 | RFID | ON / OFF | Enables RFID reading during the game |
| 10 | RF DEFU | (learn) | Bring a tag close to defuse |
| 11 | RF DETO | (learn) | Bring a tag close to detonate |
| 12 | TDET | 1-10s | Time DETONATE relays stay active |
| 13 | TDEF | 1-10s | Time DEFUSE relays stay active |
| 14 | ALARM H | 0-23 | Alarm hour |
| 15 | ALARM M | 0-59 (step 5) | Alarm minute |
| 16 | CLR DEFU | (confirm) | Clears all defuse tags |
| 17 | CLR DETO | (confirm) | Clears all detonation tags |

---

## 6. Initial Configuration (WiFi and MQTT)

### First boot

1. Connect the StamPLC via USB-C
2. The system tries to connect to the saved WiFi
3. If there is no WiFi configured, it enters **AP Mode**
4. Connect to the WiFi `BombTimer-STAMPLC` (no password)
5. Open http://192.168.4.1 in the browser
6. Fill in:
   - **WiFi SSID and Password** (of your network)
   - **MQTT Broker** (Home Assistant address)
   - **Port** (default 1883)
   - **User/Pass** (if needed)
   - **Device name** (optional)
7. Click "Save and Restart"

The StamPLC restarts, connects to WiFi and MQTT, and the entities appear in Home Assistant automatically.

### To reconfigure
- If the WiFi fails, the system returns to AP Mode automatically
- To force AP Mode, turn off the router and turn the StamPLC back on

---

## 7. Home Assistant

### Available entities

After connecting to MQTT, the following entities appear in Home Assistant:

**Sensors (monitoring):**
- `sensor.bombtimer_state` — Current state (idle/running/defused/detonated)
- `sensor.bombtimer_time` — Remaining time in seconds
- `sensor.bombtimer_wires_cut` — How many wires have already been cut
- `sensor.bombtimer_code_attempts` — Code attempts (CODE mode)
- `sensor.bombtimer_mode` — Current defuse mode
- `sensor.bombtimer_i2c_status` — I2C devices detected at boot
- `sensor.bombtimer_wire1` to `wire4` — State of each wire (0=connected, 1=cut)

**Controls:**
- `button.bombtimer_start` — Start the countdown
- `button.bombtimer_reset` — Reset to IDLE
- `number.bombtimer_countdown` — Adjust time (5-5999s)
- `number.bombtimer_defuse_code` — Change defuse code
- `number.bombtimer_brightness` — Adjust brightness (0-15)
- `select.bombtimer_mode` — Select mode (CODE/WIRE/ORDER)
- `select.bombtimer_det_wire` — Configure DET wire (RAND/1-4/NONE)
- `select.bombtimer_defuse_wire` — Configure DEFUSE wire
- `select.bombtimer_speed_wire` — Configure SPEED wire
- `select.bombtimer_pause_wire` — Configure PAUSE wire
- `switch.bombtimer_tick_sound` — Turn tick sound on/off

### Suggested automations

**Example: notify when it detonates**
```yaml
automation:
  - alias: "Bomb detonated!"
    trigger:
      - platform: state
        entity_id: sensor.bombtimer_state
        to: "detonated"
    action:
      - service: notify.mobile_app_your_phone
        data:
          message: "💥 THE BOMB EXPLODED!"
```

**Example: start game automatically**
```yaml
automation:
  - alias: "Start bomb at 19:00"
    trigger:
      - platform: time
        at: "19:00:00"
    action:
      - service: button.press
        target:
          entity_id: button.bombtimer_start
```

---

## 8. Game Scenarios

### 8.1 CODE Mode (default)

The player must type the 4-digit code on the keypad.

**Configuration:**
- MODE = CODE
- CODE = 1234 (or another)
- DET WIRE = RAND (a random wire detonates)
- SPEED WIRE = NONE
- PAUSE WIRE = NONE

**Gameplay:**
- Cut the DET wire → detonates instantly
- Type the wrong code → 3 attempts, on the 4th it detonates
- Type the correct code → defuses

### 8.2 WIRE Mode

The player must cut the correct wire (and only it).

**Configuration:**
- MODE = WIRE
- DEFUSE WIRE = 2 (for example, red wire)
- DET WIRE = RAND
- SPEED WIRE = RAND
- PAUSE WIRE = RAND

**Gameplay:**
- Cut the DEFUSE wire (red) → defuses
- Cut any other wire → may detonate (DET), speed up or pause

### 8.3 ORDER Mode

The player must cut the 4 wires in a specific sequence.

**Configuration:**
- MODE = ORDER
- ORDER = 3142 (for example)
- DET WIRE = NONE (or RAND for extra difficulty)

**Gameplay:**
- Correct order (yellow→blue→green→red) → defuses on cutting the 4th
- Any error in the order → detonates immediately

### 8.4 With RFID

Bringing an RFID tag close can defuse or detonate.

**Configuration:**
- RFID = ON
- RF DEFU: bring a tag close to teach it (up to 4 tags)
- RF DETO: bring a tag close to teach it

**Gameplay:**
- Bring the defuse tag close → defuses
- Bring the detonation tag close → detonates

### 8.5 With RF433

Remote control via 433MHz RF.

**Available commands:**
- PREV / NEXT / MENU / START — same as the physical buttons
- DETONATE — detonates remotely
- DEFUSE — defuses remotely
- SPEED — speeds up/slows down (toggle)
- PAUSE — pauses for 2s

---

## 9. Maintenance

### Firmware update
1. Connect via USB-C to the computer
2. Open the Arduino IDE or arduino-cli
3. Select board: M5Stack StamPLC (ESP32-S3)
4. Upload the new firmware

### I2C device check
At boot, the system scans both I2C buses and publishes the result to the `i2c_status` sensor in Home Assistant. Expected devices:

**Internal bus:** PI4IOE5V6408(0x43), AW9523B(0x59), RX8130CE(0x32), LM75B(0x48), INA226(0x40)

**PORT.A:** PCF8574(0x20-0x27), MFRC522(0x28)

### Troubleshooting

| Problem | Probable cause | Solution |
|----------|---------------|---------|
| Does not connect to WiFi | Incorrect password | Enter AP Mode and reconfigure |
| Keypad does not work | PCF8574 not detected | Check the PORT.A Grove cables |
| RFID does not work | Chip not present or I2C 0x28 busy | Check the I2C address |
| Tick does not play | TICK off in the menu | Turn it on in the menu (item 8) |
| Relays do not trigger | TDET/TDEF = 0 or AW9523B failure | Check the configuration and I2C |
| Display off | Brightness = 0 | Adjust BRIGHT in the menu (item 7) |
| Buttons do not respond | PI4IOE5V6408 not detected | Check the internal I2C bus |

---

## 10. Technical Specifications

| Item | Specification |
|------|---------------|
| Microcontroller | ESP32-S3 (M5Stack StamPLC) |
| Display | TFT ST7789v2 240×135, rotation 1 |
| Timer | Countdown 5-5999s |
| Precision | ±1s (based on millis()) |
| Wires | 4 opto-isolated (YEL/RED/BLU/GRN) |
| Relays | 4 channels (2 DETONATE + 2 DEFUSE) |
| Keypad | 4×3 matrix via PCF8574 I2C |
| RFID | MFRC522/WS1850S I2C (RFID 2 Unit) |
| RF433 | SYN531R (rc-switch) |
| RTC | RX8130CE (programmable alarm) |
| Connectivity | WiFi 2.4GHz + MQTT (ArduinoHA) |
| Power | USB-C 5V |
| Consumption | ~200mA (with WiFi active) |
| Persistence | NVS (Preferences) + SPIFFS (WiFi config) |

---

## 11. License and Credits

Project developed for use in escape rooms. Based on the original GameTimerPro (AVR/RP2040) and BombTimer XIAO S3.

**Libraries used:**
- M5StamPLC / M5Unified / M5GFX — display and hardware
- ArduinoHA — Home Assistant integration
- MFRC522_I2C — RFID reading
- rc-switch — RF433
- ArduinoJson — WiFi configuration
- PubSubClient — MQTT

---

## 12. Appendix: PCF8574 → Keypad Mapping

The 4×3 keypad uses 7 pins of the PCF8574: 4 for rows (output) and 3 for columns (input). The pinout varies depending on the keypad model.

**To adjust the pinout:** edit `Pins.h` and change the arrays:

```cpp
// PCF8574 pin → keypad row (pin that receives LOW to activate the row)
static const uint8_t KEYPAD_ROW_PINS[4] PROGMEM = {0, 4, 5, 6};

// PCF8574 pin → keypad col (pin read to detect a key)
static const uint8_t KEYPAD_COL_PINS[3] PROGMEM = {3, 1, 2};
```

Use a multimeter or the keypad helper program to figure out the correct mapping for your model.
