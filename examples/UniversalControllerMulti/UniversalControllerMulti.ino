/**
 * UNIVERSAL CONTROLLER MULTI - Multiple puzzles on one board
 *
 * Port of the RP2040 PropMaker to ESP32-S3 using ESP32EscapeRoom
 *
 * Example with 2 instances of UniversalController, each with:
 * - Its own PCF8575 (unique I2C address)
 * - Its own JSON configuration (mode, sequence, audio, color)
 * - Its own NeoPixel range on the strip
 * - Its own output pin (lock/relay)
 *
 * Hardware: Seeed XIAO ESP32-S3
 * - PCF8575 #1 at 0x20 (Puzzle A)
 * - PCF8575 #2 at 0x21 (Puzzle B)
 * - Shared NeoPixel strip
 *
 * Home Assistant: Game Master monitors each puzzle independently.
 * Controls START / RESET of each one remotely.
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

#include "ConfigManager.h"
#include "PuzzleEngine.h"

using namespace esp32_escaperoom;
using namespace universal_controller;

// ============================================================================
// D-PIN -> GPIO
// ============================================================================
#define D0   1
#define D1   2
#define D2   3
#define D3   4
#define D4   5
#define D5   6
#define D6   43
#define D7   44
#define D8   7
#define D9   8
#define D10  9
#define D11  42
#define D12  41

// ============================================================================
// HOME ASSISTANT + WIFI
// ============================================================================
esp32_escaperoom::ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);

// Puzzle A
HASensor haStateA("state_a");
HASensor haProgressA("progress_a");
HAButton haStartA("start_a");
HAButton haResetA("reset_a");

// Puzzle B
HASensor haStateB("state_b");
HASensor haProgressB("progress_b");
HAButton haStartB("start_b");
HAButton haResetB("reset_b");

bool haOK = false;

// Forward declarations
void initHA();
void loopHA();
void publishHA();

// ============================================================================
// PUZZLE CLASS (reuses ConfigManager + PuzzleEngine)
// ============================================================================

class MultiControllerPuzzle : public PuzzleBase {
public:
    MultiControllerPuzzle(uint8_t id, const char* configPath,
                          uint8_t pcfAddress, uint8_t pixelOffset, uint8_t pixelCount)
        : PuzzleBase(id), configManager_(configPath),
          pcfAddress_(pcfAddress), pixelOffset_(pixelOffset), pixelCount_(pixelCount) {}

    void setupHardware() override;
    void update() override;

    universal_controller::ConfigManager configManager_;
    PuzzleEngine* engine() { return engine_; }

private:
    PuzzleEngine* engine_ = nullptr;

    uint8_t pcfAddress_;
    uint8_t pixelOffset_;
    uint8_t pixelCount_;

    bool configLoaded_ = false;
    unsigned long lastStatusPrint_ = 0;

    void printStatus();
};

void MultiControllerPuzzle::setupHardware() {
    Serial.printf("[Puzzle %d] Starting (PCF=0x%02X, pixels=%d-%d)\n",
                  getId(), pcfAddress_, pixelOffset_, pixelOffset_ + pixelCount_ - 1);

    // Mounts LittleFS (only on the first call)
    static bool fsMounted = false;
    if (!fsMounted) {
        fsMounted = LittleFS.begin();
    }

    configLoaded_ = configManager_.loadConfig();
    if (!configLoaded_) {
        configManager_.loadDefaults();
    }

    const auto& inputs = configManager_.getInputConfig();

    // Configures the PCF specific to this puzzle
    hw.addPcf8575(pcfAddress_);
    if (hw.hasExpander()) {
        auto* io = hw.io();
        for (uint8_t i = 0; i < inputs.count; i++) {
            io->pinMode(inputs.pinMapping[i], INPUT_PULLUP);
        }
    }

    // Inicializa engine
    engine_ = new PuzzleEngine(configManager_);
    engine_->begin();

    // Configures the output
    const auto& output = configManager_.getOutputConfig();
    hw.pinMode(output.lockPin, OUTPUT);
    hw.digitalWrite(output.lockPin, output.activeHigh ? LOW : HIGH);

    // Sets this puzzle's NeoPixel range
    setPixelRange(pixelOffset_, pixelCount_);

    setState(PuzzleState::Idle);
}

void MultiControllerPuzzle::update() {
    hw.sound().loop();

    if (getState() == PuzzleState::Idle) {
        // Starts automatically in setup()
    }

    if (engine_ && getState() == PuzzleState::Running) {
        engine_->update();
        if (engine_->isSolved()) {
            setState(PuzzleState::Solved);
            publishHA();  // publishes immediately on solve
        }
    }

    if (getState() == PuzzleState::Running && millis() - lastStatusPrint_ > 5000) {
        printStatus();
        lastStatusPrint_ = millis();
    }
}

void MultiControllerPuzzle::printStatus() {
    if (!engine_) return;
    Serial.printf("[Puzzle %d] Progress: %d%% (%d/%d)\n",
                  getId(), engine_->getProgress(),
                  engine_->getCurrentStep(), engine_->getTotalSteps());
}

// ============================================================================
// INSTANCES
// ============================================================================

MultiControllerPuzzle puzzleA(1, "/config_puzzleA.json", 0x20, 0, 8);
MultiControllerPuzzle puzzleB(2, "/config_puzzleB.json", 0x21, 8, 8);

LogicManager master(0, false);

void initHA() {
  if (haOK || !WiFi.isConnected() || !strlen(netConfig.mqttBroker())) return;
  byte mac[6]; WiFi.macAddress(mac);
  haDevice.setUniqueId(mac, sizeof(mac));
  haDevice.setName(netConfig.devName());
  haDevice.setSoftwareVersion("1.0.0");

  haStateA.setName("Puzzle A State");
  haStateA.setIcon("mdi:puzzle");
  haProgressA.setName("Puzzle A Progress");
  haProgressA.setUnitOfMeasurement("%");
  haProgressA.setIcon("mdi:progress-check");

  haStartA.setName("Start Puzzle A");
  haStartA.setIcon("mdi:play");
  haStartA.onCommand([](HAButton*){ puzzleA.run(); if(puzzleA.engine()) puzzleA.engine()->start(); });

  haResetA.setName("Reset Puzzle A");
  haResetA.setIcon("mdi:restart");
  haResetA.onCommand([](HAButton*){ puzzleA.setState(PuzzleState::Idle); if(puzzleA.engine()) puzzleA.engine()->reset(); });

  haStateB.setName("Puzzle B State");
  haStateB.setIcon("mdi:puzzle");
  haProgressB.setName("Puzzle B Progress");
  haProgressB.setUnitOfMeasurement("%");
  haProgressB.setIcon("mdi:progress-check");

  haStartB.setName("Start Puzzle B");
  haStartB.setIcon("mdi:play");
  haStartB.onCommand([](HAButton*){ puzzleB.run(); if(puzzleB.engine()) puzzleB.engine()->start(); });

  haResetB.setName("Reset Puzzle B");
  haResetB.setIcon("mdi:restart");
  haResetB.onCommand([](HAButton*){ puzzleB.setState(PuzzleState::Idle); if(puzzleB.engine()) puzzleB.engine()->reset(); });

  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
  haOK = true;
  Serial.println("[HA] Connected to Home Assistant");
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}

void publishHA() {
  if (!haOK) return;
  haStateA.setValue(PuzzleBase::stateName(puzzleA.getState()));
  if (puzzleA.engine()) haProgressA.setValue(String(puzzleA.engine()->getProgress()).c_str());
  haStateB.setValue(PuzzleBase::stateName(puzzleB.getState()));
  if (puzzleB.engine()) haProgressB.setValue(String(puzzleB.engine()->getProgress()).c_str());
}

void setup() {
    Serial.begin(115200);

    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    hw.begin();

    // WiFi + MQTT
    netConfig.begin();
    if (netConfig.isAPMode()) {
        Serial.println(F("[CFG] AP Mode: http://192.168.4.1"));
    }

    master.addPuzzle(&puzzleA);
    master.addPuzzle(&puzzleB);
    master.begin();

    puzzleA.run();
    puzzleB.run();

    // Home Assistant
    initHA();
    publishHA();

    Serial.println(F("[MAIN] Puzzle A and B running"));
}

void loop() {
    loopHA();
    hw.sound().loop();
    puzzleA.update();
    puzzleB.update();
    delay(1);
}
