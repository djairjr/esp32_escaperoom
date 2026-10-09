/**
 * UNIVERSAL UNIQUE RFID - Leitor RFID Universal Configuravel
 *
 * Port do RP2040 PropMaker para ESP32-S3 usando ESP32EscapeRoom
 *
 * Hardware: Seeed XIAO ESP32-S3
 * - PN532 NFC/RFID via I2C (hw.createNfcReader())
 * - NeoPixel para feedback visual
 * - Audio MP3 via I2S
 *
 * Arquivo de configuracao: /config.json
 *
 * Home Assistant: Game Master monitora estado, progresso, modo e ultima tag.
 * Controla START / RESET / FORCE_SOLVE remotamente.
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

#include "ConfigManager.h"
#include "PuzzleEngine.h"

using namespace esp32_escaperoom;
using namespace universal_unique_rfid;

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
HASensor haLastTag("last_tag");
HAButton haStart("start");
HAButton haReset("reset");
HAButton haForceSolve("force_solve");
bool haOK = false;

// Forward
class UniversalUniqueRFIDPuzzle;
extern UniversalUniqueRFIDPuzzle puzzle;

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

  haLastTag.setName("Ultima Tag");
  haLastTag.setIcon("mdi:nfc");

  haStart.setName("Iniciar");
  haStart.setIcon("mdi:play");
  haStart.onCommand([](HAButton*){ puzzle.run(); if(puzzle.engine_) puzzle.engine_->start(); });

  haReset.setName("Resetar");
  haReset.setIcon("mdi:restart");
  haReset.onCommand([](HAButton*){ if(puzzle.engine_) puzzle.engine_->reset(); puzzle.setState(PuzzleState::Idle); });

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
  const auto& rfid = puzzle.configManager_.getRfidConfig();
  haMode.setValue(ConfigManager::modeToString(rfid.mode));
  // lastTag seria atualizada pelo engine quando uma tag é lida
}

// ============================================================================
// CONFIGURACOES
// ============================================================================

#define BUTTON_START_PIN D0   // GPIO1

// ============================================================================
// CLASSE PRINCIPAL DO PUZZLE
// ============================================================================

static bool cmdStart(const char*);
static bool cmdReset(const char*);
static bool cmdStatus(const char*);
static bool cmdSolve(const char*);
static bool cmdConfig(const char*);
static bool cmdTags(const char*);

class UniversalUniqueRFIDPuzzle : public PuzzleBase {
public:
    UniversalUniqueRFIDPuzzle();
    void setupHardware() override;
    void update() override;

    ConfigManager configManager_;
    PuzzleEngine* engine_ = nullptr;
    SerialDebug dbg_;

private:
    bool configLoaded_ = false;
    unsigned long lastStatusPrint_ = 0;
    unsigned long lastHAPublish_ = 0;

public:
    void printStatus();
};

UniversalUniqueRFIDPuzzle::UniversalUniqueRFIDPuzzle()
    : PuzzleBase(2) {}

void UniversalUniqueRFIDPuzzle::setupHardware() {
    Serial.println(F("\n========================================"));
    Serial.println(F("   UNIVERSAL UNIQUE RFID v1.0"));
    Serial.println(F("========================================\n"));

    if (!configManager_.begin()) {
        Serial.println("[Setup] ERRO: Falha LittleFS");
    }

    configLoaded_ = configManager_.loadConfig();
    if (!configLoaded_) {
        Serial.println("[Setup] Usando configuracao padrao");
        configManager_.loadDefaults();
    }

    const auto& controller = configManager_.getControllerConfig();
    Serial.printf("[Setup] Nome: %s | ID: %d\n", controller.name, controller.id);

    const auto& rfid = configManager_.getRfidConfig();
    Serial.printf("[Setup] Modo: %s | Tags: %d | Timeout: %ds\n",
                  ConfigManager::modeToString(rfid.mode), rfid.tagCount, rfid.timeoutSeconds);

    engine_ = new PuzzleEngine(configManager_);
    engine_->begin();

    hw.pinMode(BUTTON_START_PIN, INPUT_PULLUP);

    const auto& output = configManager_.getOutputConfig();
    hw.pinMode(output.lockPin, OUTPUT);
    hw.digitalWrite(output.lockPin, output.activeHigh ? LOW : HIGH);

    dbg_.begin();
    dbg_.addCommand("start",  "  Inicia o puzzle",                   cmdStart);
    dbg_.addCommand("reset",  "  Reseta o puzzle",                   cmdReset);
    dbg_.addCommand("status", "  Mostra status atual",               cmdStatus);
    dbg_.addCommand("solve",  "  Forca puzzle resolvido",            cmdSolve);
    dbg_.addCommand("config", "  Mostra configuracao",               cmdConfig);
    dbg_.addCommand("tags",   "  Lista tags programadas",            cmdTags);
    dbg_.printHelp();

    setState(PuzzleState::Idle);
}

void UniversalUniqueRFIDPuzzle::update() {
    hw.sound().loop();
    dbg_.update();

    // Publica estado no HA a cada 2 segundos
    if (haOK && millis() - lastHAPublish_ > 2000) {
        publishHA();
        lastHAPublish_ = millis();
    }

    if (getState() == PuzzleState::Idle) {
        if (hw.digitalRead(BUTTON_START_PIN) == LOW) {
            delay(50);
            if (hw.digitalRead(BUTTON_START_PIN) == LOW) {
                Serial.println("[Puzzle] Botao START pressionado");
                run();
                if (engine_) engine_->start();
                publishHA();
            }
        }
    }

    if (engine_ && getState() == PuzzleState::Running) {
        engine_->update();
        if (engine_->isSolved()) {
            setState(PuzzleState::Solved);
            publishHA();
        }
    }

    if (getState() == PuzzleState::Running && millis() - lastStatusPrint_ > 5000) {
        printStatus();
        lastStatusPrint_ = millis();
    }
}

void UniversalUniqueRFIDPuzzle::printStatus() {
    if (!engine_) return;
    Serial.println("\n----- STATUS -----");
    Serial.printf("Estado: %s\n", PuzzleBase::stateName(getState()));
    Serial.printf("Progresso: %d%% (%d/%d)\n", engine_->getProgress(),
                  engine_->getCurrentStep(), engine_->getTotalSteps());
    const auto& rfid = configManager_.getRfidConfig();
    if (rfid.timeoutSeconds > 0)
        Serial.printf("Tempo restante: %ds\n", engine_->getRemainingTime());
    Serial.println("------------------\n");
}

UniversalUniqueRFIDPuzzle puzzle;

// ============================================================================
// COMANDOS SERIAL
// ============================================================================
static bool cmdStart(const char*) {
    if (puzzle.getState() == PuzzleState::Idle) {
        puzzle.run();
        if (puzzle.engine_) puzzle.engine_->start();
        Serial.println("[CMD] Puzzle iniciado");
        publishHA();
    } else Serial.println("[CMD] Puzzle ja rodando");
    return true;
}

static bool cmdReset(const char*) {
    if (puzzle.engine_) puzzle.engine_->reset();
    puzzle.setState(PuzzleState::Idle);
    Serial.println("[CMD] Puzzle resetado");
    publishHA();
    return true;
}

static bool cmdStatus(const char*) { puzzle.printStatus(); return true; }

static bool cmdSolve(const char*) {
    puzzle.setState(PuzzleState::Solved);
    Serial.println("[CMD] Puzzle forcado resolvido");
    publishHA();
    return true;
}

static bool cmdConfig(const char*) {
    const auto& ctrl = puzzle.configManager_.getControllerConfig();
    const auto& rfid = puzzle.configManager_.getRfidConfig();
    Serial.println("\n----- CONFIGURACAO -----");
    Serial.printf("Nome: %s\n", ctrl.name);
    Serial.printf("Modo: %s\n", ConfigManager::modeToString(rfid.mode));
    Serial.printf("Tags: %d\n", rfid.tagCount);
    for (uint8_t i = 0; i < rfid.tagCount; i++)
        Serial.printf("  [%d] %s -> %s\n", i, rfid.tags[i].name, rfid.tags[i].uid);
    if (rfid.mode == RfidMode::SEQUENCE) {
        Serial.print("Sequencia:");
        for (uint8_t i = 0; i < rfid.sequenceLength; i++) Serial.printf(" %d", rfid.sequence[i]);
        Serial.println();
    }
    return true;
}

static bool cmdTags(const char*) {
    const auto& rfid = puzzle.configManager_.getRfidConfig();
    Serial.printf("\nTags programadas (%d):\n", rfid.tagCount);
    for (uint8_t i = 0; i < rfid.tagCount; i++)
        Serial.printf("  [%d] %s -> %s\n", i, rfid.tags[i].name, rfid.tags[i].uid);
    Serial.printf("\nTags validas (%d):\n", rfid.validTagCount);
    for (uint8_t i = 0; i < rfid.validTagCount; i++)
        Serial.printf("  [%d] %s -> %s\n", i, rfid.validTags[i].name, rfid.validTags[i].uid);
    return true;
}

// ============================================================================
// MAIN
// ============================================================================

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    hw.begin();

    // WiFi + MQTT
    netConfig.begin();
    if (netConfig.isAPMode()) {
        Serial.println(F("[CFG] Modo AP: http://192.168.4.1"));
    }

    puzzle.begin();

    // Home Assistant
    initHA();
    publishHA();
}

void loop() {
    loopHA();
    puzzle.update();
    delay(1);
}
