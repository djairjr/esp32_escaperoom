# GameTimerPro Manual - Escape Room Bomb

A complete guide for room owners, game masters and players.

---

## 1. Overview

### What is this device?

The **GameTimerPro** is an **electronic timed bomb** designed for Escape Room games. It simulates a real bomb with a countdown, wires to cut, a numeric keypad and immersive sound and light effects.

### How does it work in the game?

- Players find the "bomb" inside the room
- The display shows the remaining time in minutes and seconds
- To win, they need to **defuse the bomb before time runs out**
- The defusal can be done in several ways: typing a secret code, cutting specific wires or following a sequence of wires in the correct order

---

## 2. Game Components

These are the elements the players interact with during the game:

| Component | Description |
|------------|-----------|
| **Digital display** | Shows the remaining time in MM:SS format (minutes and seconds) |
| **Numeric keypad** | Keys 0 to 9 to type defuse codes |
| **Colored wires** | Four wires that can be cut by the players |
| **DET button (Detonate)** | Red button that starts the countdown |
| **Navigation buttons** | Left, right and selector buttons (for configuration) |
| **LED lights** | Strip of colored lights that indicate the bomb's state |
| **Speaker** | Emits tick, explosion, error and success sounds |

---

## 3. Initial Configuration (for the room owner)

### How to adjust the countdown time

1. **Turn on the device** - It will show the current time on the display
2. **Use the side buttons** to adjust:
   - **Left** button: decreases the time
   - **Right** button: increases the time
   - **Quick press**: changes by 1 second
   - **Hold pressed**: changes by 5 seconds continuously
3. **Hold the SELECT button for 1 second** to save

> **Tip**: The minimum time is 10 seconds and the maximum is 99 minutes and 59 seconds.

### How to enter the configuration menu

1. With the device on, **quickly press the SELECT button** (release in less than 1 second)
2. The display will show the first menu item: **BRIG**
3. Use the left/right buttons to navigate between the options
4. To enter an option, **quickly press SELECT**
5. To save and exit, **hold SELECT for 1 second**

### Explanation of each menu option

#### BRIG (Screen brightness)
- Adjusts how bright the screen is
- Values from 0 (off) to 15 (maximum)
- Recommended: 12-15 for well-lit rooms

#### VOL (Sound Volume)
- Adjusts the volume of the sound effects
- Values from 0 (mute) to 15 (maximum)
- Recommended: 10-12 for medium-sized rooms

#### TICK (Tick sound)
- Turns the "tick" sound on or off every second
- **ON**: makes tick-tock during the countdown
- **OFF**: silent, only event sounds

#### SHAK (Accelerometer mode)
The accelerometer detects when someone moves or shakes the device. Choose an option:

| Option | Meaning | Effect in the game |
|-------|-------------|----------------|
| **OFF** | Off | The accelerometer does nothing |
| **STRT** | Start | The countdown starts when someone moves the bomb |
| **FAST** | 2x Speed | Shaking the bomb makes time pass 2x faster |
| **PAUS** | Pause | Shaking the bomb freezes the clock for 2 seconds (and emits an error) |
| **BOOM** | Immediate explosion | Shaking the bomb hard causes instant detonation |

> **Tip**: STRT mode is great for surprising players who pick up the bomb by accident!

#### NET (Communication with other devices)
- **ON**: The bomb communicates with other puzzles via network cable
- **OFF**: Works on its own, without connection
- Use ON if you have other synced devices in the room

#### WIRE (Wire configuration)
Access a submenu with 5 options to configure the wires:

**ORDR** (Order): Defines the cutting sequence when in "Wire Order" mode
- Type 4 digits from 1 to 4 (example: 2413 = wire 2, then 4, then 1, then 3)
- Each number must be unique (do not repeat)

**DET** (Detonation Wire): Which wire explodes the bomb if cut?
- **NONE**: No wire explodes
- **1/2/3/4**: Specific wire explodes
- **RAND**: Draws randomly each game

**DEF** (Defuse Wire): Which wire defuses the bomb if cut?
- **NONE**: No wire defuses
- **1/2/3/4**: Specific wire defuses
- **RAND**: Draws randomly each game

**SPD** (Accelerator Wire): Which wire speeds up the countdown if cut?
- **NONE**: No wire speeds up
- **1/2/3/4**: Specific wire speeds up to 2x
- **RAND**: Draws randomly

**PAUS** (Pause Wire): Which wire pauses the countdown for 2 seconds?
- **NONE**: No wire pauses
- **1/2/3/4**: Specific wire pauses
- **RAND**: Draws randomly

