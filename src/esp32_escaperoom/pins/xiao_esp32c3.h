#pragma once

/**
 * pins/xiao_esp32c3.h — Seeed XIAO ESP32-C3
 *
 * Mini RISC-V IoT board. 4MB Flash, no PSRAM.
 * WiFi 2.4GHz + BLE 5.0. Low consumption (44μA deep sleep).
 *
 * Onboard peripherals:
 *   - USB-C (UART + JTAG)
 *   - LiPo battery with charger
 *   - External U.FL antenna (optional)
 *
 * Compatible with the Expansion Board Base for XIAO (except SWD).
 *
 * ⚠️ GPIO2(D0), GPIO8(D8), GPIO9(D9): strapping pins.
 * ⚠️ D3 (GPIO5) uses ADC2 — readings may be unreliable.
 * ⚠️ No NeoPixel, no microphone, no PSRAM.
 *
 * Ref: https://wiki.seeedstudio.com/xiao_esp32c3_getting_started/
 */

#include <Arduino.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "Seeed XIAO ESP32-C3";
constexpr const char* BOARD_MANUFACTURER = "Seeed Studio";
constexpr const char* BOARD_CHIP         = "ESP32-C3 (RISC-V)";

// ── Memory ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  4
#define BOARD_PSRAM_MB  0
#define HAS_PSRAM       0

// ── Onboard peripherals ──────────────────────────────────────

#define HAS_ONBOARD_NEOPIXEL   0
#define HAS_ONBOARD_BATTERY    1    // LiPo charger

// ── I2C ──────────────────────────────────────────────────────
constexpr int PIN_I2C_SDA = 6;   // D4 (GPIO6)
constexpr int PIN_I2C_SCL = 7;   // D5 (GPIO7)

// ── I2S ──────────────────────────────────────────────────────
#define HAS_ONBOARD_I2S_AMP     0
constexpr int PIN_I2S_BCLK = 4;   // D2 (GPIO4) — I2S BCLK (NeoPixel/SD_CS not used)
constexpr int PIN_I2S_WS   = 5;   // D3 (GPIO5) — I2S WS (Buzzer not used)
constexpr int PIN_I2S_DATA = 6;   // D4 (GPIO6) — I2S DATA (I2C SDA not used)
constexpr bool HAS_I2S     = true;  // I2S via GPIO matrix (Espressif doc)

// ── NeoPixel ─────────────────────────────────────────────────
// No onboard NeoPixel. D2 (GPIO4) can be used for an external one.
constexpr int PIN_EXTERNAL_NEOPIXEL = 4;   // D2 (GPIO4)
constexpr int PIN_NEOPIXEL          = PIN_EXTERNAL_NEOPIXEL;
constexpr int PIN_NEOPIXEL_INTERNAL = 255;

// ── SPI ──────────────────────────────────────────────────────
// ⚠️ D8(SCK=GPIO8) and D9(MISO=GPIO9) are strapping pins
constexpr int PIN_SPI_SCK  = 8;   // D8 (⚠️ strapping)
constexpr int PIN_SPI_MISO = 9;   // D9 (⚠️ strapping)
constexpr int PIN_SPI_MOSI = 10;  // D10

// ── SD Card ──────────────────────────────────────────────────
#define HAS_ONBOARD_SD         0
constexpr int PIN_SD_CS    = 4;   // D2 (GPIO4)
#define USE_SD_LIBRARY SD

// ── Actuators ────────────────────────────────────────────────
#define HAS_ONBOARD_RELAY    0
constexpr int PIN_RELAY      = 2;   // D0 (GPIO2 ⚠️ strapping)
constexpr int PIN_SERVO_PWM  = 3;   // D1 (GPIO3)

// ── Expansion Board ──────────────────────────────────────────
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t RTC_I2C_ADDRESS  = 0x51;
#define HAS_ONBOARD_RTC 1    // via Expansion Board
constexpr int PIN_BUZZER = 5;   // D3 (GPIO5 ⚠️ ADC2)
constexpr int PIN_EXPANSION_BUTTON = 3;  // D1 (GPIO3)

// ── Buttons ───────────────────────────────────────────────────
// ⚠️ BOOT = GPIO9 (strapping!) on the C3 — different from the S3
constexpr int PIN_BTN = 9;  // BOOT

// ── Power ────────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = 10;
constexpr int PIN_BATTERY_SENSE      = -1;  // no battery monitor

// ── External display ──────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 0
// OLED via Expansion Board: OLED_I2C_ADDRESS already defined

// ── Flags ────────────────────────────────────────────────────
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1
#define HAS_ONBOARD_LEDS    0
#define HAS_ONBOARD_BUZZER  0

// ── Available IOs ───────────────────────────────────────────
constexpr int PIN_IO0  = 2;    // D0  ⚠️ strapping
constexpr int PIN_IO1  = 3;    // D1
constexpr int PIN_IO2  = 4;    // D2
constexpr int PIN_IO3  = 5;    // D3  ⚠️ ADC2
constexpr int PIN_IO4  = 6;    // D4  I2C SDA
constexpr int PIN_IO5  = 7;    // D5  I2C SCL
constexpr int PIN_IO6  = 21;   // D6  U0TXD
constexpr int PIN_IO7  = 20;   // D7  U0RXD
constexpr int PIN_IO8  = 8;    // D8  ⚠️ strapping
constexpr int PIN_IO9  = 9;    // D9  ⚠️ strapping
constexpr int PIN_IO10 = 10;   // D10
constexpr int PIN_IO11 = 255;
constexpr int PIN_IO12 = 255;

// ── Deep sleep wake ────────────────────────────────────────
constexpr int PIN_WAKE_SENSOR = 3;   // D1 (GPIO3) — RTC GPIO, wake ext0 (Servo/Button not used)

// ── Constants ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  // The C3 needs no special init
}

}  // namespace esp32_escaperoom
