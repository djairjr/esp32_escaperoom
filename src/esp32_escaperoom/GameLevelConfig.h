#pragma once

#include <Arduino.h>

namespace esp32_escaperoom {

enum class GameLevelMode : uint8_t {
    PARALLEL = 0,   // All puzzles in the level run simultaneously
    SEQUENCE = 1    // Puzzles run one after the other
};

struct GameLevelConfig {
    const uint8_t* puzzleIds;   // Array of puzzle IDs in this level
    uint8_t puzzleCount;        // Number of puzzles in the level
    GameLevelMode mode;         // PARALLEL or SEQUENCE
};

} // namespace esp32_escaperoom
