# BombTimer — Game Algorithm

## Overview

BombTimer is a countdown timer with multiple defuse modes. The player must defuse the bomb before time runs out, using one of the configured modes: numeric code via keypad, cutting a specific wire, or cutting wires in the correct order.

## Initial Configuration

The game loads a JSON file (`bomb.json`) with the following parameters:

- **countdownSeconds**: initial countdown time (5-5999 seconds)
- **defuseMode**: defuse mode (CODE=0, WIRE=1, ORDER=2)
- **defuseCode**: 4-digit code for CODE mode
- **defuseOrder**: 4-digit sequence (1-4) for ORDER mode
- **detWire**: detonation wire (0=RANDOM, 1-4=specific wire, 5=NONE)
- **defuseWire**: defuse wire (0=RANDOM, 1-4=specific wire, 5=NONE)
- **speedupWire**: accelerator wire (doubles countdown speed)
- **pauseWire**: pause wire (pauses the countdown for 2 seconds)
- **brightness**: display brightness (0-15)
- **volume**: audio volume (0-255)
- **tickEnabled**: tick sound every second
- **accelMode**: accelerometer mode (OFF=0, START=1, SPEED2X=2, PAUSE=3, DETONATE=4)
- **rfidEnabled**: reading of RFID tags for defuse/detonation
- **defuseTags**: up to 4 RFID tag UIDs that defuse the bomb
- **detonateTags**: up to 4 RFID tag UIDs that detonate the bomb
- **maxCodeAttempts**: maximum code attempts (default 3)
- **detTriggerSec**: how long the detonation relay stays active (1-10s)
- **defuseTriggerSec**: how long the defuse relay stays active (1-10s)

## Game States

```
IDLE       → Waiting to start. Player adjusts time and settings.
RUNNING    → Countdown active. Player tries to defuse.
DEFUSED    → Bomb defused successfully. Defuse relay active for X seconds.
DETONATED  → Bomb exploded (time ran out or wrong wire). Detonation relay active.
MENU       → Local configuration menu (display + buttons).
ALARM      → RTC alarm triggered at a scheduled time.
```

### Transitions

- **IDLE → RUNNING**: START button pressed, or accelerometer shake (ACCEL_START mode), or HA/MQTT command
- **RUNNING → DEFUSED**: correct code entered, defuse wire cut, ORDER sequence completed, defuse RFID tag read
- **RUNNING → DETONATED**: time ran out, detonation wire cut, wrong code exceeded attempts, detonation RFID tag read, shake in ACCEL_DETONATE mode
- **DEFUSED/DETONATED → IDLE**: START button pressed to reset
- **IDLE → MENU**: MENU button pressed (long press on SELECT)
- **MENU → IDLE**: 15s timeout without input, or START button

## Defuse Modes

### CODE Mode (0)
1. During RUNNING, the player types numbers on the keypad
2. On pressing the first digit, the display shows underscores for the 4 positions
3. Each digit appears on the display
4. After 4 digits, the system compares with **defuseCode**
5. If correct → DEFUSED
6. If wrong → increments the attempt count. If it exceeded **maxCodeAttempts** → DETONATED
7. The `*` or `#` keys cancel the current entry

### WIRE Mode (1)
1. On start, the wires are drawn at random (if configured as RANDOM)
2. **detWire** wire: if cut → DETONATED
3. **defuseWire** wire: if cut → DEFUSED
4. **speedupWire** wire: if cut → doubles countdown speed (each real second counts as 2)
5. **pauseWire** wire: if cut → pauses countdown for 2 seconds
6. If all 4 wires are cut → DETONATED

### ORDER Mode (2)
1. The player must cut the wires in the exact sequence defined by **defuseOrder**
2. The order is a 4-digit number where each digit (1-4) represents a wire
3. Example: order "1324" means cut wire 1 first, then wire 3, then wire 2, then wire 4
4. If cut in the correct order up to the 4th wire → DEFUSED
5. If a wire is cut out of the expected order → DETONATED

