#pragma once

/**
 * PlatformPins.h — Automatic pin selection per ESP32 board.
 *
 * Detects the board via Arduino IDE macros and includes the
 * corresponding pin file from the pins/ folder.
 *
 * For boards without automatic detection, define BOARD_xxx manually
 * in the PlatformIO build flags or in the Arduino IDE options:
 *
 *   YD-ESP32-S3:      -DBOARD_YD_ESP32S3
 *   STAMPLC:           -DBOARD_STAMPLC
 *   Audio Kit:         -DBOARD_AUDIO_KIT
 *   MatrixPortal S3:   -DBOARD_MATRIXPORTAL
 *
 * Boards with automatic detection:
 *   XIAO ESP32-S3, XIAO ESP32-C3, M5Stack Cardputer
 */

#include <Arduino.h>

// ── Automatic board detection ───────────────────────────────

// M5Stack Cardputer
#if defined(ARDUINO_M5STACK_CARDPUTER) || defined(ARDUINO_M5Stack_Cardputer) || defined(BOARD_CARDPUTER)
  #include "pins/cardputer.h"

// M5Stack StampPLC
#elif defined(ARDUINO_M5STACK_STAMPLC) || defined(BOARD_STAMPLC)
  #include "pins/stamplc.h"

// Seeed XIAO ESP32-S3
#elif defined(ARDUINO_XIAO_ESP32S3) || defined(SEEED_XIAO_ESP32S3)
  #include "pins/xiao_esp32s3.h"

// Seeed XIAO ESP32-C3
#elif defined(ARDUINO_XIAO_ESP32C3)
  #include "pins/xiao_esp32c3.h"

// Adafruit MatrixPortal S3
#elif defined(ARDUINO_ADAFRUIT_MATRIXPORTAL_ESP32S3) || defined(BOARD_MATRIXPORTAL)
  #include "pins/matrixportal.h"

// Ai-Thinker ESP32-A1S Audio Kit
#elif defined(ARDUINO_ESP32_A1S_AUDIO_KIT) || defined(BOARD_AUDIO_KIT)
  #include "pins/audio_kit.h"

// VCC-GND Studio YD-ESP32-S3 (generic — requires manual define)
#elif defined(BOARD_YD_ESP32S3)
  #include "pins/yd_esp32s3.h"

// ── Fallback: XIAO ESP32-S3 (default) ────────────────────────
#else
  #pragma message("PlatformPins: No board detected. Using default (XIAO ESP32-S3).")
  #pragma message("Define BOARD_xxx in the build flags for your specific board.")
  #include "pins/xiao_esp32s3.h"
#endif

// ── RS485 constants (valid for all boards) ──────────

#ifndef PUZZLE_ENABLE_RS485
#define PUZZLE_ENABLE_RS485 0
#endif

#ifndef PUZZLE_RS485_BAUD
#define PUZZLE_RS485_BAUD 115200
#endif

#ifndef PUZZLE_RS485_STATUS_INTERVAL_MS
#define PUZZLE_RS485_STATUS_INTERVAL_MS 1000UL
#endif

#ifndef PUZZLE_RS485_NODE_ID
#define PUZZLE_RS485_NODE_ID 1
#endif

// ── Global aliases (compatibility with legacy sketches) ────

constexpr int PUZZLE_PIN_SPI_MISO          = esp32_escaperoom::PIN_SPI_MISO;
constexpr int PUZZLE_PIN_SD_CS             = esp32_escaperoom::PIN_SD_CS;
constexpr int PUZZLE_PIN_RELAY             = esp32_escaperoom::PIN_RELAY;
constexpr int PUZZLE_PIN_I2S_DATA          = esp32_escaperoom::PIN_I2S_DATA;
constexpr int PUZZLE_PIN_I2S_BCLK          = esp32_escaperoom::PIN_I2S_BCLK;
constexpr int PUZZLE_PIN_I2S_WS            = esp32_escaperoom::PIN_I2S_WS;
constexpr int PUZZLE_PIN_BTN               = esp32_escaperoom::PIN_BTN;
constexpr int PUZZLE_PIN_SERVO_PWM         = esp32_escaperoom::PIN_SERVO_PWM;
constexpr int PUZZLE_PIN_EXTERNAL_NEOPIXEL = esp32_escaperoom::PIN_EXTERNAL_NEOPIXEL;
constexpr int PUZZLE_PIN_NEOPIXEL          = esp32_escaperoom::PIN_NEOPIXEL;
constexpr int PUZZLE_PIN_EXTERNAL_POWER    = esp32_escaperoom::PIN_EXTERNAL_POWER;
constexpr int PUZZLE_PIN_BATTERY_SENSE     = esp32_escaperoom::PIN_BATTERY_SENSE;
