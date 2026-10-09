# Roulette — Game Algorithm

## Overview

Light roulette. A light travels around a ring of NeoPixels. The player presses SELECT to stop the roulette. If it stops on the winning sector, the puzzle is solved. Similar to Cyclone but with simulated deceleration.

## Configuration (JSON: roulette.json)

- **sectors**: numSectors, winningSector
- **Hardware**: lockPin, neoCount, volume

## States

IDLE → SPINNING → DECELERATING → SOLVED / MISS → IDLE

## Flow

1. IDLE: light stopped. SELECT starts.
2. SPINNING: light travels around the ring at maximum speed
3. DECELERATING: SELECT pressed → speed gradually reduces until it stops
4. Final position == winningSector → SOLVED
5. Final position != winningSector → MISS (tries again)

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1.
