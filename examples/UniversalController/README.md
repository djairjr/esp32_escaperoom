# UniversalController

Universal Configurable Escape Room Controller using the PropMaker EscapeRoom library.

## Overview

The UniversalController is a JSON-configurable puzzle that supports multiple operating modes, integrated audiovisual feedback, and optional RS485 communication.

## Puzzle Modes

### 1. CUSTOM_SEQUENCE
Custom sequence defined in the JSON file.
- **Example**: `sequence: [1, 5, 3, 8, 2]`
- Inputs must be triggered in the specified order

### 2. LINEAR_SEQUENCE
Linear order of inputs.
- **Behavior**: 0 → 1 → 2 → 3 → ...
- Useful for simple progression puzzles

### 3. ALL_CONNECTED
All inputs must be activated, order does not matter.
- **Behavior**: logical AND of the inputs
- Progress increases as each input is activated

### 4. RANDOM
Random sequence generated automatically at boot.
- Each time it starts, a new order is generated
- There are no repetitions in the sequence

## Hardware

### Requirements
- Adafruit PropMaker Feather RP2040
- PCF8575 (I2C expander for inputs)
- NeoPixel (addressable LEDs)
- SD Card (for MP3 sounds)
- PCA9685 (optional, for PWM output)

### Connections
```
PCF8575:
- SDA -> GPIO 24 (I2C)
- SCL -> GPIO 25 (I2C)
- VCC -> 3.3V
- GND -> GND

Digital inputs (PCF8575):
- P0-P15: Connect buttons (INPUT_PULLUP, active on LOW)

NeoPixel:
- Data -> GPIO 21 (EXTERNAL_NEOPIXEL)
- VCC -> 5V (via POWER_EXT)
- GND -> GND

Relay/Lock Output:
- Pin 10 (RELAY)
```

## Configuration

### config.json File

The `data/config.json` file must be written to the board's LittleFS.

#### Structure

```json
{
  "controller": {
    "name": "Puzzle Name",
    "id": 1,
    "enabled": true
  },

  "inputs": {
    "count": 5,
    "pcfAddress": "0x20",
    "pinMapping": [0, 1, 2, 3, 4],
    "debounceMs": 50
  },

  "logic": {
    "mode": "CUSTOM_SEQUENCE",
    "sequence": [1, 3, 5, 2, 4],
    "timeoutSeconds": 300,
    "resetOnError": true,
    "allowRepeat": false
  },

  "pwm": {
    "enabled": true,
    "channel": 0,
    "minValue": 0,
    "maxValue": 100,
    "steps": 5
  },

  "audio": {
    "enabled": true,
    "inputSounds": ["/input1.mp3", "/input2.mp3"],
    "unlockSound": "/unlock.mp3",
    "errorSound": "/error.mp3",
    "resetSound": "/reset.mp3"
  },

  "neopixel": {
    "enabled": true,
    "count": 16,
    "colors": {
      "idle": [0, 0, 255],
      "active": [255, 255, 0],
      "solved": [0, 255, 0],
      "error": [255, 0, 0],
      "progress": [0, 255, 100]
    }
  },

  "rs485": {
    "enabled": false,
    "address": 1
  },

  "output": {
    "lockPin": 10,
    "activeHigh": false
  }
}
```

### Configuration Examples

Several examples are available in the `data/` folder:

- **config.json**: Custom sequence (default)
- **config_linear.json**: Linear sequence
- **config_all_connected.json**: All connected
- **config_random.json**: Random sequence

## Audio

### Required Files (SD Card root folder)

| File | Description |
|---------|-----------|
| `input1.mp3` - `inputN.mp3` | Sounds for each correct input |
| `unlock.mp3` | Sound when puzzle solved |
| `error.mp3` | Sound when input wrong |
| `reset.mp3` | Sound when puzzle reset |

### MP3 Format
- Bitrate: 128 Kbps
- Sample rate: 44.1 kHz
- Stereo
- No ID3 tags (remove with: `ffmpeg -map_metadata -1`)

## Serial Commands

During execution, commands can be sent via Serial:

| Command | Description |
|---------|-----------|
| `start` / `run` | Starts the puzzle |
| `reset` | Resets the puzzle to its initial state |
| `status` | Shows current status |
| `solve` / `force` | Forces puzzle as solved (debug) |
| `config` | Shows loaded configuration |
| `help` | Shows help |

## Puzzle States

```
IDLE → RUNNING → SOLVED
       ↓
      ERROR → (resetOnError ?) → RUNNING/IDLE
```

- **IDLE**: Waiting to start
- **RUNNING**: Processing inputs
- **INPUT_CORRECT**: Correct input feedback
- **INPUT_WRONG**: Wrong input feedback
- **SOLVED**: Puzzle complete
- **ERROR**: Error in the sequence
- **TIMEOUT**: Time elapsed

## Visual Feedback (NeoPixel)

### Modes

1. **PROGRESS_BAR** (mode 0)
   - LEDs light up as progress advances
   - Background color: `idle`
   - Progress color: `progress`

