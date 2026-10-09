#pragma once

/**
 * pins/yd_esp32s3.h — VCC-GND YD-ESP32-S3 N16R8 (44P DevKitC-1 clone)
 *
 * Clone do ESP32-S3-DevKitC-1. Módulo WROOM-2-N16R8V.
 * 16MB Flash Octal + 8MB PSRAM Octal, 1.8V SPI.
 *
 * Periféricos onboard:
 *   - RGB LED NeoPixel (GPIO48)
 *   - 2× USB Type-C (UART + OTG/JTAG)
 *   - 44 pinos (2×22 headers)
 *
 * ⚠️ GPIO35,36,37: RESERVADOS (comunicação octal flash/PSRAM).
 * ⚠️ GPIO48: NeoPixel — usar neopixelWrite(), NÃO digitalWrite().
 */

#include <Arduino.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "VCC-GND YD-ESP32-S3 N16R8";
constexpr const char* BOARD_MANUFACTURER = "VCC-GND Studio";
constexpr const char* BOARD_CHIP         = "ESP32-S3";

// ── Memória ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  16
#define BOARD_PSRAM_MB  8
#define HAS_PSRAM       1
#define FLASH_IS_OCTAL  1    // SPI Octal (WROOM-2)

// ── Periféricos onboard ──────────────────────────────────────

#define HAS_ONBOARD_NEOPIXEL   1    // RGB LED GPIO48
#define HAS_ONBOARD_USB_OTG    1    // USB OTG GPIO19/20

// ── I2C ──────────────────────────────────────────────────────
constexpr int PIN_I2C_SDA = 4;   // GPIO4  (I2C0)
constexpr int PIN_I2C_SCL = 5;   // GPIO5  (I2C0)

// ── I2S ──────────────────────────────────────────────────────
#define HAS_ONBOARD_I2S_AMP     0
constexpr int PIN_I2S_BCLK = 6;
constexpr int PIN_I2S_WS   = 7;
constexpr int PIN_I2S_DATA = 15;
constexpr bool HAS_I2S     = true;

// ── NeoPixel ─────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_NEOPIXEL = 48;
constexpr int PIN_NEOPIXEL          = 48;
constexpr int PIN_NEOPIXEL_INTERNAL = 48;

// ── SPI (FSPI) ───────────────────────────────────────────────
constexpr int PIN_SPI_SCK  = 12;  // FSPICLK
constexpr int PIN_SPI_MISO = 13;  // FSPID4
constexpr int PIN_SPI_MOSI = 11;  // FSPID

// ── SD Card ──────────────────────────────────────────────────
#define HAS_ONBOARD_SD         0
constexpr int PIN_SD_CS    = 10;
#define USE_SD_LIBRARY SD

// ── Atuadores ────────────────────────────────────────────────
#define HAS_ONBOARD_RELAY    0
constexpr int PIN_RELAY      = 21;
constexpr int PIN_SERVO_PWM  = 14;

// ── Botões ───────────────────────────────────────────────────
constexpr int PIN_BTN              = 0;   // BOOT
constexpr int PIN_EXPANSION_BUTTON = 1;

// ── Power ────────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = 23;
constexpr int PIN_BATTERY_SENSE      = -1;

// ── Display externo ──────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 0
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

// ── RTC ──────────────────────────────────────────────────────
#define HAS_ONBOARD_RTC 0
constexpr uint8_t RTC_I2C_ADDRESS = 0x51;

// ── Buzzer ───────────────────────────────────────────────────
constexpr int PIN_BUZZER = 17;

// ── Flags ────────────────────────────────────────────────────
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1
#define HAS_ONBOARD_LEDS    1
#define HAS_ONBOARD_BUZZER  0

// ── IO disponíveis (44 pinos, evitando 35-37) ────────────────
constexpr int PIN_IO0  = 1;
constexpr int PIN_IO1  = 2;
constexpr int PIN_IO2  = 3;
constexpr int PIN_IO3  = 4;
constexpr int PIN_IO4  = 5;
constexpr int PIN_IO5  = 6;
constexpr int PIN_IO6  = 7;
constexpr int PIN_IO7  = 8;
constexpr int PIN_IO8  = 9;
constexpr int PIN_IO9  = 10;
constexpr int PIN_IO10 = 11;
constexpr int PIN_IO11 = 12;
constexpr int PIN_IO12 = 13;
constexpr int PIN_IO13 = 14;
constexpr int PIN_IO14 = 15;
constexpr int PIN_IO15 = 16;
constexpr int PIN_IO16 = 17;
constexpr int PIN_IO17 = 18;
constexpr int PIN_IO18 = 21;
constexpr int PIN_IO19 = 38;
constexpr int PIN_IO20 = 39;
constexpr int PIN_IO21 = 40;
constexpr int PIN_IO22 = 41;
constexpr int PIN_IO23 = 42;
constexpr int PIN_IO24 = 47;
constexpr int PIN_IO25 = 48;

// ── Constantes ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  #if HAS_ONBOARD_NEOPIXEL
  pinMode(PIN_NEOPIXEL, OUTPUT);
  neopixelWrite(PIN_NEOPIXEL, 0, 0, 10);  // azul fraco = boot
  #endif
}

}  // namespace esp32_escaperoom
