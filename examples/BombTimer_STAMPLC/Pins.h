#pragma once

// ============================================================================
// PINS.H — Mapeamento de pinos para M5Stack StamPLC
// ============================================================================
// Nota: pinos GROVE RED (PORT.A) já estão definidos em
// pin_config.h do M5StamPLC como STAMPLC_PIN_GROVE_RED_SDA=2 e _SCL=1.
// Usamos os defines do M5StamPLC para evitar duplicação.

// I2C Bus Interno (G13=SDA, G15=SCL)
//   PI4IOE5V6408 @ 0x43  (buttons + RGB)
//   AW9523B @ 0x59        (relays + opto inputs)
//   RX8130CE @ 0x32       (RTC)
//   LM75B @ 0x48          (temperature)
//   INA226 @ 0x40         (power voltage/current)

// I2C Bus Externo — PORT.A / Grove RED (G2=SDA, G1=SCL)
//   PCF8574 @ 0x20-0x27   (keypad matrix 4x3)
//   MFRC522/WS1850S @ 0x28 (RFID 2 Unit)
// PORT.A pins (referência ao pin_config.h do M5StamPLC)
#define PIN_PORT_A_SDA   STAMPLC_PIN_GROVE_RED_SDA
#define PIN_PORT_A_SCL   STAMPLC_PIN_GROVE_RED_SCL

// Botões (via PI4IOE5V6408, gerenciados por M5.BtnA/B/C)
// M5.BtnA = PREV
// M5.BtnB = SELECT
// M5.BtnC = NEXT

// DET button = BOOT (GPIO0)
#define PIN_BOOT        0

// Keypad matrix via PCF8574 (endereço detectado automaticamente no scan)
#define PCF_KEYPAD_ADDR  0x24
#define KEYPAD_ROWS      4
#define KEYPAD_COLS      3
// Mapeamento dos pinos do PCF8574 para linhas e colunas do keypad 4x3.
// Diferentes modelos de keypad têm fiações diferentes — ajuste estes arrays
// conforme a ligação física do seu modelo.
// PCF pin → ROW: pino que recebe LOW para ativar a linha.
// PCF pin → COL: pino lido para detectar se a tecla foi pressionada.
static const uint8_t KEYPAD_ROW_PINS[4] PROGMEM = {5, 0, 1, 3}; 
static const uint8_t KEYPAD_COL_PINS[3] PROGMEM = {4, 6, 2};

// PORT.C — Grove BLUE: G5=Amarelo, G4=Branco
// SYN531R RF433 Receiver: DATA → G4 (branco)
#define PIN_RF433_DATA  4

// AW9523B — Relay outputs
// P0_0 = DETONATE relay 1
// P0_1 = DETONATE relay 2
// P0_2 = DEFUSE relay 1
// P0_3 = DEFUSE relay 2

// AW9523B — Opto inputs
// P0_4 = DET opto (LOW = detonate signal)
// P0_5 = WIRE1
// P0_6 = WIRE2
// P0_7 = WIRE3
// P1_4 = WIRE4

// Display — usar M5.Lcd.width() / height() em vez de constantes fixas
