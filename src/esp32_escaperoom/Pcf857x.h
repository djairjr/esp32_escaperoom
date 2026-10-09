#pragma once

#include <Arduino.h>
#include <IoAbstractionWire.h>

namespace esp32_escaperoom {

/**
 * Wrapper for PCF8574/PCF8575 using IoAbstraction.
 * Provides a simple interface to control multiple pins simultaneously.
 * Used mainly by RfidBus to select RFID readers.
 */
class Pcf857x {
 public:
  Pcf857x(uint8_t address, bool is16bit = true)
      : expander_(address, IO_PIN_NOT_DEFINED, &Wire, is16bit),
        currentState_(0xFFFF),
        is16bit_(is16bit) {}

  bool begin(uint16_t initialState = 0xFFFF) {
    currentState_ = initialState;
    expander_.runLoop();
    return write(currentState_);
  }

  bool write(uint16_t value) {
    currentState_ = value;
    if (is16bit_) {
      // PCF8575 - 16 bits
      expander_.writePort(0, value & 0xFF);
      expander_.writePort(1, (value >> 8) & 0xFF);
    } else {
      // PCF8574 - 8 bits
      expander_.writePort(0, value & 0xFF);
    }
    expander_.runLoop();
    return true;
  }

  uint16_t read() {
    expander_.runLoop();
    uint16_t result = expander_.readPort(0);
    if (is16bit_) {
      result |= ((uint16_t)expander_.readPort(1) << 8);
    }
    return result;
  }

  void pinMode(uint8_t pin, uint8_t mode) {
    expander_.pinMode(pin, mode);
  }

  void digitalWrite(uint8_t pin, uint8_t value) {
    expander_.digitalWrite(pin, value);
    expander_.runLoop();
  }

  uint8_t digitalRead(uint8_t pin) {
    expander_.runLoop();
    return expander_.digitalRead(pin);
  }

 private:
  PCF8574IoAbstraction expander_;
  uint16_t currentState_;
  bool is16bit_;
};

}  // namespace esp32_escaperoom
