# MasterController — Game Algorithm

## Overview

Central controller of the escape room. Manages the global room timer, controls relays (main door, lights, smoke, auxiliary) and coordinates the puzzles via the RS485 network.

## Configuration

- **masterId**: identifier on the RS485 network
- **totalGameTime**: total room time in seconds
- **Relays**: MAIN_DOOR, LIGHTS, SMOKE_GEN, AUX_1..AUX_4

## States

WAITING → RUNNING → COMPLETE/EXPIRED

## Flow

1. WAITING: waiting for the command to start
2. RUNNING:
   - Global timer counting down
   - Monitors puzzles via RS485 (StatusReport)
   - GameMaster can open doors via serial ('o') or reset the timer ('r')
3. Timer reaches zero → opens the main door, turns off the lights
4. All puzzles solved → COMPLETE

## Relays

- MAIN_DOOR: open when the game ends or the GameMaster commands it
- LIGHTS: room lighting control
- SMOKE_GEN: smoke generator for atmosphere
- AUX_1..4: configurable auxiliary devices

## AP Mode + WiFi

Web portal allows full control of the room: start, pause, reset, open doors, adjust timer.
