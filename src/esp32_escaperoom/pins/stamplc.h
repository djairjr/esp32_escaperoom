#pragma once

/**
 * pins/stamplc.h — M5Stack StampPLC (SKU: K141)
 *
 * Support via M5Unified. Automatic initialization of the TFT display
 * and onboard peripherals. Relays are managed via a PCF8574
 * expander on the I2C bus.
 *
 * Dependency: M5Unified (Arduino Library Manager)
 *
 * Compact industrial PLC based on the M5StampS3 (ESP32-S3).
 * 8MB Flash, no PSRAM.
 *
 * Ideal for central escape room control — relays,
 * opto-isolated inputs and PWM outputs for DC motors.
 */

#include <Arduino.h>
#include <M5Unified.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "M5Stack StampPLC";
constexpr const char* BOARD_MANUFACTURER = "M5Stack";
constexpr const char* BOARD_CHIP         = "ESP32-S3";

// ── Memory ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  8
#define BOARD_PSRAM_MB  0
#define HAS_PSRAM       0

// ── Onboard peripherals (managed by M5Unified) ─────────

#define HAS_M5UNIFIED           1
#define HAS_ONBOARD_TFT         1    // TFT (M5.Display)
#define HAS_ONBOARD_RELAY       1    // relays via PCF8574 I2C
#define HAS_ONBOARD_OPTO_IN     1    // isolated digital inputs
#define HAS_ONBOARD_PWM_OUT     1    // PWM outputs for DC motors

// ── I2C ──────────────────────────────────────────────────────
constexpr int PIN_I2C_SDA = 2;
constexpr int PIN_I2C_SCL = 1;

// ── I2S ──────────────────────────────────────────────────────
#define HAS_ONBOARD_I2S_AMP     0
constexpr int PIN_I2S_BCLK = -1;
constexpr int PIN_I2S_WS   = -1;
constexpr int PIN_I2S_DATA = -1;
constexpr bool HAS_I2S     = false;

// ── NeoPixel ─────────────────────────────────────────────────
#define HAS_ONBOARD_NEOPIXEL    0
constexpr int PIN_EXTERNAL_NEOPIXEL = 4;
constexpr int PIN_NEOPIXEL          = 4;
constexpr int PIN_NEOPIXEL_INTERNAL = 255;

// ── SPI ──────────────────────────────────────────────────────
constexpr int PIN_SPI_SCK  = 12;
constexpr int PIN_SPI_MISO = 13;
constexpr int PIN_SPI_MOSI = 11;

// ── SD Card ──────────────────────────────────────────────────
#define HAS_ONBOARD_SD         0
constexpr int PIN_SD_CS    = 10;
#define USE_SD_LIBRARY SD

// ── Relays (via PCF8574 on the I2C bus) ────────────────────
// The expander must be configured by the user:
//   hw.addPcf8574(0x20)  — in setup()
constexpr int PIN_RELAY_1 = 0;   // PCF8574 pin 0
constexpr int PIN_RELAY_2 = 1;
constexpr int PIN_RELAY_3 = 2;
constexpr int PIN_RELAY_4 = 3;
constexpr int PIN_RELAY = 21;    // direct auxiliary relay (GPIO)

// ── Servo ────────────────────────────────────────────────────
constexpr int PIN_SERVO_PWM = 14;

// ── Buttons ───────────────────────────────────────────────────
constexpr int PIN_BTN              = 0;   // BOOT
constexpr int PIN_EXPANSION_BUTTON = 1;

// ── Power ────────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = 23;
constexpr int PIN_BATTERY_SENSE      = -1;

// ── Flags ────────────────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 1    // TFT
#define HAS_ONBOARD_RTC     0
#define HAS_ONBOARD_LEDS    0
#define HAS_ONBOARD_BUZZER  0
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1

constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
constexpr uint8_t RTC_I2C_ADDRESS  = 0x51;
constexpr int     PIN_BUZZER       = 17;

// ── Constants ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.output_power    = true;
  cfg.internal_spk    = false;  // no onboard speaker
  cfg.internal_mic    = false;
  cfg.external_imu    = false;
  cfg.external_rtc    = false;

  M5.begin(cfg);

  if (M5.Display.isEnabled()) {
    M5.Display.setBrightness(128);
  }
}

}  // namespace esp32_escaperoom
