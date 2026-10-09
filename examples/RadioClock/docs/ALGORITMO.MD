# RadioClock — Game Algorithm

## Overview

Vintage radio tuner. The player presses SELECT to cycle through "frequencies" shown on the display and on a ring of NeoPixels. Audio plays static on the wrong frequencies and a clean signal on the correct one. Hitting 3 target frequencies solves the puzzle.

## Configuration (JSON: radioClock.json)

- **frequencyCount**: how many positions on the dial
- **targetFrequencies[3]**: target positions for each round
- **roundsToWin**: rounds required (default 3)
- **Audio**: sfxStatic, sfxSignal, sfxSolved, sfxSelect

## States

IDLE → RUNNING → SOLVED → IDLE (auto-reset 8s)

## Flow

1. IDLE: pulsing dim blue NeoPixel
2. RUNNING:
   - SELECT advances to the next frequency
   - NeoPixel shows the current position (white) with a trail
   - HT16K33 display shows "F--X" (X = current frequency)
3. If current frequency == targetFrequencies[round]:
   - Plays sfxSignal
   - Pulsing green NeoPixel
   - Display "GOOd"
   - Increments the round
4. If not: plays sfxStatic
5. All rounds complete → SOLVED

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
