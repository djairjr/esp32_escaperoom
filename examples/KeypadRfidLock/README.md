# KeypadRfidLock - Lock with Password + RFID

Combines a 4x3 keypad (I2C/PCF8575) + PN532 reader (I2C) for access control.

## Authentication Modes

| Mode | Behavior |
|------|---------------|
| `KEYPAD_ONLY` | Numeric password only |
| `RFID_ONLY` | RFID tag only |
| `KEYPAD_OR_RFID` | Password OR tag (default) |
| `KEYPAD_AND_RFID` | Password AND tag (2 factors) |

## Hardware

- 4x3 keypad via PCF8575 at 0x20
- PN532 NFC Reader via I2C
- External NeoPixel for feedback
- Relay on the configured pin (lockPin, default = 10)

## Configuration

Edit `data/lock.json` and write it to LittleFS:

```json
{
  "mode": "KEYPAD_OR_RFID",
  "lockPin": 10,
  "unlockDurationMs": 3000,
  "codes": ["1234", "5678"],
  "rfidTags": ["04:2A:6B:01", "DE:AD:BE:EF"],
  "audio": {
    "keypress": "/sfx/beep.mp3",
    "unlock": "/sfx/magic.mp3",
    "denied": "/sfx/error.mp3",
    "reset": "/sfx/cancel.mp3"
  }
}
```

If the file does not exist, it uses the default (code=1234, mode=KEYPAD_OR_RFID).

## Audio (LittleFS or SD)

The audio files must be in /sfx/:
- /sfx/beep.mp3 - Key pressed
- /sfx/magic.mp3 - Access granted
- /sfx/error.mp3 - Access denied
- /sfx/cancel.mp3 - Reset/Clear

## Build

```bash
arduino-cli compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker KeypadRfidLock.ino
```

## Dependencies

- PropMakerEscapeRoom (includes PN532, NeoPixel, Audio)
- IoAbstraction + TaskManager (keypad)
