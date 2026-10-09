/**
 * EXAMPLE: KEYPAD CODE MULTIPLE - MULTIPLE PASSWORDS VIA JSON
 * ----------------------------------------------------------
 * Numeric password entry via a 4x3 I2C keypad (PCF8574/8575).
 * Accepts multiple codes configured via /keypadmulti.json.
 * Any valid code solves the puzzle.
 *
 * Ported to ESP32-S3 + Home Assistant (ArduinoHA).
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>
#include <KeyboardManager.h>

using namespace esp32_escaperoom;

// ============================================================================
// JSON CONFIGURATION
// ============================================================================
#define MAX_CODES 8

struct KeypadMultiConfig {
  char codes[MAX_CODES][5] = {"1234", "5678", "", "", "", "", "", ""};
  uint8_t codeCount = 2;
  uint8_t lockPin = 20;
  char sfxKeypress[24] = "/sfx/beep.mp3";
  char sfxUnlock[24]  = "/sfx/magic.mp3";
  char sfxDenied[24]  = "/sfx/error.mp3";
  char sfxCancel[24]  = "/sfx/cancel.mp3";
  uint8_t neoCount = 8;
  uint32_t colorIdle   = 0x050514;
  uint32_t colorUnlock = 0x003200;
  uint32_t colorDenied = 0x320000;
} config;

static void loadConfig() {
  if (!LittleFS.exists("/keypadmulti.json")) {
    Serial.println(F("[CONFIG] /keypadmulti.json does not exist, using defaults"));
    return;
  }
  File f = LittleFS.open("/keypadmulti.json", "r");
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("[CONFIG] JSON error: %s\n", err.c_str());
    return;
  }

  if (doc["codes"].is<JsonArray>()) {
    auto arr = doc["codes"].as<JsonArray>();
    config.codeCount = min((uint8_t)arr.size(), (uint8_t)MAX_CODES);
    for (uint8_t i = 0; i < config.codeCount; i++) {
      strncpy(config.codes[i], arr[i] | "", sizeof(config.codes[i]) - 1);
      config.codes[i][sizeof(config.codes[i]) - 1] = '\0';
    }
  } else {
    config.codeCount = doc["codeCount"] | config.codeCount;
    if (config.codeCount > MAX_CODES) config.codeCount = MAX_CODES;
  }

  config.lockPin = doc["lockPin"] | config.lockPin;

  strncpy(config.sfxKeypress, doc["audio"]["keypress"] | config.sfxKeypress, sizeof(config.sfxKeypress));
  strncpy(config.sfxUnlock,   doc["audio"]["unlock"]   | config.sfxUnlock,   sizeof(config.sfxUnlock));
  strncpy(config.sfxDenied,   doc["audio"]["denied"]   | config.sfxDenied,   sizeof(config.sfxDenied));
  strncpy(config.sfxCancel,   doc["audio"]["cancel"]   | config.sfxCancel,   sizeof(config.sfxCancel));

  config.neoCount = doc["neopixel"]["count"] | config.neoCount;
  if (doc["neopixel"]["colorIdle"].is<JsonArray>()) {
    auto a = doc["neopixel"]["colorIdle"].as<JsonArray>();
    config.colorIdle = hw.pixelsExternal().Color(a[0] | 5, a[1] | 5, a[2] | 20);
  }
  if (doc["neopixel"]["colorUnlock"].is<JsonArray>()) {
    auto a = doc["neopixel"]["colorUnlock"].as<JsonArray>();
    config.colorUnlock = hw.pixelsExternal().Color(a[0] | 0, a[1] | 50, a[2] | 0);
  }
  if (doc["neopixel"]["colorDenied"].is<JsonArray>()) {
    auto a = doc["neopixel"]["colorDenied"].as<JsonArray>();
    config.colorDenied = hw.pixelsExternal().Color(a[0] | 50, a[1] | 0, a[2] | 0);
  }

  Serial.printf("[CONFIG] Codes=%d, lockPin=%d\n", config.codeCount, config.lockPin);
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
    else Serial.printf("[DISPLAY] Number: %d\n", num);
  }
  void printString(const char* str) {
    if (checkDisplay()) {
      hw.display().clear();
      hw.display().printString(str);
    } else Serial.printf("[DISPLAY] Text: %s\n", str);
  }
  void writeDigitAscii(int digit, char c) {
    if (checkDisplay()) hw.display().writeDigitAscii(digit, c);
    else Serial.printf("[DISPLAY] Digit %d: %c\n", digit, c);
  }
  void setBrightness(uint8_t b) {
    if (checkDisplay()) hw.display().setBrightness(b);
  }
};
DisplayWrapper display;

// ============================================================================
// PUZZLE
// ============================================================================
String enteredCode = "";

class KeypadCodeMultiple;
extern KeypadCodeMultiple puzzle;

class KeypadCodeMultiple : public PuzzleBase {
public:
  KeypadCodeMultiple() : PuzzleBase(14) {}

  void setupHardware() override {
    loadConfig();
    hw.pinMode(config.lockPin, OUTPUT);
    hw.digitalWrite(config.lockPin, HIGH);
    display.setBrightness(15);
    display.printString("____");
    setVisualIdle();
    Serial.printf("[KEYPAD] Waiting for password (%d valid codes)...\n", config.codeCount);
  }

  void setupHA() {
    haSensorState_.setName("State");      haSensorState_.setIcon("mdi:lock");
    haSensorAttempts_.setName("Attempts"); haSensorAttempts_.setIcon("mdi:counter");
    haBtnReset_.setName("Reset");        haBtnReset_.setIcon("mdi:restart");
    haBtnReset_.onCommand(haResetCallback);
    haBtnUnlock_.setName("Unlock");      haBtnUnlock_.setIcon("mdi:lock-open");
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
    if (key != 0) { keypadListener.lastKey = 0; handleKey(key); }
  }

  static void haResetCallback(HAButton*) {
    hw.digitalWrite(config.lockPin, HIGH);
    enteredCode = "";
    puzzle.setState(PuzzleState::Running);
    puzzle.attempts_ = 0;
    hw.setLightAll(0); hw.setLight(0, config.colorIdle);
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
    Serial.printf("[KEYPAD] Key: %c\n", key);
    if (key == '*') {
      enteredCode = "";
      hw.sound().playOnce(config.sfxCancel);
      display.printString("____");
      setVisualIdle();
    } else if (key == '#') {
      checkCode();
    } else {
      enteredCode += key;
      hw.sound().playOnce(config.sfxKeypress);
      display.writeDigitAscii(enteredCode.length() - 1, key);
      setVisualProgress(enteredCode.length());
      if (enteredCode.length() >= 4) checkCode();
    }
  }

  void checkCode() {
    for (uint8_t i = 0; i < config.codeCount; i++) {
      if (enteredCode == config.codes[i]) { handleSolve(); return; }
    }
    handleWrong();
  }

  void handleSolve() {
    Serial.println(F("[KEYPAD] CORRECT!"));
    hw.digitalWrite(config.lockPin, LOW);
    hw.sound().playOnce(config.sfxUnlock);
    display.printString(enteredCode.c_str());
    hw.setLightAll(config.colorUnlock);
    setState(PuzzleState::Solved);
    publishAllHA();
  }

  void handleWrong() {
    attempts_++;
    Serial.println(F("[KEYPAD] WRONG PASSWORD."));
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
    hw.setLightAll(0); hw.setLight(0, config.colorIdle);
  }
  void setVisualProgress(uint8_t step) {
    uint32_t c = hw.pixelsExternal().Color(0, 0, 50);
    for (uint8_t i = 0; i < step && i < config.neoCount; i++) hw.setLight(i, c);
  }

  HASensor haSensorState_{"state"};
  HASensor haSensorAttempts_{"attempts"};
  HAButton haBtnReset_{"reset"};
  HAButton haBtnUnlock_{"unlock"};
  int attempts_ = 0;
};

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================
LogicManager master(14);
KeypadCodeMultiple puzzle;
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);

void setup() {
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);
  Serial.println(F("========================================"));
  Serial.println(F("  KeypadCodeMultiple - ESP32-S3"));
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
  Serial.println(F("[KEYPAD] Ready!"));
}

void loop() {
  haMqtt.loop();
  netConfig.handleAP();
  master.update();
  taskManager.runLoop();
  delay(1);
}