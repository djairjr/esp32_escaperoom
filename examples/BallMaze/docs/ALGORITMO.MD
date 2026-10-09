# BallMaze — Game Algorithm

## Overview

It’s a physical maze where the player uses an LIS3DH accelerometer to tilt boxes, thereby moving a virtual ball across a 2D plane. The ball must reach N target areas scattered throughout the maze. The game is won when all targets are reached.

## Configuration (JSON: ballmaze.json)

- **Physics**: accelScale (sensitivity), damping, bounce (restitution), maxSpeed (terminal velocity), deadZone (dead zone)
- **Maze**: mazeW/H (dimensions), startX/Y (starting position)
- **Targets**: targetCount (2-8), targetRadius, requireOrder (sequential or free)
- **Hardware**: lockPin, neoCount, volume
- **Audio**: sfxTargetHit, sfxSolved, sfxStart

## States

IDLE → RUNNING → SOLVED → IDLE (auto-reset 10s)

## Physics (Euler integration)

1. Reads the calibrated accelerometer tilt (X, Y)
2. Applies the dead zone (ignores minimal tilts)
3. Converts tilt into acceleration (ax, ay)
4. Integrates velocity: v += a * dt
5. Applies damping: v *= (1 - damping * dt)
6. Clamps to terminal velocity
7. Integrates position: pos += v * dt
8. Wall collision: bounce (inverts velocity with restitution)
9. Detects whether the ball is over a target (distance < radius)

## Target Detection

- If requireOrder=true: targets must be visited in array order
- If requireOrder=false: any order
- On reaching all → SOLVED (releases lockPin)

## Feedback

- NeoPixel: 2D mapping (ball=white, targets=yellow, hit=green)
- Audio: sound when hitting a target, victory sound
- HT16K33 display: number of remaining targets

## AP Mode + WiFi

Network configuration via web portal at 192.168.4.1 (SSID, password, MQTT broker, device name).
