# KeypadCodeEntry — Game Algorithm

## Overview

Numeric code puzzle. The player enters a 4-digit code via a matrix keypad. If correct, it unlocks. Configurable via JSON.

## Configuration (JSON: keypad.json)

- **code**: 4-digit code
- **maxAttempts**: maximum attempts
- **Hardware**: lockPin, neoCount, volume

## States

IDLE → ENTERING → CHECKING → SOLVED/ERROR → IDLE

## Flow

1. IDLE: waiting for the first key
2. Key pressed: enters ENTERING mode
3. 4 digits accumulated: checks the code
4. Correct → SOLVED (unlocks, success audio)
5. Wrong → increments the attempt count. If maxAttempts was exceeded → ERROR. Otherwise, returns to ENTERING
6. `*` or `#`: clears the entry

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
