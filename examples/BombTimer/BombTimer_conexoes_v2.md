# Conexões: BombTimer (Versão PCB - Custom Board)

## 1. Visão Geral da Arquitetura
- **MCU**: ESP32-S3 (Solder-down SoC)
- **Alimentação**: USB 5V -> RT9080 LDO -> VCC 3.3V
- **Interface de Expansão**: Multiplexador I2C (TCA9548A) com 8 Headers externos.
- **I/O Expandido**: PCF8575 (Solder-down) -> Header 2x10 (16ch + Power).
- **Sensing**: LIS3DH Acelerômetro (Solder-down).
- **Áudio**: MAX98357A (Solder-down) + Slot MicroSD.
- **Tempo**: PCF8563 RTC (Solder-down) + Célula CR1220.
- **Indústria**: MAX13487E RS485 (Solder-down).

## 2. Árvore de Alimentação (Power Tree)
| Net | Origem | Destino / Consumidores | Tensão |
| :--- | :--- | :--- | :--- |
| **VCC_USB** | Conector USB | Vin RT9080, NeoPixel Power | 5.0V |
| **VCC_3V3** | RT9080 (VOUT) | ESP32-S3, PCF8575, LIS3DH, MAX98357A, TCA9548A, PCF8563, RTC Backup | 3.3V |
| **V_BATT** | Bateria CR1220 | PCF8563 (via Diodo de Backup) | 3.0V |
| **GND** | USB / RT9080 | Todos os componentes (Plano de Massa) | 0V |

## 3. Barramentos e Redes (Nets)

### I2C Main Bus (SDA=5, SCL=6)
| Componente | Pino SDA | Pino SCL | Função |
| :--- | :---: | :---: | :--- |
| ESP32-S3 | GPIO5 | GPIO6 | Master |
| TCA9548A | Pin 4 | Pin 5 | Multiplexador |
| PCF8575 | Pin 23 | Pin 22 | I/O Expander |
| LIS3DH | SDA | SCL | Acelerômetro |
| PCF8563 | Pin 5 | Pin 6 | RTC |

### I2C Expansion (TCA9548A $\rightarrow$ Headers)
| Header | Sinal SDA | Sinal SCL | Destino Sugerido |
| :--- | :--- | :--- | :--- |
| **H_I2C_1** | SD0 (Pin 7) | SC0 (Pin 8) | HT16K33 Display |
| **H_I2C_2** | SD1 (Pin 9) | SC1 (Pin 10) | PN532 NFC |
| **H_I2C_3** | SD2 (Pin 11) | SC2 (Pin 12) | Extra 1 |
| **H_I2C_4** | SD3 (Pin 13) | SC3 (Pin 14) | Extra 2 |
| **H_I2C_5** | SD4 (Pin 15) | SC4 (Pin 16) | Extra 3 |
| **H_I2C_6** | SD5 (Pin 17) | SC5 (Pin 18) | Extra 4 |
| **H_I2C_7** | SD6 (Pin 19) | SC6 (Pin 20) | Extra 5 |
| **H_I2C_8** | SD7 (Pin 21) | SC7 (Pin 22) | Extra 6 |

### I2S Audio Bus
| Sinal | Pino ESP32-S3 | Pino MAX98357A | Função |
| :--- | :---: | :---: | :--- |
| **BCLK** | GPIO43 | Pin 2 | Bit Clock |
| **WS/LRCLK**| GPIO44 | Pin 3 | Word Select |
| **DATA** | GPIO4 | Pin 1 | Data In |

### RS485 Bus
| Sinal | Pino ESP32-S3 | Pino MAX13487E | Função |
| :--- | :---: | :---: | :--- |
| **TX** | GPIO\_TX | Pin 4 (DI) | Data In |
| **RX** | GPIO\_RX | Pin 1 (RO) | Data Out |

### SPI SDCard (Expansion Core)
| Sinal | Pino ESP32-S3 | Slot MicroSD |
| :--- | :---: | :---: |
| **SCK** | GPIO7 | CLK |
| **MISO** | GPIO8 | DO |
| **MOSI** | GPIO9 | DI |
| **CS** | GPIO3 | CS |

## 4. Detalhamento de Headers Específicos

### Header PCF8575 (2x10)
| Pino | Sinal | Tipo | Função |
| :---: | :--- | :--- | :--- |
| 1-16 | P00...P15 | GPIO | Canais do Expander |
| 17-18 | VCC | Power | 3.3V |
| 19-20 | GND | Power | Ground |

### Header SPI / General Purpose (Symmetry Check)
*Composição: 2x5V, 2x3V3, 4xGND, SPI Bus, GPIOs. Total: PAR.*
| Pino | Sinal | Tipo | Nota |
| :---: | :--- | :--- | :--- |
| 1 | 5V | Power | Vin |
| 2 | 3V3 | Power | LDO Out |
| 3 | GND | Power | Ground |
| 4 | GND | Power | Ground |
| 5 | SCK | SPI | Clock |
| 6 | MISO | SPI | Master In |
| 7 | MOSI | SPI | Master Out |
| 8 | CS_EXT | SPI | Dedicated CS (Non-SD) |
| 9 | GPIO_X | Digital | Available |
| 10 | GPIO_Y | Digital | Available |
| 11 | 5V | Power | Vin |
| 12 | 3V3 | Power | LDO Out |
| 13 | GND | Power | Ground |
| 14 | GND | Power | Ground |
| 15 | GPIO_Z | Digital | Available |
| 16 | GND | Power | Ground (Padding for Parity) |

---
**Sinalização de Status**: LED NeoPixel conectado ao GPIO 3 (D2) com capacitor de desacoplamento.
