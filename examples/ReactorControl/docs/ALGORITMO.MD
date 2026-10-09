# ReactorControl — Game Algorithm

## Overview

Nuclear reactor control panel. The player must keep temperature and pressure within safe limits using the control buttons. If the parameters go out of bounds or the player fails the confirmation (pressing two buttons simultaneously), the reactor "explodes" (the puzzle fails).

## Configuration (JSON: reactor.json)

- **Limits**: maxTemperature, maxPressure
- **Rates**: tempRiseRate, pressureRiseRate, coolingRate
- **Hardware**: lockPin, neoCount, volume
- **Audio**: sfxAlarm, sfxVent, sfxScram, sfxStable

## States

IDLE → RUNNING → STABLE / WARNING / CRITICAL → SOLVED / MELTDOWN

## Flow

1. IDLE: waiting to start
2. RUNNING:
   - Temperature and pressure rise continuously
   - LEFT button: reduces temperature (venting)
   - RIGHT button: reduces pressure (valve)
   - Both simultaneous: SCRAM (emergency shutdown)
3. STABLE: parameters within limits
4. WARNING: one parameter exceeded 80%
5. CRITICAL: parameter exceeded the limit → confirmation timer
   - Player must press both buttons within X seconds
   - Failure → MELTDOWN
6. Total elapsed time without meltdown → SOLVED

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
