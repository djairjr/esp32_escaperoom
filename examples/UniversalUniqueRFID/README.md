# UniversalUniqueRFID

Universal Configurable RFID Reader for Escape Room using the PropMaker EscapeRoom library.

## Overview

The UniversalUniqueRFID is a JSON-configurable RFID puzzle that supports two operating modes with audiovisual feedback and optional RS485 communication.

## Puzzle Modes

### 1. UNIQUE (Unique)

Any RFID tag programmed in the JSON solves the puzzle.

**Behavior:**
- Present any registered tag
- Puzzle solved immediately
- Ideal for "single key" puzzles

### 2. SEQUENCE (Sequence)

Tags must be read in a specific order.

**Example:** `sequence: [0, 2, 1]` means Tag 0 -> Tag 2 -> Tag 1

**Behavior:**
- Mandatory order according to the sequence
- Wrong tag: plays error sound (resets if configured)
- Ideal for "RFID code" puzzles

## Hardware

### Requirements
- Adafruit PropMaker Feather RP2040
- PN532 NFC/RFID module (I2C)
- NeoPixel (addressable LEDs)
- SD Card (for MP3 sounds)
- RFID Tags (MIFARE Classic, NTAG, etc.)

### Connections
```
PN532 (I2C):
- SDA -> GPIO 24 (I2C SDA)
- SCL -> GPIO 25 (I2C SCL)
- VCC -> 3.3V
- GND -> GND
- IRQ  -> (optional, not connected by default)

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
    "id": 2,
    "enabled": true
  },

  "rfid": {
    "mode": "UNIQUE",
    "tags": [
      { "uid": "A1B2C3D4", "name": "Red Tag" },
      { "uid": "E5F6A7B8", "name": "Blue Tag" },
      { "uid": "9A0B1C2D", "name": "Green Tag" }
    ],
    "validTags": [
      { "uid": "A1B2C3D4", "name": "Red Tag" },
      { "uid": "E5F6A7B8", "name": "Blue Tag" },
      { "uid": "9A0B1C2D", "name": "Green Tag" },
      { "uid": "11223344", "name": "Yellow Tag" }
    ],
    "sequence": [0, 1, 2],
    "timeoutSeconds": 300,
    "resetOnError": true
  },

  "audio": {
    "enabled": true,
    "tagSound": "/tag.mp3",
    "correctSound": "/correct.mp3",
    "wrongSound": "/wrong.mp3",
    "unlockSound": "/unlock.mp3",
    "errorSound": "/error.mp3",
    "resetSound": "/reset.mp3"
  },

  "neopixel": {
    "enabled": true,
    "count": 8,
    "colors": {
      "idle": [0, 0, 255],
      "tagRead": [255, 255, 0],
      "correct": [0, 255, 0],
      "wrong": [255, 0, 0],
      "solved": [0, 255, 0]
    }
  },

  "output": {
    "lockPin": 10,
    "activeHigh": false
  }
}
```

### Obtaining Tag UIDs

To find a tag's UID:
1. Compile and upload the sketch
2. Open the serial monitor (115200 baud)
3. Bring the tag close to the PN532 reader
4. The UID will be displayed in hexadecimal format

## Audio

### Required Files (LittleFS or SD Card root)

| File | Description |
|---------|-----------|
| `tag.mp3` | Sound when any tag is detected |
| `correct.mp3` | Sound when tag is correct |
| `wrong.mp3` | Sound when tag is wrong |
| `unlock.mp3` | Sound when puzzle solved |
| `error.mp3` | Error sound |
| `reset.mp3` | Reset sound |

### MP3 Format
- Bitrate: 128 Kbps
- Sample rate: 44.1 kHz
- Stereo
- No ID3 tags (remove with: `ffmpeg -map_metadata -1`)

## Serial Commands

| Command | Description |
|---------|-----------|
| `start` / `run` | Starts the puzzle |
| `reset` | Resets the puzzle to its initial state |
| `status` | Shows current status |
| `solve` / `force` | Forces puzzle as solved (debug) |
| `config` | Shows loaded configuration |
| `tags` | Lists programmed tags |
| `help` | Shows help |

## Puzzle States

```
IDLE -> RUNNING -> SOLVED
              -> TAG_WRONG -> (resetOnError? ?) -> RUNNING/IDLE
```

- **IDLE**: Waiting to start
- **RUNNING**: Processing tag reads
- **TAG_CORRECT**: Correct tag feedback
- **TAG_WRONG**: Wrong tag feedback
- **SOLVED**: Puzzle complete
- **TIMEOUT**: Time elapsed

## Compilation

```bash
arduino-cli compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker UniversalUniqueRFID.ino
```

## Sketch Upload

```bash
arduino-cli upload --fqbn rp2040:rp2040:adafruit_feather_prop_maker --port COM3 UniversalUniqueRFID.ino
```

## Dependencies

- PropMakerEscapeRoom
- ArduinoJson
- LittleFS
- Adafruit NeoPixel
- Adafruit PN532
- IOAbstraction
- TaskManager
- SwitchInput

## License

MIT License - Same as the PropMakerEscapeRoom library.
