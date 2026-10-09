// ============================================================================
// Nuclear Reactor Escape Room - Multi-Puzzle Game
// ============================================================================
// Demonstra o sistema de Game Levels do LogicManager com 5 niveis e 9 puzzles.
//
// Arquitetura: 1 PropMaker RP2040, 5 PCF8575, 1 PN532, 2 ADS1115,
//              1 PCA9685, 1 strip NeoPixel 48 LEDs, 1 TM1637, audio I2S
//
// RFID readers: modulo CH552G + MFRC522 por base (sensor digital LOW=tag OK).
// Nao usa multiplos PN532 no I2C - cada CH552G vira uma entrada digital no PCF.
//
// Game Levels:
//   Level 0 (ACESSO - PARALLEL):    P1 KeypadRFID + P2 CoolantValves + P3 ElectricalPanel
//   Level 1 (POSICIONAMENTO - PAR): P4 PositionRFID
//   Level 2 (REATOR - PAR):         P5 ReactorControl
//   Level 3 (TAMPA - PAR):          P6 LidSimonSays
//   Level 4 (INTERIOR - SEQUENCE):  P7 RadioRepair -> P8 RotaryDial -> P9 BombTimer
//
// Hardware Mapeamento I2C:
//   PCF8575 0x20 - Puzzle 1 (KeypadRFID keypad matrix + lock) (16 pinos)
//   PCF8575 0x21 - Puzzle 2 (CoolantValves switches + lock)
//   PCF8575 0x22 - Puzzle 3 (ElectricalPanel switches + lock + gaveta)
//   PCF8575 0x23 - Puzzle 6 (SimonSays botoes + lock)
//   PCF8575 0x25 - P4 (3x CH552G bases A/B/C) + P7 RadioRepair + P9 BombTimer
//                  P0-P2: CH552G bases | P3-P8: RadioRepair | P9-P14: BombTimer
//   PN532   0x24 - Puzzle 1 (cracha RFID) - via hw.createNfcReader() (unico, sem conflito)
//   ADS1115 0x48 - Puzzle 5 (3 pots)
//   PCA9685 0x40 - Servos (plataformas + tampa) + atuadores
//   TM1637       - Timer global
//   NeoPixel     - 48 LEDs compartilhados
// ============================================================================

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

// Puzzle implementations
#include "GameConfig.h"
#include "puzzles/MotionController.h"
#include "puzzles/AcessoPuzzles.h"
#include "puzzles/PositionReactorPuzzles.h"
#include "puzzles/InteriorPuzzles.h"

using namespace esp32_escaperoom;

// ============================================================================
// HOME ASSISTANT + WIFI (adicionado pelo port ESP32)
// ============================================================================
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);
HASensor haSensorState("state");

void initHA() {
  netConfig.begin();
  if (netConfig.isAPMode()) Serial.println(F("[CFG] AP: http://192.168.4.1"));
  byte mac[6]; WiFi.macAddress(mac);
  haDevice.setUniqueId(mac, sizeof(mac));
  haDevice.setName(netConfig.devName());
  haDevice.setSoftwareVersion("1.0.0");
  haSensorState.setName("Estado");
  haSensorState.setIcon("mdi:puzzle");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}


// ============================================================================
// GAME FLOW
// ============================================================================

class ReactorGameLogicManager : public LogicManager {
public:
    ReactorGameLogicManager() : LogicManager(0, false) {}

    void onGameLevelEnter(uint8_t levelIndex) override {
        Serial.printf("[GAME] === ENTERING LEVEL %d ===\n", levelIndex);

        switch (levelIndex) {
        case 0:
            Serial.println("[GAME] Libere os 3 tecnicos!");
            hw.sound().playOnce("/sfx/level_acesso.mp3");
            break;
        case 1:
            Serial.println("[GAME] Posicione os tecnicos nas plataformas!");
            motion_.extendAllPlatforms(0, 1, 2);
            hw.sound().playOnce("/sfx/level_position.mp3");
            break;
        case 2:
            Serial.println("[GAME] Estabilize o nucleo do reator!");
            hw.sound().playOnce("/sfx/level_reactor.mp3");
            break;
        case 3:
            Serial.println("[GAME] SimonSays na tampa - pressurize as bombas!");
            hw.sound().playOnce("/sfx/level_lid.mp3");
            break;
        case 4:
            Serial.println("[GAME] Interior - conserte, discute, desarme!");
            hw.sound().playOnce("/sfx/level_interior.mp3");
            break;
        }

        hw.setLightAll(0x222222);
    }

