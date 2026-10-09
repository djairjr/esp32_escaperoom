# BallMaze Manual - Maze with Accelerometer

Guide for room owners, game masters and players.

---

## 1. Overview

### What is this device?

**BallMaze** is a physical virtual maze puzzle. The box contains a PropMaker board with an LIS3DH accelerometer that detects the tilt of the box. A virtual “ball” moves on the LED screen in the direction of the tilt. The player must guide the ball to all target areas to solve the puzzle.

### How does it work in the game?

- Players find a box with LED lights
- When the box is tilted, a virtual ball moves in the direction of the tilt.
- The goal is to visit all target zones (marked in yellow on the LEDs)
- When the ball passes over a target zone, it turns green and a sound plays
- On visiting all zones, the puzzle is solved

---

## 2. Game Components

| Component | Description |
|------------|-----------|
| **NeoPixel LEDs** | A row of lights that shows the position of the ball and the target zones |
| **Box** | The device's own body - it must be tilted |
| **Speaker** | Emits movement, hit and victory sounds |

---

## 3. Initial Configuration

### Before first use

1. Upload the audio files to LittleFS or the SD Card
2. Adjust the `/ballmaze.json` file if needed (see section 8)
3. Power on the device

### Operation check

1. Power on the device
2. The LEDs should light up in a soft, pulsing blue
3. Tilt the box - the ball (white LED) should move in the direction of the tilt
4. If the LEDs blink red, the accelerometer was not detected

---

## 4. How to Play

### For the Game Master - Starting the game

1. Place the box on a flat surface
2. Explain to the players that they must tilt the box to move the ball
3. When they are ready, ask them to shake the box lightly to start
4. The puzzle is in progress when the ball starts moving

### For the players - During the game

1. **Start**: Shake the box lightly to start
2. **Move the ball**: Tilt the box in the direction you want the ball to go
3. **Find the targets**: Guide the ball to the yellow zones
4. **Watch the progress**: Each visited zone turns green
5. **Win**: When all zones are green, the puzzle is solved!

### Gameplay tips

- Gentle tilts work better than abrupt movements
- The ball has inertia - it keeps moving a bit after you stop tilting
- If the ball hits the edge, it bounces back
- Shake the box to start a new game after winning

---

## 5. What happens when it is solved

When all target zones are visited:

1. The LEDs do a rainbow effect (all colors)
2. The victory sound plays
3. The lock (if configured) is released
4. After 10 seconds, the puzzle returns to its initial state

---

## 6. Advanced Configuration (ballmaze.json)

### Physics parameters

| Parameter | What it does | Recommended Value |
|-----------|-----------|-------------------|
| `phys.accel` | Sensitivity to tilt | 600 (higher = faster) |
| `phys.damp` | Ball friction | 3.5 (higher = stops faster) |
| `phys.bounce` | Bounce off the walls | 0.3 (0 = no bounce, 1 = full bounce) |
| `phys.maxSpd` | Maximum speed | 250 |
| `phys.deadZ` | Dead zone (ignores noise) | 0.25 (lower = more sensitive) |

### Tuning tips

- **Ball too slow**: Increase `phys.accel` or decrease `phys.damp`
- **Ball too fast**: Decrease `phys.accel` or `phys.maxSpd`
- **Ball jitters while still**: Increase `phys.deadZ`
- **Targets too difficult**: Increase `targetRadius`
- **Targets too easy**: Decrease `targetRadius`

### Target zone positions

The JSON file contains a `targets` array with X, Y positions and radius:

```json
"targets": [
    {"x": 30, "y": 30, "r": 12},
    {"x": 170, "y": 30, "r": 12},
    {"x": 100, "y": 170, "r": 12}
]
```

The virtual maze has a size of 200x200 units. X=0 and Y=0 represents the top-left corner.

---

## 7. Audio Checklist

| File | When It Plays | Required? |
|---------|-------------|--------------|
| `ball_roll.mp3` | While the ball moves | Yes |
| `target_hit.mp3` | On visiting a target zone | Yes |
| `solved.mp3` | Puzzle solved | Yes |
| `idle_chime.mp3` | Every 15s while idle (optional) | No |

### File format

- **Format**: MP3
- **Bit rate**: 128 kbps
- **Frequency**: 44.1 kHz
- **Channels**: Stereo
- **No ID3 tags**

---

## 8. Troubleshooting

### The ball does not move

1. Check whether the accelerometer is working (hw.test() on the Serial)
2. Increase `phys.accel` in the JSON
3. Decrease `phys.deadZ` for greater sensitivity

### The ball jitters without moving

1. Increase `phys.deadZ` in the JSON
2. Place the box on a more stable surface
3. Check for external vibration

### Red LEDs blinking

- The LIS3DH accelerometer was not detected
- Check the I2C connection (address 0x18)
- The board may need to be soldered or replaced

### The puzzle does not start

1. Shake the box harder (minimum threshold of 15 m/s^2)
2. Check on the Serial whether the shake is being detected
3. Decrease the threshold in the code (if necessary)

---

## Quick Summary

| Action | How To Do It |
|------|------------|
| Start game | Shake the box |
| Move the ball | Tilt the box |
| Visit a target | Pass the ball over the yellow zone |
| Solve puzzle | Visit all target zones |
| Restart | After solved, wait 10s or shake |

### LED States

| Color/Pattern | Meaning |
|------------|-------------|
| Soft pulsing blue | Waiting to start (idle) |
| White + yellows | Game in progress |
| White + greens | Targets already visited |
| Rainbow | Puzzle solved! |
| Blinking red | Error: accelerometer missing |

---

*Good game! Don't let the ball fall...*

*BallMaze Manual - Version 1.0 for the PropMaker EscapeRoom library*
