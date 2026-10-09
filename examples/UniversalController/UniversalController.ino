/**
 * UNIVERSAL CONTROLLER - Controladora Universal para Escape Room
 *
 * Port do RP2040 PropMaker para ESP32-S3 usando ESP32EscapeRoom
 *
 * Um puzzle configurável via JSON que suporta múltiplos modos de operação:
 * - CUSTOM_SEQUENCE: Sequência customizada definida no JSON
 * - LINEAR_SEQUENCE: Ordem linear (0,1,2,3...)
 * - ALL_CONNECTED: Todas entradas ativadas, ordem não importa
 * - RANDOM: Sequência aleatória gerada no boot
 *
 * Hardware: Seeed XIAO ESP32-S3
 * - PCF8575 para até 16 entradas digitais
 * - NeoPixel para feedback visual
 * - Saída PWM gradativa (PCA9685)
 * - Áudio MP3 via I2S
 *
 * Arquivo de configuração: /config.json
 *
 * Home Assistant: Game Master monitora estado, progresso e modo.
 * Controla START / RESET / FORCE_SOLVE remotamente.
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

  haState.setName("Estado");
  haState.setIcon("mdi:puzzle");

  haProgress.setName("Progresso");
  haProgress.setUnitOfMeasurement("%");
  haProgress.setIcon("mdi:progress-check");

  haMode.setName("Modo");
  haMode.setIcon("mdi:cog");

  haStart.setName("Iniciar");
  haStart.setIcon("mdi:play");
  haStart.onCommand([](HAButton*){ puzzle.run(); if(puzzle.engine_) puzzle.engine_->start(); });

  haReset.setName("Resetar");
  haReset.setIcon("mdi:restart");
  haReset.onCommand([](HAButton*){ puzzle.setState(PuzzleState::Idle); if(puzzle.engine_) puzzle.engine_->reset(); });

  haForceSolve.setName("Forcar Solucao");
  haForceSolve.setIcon("mdi:check-bold");
  haForceSolve.onCommand([](HAButton*){ puzzle.setState(PuzzleState::Solved); });

  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
  haOK = true;
  Serial.println("[HA] Conectado ao Home Assistant");
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
// CONFIGURAÇÕES
// ============================================================================

#define BUTTON_START_PIN D0   // GPIO1 - Botão de START (via PCF ou nativo)

// ============================================================================
// CLASSE PRINCIPAL DO PUZZLE
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

    // Carrega configuração
    configLoaded_ = configManager_.loadConfig();
    if (!configLoaded_) {
        Serial.println("[Setup] Usando configuracao padrao");
        configManager_.loadDefaults();
    }

    const auto& controller = configManager_.getControllerConfig();
    Serial.printf("[Setup] Nome: %s | ID: %d | Habilitado: %s\n",
                  controller.name, controller.id, controller.enabled ? "Sim" : "Nao");

    const auto& inputs = configManager_.getInputConfig();
    Serial.printf("[Setup] Entradas: %d | PCF Address: 0x%02X | Debounce: %dms\n",
                  inputs.count, inputs.pcfAddress, inputs.debounceMs);

    const auto& logic = configManager_.getLogicConfig();
    Serial.printf("[Setup] Modo: %s | Timeout: %ds | ResetOnError: %s\n",
                  ConfigManager::modeToString(logic.mode),
                  logic.timeoutSeconds,
                  logic.resetOnError ? "Sim" : "Nao");

    // Configura PCF8575
    if (inputs.pcfAddress >= 0x20 && inputs.pcfAddress <= 0x27) {
        hw.addPcf8575(inputs.pcfAddress);
        Serial.printf("[Setup] PCF8575 adicionado no endereco 0x%02X\n", inputs.pcfAddress);

        if (hw.hasExpander()) {
            auto* io = hw.io();
            for (uint8_t i = 0; i < inputs.count; i++) {
                io->pinMode(inputs.pinMapping[i], INPUT_PULLUP);
            }
            Serial.println("[Setup] Pinos configurados como INPUT_PULLUP");
        }
    }

    // Inicializa engine do puzzle
    engine_ = new PuzzleEngine(configManager_);
    engine_->begin();

    // Configura botão de start (GPIO nativo)
    hw.pinMode(BUTTON_START_PIN, INPUT_PULLUP);

    // Configura saída de trava
    const auto& output = configManager_.getOutputConfig();
    hw.pinMode(output.lockPin, OUTPUT);
    hw.digitalWrite(output.lockPin, output.activeHigh ? LOW : HIGH);

    Serial.println("[Setup] Hardware configurado. Aguardando inicio...");
    printHelp();

    setState(PuzzleState::Idle);
}

void UniversalControllerPuzzle::update() {
    hw.sound().loop();
    handleSerialCommands();

    // Publica estado no HA a cada 2 segundos
    if (haOK && millis() - lastHAPublish_ > 2000) {
        publishHA();
        lastHAPublish_ = millis();
    }

    // Verifica botão de start
    if (getState() == PuzzleState::Idle) {
        if (hw.digitalRead(BUTTON_START_PIN) == LOW) {
            delay(50);
            if (hw.digitalRead(BUTTON_START_PIN) == LOW) {
                Serial.println("[Puzzle] Botao START pressionado");
                run();
                if (engine_) {
                    engine_->start();
                }
            }
        }
    }

    // Atualiza engine
    if (engine_ && getState() == PuzzleState::Running) {
        engine_->update();
        if (engine_->isSolved()) {
            setState(PuzzleState::Solved);
            publishHA();  // publica imediatamente ao resolver
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
    Serial.printf("Estado: %s\n", PuzzleBase::stateName(getState()));
    Serial.printf("Progresso: %d%% (%d/%d)\n",
                  engine_->getProgress(),
                  engine_->getCurrentStep(),
                  engine_->getTotalSteps());
    const auto& logic = configManager_.getLogicConfig();
    if (logic.timeoutSeconds > 0) {
        Serial.printf("Tempo restante: %ds\n", engine_->getRemainingTime());
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
            Serial.println("[CMD] Puzzle iniciado");
            publishHA();
        } else {
            Serial.println("[CMD] Puzzle ja esta rodando ou foi resolvido");
        }
    } else if (cmd == "reset") {
        if (engine_) engine_->reset();
        setState(PuzzleState::Idle);
        Serial.println("[CMD] Puzzle resetado");
        publishHA();
    } else if (cmd == "status") {
        printStatus();
    } else if (cmd == "solve" || cmd == "force") {
        setState(PuzzleState::Solved);
        Serial.println("[CMD] Puzzle forcado como resolvido");
        publishHA();
    } else if (cmd == "config") {
        const auto& controller = configManager_.getControllerConfig();
        const auto& inputs = configManager_.getInputConfig();
        const auto& logic = configManager_.getLogicConfig();
        Serial.println("\n----- CONFIGURACAO -----");
        Serial.printf("Nome: %s\n", controller.name);
        Serial.printf("ID: %d\n", controller.id);
        Serial.printf("Entradas: %d\n", inputs.count);
        Serial.printf("Modo: %s\n", ConfigManager::modeToString(logic.mode));
        Serial.printf("Timeout: %ds\n", logic.timeoutSeconds);
        Serial.println("------------------------\n");
    } else if (cmd == "help" || cmd == "?") {
        printHelp();
    } else if (cmd.length() > 0) {
        Serial.printf("[CMD] Comando desconhecido: '%s'\n", cmd.c_str());
    }
}

void UniversalControllerPuzzle::printHelp() {
    Serial.println("\n----- COMANDOS SERIAL -----");
    Serial.println("start    - Inicia o puzzle");
    Serial.println("reset    - Reseta o puzzle");
    Serial.println("status   - Mostra status atual");
    Serial.println("solve    - Forca puzzle como resolvido");
    Serial.println("config   - Mostra configuracao carregada");
    Serial.println("help     - Mostra esta ajuda");
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

    // WiFi + MQTT (portal AP se não configurado)
    netConfig.begin();
    if (netConfig.isAPMode()) {
        Serial.println(F("[CFG] Modo AP: http://192.168.4.1"));
    }

    puzzle.begin();

    // Home Assistant (após WiFi conectado)
    initHA();
    publishHA();
}

void loop() {
    loopHA();
    puzzle.update();
    delay(1);
}
