#pragma once

/**
 * pins/matrixportal.h — Adafruit MatrixPortal S3
 *
 * Display IoT com matriz LED RGB 64×64 onboard.
 * ESP32-S3, 8MB Flash, 2MB PSRAM.
 *
 * Periféricos onboard:
 *   - Matriz LED RGB 64×32 (HUB75)
 *   - Acelerômetro LIS3DH (I2C 0x18)
 *   - NeoPixel GPIO2
 *   - STEMMA QT I2C
 *   - Botão USER (GPIO6) + BOOT (GPIO0)
 *
 * ⚠️ A matriz LED consome GPIOs 8,13,21,35,36,40,41,42,45,48.
 * ⚠️ Poucos GPIOs livres: 16,17,18.
 */

#include <Arduino.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "Adafruit MatrixPortal S3";
constexpr const char* BOARD_MANUFACTURER = "Adafruit";
constexpr const char* BOARD_CHIP         = "ESP32-S3";

// ── Memória ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  8
#define BOARD_PSRAM_MB  2
#define HAS_PSRAM       1

// ── Periféricos onboard ──────────────────────────────────────

#define HAS_ONBOARD_LED_MATRIX  1    // 64×64 RGB HUB75
#define HAS_ONBOARD_ACCEL       1    // LIS3DH
#define HAS_ONBOARD_NEOPIXEL    1    // GPIO2

// ── I2C (STEMMA QT) ──────────────────────────────────────────
constexpr int PIN_I2C_SDA = 14;
constexpr int PIN_I2C_SCL = 15;

// ── Acelerômetro LIS3DH ──────────────────────────────────────
constexpr uint8_t LIS3DH_I2C_ADDRESS = 0x18;
constexpr int PIN_LIS3DH_INT1 = 9;   // interrupção (opcional)
constexpr int PIN_LIS3DH_INT2 = -1;

// ── Matriz LED RGB 64×32 (HUB75) ─────────────────────────────
#define MATRIX_WIDTH   64
#define MATRIX_HEIGHT  64
constexpr int PIN_MATRIX_R1   = 40;
constexpr int PIN_MATRIX_G1   = 41;
constexpr int PIN_MATRIX_B1   = 42;
constexpr int PIN_MATRIX_R2   = 45;
constexpr int PIN_MATRIX_G2   = 35;
constexpr int PIN_MATRIX_B2   = 48;
constexpr int PIN_MATRIX_ADDR = 8;   // A
constexpr int PIN_MATRIX_LAT  = 13;
constexpr int PIN_MATRIX_CLK  = 36;
constexpr int PIN_MATRIX_OE   = 21;

// ── I2S ──────────────────────────────────────────────────────
#define HAS_ONBOARD_I2S_AMP     0
constexpr int PIN_I2S_BCLK = -1;
constexpr int PIN_I2S_WS   = -1;
constexpr int PIN_I2S_DATA = -1;
constexpr bool HAS_I2S     = false;

// ── NeoPixel ─────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_NEOPIXEL = 2;
constexpr int PIN_NEOPIXEL          = 2;
constexpr int PIN_NEOPIXEL_INTERNAL = 2;

// ── SPI ──────────────────────────────────────────────────────
constexpr int PIN_SPI_SCK  = -1;  // matriz LED consome
constexpr int PIN_SPI_MISO = -1;
constexpr int PIN_SPI_MOSI = -1;

// ── SD Card ──────────────────────────────────────────────────
#define HAS_ONBOARD_SD         0
constexpr int PIN_SD_CS    = -1;

// ── Atuadores ────────────────────────────────────────────────
#define HAS_ONBOARD_RELAY    0
constexpr int PIN_RELAY      = 16;  // GPIO16
constexpr int PIN_SERVO_PWM  = 17;  // GPIO17

// ── Botões ───────────────────────────────────────────────────
constexpr int PIN_BTN              = 0;   // BOOT
constexpr int PIN_EXPANSION_BUTTON = 6;   // USER button

// ── Power ────────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = -1;
constexpr int PIN_BATTERY_SENSE      = -1;

// ── Display externo ──────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 1    // matriz LED onboard
constexpr uint8_t OLED_I2C_ADDRESS = 0x00;

// ── RTC ──────────────────────────────────────────────────────
#define HAS_ONBOARD_RTC 0
constexpr uint8_t RTC_I2C_ADDRESS = 0x51;

// ── Buzzer ───────────────────────────────────────────────────
constexpr int PIN_BUZZER = 18;  // GPIO18 (PWM/tone)

// ── Flags ────────────────────────────────────────────────────
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1
#define HAS_ONBOARD_LEDS    1
#define HAS_ONBOARD_BUZZER  0

// ── IO disponíveis ───────────────────────────────────────────
constexpr int PIN_IO0  = 16;   // GPIO16
constexpr int PIN_IO1  = 17;   // GPIO17
constexpr int PIN_IO2  = 18;   // GPIO18
constexpr int PIN_IO3  = 6;    // GPIO6  (USER)
constexpr int PIN_IO4  = 255;
constexpr int PIN_IO5  = 255;
constexpr int PIN_IO6  = 255;
constexpr int PIN_IO7  = 255;
constexpr int PIN_IO8  = 255;
constexpr int PIN_IO9  = 255;
constexpr int PIN_IO10 = 255;
constexpr int PIN_IO11 = 255;
constexpr int PIN_IO12 = 255;

// ── Constantes ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  // LIS3DH é detectado pelo scan I2C automático
  // Matriz LED é inicializada pelo framework Adafruit_Protomatter
}

}  // namespace esp32_escaperoom