> **Important**: A wire cannot have two functions at the same time. The system warns if there is a conflict.

#### CODE (Defuse code)
- Defines the 4-digit numeric password to defuse the bomb
- When selecting, type the 4 numbers on the keypad
- The default code is **1234**

#### RELE (Trigger times)
Configures how long the outputs stay activated:

**TDET** (Detonation Time): How long the explosion trigger stays on (1 to 10 seconds)

**TDEF** (Defuse Time): How long the success trigger stays on (1 to 10 seconds)

---

## 4. How to Play

### For the Game Master - Starting the game

1. **Configure the time** desired for the match
2. **Choose the defuse mode** (code, specific wire or wire order)
3. **Explain the rules** to the players before they enter
4. **Press the DET button** (red) to start the countdown when they are ready

> Alternative: In **SHAK = STRT** mode, the countdown starts automatically when someone moves the bomb!

### For the players - What happens during the game

**At the start:**
- The display shows the time starting to decrease
- The lights turn red and pulse
- If TICK is on, a "tick" is heard every second
- The **armed.mp3** sound plays indicating the bomb is active

**During the countdown:**
- The time decreases in seconds
- The lights blink faster when little time remains
- If the SHAK mode is FAST, shaking the bomb speeds up the time!

### How to defuse the bomb

There are **4 ways** to defuse, depending on the configuration:

#### 1. CODE Mode (CODE)

When choosing the code defuse mode (CODE), the player must enter a 4-digit code on the keypad.

**Attempt System:**
- The player has **3 attempts** to get the code right
- If the code is correct: the bomb is defused
- If the code is wrong:
  - **1st to 3rd attempt:** The display shows "_Err" and the player can try again
  - **4th attempt:** The bomb explodes immediately!

**Usage example:**
1. Correct code: 1234
2. Player types: 1111 → Display shows "_Err" (attempt 1/3)
3. Player types: 2222 → Display shows "_Err" (attempt 2/3)
4. Player types: 3333 → Display shows "_Err" (attempt 3/3)
5. Player types: 4444 → **EXPLOSION!**

**Tip:** Pay attention to the code configured in the menu before starting the game!

#### 2. SPECIFIC WIRE Mode (WIRE + DEF)
- Cut the correct wire (the one configured as DEF)
- If you cut the wrong wire... BOOM!

#### 3. WIRE ORDER Mode (WIRE + ORDR)
- Cut the wires in the exact configured sequence
- Example: if the order is 2413, you must cut wire 2 first, then 4, then 1, and finally 3
- If you get the order wrong: **EXPLODES!**

#### 4. NETWORK Mode (NET)
- The bomb defuses automatically when other puzzles in the room are solved
- Requires connection to other devices

### What happens if time runs out

When the display reaches **00:00**:

1. The display shows **"boom"**
2. The explosion sound plays very loud
3. The lights do an explosion effect (yellow, orange, red)
4. The output trigger is activated (can trigger smoke, lights, another puzzle, etc.)
5. The game ends - the players lost!

### What happens on defusal

When the bomb is successfully defused:

1. The display shows the defuse code (4 numbers)
2. The success sound plays
3. The lights turn green and bright
4. The output trigger is activated (can unlock a door, release an object, etc.)
5. The players beat this puzzle!

### Understanding the wire system

The 4 wires can have different effects when cut:

| Wire Type | Effect when cut |
|-------------|------------------|
| **Detonation Wire (DET)** | **EXPLODES IMMEDIATELY!** |
| **Defuse Wire (DEF)** | Defuses the bomb - you win! |
| **Accelerator Wire (SPD)** | Time passes 2x faster! |
| **Pause Wire (PAUS)** | The clock stops for 2 seconds (gains time) |
| **Normal Wire** | No special effect |

> **Player tip**: Be careful! Cutting all the wires at once is a terrible idea... one of them probably explodes!

---

## 5. Tips for the Game Master

### How to incorporate it into the room's plot

**Story ideas:**
- **Corporate sabotage**: The players are spies who need to defuse a bomb at the headquarters of a corrupt company
- **Chemical laboratory**: A mad scientist armed a device - defuse it before the poison is released!
- **Train rescue**: A bomb on the train line - defuse it to save the passengers
- **Haunted house**: A "spirit" left a timed trap

**Clues to give the players:**
- Leave a note with part of the code (e.g. "The code starts with 12...")
- Hide a diagram showing which wire to cut
- Use another puzzle to reveal the wire order
- Leave an audio recorder with scrambled instructions

