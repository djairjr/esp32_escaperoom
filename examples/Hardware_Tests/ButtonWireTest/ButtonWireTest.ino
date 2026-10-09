/**
 * EXEMPLO: BUTTON WIRE TEST - TESTE DE BOTÕES E FIOS
 * ------------------------------------------------------
 * Teste isolado de 4 botões e 4 fios via PCF8575 (I2C) usando SwitchInput.
 *
 * HARDWARE: Seeed XIAO ESP32-S3 + PCF8575
 * - PCF8575 endereço I2C: 0x20
 * - Botões: P10-P13 (ativos em LOW, INPUT_PULLUP)
 * - Fios: P14-P17 (ativos em LOW quando cortados, INPUT_PULLUP)
 *
 * MENSAGENS SERIAL:
 * - [BTN] LEFT/SEL/RIGHT/DET PRESSED
 * - [WIRE] Fio X CORTADO
 * - [STATUS] Estado atual de todos os pinos
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

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
// CONFIGURAÇÃO
// ============================================================================
#define PCF_ADDRESS     0x20   // Endereço I2C do PCF8575

// Botões (P10-P13 físicos = 8-11 na IoAbstraction) - ativos em LOW
#define BUTTON_LEFT_PIN     8   // P10 físico
#define BUTTON_SELECT_PIN   9   // P11 físico
#define BUTTON_RIGHT_PIN    10  // P12 físico
#define BUTTON_DET_PIN      11  // P13 físico

// Fios (P14-P17 físicos = 12-15 na IoAbstraction) - ativos em LOW quando cortados
#define WIRE1_PIN           12  // P14 físico
#define WIRE2_PIN           13  // P15 físico
#define WIRE3_PIN           14  // P16 físico
#define WIRE4_PIN           15  // P17 físico

// Nomes para debug
const char* BTN_NAMES[] = {"LEFT", "SELECT", "RIGHT", "DET"};
const char* WIRE_NAMES[] = {"WIRE1", "WIRE2", "WIRE3", "WIRE4"};

// Variáveis de estado (setadas pelos callbacks)
volatile bool btnPressed[4] = {false, false, false, false};
volatile bool wireCutFlag[4] = {false, false, false, false};

// Contadores para estatísticas
unsigned long btnPressCount[4] = {0, 0, 0, 0};
unsigned long wireCutCount[4] = {0, 0, 0, 0};

// Forward declarations
void onButtonPressed(pinid_t pin, bool held);
void onWireChanged(pinid_t pin, bool held);
void printStatus();

// ============================================================================
// LISTENER PARA FIOS (global para acesso no setup)
// ============================================================================
class WireListener : public SwitchListener {
    int wireIdx;
public:
    WireListener(int idx) : wireIdx(idx) {}
    void onPressed(pinid_t pin, bool held) override {
        onWireChanged(pin, held);
    }
    void onReleased(pinid_t pin, bool held) override {
        onWireChanged(pin, false);
    }
};

// Instâncias globais dos listeners
WireListener wireListeners[4] = {WireListener(0), WireListener(1), WireListener(2), WireListener(3)};

// ============================================================================
// CALLBACKS SwitchInput
// ============================================================================

void onButtonPressed(pinid_t pin, bool held) {
    int idx = pin - BUTTON_LEFT_PIN;
    if (idx >= 0 && idx < 4) {
        btnPressed[idx] = true;
        btnPressCount[idx]++;
        Serial.printf("[BTN] %s PRESSED%s (count=%lu)\n",
                      BTN_NAMES[idx], held ? " (HELD)" : "", btnPressCount[idx]);

        // Feedback visual no NeoPixel
        hw.setLightAll(0);
        hw.setLight(idx, Adafruit_NeoPixel::Color(0, 50, 0));  // Verde no índice
    }
}

// Mapeia pino físico para índice do fio (0-3)
int wirePinToIndex(pinid_t pin) {
  switch(pin) {
    case WIRE1_PIN: return 0;
    case WIRE2_PIN: return 1;
    case WIRE3_PIN: return 2;
    case WIRE4_PIN: return 3;
    default: return -1;
  }
}

void onWireChanged(pinid_t pin, bool held) {
    int idx = wirePinToIndex(pin);
    if (idx >= 0 && idx < 4) {
        if (held && !wireCutFlag[idx]) {
            wireCutFlag[idx] = true;
            wireCutCount[idx]++;
            Serial.printf("[WIRE] %s CORTADO (count=%lu)\n", WIRE_NAMES[idx], wireCutCount[idx]);
            hw.setLightAll(Adafruit_NeoPixel::Color(50, 0, 0));
        }
        else if (!held && wireCutFlag[idx]) {
            wireCutFlag[idx] = false;
            Serial.printf("[WIRE] %s RECONECTADO\n", WIRE_NAMES[idx]);
            hw.setLightAll(Adafruit_NeoPixel::Color(0, 0, 50));
        }
    }
}

// ============================================================================
// CLASSE PUZZLE
// ============================================================================
class ButtonWireTest : public PuzzleBase {
public:
    ButtonWireTest() : PuzzleBase(99) {}  // ID 99 (teste)

    void setupHardware() override {
        Serial.println(F("\n========================================"));
        Serial.println(F("  ButtonWireTest - Teste de Entradas"));
        Serial.println(F("========================================"));

        // Inicializa SwitchInput
        if (hw.hasExpander()) {
            Serial.println(F("[TEST] Inicializando SwitchInput no PCF8575..."));
            switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);  // pull up

            // Adiciona 4 botões
            switches.addSwitch(BUTTON_LEFT_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_SELECT_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_RIGHT_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_DET_PIN, onButtonPressed, NO_REPEAT, false);

            // Adiciona 4 fios (usando listeners globais)
            // IMPORTANTE: invertLogic=true porque queremos detectar HIGH como "cortado"
            switches.addSwitchListener(WIRE1_PIN, &wireListeners[0], NO_REPEAT, true);
            switches.addSwitchListener(WIRE2_PIN, &wireListeners[1], NO_REPEAT, true);
            switches.addSwitchListener(WIRE3_PIN, &wireListeners[2], NO_REPEAT, true);
            switches.addSwitchListener(WIRE4_PIN, &wireListeners[3], NO_REPEAT, true);

            Serial.println(F("[TEST] 4 botoes + 4 fios configurados"));

            delay(100);
            printStatus();
        } else {
            Serial.println(F("[TEST] ERRO: PCF8575 nao detectado!"));
        }

        Serial.println(F("\n[TEST] Pronto para teste."));
        Serial.println(F("[TEST] Pressione botoes ou corte fios para ver eventos."));
        Serial.println(F("[TEST] Status completo a cada 5 segundos.\n"));

        setState(PuzzleState::Running);
    }

    void update() override {
        switches.runLoop();

        for (int i = 0; i < 4; i++) {
            btnPressed[i] = false;
        }

        scanAllPins();

        static unsigned long lastStatus = 0;
        if (millis() - lastStatus >= 5000) {
            lastStatus = millis();
            printStatus();
        }
    }

private:
    void scanAllPins() {
        static unsigned long lastScan = 0;
        static uint16_t lastState = 0xFFFF;

        if (millis() - lastScan < 100) return;
        lastScan = millis();

        uint16_t currentState = 0;
        for (int i = 0; i < 16; i++) {
            int val = hw.io()->readValue(i);
            if (val == LOW) {
                currentState |= (1 << i);
            }
        }

        if (currentState != lastState) {
            Serial.println(F("\n--- SCAN PINOS 0-15 ---"));
            Serial.print(F("Estado LOW: "));
            for (int i = 0; i < 16; i++) {
                bool isLow = (currentState >> i) & 1;
                if (isLow) {
                    Serial.printf("P%d ", i);
                }
            }
            Serial.println();

            uint16_t changed = currentState ^ lastState;
            for (int i = 0; i < 16; i++) {
                if (changed & (1 << i)) {
                    bool nowLow = (currentState >> i) & 1;
                    Serial.printf("MUDANCA: P%d foi para %s\n", i, nowLow ? "LOW" : "HIGH");
                }
            }
            Serial.println(F("------------------------"));
            lastState = currentState;
        }
    }

    void printStatus() {
        Serial.println(F("\n--- Status Atual ---"));

        Serial.print(F("Botoes: "));
        for (int i = 0; i < 4; i++) {
            int pin = BUTTON_LEFT_PIN + i;
            bool pressed = switches.isSwitchPressed(pin);
            Serial.printf("%s=%s ", BTN_NAMES[i], pressed ? "PRESSED" : "open");
        }
        Serial.println();

        Serial.print(F("Fios: "));
        int wirePins[] = {WIRE1_PIN, WIRE2_PIN, WIRE3_PIN, WIRE4_PIN};
        for (int i = 0; i < 4; i++) {
            bool cut = switches.isSwitchPressed(wirePins[i]);
            Serial.printf("%s=%s ", WIRE_NAMES[i], cut ? "CUT" : "conn");
        }
        Serial.println();

        Serial.print(F("Stats BTN: "));
        for (int i = 0; i < 4; i++) {
            Serial.printf("%s=%lu ", BTN_NAMES[i], btnPressCount[i]);
        }
        Serial.println();

        Serial.print(F("Stats WIRE: "));
        for (int i = 0; i < 4; i++) {
            Serial.printf("%s=%lu ", WIRE_NAMES[i], wireCutCount[i]);
        }
        Serial.println();
        Serial.println(F("--------------------"));
    }
};

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(99);
ButtonWireTest puzzle;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    hw.begin();
    hw.addPcf8575(PCF_ADDRESS);

    master.addPuzzle(&puzzle);
    master.begin();
      initHA();
  puzzle.run();
}

void loop() {
  loopHA();
    master.update();
}
