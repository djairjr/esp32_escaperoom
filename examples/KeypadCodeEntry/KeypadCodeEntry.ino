/**
 * EXEMPLO: KEYPAD CODE ENTRY - ENTRADA DE SENHA VIA JSON
 * ------------------------------------------------------
 * Entrada de senha numerica via teclado 4x3 I2C (PCF8574/8575).
 * Configuracao via /keypad.json no LittleFS.
 *
 * Portado para ESP32-S3 + Home Assistant (ArduinoHA).
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>
#include <KeyboardManager.h>

using namespace esp32_escaperoom;

// ============================================================================
// CONFIGURACAO JSON
// ============================================================================
struct KeypadConfig {
    char code[5] = "8671";         // Codigo de 4 digitos + null
    uint8_t lockPin = 20;
    char sfxKeypress[24] = "/sfx/beep.mp3";
    char sfxUnlock[24] = "/sfx/magic.mp3";
    char sfxDenied[24] = "/sfx/error.mp3";
    char sfxCancel[24] = "/sfx/cancel.mp3";
    uint8_t neoCount = 8;
    uint32_t colorIdle = 0x050514;     // Azul suave
    uint32_t colorUnlock = 0x003200;   // Verde
    uint32_t colorDenied = 0x320000;   // Vermelho
} config;

static void loadConfig() {
    if (!LittleFS.exists("/keypad.json")) {
        Serial.println(F("[CONFIG] /keypad.json nao existe, usando padrao"));
        return;
    }
    File f = LittleFS.open("/keypad.json", "r");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) {
        Serial.printf("[CONFIG] Erro parsing JSON: %s\n", err.c_str());
        return;
    }

    if (doc["code"].is<const char*>()) {
        strncpy(config.code, doc["code"], sizeof(config.code) - 1);
        config.code[sizeof(config.code) - 1] = '\0';
    }
    config.lockPin = doc["lockPin"] | config.lockPin;

    strncpy(config.sfxKeypress, doc["audio"]["keypress"] | config.sfxKeypress, sizeof(config.sfxKeypress));
    strncpy(config.sfxUnlock,   doc["audio"]["unlock"]   | config.sfxUnlock,   sizeof(config.sfxUnlock));
    strncpy(config.sfxDenied,   doc["audio"]["denied"]   | config.sfxDenied,   sizeof(config.sfxDenied));
    strncpy(config.sfxCancel,   doc["audio"]["cancel"]   | config.sfxCancel,   sizeof(config.sfxCancel));

    config.neoCount = doc["neopixel"]["count"] | config.neoCount;
    if (doc["neopixel"]["colorIdle"].is<JsonArray>()) {
        auto arr = doc["neopixel"]["colorIdle"].as<JsonArray>();
        config.colorIdle = hw.pixelsExternal().Color(arr[0] | 5, arr[1] | 5, arr[2] | 20);
    }
    if (doc["neopixel"]["colorUnlock"].is<JsonArray>()) {
        auto arr = doc["neopixel"]["colorUnlock"].as<JsonArray>();
        config.colorUnlock = hw.pixelsExternal().Color(arr[0] | 0, arr[1] | 50, arr[2] | 0);
    }
    if (doc["neopixel"]["colorDenied"].is<JsonArray>()) {
        auto arr = doc["neopixel"]["colorDenied"].as<JsonArray>();
        config.colorDenied = hw.pixelsExternal().Color(arr[0] | 50, arr[1] | 0, arr[2] | 0);
    }

    Serial.printf("[CONFIG] Code=%s, lockPin=%d\n", config.code, config.lockPin);
}

// ============================================================================
// HARDWARE
// ============================================================================
#define PCF_ADDRESS 0x20
#define PCF_IS_8575 true

static const char keymap[] PROGMEM = "123456789*0#";
byte rowPins[4] = {3, 2, 1, 0};
byte colPins[3] = {6, 5, 4};

KeyboardLayout keypadLayout(4, 3, keymap);

class KeypadListener : public KeyboardListener {
public:
    char lastKey = 0;
    void keyPressed(char key, bool held) override { if (!held) lastKey = key; }
    void keyReleased(char key) override { lastKey = 0; }
};
KeypadListener keypadListener;
MatrixKeyboardManager keypadManager;

// ============================================================================
// DISPLAY WRAPPER
// ============================================================================
class DisplayWrapper {
public:
  bool checkDisplay() { return hw.hasDisplay(); }

  void printNumber(int num) {
    if (checkDisplay()) hw.display().printNum(num);
    else Serial.printf("[DISPLAY] Numero: %d\n", num);
  }

  void printString(const char* str) {
    if (checkDisplay()) {
      hw.display().clear();
      hw.display().printString(str);
    } else Serial.printf("[DISPLAY] Texto: %s\n", str);
  }

  void writeDigitAscii(int digit, char c) {
    if (checkDisplay()) hw.display().writeDigitAscii(digit, c);
    else Serial.printf("[DISPLAY] Digito %d: %c\n", digit, c);
  }

  void setBrightness(uint8_t b) {
    if (checkDisplay()) hw.display().setBrightness(b);
    else Serial.printf("[DISPLAY] Brilho: %d\n", b);
  }
};
DisplayWrapper display;

// ============================================================================
// PUZZLE
// ============================================================================
String enteredCode = "";

class KeypadCodeEntry;
extern KeypadCodeEntry puzzle;

class KeypadCodeEntry : public PuzzleBase {
public:
    KeypadCodeEntry() : PuzzleBase(12) {}

    void setupHardware() override {
        loadConfig();

        hw.pinMode(config.lockPin, OUTPUT);
        hw.digitalWrite(config.lockPin, HIGH);

        display.setBrightness(15);
        display.printString("____");

        setVisualIdle();
        Serial.printf("[KEYPAD] Aguardando senha (code=%s)...\n", config.code);
    }

    void setupHA() {
        haSensorState_.setName("Estado");    haSensorState_.setIcon("mdi:lock");
        haSensorAttempts_.setName("Tentativas"); haSensorAttempts_.setIcon("mdi:counter");
        haBtnReset_.setName("Resetar");      haBtnReset_.setIcon("mdi:restart");
        haBtnReset_.onCommand(haResetCallback);
        haBtnUnlock_.setName("Destravar");    haBtnUnlock_.setIcon("mdi:lock-open");
        haBtnUnlock_.onCommand(haUnlockCallback);

        publishAllHA();
    }

    void publishAllHA() {
        const char* s = getState() == PuzzleState::Idle ? "idle" :
                        getState() == PuzzleState::Running ? "running" : "solved";
        haSensorState_.setValue(s);
        char buf[8]; snprintf(buf, sizeof(buf), "%d", attempts_);
        haSensorAttempts_.setValue(buf);
    }

    void update() override {
        if (getState() != PuzzleState::Running && getState() != PuzzleState::Idle) return;
        if (getState() == PuzzleState::Idle) setState(PuzzleState::Running);

        taskManager.runLoop();

        char key = keypadListener.lastKey;
        if (key != 0) {
            keypadListener.lastKey = 0;
            handleKey(key);
        }
    }

    // HA callbacks
    static void haResetCallback(HAButton*) {
        hw.digitalWrite(config.lockPin, HIGH);
        enteredCode = "";
        puzzle.setState(PuzzleState::Running);
        puzzle.attempts_ = 0;
        hw.setLightAll(0);
        hw.setLight(0, config.colorIdle);
        display.printString("____");
        puzzle.publishAllHA();
    }
    static void haUnlockCallback(HAButton*) {
        hw.digitalWrite(config.lockPin, LOW);
        hw.setLightAll(config.colorUnlock);
        puzzle.setState(PuzzleState::Solved);
        puzzle.publishAllHA();
    }

private:
    void handleKey(char key) {
        Serial.printf("[KEYPAD] Tecla: %c\n", key);

        if (key == '*') {
            enteredCode = "";
            hw.sound().playOnce(config.sfxCancel);
            display.printString("____");
            setVisualIdle();
        }
        else if (key == '#') {
            checkCode();
        }
        else {
            enteredCode += key;
            hw.sound().playOnce(config.sfxKeypress);
            display.writeDigitAscii(enteredCode.length() - 1, key);
            setVisualProgress(enteredCode.length());

            if (enteredCode.length() >= 4) {
                checkCode();
            }
        }
    }

    void checkCode() {
        if (enteredCode == config.code) {
            handleSolve();
        } else {
            handleWrong();
        }
    }

    void handleSolve() {
        Serial.println(F("[KEYPAD] ACERTOU!"));
        hw.digitalWrite(config.lockPin, LOW);
        hw.sound().playOnce(config.sfxUnlock);
        display.printString(config.code);
        hw.setLightAll(config.colorUnlock);
        setState(PuzzleState::Solved);
        publishAllHA();
    }

    void handleWrong() {
        attempts_++;
        Serial.println(F("[KEYPAD] SENHA INCORRETA."));
        hw.sound().playOnce(config.sfxDenied);
        enteredCode = "";
        display.printString("Err ");
        hw.setLightAll(config.colorDenied);
        delay(800);
        display.printString("____");
        setVisualIdle();
        publishAllHA();
    }

    void setVisualIdle() {
        hw.setLightAll(0);
        hw.setLight(0, config.colorIdle);
    }

    void setVisualProgress(uint8_t step) {
        uint32_t c = hw.pixelsExternal().Color(0, 0, 50);
        for (uint8_t i = 0; i < step && i < config.neoCount; i++) {
            hw.setLight(i, c);
        }
    }

    HASensor haSensorState_{"state"};
    HASensor haSensorAttempts_{"attempts"};
    HAButton haBtnReset_{"reset"};
    HAButton haBtnUnlock_{"unlock"};
    int attempts_ = 0;
};

// ============================================================================
// INSTANCIAS GLOBAIS
// ============================================================================
LogicManager master(12);
KeypadCodeEntry puzzle;
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);

void setup() {
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    Serial.println(F("========================================"));
    Serial.println(F("  KeypadCodeEntry - ESP32-S3"));
    Serial.println(F("========================================"));

    hw.begin();
    hw.addPcf8575(PCF_ADDRESS);

    netConfig.begin();
    if (netConfig.isAPMode()) Serial.println(F("[CFG] AP: http://192.168.4.1"));

    byte mac[6]; WiFi.macAddress(mac);
    haDevice.setUniqueId(mac, sizeof(mac));
    haDevice.setName(netConfig.devName());
    haDevice.setSoftwareVersion("1.0.0");

    puzzle.setupHA();
    haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());

    for (int i = 0; i < 4; i++) keypadLayout.setRowPin(i, rowPins[i]);
    for (int i = 0; i < 3; i++) keypadLayout.setColPin(i, colPins[i]);
    keypadManager.initialise(hw.portExpander(), &keypadLayout, &keypadListener, false);

    master.addPuzzle(&puzzle);
    master.begin();
    puzzle.run();
    Serial.println(F("[KEYPAD] Pronto!"));
}

void loop() {
    haMqtt.loop();
    netConfig.handleAP();
    master.update();
    taskManager.runLoop();
    delay(1);
}