    void onGameLevelExit(uint8_t levelIndex) override {
        Serial.printf("[GAME] === LEVEL %d COMPLETE ===\n", levelIndex);
        hw.setLightAll(0x00FF00);
        delay(500);
        hw.setLightAll(0x000000);
        hw.sound().playOnce("/sfx/level_complete.mp3");
    }

    void onGameComplete() override {
        Serial.println("[GAME] NUCLEAR REACTOR CONTAINED!");
        hw.setLightAll(0x00FF00);
        hw.sound().playOnce("/sfx/victory.mp3");
    }

    void onPuzzleSolved(uint8_t puzzleIndex) override {
        Serial.printf("[GAME] Puzzle idx %d solved\n", puzzleIndex);
    }

    MotionController& motion() { return motion_; }

private:
    MotionController motion_;
};

// ============================================================================
// PUZZLE INSTANCES
// ============================================================================

// Level 0: Acesso
KeypadRfidPuzzle        puzzle1(0);       // PCF[0]=0x20
CoolantValvesPuzzle     puzzle2(1);       // PCF[1]=0x21
ElectricalPanelPuzzle   puzzle3(2);       // PCF[2]=0x22

// Level 1: Posicionamento
PositionRFIDPuzzle      puzzle4(4);       // PCF[4]=0x25 (3 CH552G bases A/B/C)

// Level 2: Reator
ReactorControlPuzzle    puzzle5;

// Level 3: Tampa
LidSimonSaysPuzzle      puzzle6(3);       // PCF[3]=0x23

// Level 4: Interior
RadioRepairPuzzle       puzzle7(4);       // PCF[4]=0x25 (pinos P3-P8)
RotaryDialPuzzle        puzzle8(16, 17);  // GPIO 16, 17
BombTimerPuzzle         puzzle9(4);       // PCF[4]=0x25 (pinos P9-P14)

// ============================================================================
// GAME MANAGER
// ============================================================================

ReactorGameLogicManager master;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(2000);

    Serial.println(F("============================================"));
    Serial.println(F(" Nuclear Reactor Escape Room v2"));
    Serial.println(F(" Multi-Puzzle Game Level Demo"));
    Serial.println(F("============================================"));

    // ========== HARDWARE BOOT ==========
    hw.begin();

    // ========== ADD I2C DEVICES ==========
    hw.addPcf8575(0x20);  // Indice 0 - Puzzle 1
    hw.addPcf8575(0x21);  // Indice 1 - Puzzle 2
    hw.addPcf8575(0x22);  // Indice 2 - Puzzle 3
    hw.addPcf8575(0x23);  // Indice 3 - Puzzle 6
    hw.addPcf8575(0x25);  // Indice 4 - P4 (bases RFID) + P7 + P9

    // ========== INIT MOTION ==========
    master.motion().begin();

    // ========== REGISTER PUZZLES ==========
    master.addPuzzle(&puzzle1);
    master.addPuzzle(&puzzle2);
    master.addPuzzle(&puzzle3);
    master.addPuzzle(&puzzle4);
    master.addPuzzle(&puzzle5);
    master.addPuzzle(&puzzle6);
    master.addPuzzle(&puzzle7);
    master.addPuzzle(&puzzle8);
    master.addPuzzle(&puzzle9);

    // ========== GAME LEVELS ==========
    master.setGameLevel(NUCLEAR_GAME_LEVELS, NUCLEAR_LEVEL_COUNT);

    // ========== BEGIN ==========
      initHA();
  master.begin();

    // ========== START TIMER ==========
    master.startTimer(1800);  // 30min

    // ========== START GAME ==========
    master.startGame();

    Serial.println(F("[SETUP] Game ready!"));
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
  loopHA();
    master.update();
    hw.sound().loop();
    delay(1);
}
