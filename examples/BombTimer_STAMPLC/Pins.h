#pragma once

// ============================================================================
// PINS.H — Pin mapping for M5Stack StamPLC
// ============================================================================
// Note: GROVE RED pins (PORT.A) are already defined in
// M5StamPLC's pin_config.h as STAMPLC_PIN_GROVE_RED_SDA=2 and _SCL=1.
// We use the M5StamPLC defines to avoid duplication.

// Internal I2C Bus (G13=SDA, G15=SCL)
//   PI4IOE5V6408 @ 0x43  (buttons + RGB)
//   AW9523B @ 0x59        (relays + opto inputs)
//   RX8130CE @ 0x32       (RTC)
//   LM75B @ 0x48          (temperature)
//   INA226 @ 0x40         (power voltage/current)

// External I2C Bus — PORT.A / Grove RED (G2=SDA, G1=SCL)
//   PCF8574 @ 0x20-0x27   (keypad matrix 4x3)
//   MFRC522/WS1850S @ 0x28 (RFID 2 Unit)
// PORT.A pins (reference to M5StamPLC's pin_config.h)
#define PIN_PORT_A_SDA   STAMPLC_PIN_GROVE_RED_SDA
#define PIN_PORT_A_SCL   STAMPLC_PIN_GROVE_RED_SCL

// Buttons (via PI4IOE5V6408, managed by M5.BtnA/B/C)
// M5.BtnA = PREV
// M5.BtnB = SELECT
// M5.BtnC = NEXT

// DET button = BOOT (GPIO0)
#define PIN_BOOT        0

// Keypad matrix via PCF8574 (address auto-detected on scan)
#define PCF_KEYPAD_ADDR  0x24
#define KEYPAD_ROWS      4
#define KEYPAD_COLS      3
// Mapping of PCF8574 pins to the rows and columns of the 4x3 keypad.
// Different keypad models have different wiring — adjust these arrays
// according to your model's physical wiring.
// PCF pin → ROW: pin that receives LOW to activate the row.
// PCF pin → COL: pin read to detect whether the key was pressed.
static const uint8_t KEYPAD_ROW_PINS[4] PROGMEM = {5, 0, 1, 3}; 
static const uint8_t KEYPAD_COL_PINS[3] PROGMEM = {4, 6, 2};

// PORT.C — Grove BLUE: G5=Yellow, G4=White
// SYN531R RF433 Receiver: DATA → G4 (white)
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

// Display — use M5.Lcd.width() / height() instead of fixed constants
