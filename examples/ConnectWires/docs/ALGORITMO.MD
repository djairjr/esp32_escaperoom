# ConnectWires — Game Algorithm

## Overview

Wire connection puzzle. The player must connect pairs of sockets with banana cables. A voltmeter (ADS1115) reads the voltage on each pair to verify the correct connection. It is solved when all pairs are correctly connected.

## Configuration (JSON: connectwires.json)

- **Sockets**: numSockets (total quantity), connectionPairs (pairs of sockets that form correct connections)
- **Hardware**: lockPin, neoCount, volume
- **Audio**: sfxConnect, sfxDisconnect, sfxSolved, sfxError

## States

IDLE → RUNNING → SOLVED → IDLE (reset by button)

## Connection Logic

1. Each socket is wired to a digital pin on the PCF8575
2. To test a pair (A, B):
   - Sets pin A as OUTPUT LOW
   - Reads pin B as INPUT_PULLUP
   - If B reads LOW: A and B are connected (closed circuit)
3. Checks all configured pairs
4. If all correct pairs are connected AND no incorrect pair: SOLVED

## Validation Modes

- **ALL_CONNECTED**: all correct pairs must be connected
- **ONLY_CORRECT**: only correct pairs may be connected (extra connection = error)
- **EXACT_MATCH**: active connections must match the configured pairs exactly

## Feedback

- NeoPixel: green for correct connections, red for incorrect ones
- Audio: beep on connecting, error sound on connecting wrong
- HT16K33 display: number of remaining connections

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
