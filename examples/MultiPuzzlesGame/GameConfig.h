// GameConfig.h - Configuration of the 5 Game Levels of the Nuclear Reactor
// This file defines the game levels. Each level contains a set
// of puzzles that run in PARALLEL or SEQUENCE.
//
// Levels:
//   0 - ACCESS:    P1 KeypadRFID, P2 CoolantValves, P3 ElectricalPanel (PARALLEL)
//   1 - POSITIONING: P4 RFID on the platform bases (PARALLEL)
//   2 - REACTOR:   P5 ReactorControl (PARALLEL)
//   3 - LID:       P6 SimonSays on the lid (PARALLEL)
//   4 - INTERIOR:  P7 RadioRepair, P8 RotaryDial, P9 BombTimer (SEQUENCE)

#pragma once

#include <ESP32EscapeRoom.h>

using namespace esp32_escaperoom;

// ===== PUZZLE IDs =====
// Each puzzle has a unique ID. Used to reference them in GameLevelConfig.
const uint8_t ID_P1_KEYPAD_RFID    = 1;
const uint8_t ID_P2_COOLANT_VALVES = 2;
const uint8_t ID_P3_ELECTRICAL_PNL = 3;
const uint8_t ID_P4_POSITION_RFID  = 4;
const uint8_t ID_P5_REACTOR_CTRL   = 5;
const uint8_t ID_P6_LID_SIMONSAYS  = 6;
const uint8_t ID_P7_RADIO_REPAIR   = 7;
const uint8_t ID_P8_ROTARY_DIAL    = 8;
const uint8_t ID_P9_BOMB_TIMER     = 9;

// ===== GAME LEVELS =====
// Declaration of the ID arrays for each level

// Level 0: Access (P1, P2, P3 in PARALLEL)
// The 3 technicians are trapped. The player solves the 3 access puzzles.
const uint8_t level0_acesso[] = {
    ID_P1_KEYPAD_RFID,
    ID_P2_COOLANT_VALVES,
    ID_P3_ELECTRICAL_PNL
};

// Level 1: Positioning (P4 in PARALLEL)
// Platforms extended. The player places each technician on the correct base.
const uint8_t level1_posicionamento[] = {
    ID_P4_POSITION_RFID
};

// Level 2: Reactor (P5 in PARALLEL)
// Adjust valves to keep the core stable.
const uint8_t level2_reator[] = {
    ID_P5_REACTOR_CTRL
};

// Level 3: Lid (P6 in PARALLEL)
// SimonSays on the closed lid. Solving it opens the lid.
const uint8_t level3_tampa[] = {
    ID_P6_LID_SIMONSAYS
};

// Level 4: Interior (P7, P8, P9 in SEQUENCE)
// Repair radio -> dial number -> cut wires in the correct order.
const uint8_t level4_interior[] = {
    ID_P7_RADIO_REPAIR,
    ID_P8_ROTARY_DIAL,
    ID_P9_BOMB_TIMER
};

// ===== GAME LEVELS CONFIGURATION =====
// Complete array of 5 levels to pass to LogicManager::setGameLevel()

const GameLevelConfig NUCLEAR_GAME_LEVELS[] = {
    { level0_acesso,          3, GameLevelMode::PARALLEL  },
    { level1_posicionamento,  1, GameLevelMode::PARALLEL  },
    { level2_reator,          1, GameLevelMode::PARALLEL  },
    { level3_tampa,           1, GameLevelMode::PARALLEL  },
    { level4_interior,        3, GameLevelMode::SEQUENCE  }
};

const uint8_t NUCLEAR_LEVEL_COUNT = 5;
