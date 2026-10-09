#pragma once

/**
 * ESP32EscapeRoom - Engine completa para puzzles de Escape Room
 * Único header necessário para usar todas as funcionalidades.
 *
 * Inclui automaticamente:
 * - ArduinoJson (JSON parsing)
 * - IoAbstraction (PCF8574/PCF8575 I/O expanders)
 * - TaskManagerIO (Keypad scanning)
 * - arduino-audio-tools + arduino-libhelix (MP3 playback)
 * - Adafruit NeoPixel (LED strips)
 * - Adafruit LED Backpack (HT16K33 displays)
 * - Adafruit LIS3DH (Accelerometer)
 * - ADS1X15 (ADC converter)
 * - PCA9685 (PWM driver)
 * - MFRC522 (RFID readers - opcional)
 */

// Bibliotecas externas (wrappers abaixo)
#include <ArduinoJson.h>

// IoAbstraction (PCF8574/8575, MCP23017, encoders avançados) — desabilitável.
// O ESP32-C3 não tem DAC e a IoAbstraction não compila para ele. Para placas
// que não usam expansores/encoders IoAbstraction, defina:
//   -DESP32_ESCAPEROOM_NO_IOABSTRACTION
// antes de incluir este header. O acordeon (sensor puro) não usa IoAbstraction.
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
#include <IoAbstraction.h>
#include <TaskManagerIO.h>
#include <SwitchInput.h>
#include <KeyboardManager.h>
#endif

// Platform e utilitários
#include "esp32_escaperoom/AudioPlayer.h"              // arduino-audio-tools
#include "esp32_escaperoom/PlatformPins.h"
#include "esp32_escaperoom/PuzzleState.h"

// Drivers de hardware (usam bibliotecas auxiliares)
#include "esp32_escaperoom/Display14Seg.h"             // Adafruit LED Backpack

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
#include "esp32_escaperoom/Pcf857x.h"                  // IoAbstraction PCF857x wrapper
#endif

#include "esp32_escaperoom/RotaryEncoder.h"            // GPIO native
#include "esp32_escaperoom/RotaryDialReader.h"         // RotaryDial library wrapper
#include "esp32_escaperoom/MicInput.h"                   // INMP441 I2S RX + FFT

// Core hardware e puzzle framework
#include "esp32_escaperoom/Hardware.h"
#include "esp32_escaperoom/NfcReader.h"
#include "esp32_escaperoom/PuzzleBase.h"
#include "esp32_escaperoom/GameLevelConfig.h"
#include "esp32_escaperoom/LogicManager.h"
#include "esp32_escaperoom/SerialDebug.h"

// RFID - opcional ( só inclui se MFRC522.h disponível)
#if __has_include(<MFRC522.h>)
#include "esp32_escaperoom/RfidBus.h"
#endif
