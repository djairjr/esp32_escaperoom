# KeypadCodeMultiple Manual - Multiple Numeric Password

Guide for room owners, game masters and players.

---

## 1. Overview

### What is this device?

The **KeypadCodeMultiple** is a numeric password puzzle where the player must enter a 4-digit code on the keypad to unlock something. Unlike single-password puzzles, this one accepts **multiple valid codes**, allowing different clues in the room to lead to different codes that all work.

### How does it work in the game?

- Players find a numeric keypad
- They must enter 4 digits and confirm
- If the code is in the list of valid codes, the puzzle is solved
- If the code is wrong, the display (if present) shows an error and they can try again
- There is no attempt limit

---

## 2. Game Components

| Component | Description |
|------------|-----------|
| **Numeric keypad** | 4x3 keypad with digits 0-9, * (clear) and # (confirm) |
| **NeoPixel LEDs** | Indicate the puzzle state (idle, typing, solved, error) |
| **HT16K33 display** | Shows the entered digits and feedback (optional) |
| **Lock/Relay** | External device that is released when solved |

---

## 3. Initial Configuration

### Default codes

By default, the puzzle accepts three codes: **1234**, **5678** and **0000**.

### How to change the codes

Edit the `/keypadmulti.json` file in LittleFS:

```json
{
    "codes": ["1234", "5678", "0000"]
}
```

Add or remove codes as needed. Each code must have exactly 4 digits.

### Via Serial

Connect via USB and use the commands:
- `codes add 4321` - Adds code 4321
- `codes list` - Shows all codes
- `codes clear` - Removes all codes

---

## 4. How to Play

### For the Game Master

1. Define the valid codes before the game starts
2. Hide clues around the room that lead to different codes
3. The puzzle is always ready - just start typing

### For the players

1. Enter a 4-digit code
2. Press **#** to confirm
3. If wrong, press ***** to clear and start over
4. If the code is correct, the puzzle is solved

### Tips for creating clues

- **Single clue**: Hide a note with the exact code
- **Multiple clues**: Each clue leads to a different code (all of them work)
- **Riddle**: "The year Brazil was discovered" = 1500
- **Math operation**: "7+5, 3+4, 9-1, 2+2" = 1284
- **Clock**: "Half past midnight" = 0030

---

## 5. What happens when it is solved

1. The success sound plays
2. The LEDs turn green
3. The lock is released (if configured)
4. The display shows a success message

---

## 6. JSON Parameters

```json
{
  "codes": ["1234", "5678", "0000"],
  "lockPin": 20,
  "audio": {
    "keypress": "/sfx/beep.mp3",
    "unlock": "/sfx/magic.mp3",
    "denied": "/sfx/error.mp3",
    "cancel": "/sfx/cancel.mp3"
  },
  "neopixel": {
    "count": 8,
    "colorIdle": [5, 5, 20],
    "colorUnlock": [0, 50, 0],
    "colorDenied": [50, 0, 0]
  }
}
```

### Fine tuning

| Parameter | What it does |
|-----------|-----------|
| `lockPin` | Physical pin of the lock (GPIO) |
| `neopixel.count` | How many LEDs are connected |
| `neopixel.colorIdle` | LED color when waiting |
| `neopixel.colorUnlock` | LED color when correct |
| `neopixel.colorDenied` | LED color when wrong |

---

## 7. Audio Checklist

| File | When It Plays | Required? |
|---------|-------------|--------------|
| `beep.mp3` | When a key is pressed | Yes |
| `magic.mp3` | Correct code | Yes |
| `error.mp3` | Wrong code | No |
| `cancel.mp3` | Cancel entry | No |

Format: MP3, 128kbps, 44.1kHz, stereo, no ID3 tags.

---

## 8. Troubleshooting

### Keypad does not respond

1. Check the PCF8575 at address 0x20
2. Test with `hw.test()` via Serial
3. Check the soldering of the keypad cables

### Valid code does not solve

1. Check that the code is in the JSON list
2. Restart the device after changing the JSON
3. Use `codes list` via Serial to verify

### Lock does not release

1. Check the lockPin pin in the JSON
2. Test the relay with `hw.test()`
3. Check the power supply of the lock

---

## Quick Summary

| Action | How To Do It |
|------|------------|
| Enter code | Keys 0-9 |
| Confirm | # key |
| Clear | * key |
| Solve puzzle | Enter a valid code and confirm |

---

*KeypadCodeMultiple Manual - Version 1.0 for the PropMaker EscapeRoom library*
