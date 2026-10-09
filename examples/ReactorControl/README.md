# ReactorControl — Nuclear Control Panel

Analog adjustment puzzle for an Escape Room using the **PropMaker EscapeRoom** library.

## Narrative

The nuclear reactor has entered a critical state. Three technicians must cooperate to adjust the parameters on a control panel with 3 faces. Each face has a **valve** (potentiometer) and a **gauge** (analog voltmeter), but the valve of one face **never** controls the gauge of the same face — the mapping is random and can change at any moment.

Furthermore, each gauge can respond in the **normal** or **reversed** direction, adding to the confusion. When all the gauges reach the green zone, the players must press **3 buttons simultaneously** (within 2 seconds) to confirm. If they fail, the panel reconfigures itself.

## Hardware

| Component | Model | Bus | Address/Channel | Function |
|------------|--------|------------|----------------|--------|
| Microcontroller | Adafruit PropMaker Feather RP2040 | — | — | Core |
| I/O Expander | PCF8575 | I2C | 0x20 | 3 buttons |
| ADC | ADS1115 | I2C | 0x48 | 3 potentiometers |
| PWM Driver | PCA9685 | I2C | 0x40 | 3 voltmeters |
| NeoPixel | External strip | GPIO | PIN_EXTERNAL_NEOPIXEL | 3 feedback LEDs |
| Lock | Electromagnet/Relay | GPIO | PIN_LOCK | Opens on victory |

### Channel Mapping

| Face | Parameter | ADS (Input) | PCA (Output) | PCF Button | NeoPixel |
|------|-----------|----------------|-------------|-----------|----------|
| 1 | Core Temperature | AIN0 | PWM0 | P0 | LED 0 |
| 2 | Coolant Pressure | AIN1 | PWM1 | P1 | LED 1 |
| 3 | Rod Insertion | AIN2 | PWM2 | P2 | LED 2 |

### Target Values (Green Zone)

| Face | Target | Tolerance |
|------|--------|------------|
| 1 | 7.0 | ±0.5 |
| 2 | 5.0 | ±0.5 |
| 3 | 3.0 | ±0.5 |

## Mechanics

1. **Start**: Press button 1 to start.
2. **Adjust**: Turn the valves to move the gauges. Note that valve N does **not** affect gauge N — it affects a gauge on another face.
3. **Green Zone**: Each gauge must indicate the target value within tolerance.
4. **Confirmation**: When all 3 gauges are in the green zone, a 2-second window opens. All 3 buttons must be pressed within this interval.
5. **Reconfiguration**: If the confirmation fails, the valve→gauge mapping and the orientation (normal/reversed) of each gauge are reshuffled. The players must rediscover the relationships.
6. **Victory**: A successful confirmation opens the lock and plays the victory sound.

## Required Sounds

Place the following MP3 files (128Kbps, 44KHz, no ID3 tags) in LittleFS or SD:

| File | Event |
|---------|--------|
| `ready.mp3` | Puzzle started |
| `alarm.mp3` | Gauge out of the green zone |
| `stabilizing.mp3` | Gauge entered the green zone |
| `warning.mp3` | All gauges in the zone (tension) |
| `button_press.mp3` | Confirmation button pressed |
| `fail.mp3` | Confirmation failed |
| `reshuffle.mp3` | Reconfiguration after failure |
| `victory.mp3` | Puzzle solved |

**Note:** Use `ffmpeg` to generate compatible MP3s:
```bash
ffmpeg -y -i input.wav -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 output.mp3
```

## Compilation

```bash
cd <Arduino>/libraries/propmaker_escaperoom
arduino-cli compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker examples/ReactorControl/ReactorControl.ino
```

## Dependencies

- `PropMakerEscapeRoom` (this library)
- `IoAbstraction`
- `TaskManager`
- `SwitchInput`
- `Adafruit ADS1X15`
- `PCA9685`

## Configuration

Edit the constants at the top of the sketch to adjust:
- `TARGETS[]` — target values of each gauge
- `TOLERANCE` — acceptable margin of error
- `CONFIRM_WINDOW_MS` — time to press the 3 buttons (ms)
- `RESHUFFLE_DURATION_MS` — duration of the reconfiguration animation
- `ADS_CHANNELS[]` — ADS1115 channels
- `PCA_CHANNELS[]` — PCA9685 channels
- `BUTTON_PINS[]` — PCF8575 pins
- `BUTTON_PIXELS[]` — NeoPixel indices

## Author

Djair Guilherme — Nicolau dos Brinquedos
April 2026
