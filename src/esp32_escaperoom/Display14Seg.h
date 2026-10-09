#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LEDBackpack.h>

namespace esp32_escaperoom {

class Display14Seg {
 public:
  Display14Seg() : initialized_(false) {}

  void begin(uint8_t address = 0x70);
  void setBrightness(uint8_t brightness);
  void setBlinkRate(uint8_t rate);
  void clear();
  void setColon(bool on);
  void printNum(uint16_t value, bool leadingZeros = false);
  void printString(const char *text);
  void printTime(uint16_t minutes, uint16_t seconds, bool blink = true);
  void writeDisplay();
  void writeDigitRaw(uint8_t digit, uint16_t value);
  void writeDigitAscii(uint8_t digit, char c);

  bool isInitialized() const { return initialized_; }

 private:
  Adafruit_AlphaNum4 display14x4_;
  uint16_t digits_[4];  // Buffer interno para manter estado dos dígitos
  bool initialized_;
};

}  // namespace esp32_escaperoom
