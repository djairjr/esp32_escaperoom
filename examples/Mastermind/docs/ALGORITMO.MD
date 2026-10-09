# Mastermind — Game Algorithm

## Overview

Color guessing game. The system generates a secret sequence of N colors (1-6). The player makes attempts by entering colors via the keypad. For each attempt, the system reports how many colors are in the correct position (P) and how many colors are present but in the wrong position (E).

## Configuration (JSON: mastermind.json)

- **codeLength**: length of the sequence (2-8)
- **maxColors**: available colors (2-6)
- **maxAttempts**: maximum attempts
- **Colors**: Red(1), Blue(2), Green(3), Yellow(4), Magenta(5), Cyan(6)

## States

IDLE → RUNNING → SOLVED / GAMEOVER

## Flow

1. System generates a random secret sequence
2. Player enters N colors via the keypad (1-6)
3. `#` submits the attempt, `*` clears
4. System computes:
   - P (exact): colors in the correct position
   - E (color): colors present but in the wrong position
5. Feedback on the display: "P=E=" and NeoPixel
6. P == N → SOLVED
7. Attempts == maxAttempts → GAMEOVER

## Feedback Algorithm

1. Marks exact positions (correct color and position)
2. Removes those positions from the comparison
3. For each remaining color in the guess, checks whether it exists in the secret (in a different position)
4. Increments the "color present" count

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
