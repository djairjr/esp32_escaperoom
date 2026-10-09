# Connections: BombTimer (XIAO ESP32-S3 + Expansion Board)

## Microcontroller: Seeed XIAO ESP32-S3

## I2C Bus (SDA=5/D4, SCL=6/D5)
| Address | Device | Function |
|----------|------------|--------|
| 0x20 | PCF8575 | I/O Expander — 4 buttons + 4 wires |
| 0x24 | PN532 | NFC/RFID Reader (optional, enabled via config) |
| 0x51 | PCF8563 | RTC (Expansion Board onboard) |
| 0x70 | HT16K33 | 14-segment 4-digit Display |

## I2S Bus
| Signal | XIAO Pin | MAX98357A |
|-------|-----------|-----------|
| BCLK | 43 (D6) | BCLK |
| WS | 44 (D7) | LRCLK |
| DATA | 4 (D3) | DIN |
| VCC | 5V | VIN |
| GND | GND | GND |

## SPI Bus (SD Card — Expansion Board)
| Signal | XIAO Pin | SD Card |
|-------|-----------|---------|
| SCK | 7 (D8) | CLK |
| MISO | 8 (D9) | DO |
| MOSI | 9 (D10) | DI |
| CS | 3 (D2) | CS |

## NeoPixel (GPIO one-wire)
| Signal | XIAO Pin | WS2812B Strip |
|-------|-----------|--------------|
| DATA | 3 (D2) | DI |
| VCC | 5V | VCC (48 LEDs ~3A peak) |
| GND | GND | GND |

## PCF8575 (I2C 0x20) — Inputs
| PCF Pin | Signal | Function | Type |
|----------|-------|--------|------|
| P10 (8) | BUTTON_LEFT | Navigate menu | INPUT (SwitchInput) |
| P11 (9) | BUTTON_SELECT | Confirm / Start | INPUT (SwitchInput) |
| P12 (10) | BUTTON_RIGHT | Navigate menu | INPUT (SwitchInput) |
| P13 (11) | BUTTON_DET | Detonate | INPUT (SwitchInput) |
| P14 (12) | WIRE1 | Defuse wire #1 | INPUT_PULLUP |
| P15 (13) | WIRE2 | Defuse wire #2 | INPUT_PULLUP |
| P16 (14) | WIRE3 | Defuse wire #3 | INPUT_PULLUP |
| P17 (15) | WIRE4 | Defuse wire #4 | INPUT_PULLUP |

## PCA9685 (I2C 0x40) — Outputs
| Channel | Function |
|-------|--------|
| 0 | Analog voltmeter (PWM → RC filter) |

## Power
| Voltage | Source | Consumers |
|--------|-------|-------------|
| 5V | HLK-10M05 | XIAO ESP32-S3, PCF8575, HT16K33, NeoPixel |
| 12V | HLK-10M12 | MagLock (via relay) |
| 3.3V | XIAO Regulator | PN532, MAX98357A, SD Card |
