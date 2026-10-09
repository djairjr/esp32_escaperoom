#pragma once

/**
 * pins/cardputer.h — M5Stack Cardputer v1.1 (StampS3A)
 *
 * Support via M5Unified. Automatic initialization of all
 * onboard peripherals: ST7789 display, keyboard, I2S speaker,
 * MEMS microphone, IR, microSD, NeoPixel, battery.
 *
 * Dependency: M5Unified (Arduino Library Manager)
 *   https://github.com/m5stack/M5Unified
 *
 * SoC: ESP32-S3FN8 @ 240MHz, 8MB Flash, no PSRAM.
 */

#include <Arduino.h>
#include <M5Unified.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "M5Stack Cardputer v1.1";
constexpr const char* BOARD_MANUFACTURER = "M5Stack";
constexpr const char* BOARD_CHIP         = "ESP32-S3FN8";

// ── Memory ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  8
#define BOARD_PSRAM_MB  0
#define HAS_PSRAM       0

// ── Onboard peripherals (managed by M5Unified) ─────────

#define HAS_M5UNIFIED           1
#define HAS_ONBOARD_TFT         1    // ST7789 240×135 (M5.Display)
#define HAS_ONBOARD_I2S_AMP     1    // NS4168 (M5.Speaker)
#define HAS_ONBOARD_MIC         1    // SPM1423 MEMS (M5.Mic)
#define HAS_ONBOARD_KEYBOARD    1    // 56 keys (M5.Keyboard)
#define HAS_ONBOARD_IR          1    // IR emitter (M5.IR)
#define HAS_ONBOARD_SD          1    // microSD SPI (M5.fs)
#define HAS_ONBOARD_GROVE       1    // HY2.0-4P
#define HAS_ONBOARD_NEOPIXEL    1    // GPIO4
#define HAS_ONBOARD_BATTERY     1    // dual battery (M5.Power)

// ── I2C (Grove) ──────────────────────────────────────────────
constexpr int PIN_I2C_SDA = 2;   // Grove yellow (G2)
constexpr int PIN_I2C_SCL = 1;   // Grove white (G1)

// ── Grove ────────────────────────────────────────────────────
constexpr int PIN_GROVE_YELLOW = 2;
constexpr int PIN_GROVE_WHITE  = 1;

// ── I2S Audio (managed by M5Unified) ────────────────────
constexpr int PIN_I2S_BCLK = 41;
constexpr int PIN_I2S_WS   = 43;
constexpr int PIN_I2S_DATA = 42;
constexpr int PIN_I2S_SDIN = 46;  // microphone
constexpr bool HAS_I2S     = true;

// ── NeoPixel ─────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_NEOPIXEL = -1;
constexpr int PIN_NEOPIXEL          = 4;
constexpr int PIN_NEOPIXEL_INTERNAL = 4;

// ── SPI (ST7789 Display — managed by M5Unified) ─────────
constexpr int PIN_SPI_SCK  = 36;
constexpr int PIN_SPI_MISO = -1;
constexpr int PIN_SPI_MOSI = 35;

// ── SD Card (dedicated SPI — managed by M5Unified) ───────
constexpr int PIN_SD_CS   = 12;
constexpr int PIN_SD_SCK  = 40;
constexpr int PIN_SD_MOSI = 14;
constexpr int PIN_SD_MISO = 39;

// ── IR ───────────────────────────────────────────────────────
constexpr int PIN_IR_TX = 44;

// ── Actuators (Grove — user-reconfigurable) ─────────
#define HAS_ONBOARD_RELAY    0
constexpr int PIN_RELAY      = 2;   // Grove yellow
constexpr int PIN_SERVO_PWM  = 1;   // Grove white

// ── Buttons ───────────────────────────────────────────────────
constexpr int PIN_BTN              = 0;    // BOOT
constexpr int PIN_EXPANSION_BUTTON = -1;   // uses M5 keyboard

// ── Battery ──────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = -1;
constexpr int PIN_BATTERY_SENSE      = 10;

// ── Flags ────────────────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 1
#define HAS_ONBOARD_RTC     0
#define HAS_ONBOARD_LEDS    1
#define HAS_ONBOARD_BUZZER  0
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1

constexpr uint8_t OLED_I2C_ADDRESS = 0x00;
constexpr uint8_t RTC_I2C_ADDRESS  = 0x51;
constexpr int     PIN_BUZZER       = -1;  // uses I2S speaker

// ── Constants ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  // M5Unified initializes display, audio, keyboard, IR, SD, battery
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  cfg.output_power    = true;   // turns on peripherals
  cfg.internal_spk    = true;   // NS4168 speaker
  cfg.internal_mic    = true;   // SPM1423 microphone
  cfg.external_imu    = false;  // no external IMU
  cfg.external_rtc    = false;  // no external RTC

  M5.begin(cfg);

  // Display backlight
  if (M5.Display.isEnabled()) {
    M5.Display.setBrightness(128);
  }
}

}  // namespace esp32_escaperoom
