# EscapeRoomPhone — Game Algorithm

## Overview

Vintage phone simulator. The player takes the handset off the hook, hears a dial tone, dials a number (keypad or rotary dial), and hears a corresponding audio message. If the dialed number has an associated .txt file, the electric lock is released.

## Configuration

- **Input**: I2C Keypad (PCF8574), direct GPIO Keypad, or Rotary Dial
- **Hardware**: hookPin (hook sensor), lockPin (lock relay), externalPower
- **Timeout**: DIAL_TIMEOUT_MS=3000 (maximum time between digits)
- **Maximum digits**: MAX_PHONE_DIGITS=8

## States

IDLE → OFF_HOOK → DIALING → PLAYING → BUSY → IDLE (hook)

## Game Flow

1. **Handset on the hook**: system at rest, any audio is stopped
2. **Handset lifted**: plays the dial tone in a loop
3. **Dialing**:
   - Keypad: keys 0-9, *, # with DTMF feedback
   - Rotary: decoded pulses with pulse feedback
   - Timeout of 3s or 8 digits → processes the call
4. **Processing**:
   - Checks whether file `/NUMBER.mp3` exists on the SD
   - If yes: plays the audio
   - If there is `/NUMBER.txt`: activates lockPin (unlocks the door)
   - If not: plays the invalid number message
5. **Post-audio**: plays busy tone until the handset is put back
6. **Handset back on the hook**: resets everything, stops any audio

## Feedback

- Audio: dialtone, DTMF per key, pulse per dial position, audio message, busy tone
- Does not use NeoPixel or display (audio only)

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
