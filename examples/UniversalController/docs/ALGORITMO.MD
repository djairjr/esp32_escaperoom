# UniversalController — Game Algorithm

## Overview

Universal puzzle configurable via JSON. Supports multiple operating modes: single sensor, multiple sensors, sequence, RFID, keypad, etc. The behavior is entirely defined by the configuration file.

## Configuration (JSON: universal.json)

- **mode**: puzzle type (SINGLE_SENSOR, MULTI_SENSOR, SEQUENCE, RFID, KEYPAD, etc.)
- **inputs**: input pins and their roles
- **outputs**: output pins (relays, NeoPixel)
- **timing**: timeouts, delays
- **Audio**: sfx for each event

## States

IDLE → RUNNING → SOLVED / ERROR → IDLE

## Supported Modes

1. **SINGLE_SENSOR**: a single sensor must be activated
2. **MULTI_SENSOR**: multiple sensors in any order
3. **SEQUENCE**: sensors in a specific order
4. **RFID**: specific RFID tag
5. **KEYPAD**: numeric code
6. **TOGGLE**: toggle sensor state
7. **TIMER**: wait a specific time

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