2. **CHASE** (mode 1)
   - Chase effect
   - Useful for RANDOM mode

3. **INDIVIDUAL** (mode 2)
   - Each LED represents one input

### Colors

- **idle**: Color when there is no activity
- **active**: Color when input is activated
- **solved**: Color when puzzle solved
- **error**: Color when error in the sequence
- **progress**: Progress bar color

## PWM Output

The PWM output can be used to:
- Control the voltage of a voltmeter
- Move a servo according to progress
- Control LED intensity

The PWM value is calculated automatically:
```
PWM = minValue + ((maxValue - minValue) * progress) / 100
```

## Alternative RFID with CH552G

A low-cost alternative for RFID readers. Each CH552G runs the MFRC522 driver independently and behaves like a conventional digital sensor (LOW = valid tag, HIGH = denied).

### Architecture

Unlike the classic MultiRFID (which uses a single controller + I2C expander + multiple MFRC522 on the same SPI), this approach uses **one CH552G per reader**:

```
PropMaker RP2040 (Universal Controller)
  │
  ├── GPIO_N  ←── CH552G #1 (P1.1)  ── MFRC522 #1
  ├── GPIO_N  ←── CH552G #2 (P1.1)  ── MFRC522 #2
  ├── GPIO_N  ←── CH552G #3 (P1.1)  ── MFRC522 #3
  └── ...     ←── CH552G #N (P1.1)  ── MFRC522 #N
```

Each CH552G:
- Stores up to 7 tags in internal EEPROM (DataFlash)
- Has its own SW1 button to register/clear tags
- Sends LOW on P1.1 when a registered tag is detected
- Sends HIGH otherwise
- Costs a fraction of a dedicated RFID reader

### Connection

```
CH552G (P1.1)  ──  PropMaker GPIO (any free pin)
CH552G (GND)   ──  PropMaker GND
```

No I2C, SPI, or level shifter needed. The CH552G operates at 3.3V.

### Reading in the Sketch

```cpp
// Each CH552G is treated as an ordinary digital pin
#define RFID_TAG1_PIN  12   // Pin connected to CH552G #1
#define RFID_TAG2_PIN  13  // Pin connected to CH552G #2

void setup() {
  pinMode(RFID_TAG1_PIN, INPUT_PULLUP);
  pinMode(RFID_TAG2_PIN, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(RFID_TAG1_PIN) == LOW) {
    // Puzzle 1 solved - known tag detected
  }
  if (digitalRead(RFID_TAG2_PIN) == LOW) {
    // Puzzle 2 solved
  }
}
```

### Configuration in config.json

Each CH552G occupies 1 digital input of the PCF8575 (just like a button or switch):

```json
{
  "inputs": {
    "count": 4,
    "pcfAddress": "0x20",
    "pinMapping": [0, 1, 2, 3],
    "debounceMs": 50,
    "labels": ["Button", "Switch", "RFID_Door1", "RFID_Door2"]
  },

  "logic": {
    "mode": "ALL_CONNECTED",
    "sequence": [0, 1, 2, 3]
  }
}
```

The CH552G pin connects to the PCF8575 exactly like a button:
```
CH552G (P1.1)  ──  PCF8575 P0  ──  GND via 10k resistor (external pull-up)
```

The PCF8575 already has an internal pull-up, so:
- LOW = valid tag (CH552G pulled to GND)
- HIGH = no tag or unknown tag (pull-up keeps it HIGH)

### Advantages

| Feature | Classic MultiRFID | CH552G alternative |
|---------------|-------------------|-------------------|
| Cost per reader | High (I2C expander + reader) | Low (CH552G + MFRC522) |
| Complexity | I2C protocol + library | Simple digital sensor |
| Readers per board | Limited to 8 (PCF) | Unlimited (1 pin each) |
| Autonomy | Depends on the hub | Independent (own EEPROM) |
| Tag registration | Via config.json | Via local SW1 button |

## RS485 (Optional)

For integration into a puzzle network:

1. Enable it in `config.json`:
   ```json
   "rs485": {
     "enabled": true,
     "address": 1,
     "baudRate": 115200
   }
   ```

2. Connect the RS485 module:
   - TX -> GPIO 0
   - RX -> GPIO 1

## Compilation

Using Arduino CLI:

```bash
arduino-cli compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker UniversalController.ino
```

## Sketch Upload

```bash
arduino-cli upload --fqbn rp2040:rp2040:adafruit_feather_prop_maker --port COM3 UniversalController.ino
```

## Configuration Upload (LittleFS)

Using the LittleFS plugin for Arduino:

```bash
# Packages the data/ folder
# Uploads to the board
```

Or via code in setup (first time):

```cpp
// Copy config.json from SD to LittleFS
File src = SD.open("/config.json");
File dst = LittleFS.open("/config.json", "w");
// ... copy content
```

## Dependencies

- PropMakerEscapeRoom
- ArduinoJson
- LittleFS
- Adafruit NeoPixel
- IOAbstraction
- TaskManager
- SwitchInput

## License

MIT License - Same as the PropMakerEscapeRoom library.
