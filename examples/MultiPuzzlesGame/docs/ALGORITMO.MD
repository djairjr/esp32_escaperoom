# MultiPuzzlesGame — Game Algorithm

## Overview

Complete escape room game with multiple puzzles (up to 9) organized in levels (up to 5). It uses the LogicManager Game Levels system. Puzzles can run in parallel (all simultaneous) or sequential (one after the other).

## Configuration (JSON: game.json)

- **Levels**: array of GameLevelConfig (puzzleIds, mode PARALLEL/SEQUENCE)
- **Global timer**: countdown for the complete room

## States

WAITING → LEVEL_1 → LEVEL_2 → ... → COMPLETE

## Flow

1. Level 1 active: puzzles run according to the mode (PARALLEL or SEQUENCE)
2. When all puzzles of the level are solved → next level
3. SEQUENCE: puzzles activated one by one, the next only after the current one is solved
4. PARALLEL: all puzzles of the level run simultaneously
5. Last level complete → COMPLETE (unlocks the final door)

## Typical Hardware

- 5x PCF8575 (40 inputs + 40 outputs)
- 2x ADS1115 (8 analog channels)
- 1x PCA9685 (16 PWM channels)
- 1x PN532 (RFID/NFC)
- 1x NeoPixel strip with 48 LEDs
- I2S Audio

## AP Mode + WiFi

Web portal shows the progress of all levels and puzzles.
