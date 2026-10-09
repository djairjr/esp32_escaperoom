#pragma once

#include <Arduino.h>

namespace esp32_escaperoom {

enum class PuzzleState : uint8_t {
  Inactive = 0,
  Initialising = 1,
  Idle = 2,
  Running = 3,
  Solved = 4,
  Error = 5,
  Disabled = 6,
};

inline const __FlashStringHelper *stateName(PuzzleState state) {
  switch (state) {
    case PuzzleState::Inactive:
      return F("INACTIVE");
    case PuzzleState::Initialising:
      return F("INITIALISING");
    case PuzzleState::Idle:
      return F("IDLE");
    case PuzzleState::Running:
      return F("RUNNING");
    case PuzzleState::Solved:
      return F("SOLVED");
    case PuzzleState::Error:
      return F("ERROR");
    case PuzzleState::Disabled:
      return F("DISABLED");
  }
  return F("UNKNOWN");
}

}  // namespace esp32_escaperoom
