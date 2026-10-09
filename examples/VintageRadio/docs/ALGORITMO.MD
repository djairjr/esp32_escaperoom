# VintageRadio — Game Algorithm

## Overview

Vintage radio with multiple interaction modes: tuning via rotary encoder, RFID reading for "station" authentication, and MP3 audio simulating old radio broadcasts.

## Configuration (JSON: vintage.json)

- **Input**: Rotary Encoder (tuning), RFID (authentication)
- **Modes**: TUNE (tune frequency), AUTH (authenticate with tag)
- **Hardware**: lockPin, neoCount, volume
- **Audio**: sfxStatic, sfxTune, sfxStation, sfxSolved

## States

IDLE → TUNING → AUTHENTICATED → SOLVED

## Flow

1. IDLE: radio off
2. TUNING: rotary encoder adjusts the "frequency"
   - NeoPixel shows the position on the dial
   - Audio plays static between frequencies
3. AUTHENTICATED: correct RFID tag presented at the right frequency
4. SOLVED: unlocks

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
