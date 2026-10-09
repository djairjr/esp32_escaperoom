#pragma once

#include <Arduino.h>

namespace esp32_escaperoom {

class EscapeRoomRotaryEncoder {
 public:
  EscapeRoomRotaryEncoder(uint8_t pinA, uint8_t pinB) 
      : pinA_(pinA), pinB_(pinB), position_(0), lastA_(LOW), direction_(0) {}

  void begin() {
    pinMode(pinA_, INPUT_PULLUP);
    pinMode(pinB_, INPUT_PULLUP);
    lastA_ = digitalRead(pinA_);
  }

  // Update logic to be called in the loop
  void update() {
    bool currentA = digitalRead(pinA_);
    direction_ = 0;

    if (currentA != lastA_) {
      if (digitalRead(pinB_) != currentA) {
        position_++;
        direction_ = 1;
      } else {
        position_--;
        direction_ = -1;
      }
      lastA_ = currentA;
    }
  }

  int32_t getPosition() const { return position_; }
  int8_t getDirection() const { return direction_; }
  
  void setPosition(int32_t pos) { position_ = pos; }

 private:
  uint8_t pinA_, pinB_;
  int32_t position_;
  bool lastA_;
  int8_t direction_;
};

}  // namespace esp32_escaperoom
