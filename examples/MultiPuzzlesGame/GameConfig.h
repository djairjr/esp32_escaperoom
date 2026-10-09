// GameConfig.h - Configuracao dos 5 Game Levels do Nuclear Reactor
// Este arquivo define os niveis do jogo. Cada nivel contem um conjunto
// de puzzles que rodam em PARALLEL ou SEQUENCE.
//
// Niveis:
//   0 - ACESSO:    P1 KeypadRFID, P2 CoolantValves, P3 ElectricalPanel (PARALLEL)
//   1 - POSICIONAMENTO: P4 RFID nas bases das plataformas (PARALLEL)
//   2 - REATOR:    P5 ReactorControl (PARALLEL)
//   3 - TAMPA:     P6 SimonSays na tampa (PARALLEL)
//   4 - INTERIOR:  P7 RadioRepair, P8 RotaryDial, P9 BombTimer (SEQUENCE)

#pragma once

#include <ESP32EscapeRoom.h>

using namespace esp32_escaperoom;

// ===== IDs DOS PUZZLES =====
// Cada puzzle tem um ID unico. Usados para referenciar nos GameLevelConfig.
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
// Declaracao dos arrays de IDs para cada nivel

// Level 0: Acesso (P1, P2, P3 em PARALLEL)
// Os 3 tecnicos estao presos. Jogador resolve os 3 puzzles de acesso.
const uint8_t level0_acesso[] = {
    ID_P1_KEYPAD_RFID,
    ID_P2_COOLANT_VALVES,
    ID_P3_ELECTRICAL_PNL
};

// Level 1: Posicionamento (P4 em PARALLEL)
// Plataformas estendidas. Jogador coloca cada tecnico na base correta.
const uint8_t level1_posicionamento[] = {
    ID_P4_POSITION_RFID
};

// Level 2: Reator (P5 em PARALLEL)
// Ajustar valvulas para manter o nucleo estavel.
const uint8_t level2_reator[] = {
    ID_P5_REACTOR_CTRL
};

// Level 3: Tampa (P6 em PARALLEL)
// SimonSays na tampa fechada. Resolver abre a tampa.
const uint8_t level3_tampa[] = {
    ID_P6_LID_SIMONSAYS
};

// Level 4: Interior (P7, P8, P9 em SEQUENCE)
// Consertar radio -> discar numero -> cortar fios na ordem correta.
const uint8_t level4_interior[] = {
    ID_P7_RADIO_REPAIR,
    ID_P8_ROTARY_DIAL,
    ID_P9_BOMB_TIMER
};

// ===== CONFIGURACAO DOS GAME LEVELS =====
// Array completo de 5 niveis para passar ao LogicManager::setGameLevel()

const GameLevelConfig NUCLEAR_GAME_LEVELS[] = {
    { level0_acesso,          3, GameLevelMode::PARALLEL  },
    { level1_posicionamento,  1, GameLevelMode::PARALLEL  },
    { level2_reator,          1, GameLevelMode::PARALLEL  },
    { level3_tampa,           1, GameLevelMode::PARALLEL  },
    { level4_interior,        3, GameLevelMode::SEQUENCE  }
};

const uint8_t NUCLEAR_LEVEL_COUNT = 5;
