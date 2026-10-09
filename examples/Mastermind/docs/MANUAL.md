# Mastermind Manual - Color Guessing Game

Guide for room owners, game masters and players.

---

## 1. Overview

### What is this device?

**Mastermind** is the electronic version of the classic board game. The system generates a secret sequence of 4 colors. The player makes guesses using the keypad and receives hints about how many colors are in the correct position (P) and how many colors are present but in the wrong position (E). The player has 10 attempts to get it right.

### How does it work in the game?

- Players find a numeric keypad and a display
- Each key from 1 to 6 represents a color
- The player enters 4 numbers (their guess of colors) and confirms
- The display shows "P=1 E=2" meaning: 1 color in the correct position, 2 correct colors in the wrong position
- The LEDs show the colors of the guess and the feedback
- The player has 10 attempts to guess the correct sequence

### Color Map

| Key | Color | LED Color |
|-------|-----|------------|
| **1** | Red | Red |
| **2** | Blue | Blue |
| **3** | Green | Green |
| **4** | Yellow | Yellow |
| **5** | Magenta | Pink/Magenta |
| **6** | Cyan | Light blue |

---

## 2. Game Components

| Component | Description |
|------------|-----------|
| **Numeric keypad** | Keys 1-6 for colors, * to clear, # to confirm |
| **4-digit display** | Shows the game status and P/E feedback |
| **NeoPixel LEDs** | The first 4 LEDs show the colors of the guess, the following 4 show feedback |
| **Speaker** | Key, hit, miss and victory sounds |

---

## 3. Initial Setup

### Difficulty adjustment

Edit the file `/mastermind.json`:

```json
{
  "codeLength": 4,
  "maxColors": 6,
  "maxAttempts": 10
}
```

| Parameter | What it does | Easy | Hard |
|-----------|-----------|-------|---------|
| `codeLength` | How many colors in the sequence | 3 | 6 |
| `maxColors` | How many colors available | 4 | 8 |
| `maxAttempts` | How many attempts | 15 | 6 |

### Possible combinations

| codeLength | maxColors | Combinations |
|------------|-----------|-------------|
| 3 | 4 | 64 |
| 4 | 6 | 1296 |
| 5 | 8 | 32768 |

---

## 4. How to Play

### For the Game Master

1. Set the difficulty in the JSON
2. Explain the color map to the players
3. The * key clears the current guess
4. The # key confirms and submits the guess

### For the players

1. **Look** at the display: it shows "CodE" (ready)
2. **Enter 4 numbers** from 1 to 6 to form your guess
3. **Look at the colors** on the LEDs - each LED shows the chosen color
4. **Confirm** with the # key
5. **Read the feedback**:
   - **P=1** = 1 color is in the correct position
   - **E=2** = 2 colors are correct but in the wrong position
6. **Use logic** to deduce the correct sequence
7. **Got it right?** The puzzle is solved!
8. **Got it wrong?** Try again (see how many attempts remain)

### Example game

```
Secret: [2, 4, 1, 3]  (Blue, Yellow, Red, Green)

Guess 1: [1, 2, 3, 4] -> P=0 E=3
  (colors 1,2,4 exist but in the wrong position; color 3 does not exist)

Guess 2: [2, 1, 4, 3] -> P=2 E=2
  (positions 1 and 4 correct; colors 1 and 4 swapped)

Guess 3: [2, 4, 1, 3] -> P=4 E=0 -> SOLVED!
```

### Strategy

- Start with all different colors to find out which ones are present
- Pay attention to P (correct position) - it means that color is already in the right place
- Use E (color exists) to know which colors are part of the code
- Process of elimination is the key

---

## 5. What happens when solved

1. The display shows "donE"
2. The victory sound plays
3. The LEDs turn green
4. The lock is released

### If the attempts run out

1. The display shows "noPE"
2. The error sound plays
3. The LEDs turn red
4. The secret code is revealed on the Serial (for debug)

---

## 6. Audio Checklist

| File | When it plays | Required? |
|---------|-------------|--------------|
| `beep.mp3` | When pressing a key | Yes |
| `magic.mp3` | Code discovered | Yes |
| `error.mp3` | Attempts exhausted | Yes |
| `cancel.mp3` | Guess cleared | No |

Format: MP3, 128kbps, 44.1kHz, stereo, no ID3 tags.

---

## 7. Troubleshooting

### Keypad does not respond

1. Check the PCF8575 at address 0x20
2. Test with `hw.test()` via Serial

### Display shows nothing

1. Check the HT16K33 at address 0x70
2. Adjust the brightness in the JSON

### The LED colors do not match

The colors are approximate. Red is FF0000, blue is 0000FF, etc. Adjust in the code if necessary.

---

## Quick Summary

| Key | Function |
|-------|--------|
| **1-6** | Selects a color |
| ***** | Clears the guess |
| **#** | Confirms the guess |

### Understanding the feedback

| Display | Meaning |
|---------|-------------|
| "CodE" | Game ready, waiting for a guess |
| "P2E1" | 2 colors in the correct position, 1 correct color in the wrong position |
| "donE" | Puzzle solved! |
| "noPE" | Attempts exhausted |

---

*Good luck! Use logic...*

*Mastermind Manual - Version 1.0 for the PropMaker EscapeRoom library*