### NET Mode (3 — RS485 only)
1. The defusal depends on N other puzzles on the RS485 network reporting SOLVED state
2. **netPuzzlesCount** defines how many puzzles need to be solved
3. When the required number reaches the count → DEFUSED

## Auxiliary Features

### Wires
- 4 physical wires connected to digital inputs
- State: CONNECTED (closed circuit, LOW) or CUT (open circuit, HIGH)
- The system detects the CONNECTED→CUT transition as a cut event
- Wire conflict detection: the menu prevents the same wire from being assigned to two different functions

### Accelerometer (LIS3DH)
- **OFF**: no effect
- **START**: shake starts the countdown from IDLE
- **SPEED2X**: shake during RUNNING doubles the speed
- **PAUSE**: shake during RUNNING pauses for 2 seconds
- **DETONATE**: strong shake during RUNNING causes immediate detonation

### RFID/NFC (PN532)
- Defuse tags: up to 4 UIDs that, when read during RUNNING, cause DEFUSED
- Detonation tags: up to 4 UIDs that, when read during RUNNING, cause DETONATED
- Tag learning via local menu or HA

### Relays
- 2 detonation relays: activated for **detTriggerSec** seconds on detonation
- 2 defuse relays: activated for **defuseTriggerSec** seconds on defusal
- Both turn off automatically after the configured time

### Audio
- `/armed.mp3`: played when the countdown starts
- `/tick.mp3`: played every second (if tickEnabled)
- `/boom.mp3`: played on detonation
- `/sucess.mp3`: played on defusal
- `/keypress.mp3`: key press feedback
- `/error.mp3`: error feedback

### NeoPixel (RGB LED)
- **NORMAL**: soft white pulse (IDLE)
- **COUNTING**: red pulse (RUNNING)
- **DOUBLE_SPEED**: fast red blink (speedup active)
- **DEFUSED**: green pulse (defused)
- **EXPLOSION**: explosion animation (detonated)
- **RADIATION**: random green/blue flicker

### Display (HT16K33 14-segment)
- IDLE: shows configured time (MM:SS)
- RUNNING: countdown with blinking colon
- Menu: 4-character names (BRIG, VOL, TICK, SHAK, NET, WIRE, CODE, RELE, RFID)
- Code: entered digits with underscores for empty positions
- DEFUSED: "SAFE"
- DETONATED: "boom"

## Local Configuration Menu

Navigation via physical buttons (LEFT/RIGHT to navigate, SELECT to enter/confirm):

1. **COUNTDOWN**: time adjustment (5-5999s, step 1 short / 5 long)
2. **MODE**: defuse mode selection (CODE/WIRE/ORDER)
3. **CODE**: 4-digit code entry via keypad
4. **ORDER**: wire order entry (4-digit sequence 1-4)
5. **DET WIRE**: detonation wire (RAND/1/2/3/4/NONE)
6. **SPD WIRE**: accelerator wire (RAND/1/2/3/4/NONE)
7. **PAUSE WIRE**: pause wire (RAND/1/2/3/4/NONE)
8. **BRIGHT**: display brightness (0-15)
9. **TICK**: tick sound (ON/OFF)
10. **RFID**: enables RFID (ON/OFF)
11. **RF DEFU**: learn defuse tag (up to 4)
12. **RF DETO**: learn detonation tag (up to 4)
13. **CLR DEFU**: clear defuse tags
14. **CLR DETO**: clear detonation tags
15. **TDET**: detonation trigger time (1-10s)
16. **TDEF**: defuse trigger time (1-10s)
17. **ALARM H**: RTC alarm hour (0-23)
18. **ALARM M**: RTC alarm minute (0-59)

A 15-second timeout without input returns to IDLE.

## Network Configuration (AP Mode + WiFi)

1. On first boot, creates an open WiFi network "BombTimer-STAMPLC"
2. Accessing `http://192.168.4.1`, the GameMaster configures:
   - WiFi SSID and password
   - MQTT broker (IP, port, user, password)
   - Device name in Home Assistant
3. On saving, it reboots and connects to the configured network
4. If it cannot connect, it returns to AP mode
