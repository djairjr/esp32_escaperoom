# RadioClock Manual - Radio Tuner

Guide for room owners, game masters and players.

---

## 1. Overview

### What is this device?

**RadioClock** is a radio tuner puzzle. The player cycles through frequencies on an illuminated dial using a button. On the wrong frequencies, the radio plays static. On the correct frequency, a clean signal is played. The player needs to find 3 different frequencies to solve the puzzle.

### How does it work in the game?

- Players see a ring of LEDs that represents a radio dial
- Each position on the ring is a different frequency
- When the button is pressed, the dial advances to the next frequency
- The display shows the number of the current frequency
- On wrong frequencies: static on the speaker
- On the correct frequency: clean sound and the player advances a round
- Three frequencies found solve the puzzle

---

## 2. Game Components

| Component | Description |
|------------|-----------|
| **NeoPixel LED ring** | Radio dial - shows the current frequency |
| **SELECT button** | Advances to the next frequency |
| **4-digit display** | Shows "F-3" (frequency 3) or feedback |
| **Speaker** | Static on the wrong frequencies, signal on the right ones |

---

## 3. Initial Setup

### Defining the target frequencies

Edit the file `/radioClock.json`:

```json
{
  "frequencyCount": 12,
  "targetFrequencies": [3, 8, 5],
  "roundsToWin": 3
}
```

| Parameter | What it does | Suggestion |
|-----------|-----------|----------|
| `frequencyCount` | How many frequencies on the dial | 8-20 |
| `targetFrequencies` | Array with the 3 target frequencies | Different from each other |
| `roundsToWin` | How many to hit to win | 1-5 |

### Configuration examples

**Easy:** frequencyCount=8, targetFrequencies=[2, 5], roundsToWin=2
**Medium:** frequencyCount=12, targetFrequencies=[3, 8, 5], roundsToWin=3
**Hard:** frequencyCount=20, targetFrequencies=[7, 15, 3], roundsToWin=3

---

## 4. How to Play

### For the Game Master

1. Set the target frequencies in the JSON
2. Position the ring in a visible location
3. The display shows "rAdi" when ready
4. Press SELECT to begin

### For the players

1. **Press SELECT** to advance to the next frequency
2. **Listen**: static = wrong frequency, clean signal = right frequency
3. **Look** at the display: it shows "F-3" (frequency 3)
4. **Look** at the dial: the white light shows where you are on the dial
5. **When you hit it**: the display shows "GOOd" and the LEDs turn green
6. **Continue**: find the 3 target frequencies to win

### Strategy

- Cycle through all the frequencies in order
- Pay attention to the sound - it is the only hint
- The target frequency changes each round
- There is no penalty for a wrong guess - keep trying

---

## 5. What happens when solved

1. The victory sound plays
2. The LEDs do a rainbow effect
3. Display shows "donE"
4. The lock is released
5. After 8 seconds, the puzzle returns to the initial state

---

## 6. Audio Checklist

| File | When it plays | Required? |
|---------|-------------|--------------|
| `static.mp3` | Wrong frequency (static) | Yes |
| `signal.mp3` | Right frequency (clean signal) | Yes |
| `solved.mp3` | Puzzle solved | Yes |
| `select.mp3` | Changing frequency | Recommended |

Format: MP3, 128kbps, 44.1kHz, stereo, no ID3 tags.

### Tip for the audio files

- **static.mp3**: Can be a white noise file or real radio static
- **signal.mp3**: A clean tone, classical music or recorded voice "You found the frequency!"
- **select.mp3**: A short radio "click"

---

## 7. Troubleshooting

### The static sound does not play

1. Check that /sfx/static.mp3 exists in LittleFS
2. Test with `hw.test()` via Serial

### The display shows strange characters

1. Check the HT16K33 at address 0x70
2. Adjust the brightness in the JSON if necessary

### It does not find the target frequency

1. Check the values in targetFrequencies in the JSON
2. The values must be smaller than frequencyCount
3. Frequencies start at 0 (F-0, F-1, etc.)

### The puzzle does not start

1. Press SELECT to leave the idle state
2. The display should change from "rAdi" to "F-0"

---

## Quick Summary

| Action | How to do it |
|------|------------|
| Advance frequency | Press SELECT |
| Wrong frequency | Static in the audio |
| Right frequency | Clean signal + "GOOd" |
| Win | Find the 3 frequencies |

### Display States

| Display | Meaning |
|---------|-------------|
| "rAdi" | Ready, press SELECT |
| "F-3" | Frequency 3 |
| "GOOd" | Right frequency! |
| "donE" | Puzzle solved! |

### LED States

| Color/Pattern | Meaning |
|------------|-------------|
| Pulsing blue | Waiting to start |
| White + trail | Navigating frequencies |
| Pulsing green | Right frequency! |
| Rainbow | Puzzle solved! |

---

*Tune to the right frequency and unravel the mystery...*

*RadioClock Manual - Version 1.0 for the PropMaker EscapeRoom library*
