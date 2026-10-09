/**
 * UNIVERSAL CONTROLLER - Universal Escape Room Controller
 *
 * Port of the RP2040 PropMaker to ESP32-S3 using ESP32EscapeRoom
 *
 * A JSON-configurable puzzle that supports multiple operating modes:
 * - CUSTOM_SEQUENCE: Custom sequence defined in JSON
 * - LINEAR_SEQUENCE: Linear order (0,1,2,3...)
 * - ALL_CONNECTED: All inputs activated, order does not matter
 * - RANDOM: Random sequence generated at boot
 *
 * Hardware: Seeed XIAO ESP32-S3
 * - PCF8575 for up to 16 digital inputs
 * - NeoPixel for visual feedback
 * - Gradual PWM output (PCA9685)
 * - MP3 audio via I2S
 *
 * Configuration file: /config.json
 *
 * Home Assistant: Game Master monitors state, progress and mode.
 * Controls START / RESET / FORCE_SOLVE remotely.
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
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);
HASensor haState("state");
HASensor haProgress("progress");
HASensor haMode("mode");
HAButton haStart("start");
HAButton haReset("reset");
HAButton haForceSolve("force_solve");
bool haOK = false;

void initHA() {
  if (haOK || !WiFi.isConnected() || !strlen(netConfig.mqttBroker())) return;
  byte mac[6]; WiFi.macAddress(mac);
  haDevice.setUniqueId(mac, sizeof(mac));
  haDevice.setName(netConfig.devName());
  haDevice.setSoftwareVersion("1.0.0");

  haState.setName("State");
  haState.setIcon("mdi:puzzle");

  haProgress.setName("Progress");
  haProgress.setUnitOfMeasurement("%");
  haProgress.setIcon("mdi:progress-check");

  haMode.setName("Mode");
  haMode.setIcon("mdi:cog");

  haStart.setName("Start");
  haStart.setIcon("mdi:play");
  haStart.onCommand([](HAButton*){ puzzle.run(); if(puzzle.engine_) puzzle.engine_->start(); });

  haReset.setName("Reset");
  haReset.setIcon("mdi:restart");
  haReset.onCommand([](HAButton*){ puzzle.setState(PuzzleState::Idle); if(puzzle.engine_) puzzle.engine_->reset(); });

  haForceSolve.setName("Force Solve");
  haForceSolve.setIcon("mdi:check-bold");
  haForceSolve.onCommand([](HAButton*){ puzzle.setState(PuzzleState::Solved); });

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
  haState.setValue(PuzzleBase::stateName(puzzle.getState()));
  if (puzzle.engine_) haProgress.setValue(String(puzzle.engine_->getProgress()).c_str());
  const auto& logic = puzzle.configManager_.getLogicConfig();
  haMode.setValue(ConfigManager::modeToString(logic.mode));
}

// ============================================================================
// SETTINGS
// ============================================================================

#define BUTTON_START_PIN D0   // GPIO1 - START button (via PCF or native)

// ============================================================================
// MAIN PUZZLE CLASS
// ============================================================================

class UniversalControllerPuzzle : public PuzzleBase {
public:
    UniversalControllerPuzzle(uint8_t id = 1, const char* configPath = "/config.json");

    void setupHardware() override;
    void update() override;

    ConfigManager configManager_;
    PuzzleEngine* engine_ = nullptr;

private:
    bool configLoaded_ = false;
    unsigned long lastStatusPrint_ = 0;
    unsigned long lastHAPublish_ = 0;

    void printStatus();
    void handleSerialCommands();
    void printHelp();
};

UniversalControllerPuzzle::UniversalControllerPuzzle(uint8_t id, const char* configPath)
    : PuzzleBase(id), configManager_(configPath)
{
}

void UniversalControllerPuzzle::setupHardware() {
    Serial.println(F("\n========================================"));
    Serial.println(F("   UNIVERSAL CONTROLLER v1.0"));
    Serial.println(F("   Escape Room Configurable Puzzle"));
    Serial.println(F("========================================\n"));

    // Loads configuration
    configLoaded_ = configManager_.loadConfig();
    if (!configLoaded_) {
        Serial.println("[Setup] Using default configuration");
        configManager_.loadDefaults();
    }

    const auto& controller = configManager_.getControllerConfig();
    Serial.printf("[Setup] Name: %s | ID: %d | Enabled: %s\n",
                  controller.name, controller.id, controller.enabled ? "Yes" : "No");

    const auto& inputs = configManager_.getInputConfig();
    Serial.printf("[Setup] Inputs: %d | PCF Address: 0x%02X | Debounce: %dms\n",
                  inputs.count, inputs.pcfAddress, inputs.debounceMs);

    const auto& logic = configManager_.getLogicConfig();
    Serial.printf("[Setup] Mode: %s | Timeout: %ds | ResetOnError: %s\n",
                  ConfigManager::modeToString(logic.mode),
                  logic.timeoutSeconds,
                  logic.resetOnError ? "Yes" : "No");

    // Configures PCF8575
    if (inputs.pcfAddress >= 0x20 && inputs.pcfAddress <= 0x27) {
        hw.addPcf8575(inputs.pcfAddress);
        Serial.printf("[Setup] PCF8575 added at address 0x%02X\n", inputs.pcfAddress);

        if (hw.hasExpander()) {
            auto* io = hw.io();
            for (uint8_t i = 0; i < inputs.count; i++) {
                io->pinMode(inputs.pinMapping[i], INPUT_PULLUP);
            }
            Serial.println("[Setup] Pins configured as INPUT_PULLUP");
        }
    }

    // Initializes the puzzle engine
    engine_ = new PuzzleEngine(configManager_);
    engine_->begin();

    // Configures the start button (native GPIO)
    hw.pinMode(BUTTON_START_PIN, INPUT_PULLUP);

    // Configures the lock output
    const auto& output = configManager_.getOutputConfig();
    hw.pinMode(output.lockPin, OUTPUT);
    hw.digitalWrite(output.lockPin, output.activeHigh ? LOW : HIGH);

    Serial.println("[Setup] Hardware configured. Waiting to start...");
    printHelp();

    setState(PuzzleState::Idle);
}

void UniversalControllerPuzzle::update() {
    hw.sound().loop();
    handleSerialCommands();

    // Publishes state to HA every 2 seconds
    if (haOK && millis() - lastHAPublish_ > 2000) {
        publishHA();
        lastHAPublish_ = millis();
    }

    // Checks the start button
    if (getState() == PuzzleState::Idle) {
        if (hw.digitalRead(BUTTON_START_PIN) == LOW) {
            delay(50);
            if (hw.digitalRead(BUTTON_START_PIN) == LOW) {
                Serial.println("[Puzzle] START button pressed");
                run();
                if (engine_) {
                    engine_->start();
                }
            }
        }
    }

    // Updates engine
    if (engine_ && getState() == PuzzleState::Running) {
        engine_->update();
        if (engine_->isSolved()) {
            setState(PuzzleState::Solved);
            publishHA();  // publishes immediately upon solving
        }
    }

    if (getState() == PuzzleState::Running && millis() - lastStatusPrint_ > 5000) {
        printStatus();
        lastStatusPrint_ = millis();
    }
}

void UniversalControllerPuzzle::printStatus() {
    if (!engine_) return;
    Serial.println("\n----- STATUS -----");
    Serial.printf("State: %s\n", PuzzleBase::stateName(getState()));
    Serial.printf("Progress: %d%% (%d/%d)\n",
                  engine_->getProgress(),
                  engine_->getCurrentStep(),
                  engine_->getTotalSteps());
    const auto& logic = configManager_.getLogicConfig();
    if (logic.timeoutSeconds > 0) {
        Serial.printf("Remaining time: %ds\n", engine_->getRemainingTime());
    }
    Serial.println("------------------\n");
}

void UniversalControllerPuzzle::handleSerialCommands() {
    if (!Serial.available()) return;
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();

    if (cmd == "start" || cmd == "run") {
        if (getState() == PuzzleState::Idle) {
            run();
            if (engine_) engine_->start();
            Serial.println("[CMD] Puzzle started");
            publishHA();
        } else {
            Serial.println("[CMD] Puzzle is already running or was solved");
        }
    } else if (cmd == "reset") {
        if (engine_) engine_->reset();
        setState(PuzzleState::Idle);
        Serial.println("[CMD] Puzzle reset");
        publishHA();
    } else if (cmd == "status") {
        printStatus();
    } else if (cmd == "solve" || cmd == "force") {
        setState(PuzzleState::Solved);
        Serial.println("[CMD] Puzzle forced as solved");
        publishHA();
    } else if (cmd == "config") {
        const auto& controller = configManager_.getControllerConfig();
        const auto& inputs = configManager_.getInputConfig();
        const auto& logic = configManager_.getLogicConfig();
        Serial.println("\n----- CONFIGURATION -----");
        Serial.printf("Name: %s\n", controller.name);
        Serial.printf("ID: %d\n", controller.id);
        Serial.printf("Inputs: %d\n", inputs.count);
        Serial.printf("Mode: %s\n", ConfigManager::modeToString(logic.mode));
        Serial.printf("Timeout: %ds\n", logic.timeoutSeconds);
        Serial.println("------------------------\n");
    } else if (cmd == "help" || cmd == "?") {
        printHelp();
    } else if (cmd.length() > 0) {
        Serial.printf("[CMD] Unknown command: '%s'\n", cmd.c_str());
    }
}

void UniversalControllerPuzzle::printHelp() {
    Serial.println("\n----- SERIAL COMMANDS -----");
    Serial.println("start    - Starts the puzzle");
    Serial.println("reset    - Resets the puzzle");
    Serial.println("status   - Shows current status");
    Serial.println("solve    - Forces puzzle as solved");
    Serial.println("config   - Shows loaded configuration");
    Serial.println("help     - Shows this help");
    Serial.println("--------------------------\n");
}

// ============================================================================
// MAIN
// ============================================================================

UniversalControllerPuzzle puzzle;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    hw.begin();

    // WiFi + MQTT (AP portal if not configured)
    netConfig.begin();
    if (netConfig.isAPMode()) {
        Serial.println(F("[CFG] AP Mode: http://192.168.4.1"));
    }

    puzzle.begin();

    // Home Assistant (after WiFi is connected)
    initHA();
    publishHA();
}

void loop() {
    loopHA();
    puzzle.update();
    delay(1);
}
