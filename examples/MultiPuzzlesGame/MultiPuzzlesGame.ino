// ============================================================================
// Nuclear Reactor Escape Room - Multi-Puzzle Game
// ============================================================================
// Demonstrates the LogicManager Game Levels system with 5 levels and 9 puzzles.
//
// Architecture: 1 PropMaker RP2040, 5 PCF8575, 1 PN532, 2 ADS1115,
//               1 PCA9685, 1 NeoPixel strip of 48 LEDs, 1 TM1637, I2S audio
//
// RFID readers: CH552G module + MFRC522 per base (digital sensor LOW=tag OK).
// Does not use multiple PN532 on the I2C bus - each CH552G becomes a digital input on the PCF.
//
// Game Levels:
//   Level 0 (ACCESS - PARALLEL):    P1 KeypadRFID + P2 CoolantValves + P3 ElectricalPanel
//   Level 1 (POSITIONING - PAR):    P4 PositionRFID
//   Level 2 (REACTOR - PAR):        P5 ReactorControl
//   Level 3 (LID - PAR):            P6 LidSimonSays
//   Level 4 (INTERIOR - SEQUENCE):  P7 RadioRepair -> P8 RotaryDial -> P9 BombTimer
//
// Hardware I2C Mapping:
//   PCF8575 0x20 - Puzzle 1 (KeypadRFID keypad matrix + lock) (16 pins)
//   PCF8575 0x21 - Puzzle 2 (CoolantValves switches + lock)
//   PCF8575 0x22 - Puzzle 3 (ElectricalPanel switches + lock + drawer)
//   PCF8575 0x23 - Puzzle 6 (SimonSays buttons + lock)
//   PCF8575 0x25 - P4 (3x CH552G bases A/B/C) + P7 RadioRepair + P9 BombTimer
//                  P0-P2: CH552G bases | P3-P8: RadioRepair | P9-P14: BombTimer
//   PN532   0x24 - Puzzle 1 (RFID badge) - via hw.createNfcReader() (unique, no conflict)
//   ADS1115 0x48 - Puzzle 5 (3 pots)
//   PCA9685 0x40 - Servos (platforms + lid) + actuators
//   TM1637       - Global timer
//   NeoPixel     - 48 shared LEDs
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
// HOME ASSISTANT + WIFI (added by the ESP32 port)
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
  haSensorState.setName("State");
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
            Serial.println("[GAME] Release the 3 technicians!");
            hw.sound().playOnce("/sfx/level_acesso.mp3");
            break;
        case 1:
            Serial.println("[GAME] Position the technicians on the platforms!");
            motion_.extendAllPlatforms(0, 1, 2);
            hw.sound().playOnce("/sfx/level_position.mp3");
            break;
        case 2:
            Serial.println("[GAME] Stabilize the reactor core!");
            hw.sound().playOnce("/sfx/level_reactor.mp3");
            break;
        case 3:
            Serial.println("[GAME] SimonSays on the lid - pressurize the pumps!");
            hw.sound().playOnce("/sfx/level_lid.mp3");
            break;
        case 4:
            Serial.println("[GAME] Interior - repair, dial, defuse!");
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

// Level 0: Access
KeypadRfidPuzzle        puzzle1(0);       // PCF[0]=0x20
CoolantValvesPuzzle     puzzle2(1);       // PCF[1]=0x21
ElectricalPanelPuzzle   puzzle3(2);       // PCF[2]=0x22

// Level 1: Positioning
PositionRFIDPuzzle      puzzle4(4);       // PCF[4]=0x25 (3 CH552G bases A/B/C)

// Level 2: Reactor
ReactorControlPuzzle    puzzle5;

// Level 3: Lid
LidSimonSaysPuzzle      puzzle6(3);       // PCF[3]=0x23

// Level 4: Interior
RadioRepairPuzzle       puzzle7(4);       // PCF[4]=0x25 (pins P3-P8)
RotaryDialPuzzle        puzzle8(16, 17);  // GPIO 16, 17
BombTimerPuzzle         puzzle9(4);       // PCF[4]=0x25 (pins P9-P14)

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
    hw.addPcf8575(0x20);  // Index 0 - Puzzle 1
    hw.addPcf8575(0x21);  // Index 1 - Puzzle 2
    hw.addPcf8575(0x22);  // Index 2 - Puzzle 3
    hw.addPcf8575(0x23);  // Index 3 - Puzzle 6
    hw.addPcf8575(0x25);  // Index 4 - P4 (RFID bases) + P7 + P9

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
