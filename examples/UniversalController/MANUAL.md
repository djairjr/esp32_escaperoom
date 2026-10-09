# User Manual - Universal Controller

## Universal Escape Room Controller

**Version:** 1.0  
**Date:** April 2026  
**Compatibility:** Adafruit PropMaker Feather RP2040

---

## Table of Contents

1. [Overview](#overview)
2. [Package Contents](#package-contents)
3. [Installation](#installation)
4. [Configuration](#configuration)
5. [Operating Modes](#operating-modes)
6. [Daily Operation](#daily-operation)
7. [Troubleshooting](#troubleshooting)
8. [Maintenance](#maintenance)
9. [Technical Specifications](#technical-specifications)

---

## Overview

The **Universal Controller** is a programmable controller for escape room puzzles. It lets you create sequential puzzles configurable via a JSON file, with no programming required.

### Main Features

- Up to **16 digital inputs** (buttons, sensors, etc.)
- **4 configurable puzzle modes**
- **Audio synchronized** with puzzle events
- **Visual effects** with addressable LEDs
- **Gradual PWM output** (voltmeter, servo, etc.)
- Optional **RS485 communication** for networks

### Typical Applications

- Boxes with buttons in sequence
- Control panels
- Lever systems
- Connection puzzles
- Multi-stage systems

---

## Package Contents

Check that all components are present:

| Item | Quantity | Description |
|------|------------|-----------|
| PropMaker Feather RP2040 | 1 | Main controller board |
| PCF8575 | 1 | I/O expander (16 inputs) |
| SD Card Module | 1 | For audio storage |
| 10kΩ Resistors | 16 | Pull-up for inputs (if needed) |
| USB-C Cable | 1 | For programming and power |
| User Manual | 1 | This document |

**Recommended accessories (not included):**

- NeoPixel LED strip (WS2812B)
- Buttons or sensors for inputs
- RS485 module (for network communication)
- 5V 3A power supply

---

## Installation

### Wiring Diagram

```
┌─────────────────────────────────────────────────────────────┐
│              Adafruit PropMaker Feather RP2040              │
│                                                             │
│   ┌──────────┐     I2C       ┌──────────┐                   │
│   │          │◄──────────────►│ PCF8575  │                   │
│   │          │  SDA: GPIO 24 │          │──► Inputs P0-P15  │
│   │          │  SCL: GPIO 25 └──────────┘                   │
│   │          │                                               │
│   │          │◄──────────────┐                              │
│   │          │  SPI         │                              │
│   │          │  MISO: GPIO 8 │                              │
│   │          │  MOSI: GPIO 15│     ┌──────────┐            │
│   │          │  SCK: GPIO 14 │     │ SD Card  │            │
│   │          │  CS: GPIO 9   │────►│          │            │
│   │          │              │     └──────────┘            │
│   │          │               │                              │
│   │          │◄──────────────┐                              │
│   │          │  NeoPixel    │                              │
│   │          │  Data: GPIO 21│     ┌──────────┐            │
│   │          │──────────────►────►│ LEDs     │            │
│   │          │               │     └──────────┘            │
│   │          │               │                              │
│   │          │◄──────────────┐                              │
│   │          │  Relay/Lock   │     ┌──────────┐            │
│   │          │  Pin 10       │────►│ Lock     │            │
│   │          │               │     └──────────┘            │
│   └──────────┘              │                              │
│                              │                              │
└──────────────────────────────┴──────────────────────────────┘
```

### Step-by-Step Installation

#### 1. PCF8575 Connection

| PCF8575 | PropMaker |
|---------|-----------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 24 (SDA) |
| SCL | GPIO 25 (SCL) |
| A0, A1, A2 | Configure address (see table below) |

**I2C Address Configuration:**

| A2 | A1 | A0 | Address |
|----|----|----|----------|
| GND | GND | GND | 0x20 |
| GND | GND | VCC | 0x21 |
| GND | VCC | GND | 0x22 |
| GND | VCC | VCC | 0x23 |
| VCC | GND | GND | 0x24 |
| VCC | GND | VCC | 0x25 |
| VCC | VCC | GND | 0x26 |
| VCC | VCC | VCC | 0x27 |

#### 2. Input Connections (P0-P15)

Each input accepts:
- Button (normally open contact)
- Proximity sensor
- Magnetic switch
- Any dry contact

**Wiring Schematic:**

```
    +3.3V
      │
     [R] 10kΩ (internal pull-up of the PCF8575)
      │
      ├──► P0 (PCF8575 input)
      │
    [BUTTON]
      │
     GND
```

> **Note:** The PCF8575 has an internal pull-up. External resistors are optional for greater stability.

#### 3. NeoPixel Connection (Optional)

| NeoPixel | PropMaker |
|----------|-----------|
| VCC | 5V (via external power pin) |
| GND | GND |
| Data | GPIO 21 |

> **Warning:** Set pin 23 (POWER_EXT) to HIGH to power the external LEDs.

#### 4. Lock Output Connection (Optional)

| Device | PropMaker |
|-------------|-----------|
| Relay/Lock | Pin 10 |
| GND | GND |

---

## Configuration

### SD Card Preparation

1. Format the SD card as FAT32
2. Create the `/sounds/` folder in the root
3. Copy the required MP3 files (see table below)

**Required Audio Files:**

| File | Description | Format |
|---------|-----------|---------|
| `input1.mp3` | Sound for correct 1st input | MP3, 128kbps, 44.1kHz |
| `input2.mp3` | Sound for correct 2nd input | MP3, 128kbps, 44.1kHz |
| ... | ... | ... |
| `unlock.mp3` | Puzzle solved sound | MP3, 128kbps, 44.1kHz |
| `error.mp3` | Error sound | MP3, 128kbps, 44.1kHz |
| `reset.mp3` | Reset sound | MP3, 128kbps, 44.1kHz |

> **Important:** Remove ID3 tags from the MP3 files using the command:  
> `ffmpeg -i input.mp3 -map_metadata -1 output.mp3`

### JSON File Configuration

The `config.json` file must be written to the board's **LittleFS**.

#### Basic Configuration Example

```json
{
  "controller": {
    "name": "Box Puzzle",
    "id": 1,
    "enabled": true
  },

  "inputs": {
    "count": 5,
    "pcfAddress": "0x20",
    "pinMapping": [0, 1, 2, 3, 4],
    "debounceMs": 50
  },

  "logic": {
    "mode": "CUSTOM_SEQUENCE",
    "sequence": [1, 3, 5, 2, 4],
    "timeoutSeconds": 300,
    "resetOnError": true
  },

  "audio": {
    "enabled": true,
    "inputSounds": [
      "/input1.mp3",
      "/input2.mp3",
      "/input3.mp3",
      "/input4.mp3",
      "/input5.mp3"
    ],
    "unlockSound": "/unlock.mp3",
    "errorSound": "/error.mp3",
    "resetSound": "/reset.mp3"
  },

  "neopixel": {
    "enabled": true,
    "count": 8,
    "colors": {
      "idle": [0, 0, 255],
      "solved": [0, 255, 0],
      "error": [255, 0, 0],
      "progress": [0, 255, 100]
    }
  },

  "output": {
    "lockPin": 10,
    "activeHigh": false
  }
}
```

---

## Operating Modes

### 1. CUSTOM_SEQUENCE (Custom Sequence)

Inputs must be triggered in the exact order specified in the JSON.

**Configuration Example:**
```json
"logic": {
  "mode": "CUSTOM_SEQUENCE",
  "sequence": [1, 3, 5, 2, 4],
  "timeoutSeconds": 300,
  "resetOnError": true
}
```

**Behavior:**
- Correct input: advances to the next
- Wrong input: plays the error sound and resets (if `resetOnError: true`)

**Application:** Puzzles where order matters, such as secret codes.

---

### 2. LINEAR_SEQUENCE (Linear Sequence)

Inputs must be triggered in ascending numerical order.

**Configuration Example:**
```json
"logic": {
  "mode": "LINEAR_SEQUENCE",
  "timeoutSeconds": 600,
  "resetOnError": true
}
```

**Behavior:**
- Mandatory order: 1 → 2 → 3 → 4 → 5...
- Does not require the `sequence` array

**Application:** Simple progression puzzles.

---

### 3. ALL_CONNECTED (All Connected)

All inputs must be active simultaneously.

**Configuration Example:**
```json
"logic": {
  "mode": "ALL_CONNECTED",
  "timeoutSeconds": 0,
  "resetOnError": false
}
```

**Behavior:**
- Activation order does not matter
- Progress bar increases as inputs are activated
- Puzzle solved when all are active

**Application:** Connection puzzles where multiple items must be activated.

---

### 4. RANDOM (Random Sequence)

Sequence generated randomly at board boot.

**Configuration Example:**
```json
"logic": {
  "mode": "RANDOM",
  "timeoutSeconds": 300,
  "resetOnError": true
}
```

**Behavior:**
- Generates a new order on each boot
- No repetitions in the sequence

**Application:** Puzzles where the order must be discovered.

---

## Daily Operation

### Startup

1. Turn on the board's power (USB-C or external supply)
2. Wait for the boot (internal LED blinks blue)
3. Puzzle enters the **IDLE** state (blue LEDs)

### Starting the Game

**Method 1 - Physical button:**
- Press the button connected to board pin 19
- LED changes to the progress color

**Method 2 - Serial command:**
- Connect via USB to the computer
- Open the serial terminal (115200 baud)
- Type: `start`

### During the Game

| Indicator | Meaning |
|-----------|-------------|
| Blue LEDs | Waiting to start |
| Yellow LEDs | Correct input detected |
| Growing bar | Puzzle progress |
| Green flash | Correct input |
| Red flash | Wrong input |
| Blinking green LEDs | Puzzle solved |
| "inputX.mp3" sound | Correct input feedback |
| "error.mp3" sound | Wrong input feedback |
| "unlock.mp3" sound | Puzzle completed |

### Restarting the Puzzle

**Via Serial:**
```
reset
```

**Via Hardware:**
- Press the board's reset button

### Checking Status

**Via Serial:**
```
status
```

Expected output:
```
----- STATUS -----
State: Running
Progress: 60% (3/5)
Remaining time: 245s
------------------
```

---

## Troubleshooting

### Common Problems

#### Does not initialize

| Symptom | Probable Cause | Solution |
|---------|----------------|---------|
| LED does not light up | No power | Check USB cable/supply |
| Solid red LED | Hardware error | Check I2C connections |
| Boot loop | Corrupted config.json file | Rewrite configuration |

#### No audio

| Symptom | Probable Cause | Solution |
|---------|----------------|---------|
| No sound at all | SD card not detected | Check the card is inserted |
| Noise instead of music | MP3 with ID3 tags | Remove tags with ffmpeg |
| Distorted sound | Incorrect bitrate | Use 128kbps, 44.1kHz |

#### Inputs do not respond

| Symptom | Probable Cause | Solution |
|---------|----------------|---------|
| No input works | Wrong I2C address | Check jumpers A0-A2 |
| Specific input does not work | Loose wire | Check connection |
| Input triggers by itself | Electrical interference | Add a 100nF capacitor |

#### LEDs do not light up

| Symptom | Probable Cause | Solution |
|---------|----------------|---------|
| LEDs dark | No external power | Activate pin 23 (POWER_EXT) |
| Wrong color | Different RGB order | Adjust configuration |
| Only some light up | Break in the strip | Check soldering |

### Serial Diagnostics

Connect via USB and open the serial monitor (115200 baud).

**During boot, you will see:**
```
========================================
   UNIVERSAL CONTROLLER v1.0
   Escape Room Configurable Puzzle
========================================

[Setup] Name: Box Puzzle | ID: 1
[Setup] Inputs: 5 | PCF Address: 0x20
[Setup] Mode: CUSTOM_SEQUENCE | Timeout: 300s
[Setup] PCF8575 added at address 0x20
[Setup] Hardware configured. Waiting to start...
```

**If there is an error:**
```
[Setup] ERROR: Failed to initialize LittleFS
[Setup] Using default configuration
```

### Factory Reset

To restore default settings:

1. Turn off the power
2. Connect pin 19 to GND
3. Turn on the power
4. Hold for 5 seconds
5. Disconnect pin 19
6. Default configuration will be loaded

---

## Maintenance

### Cleaning

- Use a dry or slightly damp cloth
- Do not use aggressive chemical products
- Turn off before cleaning

### Firmware Update

1. Download the latest .uf2 file
2. Connect the board via USB
3. Press BOOTSEL + RESET
4. Copy the .uf2 file to the RPI-RP2 drive

### Configuration Backup

1. Connect via USB
2. Use the LittleFS tool to extract `/config.json`
3. Keep a copy in a safe place

---

## Technical Specifications

### PropMaker Feather RP2040

| Parameter | Specification |
|-----------|---------------|
| Microcontroller | Raspberry Pi RP2040 |
| Clock | 133 MHz dual-core |
| RAM Memory | 264 KB |
| Flash Memory | 8 MB (QSPI) |
| Operating voltage | 3.3V |
| Power | USB-C or external 5V |
| Consumption | ~100mA (idle), ~500mA (with LEDs) |

### PCF8575

| Parameter | Specification |
|-----------|---------------|
| I/O Channels | 16 |
| Voltage | 2.5V - 5.5V |
| Current per pin | 25mA (sink) |
| I2C Address | 0x20 - 0x27 |
| Internal pull-up | 100μA |

### NeoPixel

| Parameter | Specification |
|-----------|---------------|
| Type | WS2812B |
| Voltage | 5V |
| Maximum current | 60mA per LED (full white) |
| Recommended maximum | 16 LEDs without amplifier |

### Digital Inputs

| Parameter | Specification |
|-----------|---------------|
| Maximum voltage | 5.5V |
| Impedance | ~100kΩ (pull-up) |
| Debounce | 50ms (configurable) |
| Response | <10ms |

---

## Warranty

The controller has a **12-month** warranty against manufacturing defects.

**Not covered:**
- Damage from misuse
- Overvoltage or short circuit
- Unauthorized modifications
- Exposure to moisture or extreme temperature

---

## Technical Support

For technical support, contact the manufacturer.

**Before contacting:**
1. Check the connections
2. Consult the "Troubleshooting" section
3. Note the error messages from Serial

---

## Version History

| Version | Date | Changes |
|--------|------|------------|
| 1.0 | April 2026 | Initial version |

---

**Complementary documentation:**
- README.md (developer)
- ALGORITMO.MD (technical specification)
- Configuration examples in the `/data/` folder

---

*Document generated in April 2026.*  
*Universal Controller - Escape Room Puzzle System*
