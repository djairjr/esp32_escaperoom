# KeypadRfidLock — Game Algorithm

## Overview

Lock with dual authentication: code via keypad OR RFID tag. The player can unlock using either method.

## Configuration (JSON: rfidlock.json)

- **Modes**: KEYPAD, RFID, BOTH
- **code**: 4-digit code
- **rfidTags**: UIDs of authorized tags
- **Hardware**: lockPin, neoCount, volume

## States

IDLE → AUTHENTICATING → SOLVED → IDLE

## Flow

1. IDLE: waiting for input
2. Keypad: same as KeypadCodeEntry
3. RFID: reads the tag, compares the UID against the authorized list
4. Both methods can be used simultaneously in BOTH mode
5. Success → unlocks for a configurable timeout, then re-locks

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