### Difficulty levels

**Easy (for beginners):**
- Time: 15-20 minutes
- Mode: Simple code (1234 or code written somewhere)
- Wires: Only the DEF defined, others without function
- SHAK: OFF

**Medium (default):**
- Time: 10-15 minutes
- Mode: Specific wire (RAND - drawn randomly)
- Wires: DET and DEF active
- SHAK: FAST or PAUS

**Hard (for experienced):**
- Time: 8-10 minutes
- Mode: Wire order (ORDR)
- Wires: All with different functions
- SHAK: BOOM (shaking explodes!)
- No TICK sound (less sound tension)

**Expert (maximum challenge):**
- Time: 5-8 minutes
- Mode: Combination of code + wire order
- Wires: All drawn randomly (nobody knows which is which!)
- SHAK: FAST + NET enabled
- Multiple synced devices

### Expected player behavior

**The Cautious**: Tries to analyze everything before acting. Give them clues along the way.

**The Impulsive**: Wants to cut wires immediately. RAND mode is perfect for surprising them!

**The Technical**: Tries to understand the logic. Complex modes like ORDR challenge this type.

**The Leader**: Organizes the team. Set up networked puzzles to force cooperation.

---

## 6. Simple Troubleshooting

### The display does not turn on

1. Check if the power cable is connected
2. Check if the display is well fitted into the connectors
3. Try adjusting the brightness in the menu (BRIG option)
4. Turn it off and on again

### There is no sound

1. Check that the volume is not at zero (VOL menu).
2. Check if the sound files are loaded on the device
3. Check if the speaker is connected correctly
4. Try restarting the device

### How to reset the device

To return all settings to factory default:

1. Turn off the device
2. Turn it on while holding the SELECT button pressed
3. Wait for the startup sound
4. All settings returned to default:
   - Time: 10 minutes (600 seconds)
   - Code: 1234
   - Volume: 50%
   - Brightness: Maximum
   - All special modes off

### The bomb does not start the countdown

1. Check if you pressed the DET button (red)
2. If in SHAK=STRT mode, try moving/shaking the bomb lightly
3. Check if it is not already counting (look if the display's colon is blinking)
4. Restart the device and try again

### The wires do not work

1. Check if the wires are well connected
2. Check if you configured some wire with a function (DET, DEF, etc.)
3. In the WIRE menu, check if the values are correct
4. RAND mode only draws when the countdown starts - the wire changes each game!

---

## 7. Audio Checklist

Make sure these files are on the device before using:

| Sound File | When It Plays | Required? |
|----------------|-------------|--------------|
| `armed.mp3` | When the countdown starts | **YES** |
| `tick.mp3` | Every second (if TICK on) | No |
| `boom.mp3` | When the bomb explodes | **YES** |
| `sucess.mp3` | When the bomb is defused | **YES** |
| `error.mp3` | When wrong code is typed or shake in PAUS mode | Recommended |
| `keypress.mp3` | When pressing buttons in the menu | Recommended |

### Correct sound file format

To ensure the sounds work perfectly:

- **Format**: MP3
- **Bit rate**: 128 kbps
- **Frequency**: 44.1 kHz (44100 Hz)
- **Channels**: Stereo (2 channels)
- **IMPORTANT**: No ID3 tags (metadata)

**How to convert your audio files:**

If you have an MP3 file and need to adjust it:

```
Use the ffmpeg program with these commands:
ffmpeg -i your_file.mp3 -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 ready_file.mp3
```

---

## Quick Summary

### Basic Controls

| Action | How To Do It |
|------|------------|
| Turn On/Off | Connect/disconnect the power |
| Start game | Press the DET button (red) or move the bomb (STRT mode) |
| Adjust time | Left/right buttons in the main menu |
| Enter menu | Quickly press SELECT |
| Save configuration | Hold SELECT for 1 second |
| Defuse (CODE mode) | Type 4 numbers (3 attempts) |
| Defuse (WIRE mode) | Cut the correct wire |
| Restart after game | Press DET again |

### LED Light States

| Color/Effect | Meaning |
|------------|-------------|
| **Soft white** | Waiting (idle) |
| **Pulsing red** | Countdown in progress |
| **Fast blinking red** | 2x speed activated |
| **Bright green** | Bomb defused! |
| **Yellow/Orange/Red** | EXPLODED! |
| **Off** | 2-second pause |

---

**Have a good game! May the players have nerves of steel...**

*GameTimerPro Manual - Version 1.0 for the PropMaker EscapeRoom library*
