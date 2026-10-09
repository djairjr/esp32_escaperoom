# UniversalUniqueRFID — Game Algorithm

## Overview

Learning RFID puzzle. Any RFID tag can be programmed as a "key". The GameMaster or player presents a tag, the system memorizes it, and from then on only that tag solves the puzzle.

## Configuration (JSON: unique_rfid.json)

- **Modes**: LEARN (learn tag), PLAY (play), RESET (clear)
- **Hardware**: lockPin, neoCount, volume

## States

IDLE → LEARNING → READY → AUTHENTICATING → SOLVED → IDLE

## Flow

1. LEARNING: presents tag → system memorizes UID → READY
2. READY: waiting for player
3. AUTHENTICATING: tag presented → compares with memorized UID
4. Match → SOLVED
5. No match → ignores

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
