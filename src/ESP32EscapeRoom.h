#pragma once

/**
 * ESP32EscapeRoom - Complete engine for Escape Room puzzles
 * Single header required to use all features.
 *
 * Automatically includes:
 * - ArduinoJson (JSON parsing)
 * - IoAbstraction (PCF8574/PCF8575 I/O expanders)
 * - TaskManagerIO (Keypad scanning)
 * - arduino-audio-tools + arduino-libhelix (MP3 playback)
 * - Adafruit NeoPixel (LED strips)
 * - Adafruit LED Backpack (HT16K33 displays)
 * - Adafruit LIS3DH (Accelerometer)
 * - ADS1X15 (ADC converter)
 * - PCA9685 (PWM driver)
 * - MFRC522 (RFID readers - optional)
 */

// External libraries (wrappers below)
#include <ArduinoJson.h>

// IoAbstraction (PCF8574/8575, MCP23017, advanced encoders) — disableable.
// The ESP32-C3 has no DAC and IoAbstraction does not compile for it. For boards
// that do not use IoAbstraction expanders/encoders, define:
//   -DESP32_ESCAPEROOM_NO_IOABSTRACTION
// before including this header. The accordion (pure sensor) does not use IoAbstraction.
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
#include <IoAbstraction.h>
#include <TaskManagerIO.h>
#include <SwitchInput.h>
#include <KeyboardManager.h>
#endif

// Platform and utilities
#include "esp32_escaperoom/AudioPlayer.h"              // arduino-audio-tools
#include "esp32_escaperoom/PlatformPins.h"
#include "esp32_escaperoom/PuzzleState.h"

// Hardware drivers (use auxiliary libraries)
#include "esp32_escaperoom/Display14Seg.h"             // Adafruit LED Backpack

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
#include "esp32_escaperoom/Pcf857x.h"                  // IoAbstraction PCF857x wrapper
#endif

#include "esp32_escaperoom/RotaryEncoder.h"            // GPIO native
#include "esp32_escaperoom/RotaryDialReader.h"         // RotaryDial library wrapper
#include "esp32_escaperoom/MicInput.h"                   // INMP441 I2S RX + FFT

// Core hardware and puzzle framework
#include "esp32_escaperoom/Hardware.h"
#include "esp32_escaperoom/NfcReader.h"
#include "esp32_escaperoom/PuzzleBase.h"
#include "esp32_escaperoom/GameLevelConfig.h"
#include "esp32_escaperoom/LogicManager.h"
#include "esp32_escaperoom/SerialDebug.h"

// RFID - optional (only included if MFRC522.h is available)
#if __has_include(<MFRC522.h>)
#include "esp32_escaperoom/RfidBus.h"
#endif
