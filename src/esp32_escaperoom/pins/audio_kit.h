#pragma once

/**
 * pins/audio_kit.h — Ai-Thinker ESP32-A1S Audio Kit v2.2 (A202)
 *
 * Audio board with ES8388 codec + headphone amplifier.
 * ESP32 dual-core, 4MB Flash, 4MB PSRAM.
 *
 * Onboard peripherals:
 *   - ES8388 codec I2S (DAC + ADC)
 *   - 3.5mm headphone amplifier
 *   - Onboard microphones (analog + digital)
 *   - microSD slot SPI
 *   - Volume and play buttons
 *
 * ⚠️ GPIOs 0,18,19,21,22,23,25,26,27 available. The rest belong to the codec.
 */

#include <Arduino.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "Ai-Thinker ESP32-A1S Audio Kit v2.2";
constexpr const char* BOARD_MANUFACTURER = "Ai-Thinker";
constexpr const char* BOARD_CHIP         = "ESP32";

// ── Memory ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  4
#define BOARD_PSRAM_MB  4
#define HAS_PSRAM       1

// ── Onboard peripherals ──────────────────────────────────────

#define HAS_ONBOARD_CODEC      1    // ES8388 I2S codec
#define HAS_ONBOARD_HP_AMP     1    // headphone amplifier
#define HAS_ONBOARD_MIC        1    // onboard microphone
#define HAS_ONBOARD_SD         1    // microSD slot SPI

// I2C — shared with the ES8388 codec (GPIO18=SDA, GPIO23=SCL)
constexpr int PIN_I2C_SDA = 18;
constexpr int PIN_I2C_SCL = 23;

// ── ES8388 Audio Codec ───────────────────────────────────────
// Codec I2C: 0x10 (ES8388)
constexpr uint8_t ES8388_I2C_ADDRESS = 0x10;

// I2S — GPIO25=BCLK, GPIO26=WS, GPIO22=DOUT, GPIO21=SDIN
constexpr int PIN_I2S_BCLK = 25;
constexpr int PIN_I2S_WS   = 26;
constexpr int PIN_I2S_DATA = 22;   // DOUT → codec
constexpr int PIN_I2S_SDIN = 21;   // SDIN ← codec (mic)
constexpr bool HAS_I2S     = true;

// ── Headphone / Line Out ─────────────────────────────────────
constexpr int PIN_HP_DETECT = 19;  // jack detect (LOW = inserted)
constexpr int PIN_HP_SEL    = -1;  // auto by the codec

// ── NeoPixel ─────────────────────────────────────────────────
#define HAS_ONBOARD_NEOPIXEL 0
constexpr int PIN_EXTERNAL_NEOPIXEL = 27;
constexpr int PIN_NEOPIXEL          = 27;
constexpr int PIN_NEOPIXEL_INTERNAL = 255;

// ── SPI (VSPI) ───────────────────────────────────────────────
constexpr int PIN_SPI_SCK  = 14;
constexpr int PIN_SPI_MISO = 12;
constexpr int PIN_SPI_MOSI = 13;

// ── SD Card ──────────────────────────────────────────────────
constexpr int PIN_SD_CS    = 15;
#define USE_SD_LIBRARY SD

// ── Actuators ────────────────────────────────────────────────
#define HAS_ONBOARD_RELAY    0
constexpr int PIN_RELAY      = 19;  // GPIO19
constexpr int PIN_SERVO_PWM  = 0;   // GPIO0 (BOOT — avoid using as servo)

// ── Buttons ───────────────────────────────────────────────────
constexpr int PIN_BTN              = 0;    // BOOT
constexpr int PIN_EXPANSION_BUTTON = 19;   // GPIO19
// The Audio Kit has dedicated buttons via a key matrix (K1-K6)
// but those are not mapped as individual GPIOs

// ── Power ────────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = -1;
constexpr int PIN_BATTERY_SENSE      = -1;

// ── External display ──────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 0
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

// ── RTC ──────────────────────────────────────────────────────
#define HAS_ONBOARD_RTC 0
constexpr uint8_t RTC_I2C_ADDRESS = 0x51;

// ── Flags ────────────────────────────────────────────────────
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1
#define HAS_ONBOARD_LEDS    0
#define HAS_ONBOARD_BUZZER  0

constexpr int PIN_BUZZER = -1;

// ── Available IOs ───────────────────────────────────────────
constexpr int PIN_IO0  = 0;    // GPIO0  (BOOT)
constexpr int PIN_IO1  = 19;   // GPIO19
constexpr int PIN_IO2  = 21;   // GPIO21
constexpr int PIN_IO3  = 27;   // GPIO27
constexpr int PIN_IO4  = 255;  // unavailable
constexpr int PIN_IO5  = 255;
constexpr int PIN_IO6  = 255;
constexpr int PIN_IO7  = 255;
constexpr int PIN_IO8  = 255;
constexpr int PIN_IO9  = 255;
constexpr int PIN_IO10 = 255;
constexpr int PIN_IO11 = 255;
constexpr int PIN_IO12 = 255;

// ── Constants ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  // ES8388 codec — configure via I2C for audio mode
  // The codec is initialized with defaults acceptable for MP3 playback
  #if HAS_ONBOARD_CODEC
  // Default volume: 70% (-12dB on the ES8388)
  // The Arduino framework already configures the I2S pins automatically
  #endif
}

}  // namespace esp32_escaperoom
