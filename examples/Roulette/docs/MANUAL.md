# Roulette Manual - Wheel of Fortune

Guide for room owners, game masters and players.

---

## 1. Overview

### What is this device?

The **Roulette** is an electronic light roulette. The player presses a button to spin the LED ring. The light runs around the ring and gradually slows down until it stops at a position. If it stops on the winning sector (marked in green), the player wins the round. Three won rounds solve the puzzle.

### How does it work in the game?

- Players see a ring of LEDs with a sector lit in green
- When the button is pressed, a white light starts running around the ring
- The light slows down until it stops
- If it stops on the green sector, the player wins the round
- If it stops outside, the player can try again
- Three hits solve the puzzle

---

## 2. Game Components

| Component | Description |
|------------|-----------|
| **NeoPixel LED Ring** | The roulette itself |
| **SELECT Button** | To spin the roulette |
| **Speaker** | Spin, win, lose and solve sounds |

---

## 3. Initial Configuration

### Roulette adjustment

Edit the `/roulette.json` file:

```json
{
  "spinSpeed": 8.0,
  "friction": 0.97,
  "minSpeed": 0.1,
  "winSectorCount": 1,
  "roundsToWin": 3
}
```

| Parameter | Effect | Easy | Hard |
|-----------|--------|-------|---------|
| `spinSpeed` | Initial speed | 6.0 | 12.0 |
| `friction` | Friction (0.99 = slows down slowly) | 0.98 | 0.95 |
| `winSectorCount` | How many LEDs are winners | 3 | 1 |
| `roundsToWin` | Hits required to win | 1 | 5 |

### Expected behavior

- friction = 0.97: roulette spins ~12 turns before stopping
- friction = 0.95: roulette spins ~6 turns (faster)
- friction = 0.99: roulette spins ~50 turns (very slow to stop)

---

## 4. How to Play

### For the Game Master

1. Adjust the difficulty in the JSON
2. Position the ring in a visible location
3. Show the players that the green sector is the target

### For the players

1. **See** the winning sector (pulsing green)
2. **Press SELECT** to spin the roulette
3. **Follow** the light slowing down
4. **If it hits**: green! One more step to win
5. **If it misses**: red. Try again!
6. **After 3 hits**: puzzle solved!

### Tips

- The winning sector is always visible (pulsing green)
- The roulette always slows down - there is no trick
- Watch the deceleration pattern to predict where it will stop
- Luck is the main factor, but persistence wins

---

## 5. What happens when it is solved

1. The victory sound plays
2. The LEDs do a rainbow effect
3. The lock is released
4. After 8 seconds, a new winning sector is drawn and the puzzle returns to the start

---

## 6. Audio Checklist

| File | When It Plays | Required? |
|---------|-------------|--------------|
| `spin.mp3` | When spinning the roulette | Yes |
| `win.mp3` | Hit the sector | Yes |
| `lose.mp3` | Missed the sector | Yes |
| `solved.mp3` | 3 hits | Yes |

Format: MP3, 128kbps, 44.1kHz, stereo, no ID3 tags.

---

## 7. Troubleshooting

### The roulette does not spin

1. Check the SELECT button (GPIO 4)
2. Check the NeoPixel ring

### The roulette stops too quickly

1. Increase `friction` in the JSON (0.98-0.99)
2. Increase `spinSpeed` (10.0-15.0)

### The roulette takes too long to stop

1. Decrease `friction` (0.95-0.96)
2. Increase `minSpeed` (0.2-0.3)

### It never hits the sector

1. Increase `winSectorCount` (3-5)
2. Check whether `roundsToWin` is too high

---

## Quick Summary

| Action | How To Do It |
|------|------------|
| Spin roulette | Press SELECT |
| Winning sector | Pulsing green LED |
| Hit | Solid green LED |
| Miss | Solid red LED |
| Win | 3 hits |

### LED States

| Color/Pattern | Meaning |
|------------|-------------|
| Pulsing green | Winning sector visible (idle) |
| White + trail | Roulette spinning |
| Solid green | Hit! |
| Solid red | Missed! |
| Rainbow | Puzzle solved! |

---

*Good luck! May the roulette be in your favor...*

*Roulette Manual - Version 1.0 for the PropMaker EscapeRoom library*
