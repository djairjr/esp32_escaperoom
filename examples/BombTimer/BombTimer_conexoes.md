# Conexoes: BombTimer (XIAO ESP32-S3 + Expansion Board)

## Microcontrolador: Seeed XIAO ESP32-S3

## I2C Bus (SDA=5/D4, SCL=6/D5)
| Endereco | Dispositivo | Funcao |
|----------|------------|--------|
| 0x20 | PCF8575 | I/O Expander — 4 botoes + 4 wires |
| 0x24 | PN532 | NFC/RFID Reader (opcional, habilitado via config) |
| 0x51 | PCF8563 | RTC (Expansion Board onboard) |
| 0x70 | HT16K33 | Display 14-segmentos 4 digitos |

## I2S Bus
| Sinal | Pino XIAO | MAX98357A |
|-------|-----------|-----------|
| BCLK | 43 (D6) | BCLK |
| WS | 44 (D7) | LRCLK |
| DATA | 4 (D3) | DIN |
| VCC | 5V | VIN |
| GND | GND | GND |

## SPI Bus (SD Card — Expansion Board)
| Sinal | Pino XIAO | SD Card |
|-------|-----------|---------|
| SCK | 7 (D8) | CLK |
| MISO | 8 (D9) | DO |
| MOSI | 9 (D10) | DI |
| CS | 3 (D2) | CS |

## NeoPixel (GPIO one-wire)
| Sinal | Pino XIAO | Fita WS2812B |
|-------|-----------|--------------|
| DATA | 3 (D2) | DI |
| VCC | 5V | VCC (48 LEDs ~3A pico) |
| GND | GND | GND |

## PCF8575 (I2C 0x20) — Entradas
| Pino PCF | Sinal | Funcao | Tipo |
|----------|-------|--------|------|
| P10 (8) | BUTTON_LEFT | Navegar menu | INPUT (SwitchInput) |
| P11 (9) | BUTTON_SELECT | Confirmar / Iniciar | INPUT (SwitchInput) |
| P12 (10) | BUTTON_RIGHT | Navegar menu | INPUT (SwitchInput) |
| P13 (11) | BUTTON_DET | Detonar | INPUT (SwitchInput) |
| P14 (12) | WIRE1 | Fio de desarme #1 | INPUT_PULLUP |
| P15 (13) | WIRE2 | Fio de desarme #2 | INPUT_PULLUP |
| P16 (14) | WIRE3 | Fio de desarme #3 | INPUT_PULLUP |
| P17 (15) | WIRE4 | Fio de desarme #4 | INPUT_PULLUP |

## PCA9685 (I2C 0x40) — Saidas
| Canal | Funcao |
|-------|--------|
| 0 | Voltimetro analogico (PWM → RC filter) |

## Power
| Tensao | Fonte | Consumidores |
|--------|-------|-------------|
| 5V | HLK-10M05 | XIAO ESP32-S3, PCF8575, HT16K33, NeoPixel |
| 12V | HLK-10M12 | MagLock (via rele) |
| 3.3V | Regulador XIAO | PN532, MAX98357A, SD Card |
