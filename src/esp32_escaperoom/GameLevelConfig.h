#pragma once

#include <Arduino.h>

namespace esp32_escaperoom {

enum class GameLevelMode : uint8_t {
    PARALLEL = 0,   // Todos os puzzles do nivel rodam simultaneamente
    SEQUENCE = 1    // Puzzles rodam um apos o outro
};

struct GameLevelConfig {
    const uint8_t* puzzleIds;   // Array de IDs dos puzzles neste nivel
    uint8_t puzzleCount;        // Quantidade de puzzles no nivel
    GameLevelMode mode;         // PARALLEL ou SEQUENCE
};

} // namespace esp32_escaperoom
