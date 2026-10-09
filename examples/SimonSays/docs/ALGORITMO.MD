# SimonSays — Game Algorithm

## Overview

Memory game in the Simon Says style. The system plays a sequence of colors/sounds (4 colored buttons). The player must repeat the sequence in the correct order. Each round, the sequence grows by 1 item.

## Configuration (JSON: simon.json)

- **Buttons**: 4 buttons with colors (Red, Blue, Green, Yellow)
- **maxRounds**: maximum rounds (or infinite until a mistake)
- **timeBetweenNotes**: interval between notes of the sequence
- **Hardware**: lockPin, neoCount, volume
- **Audio**: 4 tones per color, sfxError, sfxRound, sfxSolved

## States

IDLE → SHOWING → WAITING → CORRECT / WRONG → SOLVED / GAMEOVER

## Flow

1. IDLE: START starts
2. SHOWING: system plays the sequence (light + sound per color)
3. WAITING: player repeats by pressing buttons
4. If the full sequence is correct → next round (sequence +1)
5. If wrong → GAMEOVER or restart (depending on config)
6. If maxRounds is completed → SOLVED

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
