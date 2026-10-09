# Cyclone — Game Algorithm

## Overview

Timing game. A light travels around a ring of NeoPixels at increasing speed. The player presses SELECT to stop the light on the green sector (target). Hitting N rounds solves the puzzle.

## Configuration (JSON: cyclone.json)

- **Target**: targetSize (width of the green sector, 1-N), roundsToWin (hits required)
- **Speed**: startSpeed, speedIncrement (increases with each hit), maxSpeed
- **Hardware**: lockPin, neoCount, volume
- **Audio**: sfxTick (movement), sfxHit (hit), sfxMiss (miss), sfxSolved

## States

IDLE → RUNNING → SOLVED → IDLE (auto-reset 8s)

## Game Flow

1. IDLE: pulsing blue NeoPixel. SELECT starts the game.
2. RUNNING:
   a. Target sector position randomly drawn
   b. Light travels around the ring every stepDelay = 100/speed ms
   c. Current position advances 1 LED per step
   d. SELECT pressed → checks whether the position is inside the target sector
3. Hit: increments round, increases speed, draws a new target
4. Miss: keeps the current round, draws a new target
5. All rounds complete → SOLVED (releases lock, victory sound)

## Feedback

- NeoPixel: target=dim green, current light=bright white, hit=bright green, miss=red
- Audio: tick on each movement, hit/miss sound
- Display: not used (NeoPixel only)

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
