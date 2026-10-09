#pragma once

/**
 * pins/xiao_esp32c3.h — Seeed XIAO ESP32-C3
 *
 * Placa IoT mini RISC-V. 4MB Flash, sem PSRAM.
 * WiFi 2.4GHz + BLE 5.0. Baixo consumo (44μA deep sleep).
 *
 * Periféricos onboard:
 *   - USB-C (UART + JTAG)
 *   - Bateria LiPo com carregador
 *   - Antena externa U.FL (opcional)
 *
 * Compatível com Expansion Board Base for XIAO (exceto SWD).
 *
 * ⚠️ GPIO2(D0), GPIO8(D8), GPIO9(D9): strapping pins.
 * ⚠️ D3 (GPIO5) usa ADC2 — leituras podem ser falsas.
 * ⚠️ Sem NeoPixel, sem microfone, sem PSRAM.
 *
 * Ref: https://wiki.seeedstudio.com/xiao_esp32c3_getting_started/
 */

#include <Arduino.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "Seeed XIAO ESP32-C3";
constexpr const char* BOARD_MANUFACTURER = "Seeed Studio";
constexpr const char* BOARD_CHIP         = "ESP32-C3 (RISC-V)";

// ── Memória ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  4
#define BOARD_PSRAM_MB  0
#define HAS_PSRAM       0

// ── Periféricos onboard ──────────────────────────────────────

#define HAS_ONBOARD_NEOPIXEL   0
#define HAS_ONBOARD_BATTERY    1    // LiPo charger

// ── I2C ──────────────────────────────────────────────────────
constexpr int PIN_I2C_SDA = 6;   // D4 (GPIO6)
constexpr int PIN_I2C_SCL = 7;   // D5 (GPIO7)

// ── I2S ──────────────────────────────────────────────────────
#define HAS_ONBOARD_I2S_AMP     0
constexpr int PIN_I2S_BCLK = 4;   // D2 (GPIO4) — INMP441 SCK (NeoPixel/SD_CS não usados)
constexpr int PIN_I2S_WS   = 5;   // D3 (GPIO5) — INMP441 WS (Buzzer não usado)
constexpr int PIN_I2S_DATA = 6;   // D4 (GPIO6) — INMP441 SD (I2C SDA não usado)
constexpr bool HAS_I2S     = true;  // I2S via GPIO matrix (doc Espressif)

// ── NeoPixel ─────────────────────────────────────────────────
// Sem NeoPixel onboard. D2 (GPIO4) pode ser usado para externo.
constexpr int PIN_EXTERNAL_NEOPIXEL = 4;   // D2 (GPIO4)
constexpr int PIN_NEOPIXEL          = PIN_EXTERNAL_NEOPIXEL;
constexpr int PIN_NEOPIXEL_INTERNAL = 255;

// ── SPI ──────────────────────────────────────────────────────
// ⚠️ D8(SCK=GPIO8) e D9(MISO=GPIO9) são strapping pins
constexpr int PIN_SPI_SCK  = 8;   // D8 (⚠️ strapping)
constexpr int PIN_SPI_MISO = 9;   // D9 (⚠️ strapping)
constexpr int PIN_SPI_MOSI = 10;  // D10

// ── SD Card ──────────────────────────────────────────────────
#define HAS_ONBOARD_SD         0
constexpr int PIN_SD_CS    = 4;   // D2 (GPIO4)
#define USE_SD_LIBRARY SD

// ── Atuadores ────────────────────────────────────────────────
#define HAS_ONBOARD_RELAY    0
constexpr int PIN_RELAY      = 2;   // D0 (GPIO2 ⚠️ strapping)
constexpr int PIN_SERVO_PWM  = 3;   // D1 (GPIO3)

// ── Expansion Board ──────────────────────────────────────────
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t RTC_I2C_ADDRESS  = 0x51;
#define HAS_ONBOARD_RTC 1    // via Expansion Board
constexpr int PIN_BUZZER = 5;   // D3 (GPIO5 ⚠️ ADC2)
constexpr int PIN_EXPANSION_BUTTON = 3;  // D1 (GPIO3)

// ── Botões ───────────────────────────────────────────────────
// ⚠️ BOOT = GPIO9 (strapping!) no C3 — diferente do S3
constexpr int PIN_BTN = 9;  // BOOT

// ── Power ────────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = 10;
constexpr int PIN_BATTERY_SENSE      = -1;  // sem monitor de bateria

// ── Display externo ──────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 0
// OLED via Expansion Board: OLED_I2C_ADDRESS já definido

// ── Flags ────────────────────────────────────────────────────
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1
#define HAS_ONBOARD_LEDS    0
#define HAS_ONBOARD_BUZZER  0

// ── IO disponíveis ───────────────────────────────────────────
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

// ── Wake de deep sleep ────────────────────────────────────────
constexpr int PIN_WAKE_SENSOR = 3;   // D1 (GPIO3) — RTC GPIO, wake ext0 (Servo/Button não usados)

// ── Constantes ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  // C3 não precisa de init especial
}

}  // namespace esp32_escaperoom
