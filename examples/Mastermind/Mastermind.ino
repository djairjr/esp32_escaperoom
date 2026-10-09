/**
 * EXEMPLO: MASTERMIND - JOGO DE ADIVINHACAO DE CORES
 * ----------------------------------------------------
 * Jogador deve adivinhar uma sequencia de 4 cores (1-6).
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
struct MastermindConfig {
  uint8_t codeLength = 4;
  uint8_t maxColors = 6;
  uint8_t maxAttempts = 10;
  uint8_t lockPin = 20;
  uint8_t neoCount = 8;
  uint32_t colorIdle   = 0x050514;
  uint32_t colorUnlock = 0x003200;
  uint32_t colorDenied = 0x320000;
  char sfxKeypress[24] = "/sfx/beep.mp3";
  char sfxUnlock[24]   = "/sfx/magic.mp3";
  char sfxDenied[24]   = "/sfx/error.mp3";
  char sfxCancel[24]   = "/sfx/cancel.mp3";
} config;

static const uint32_t colorValues[6] = {
  0xFF0000, 0x0000FF, 0x00FF00, 0xFFFF00, 0xFF00FF, 0x00FFFF
};

static void loadConfig() {
  if (!LittleFS.exists("/mastermind.json")) return;
  File f = LittleFS.open("/mastermind.json", "r");
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return;
  config.codeLength  = doc["codeLength"]  | config.codeLength;
  config.maxColors   = doc["maxColors"]   | config.maxColors;
  config.maxAttempts = doc["maxAttempts"] | config.maxAttempts;
  config.lockPin     = doc["lockPin"]     | config.lockPin;
  config.neoCount    = doc["neoCount"]    | config.neoCount;
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
  strncpy(config.sfxKeypress, doc["audio"]["keypress"] | config.sfxKeypress, sizeof(config.sfxKeypress));
  strncpy(config.sfxUnlock,   doc["audio"]["unlock"]   | config.sfxUnlock,   sizeof(config.sfxUnlock));
  strncpy(config.sfxDenied,   doc["audio"]["denied"]   | config.sfxDenied,   sizeof(config.sfxDenied));
  strncpy(config.sfxCancel,   doc["audio"]["cancel"]   | config.sfxCancel,   sizeof(config.sfxCancel));
}

// ============================================================================
// HARDWARE - KEYPAD
// ============================================================================
#define PCF_ADDRESS 0x20
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
  bool check() { return hw.hasDisplay(); }
  void print(const char* s) { if (check()) { hw.display().clear(); hw.display().printString(s); } }
  void num(int v) { if (check()) hw.display().printNum(v); }
  void digit(int pos, char c) { if (check()) hw.display().writeDigitAscii(pos, c); }
  void bright(uint8_t b) { if (check()) hw.display().setBrightness(b); }
};
DisplayWrapper disp;

// ============================================================================
// PUZZLE
// ============================================================================
class MastermindPuzzle;
extern MastermindPuzzle puzzle;

class MastermindPuzzle : public PuzzleBase {
public:
  MastermindPuzzle() : PuzzleBase(17) {}

  void setupHardware() override {
    loadConfig();
    hw.pinMode(config.lockPin, OUTPUT);
    hw.digitalWrite(config.lockPin, HIGH);
    if (config.neoCount > 0) setPixelRange(0, config.neoCount);
    disp.bright(15);
    disp.print("CodE");
    randomSeed(analogRead(28));
    newGame();
    Serial.printf("[MM] Cores: 1=Verm 2=Azul 3=Verde 4=Amarelo 5=Magenta 6=Ciano\n");
  }

  void setupHA() {
    haSensorState_.setName("Estado");        haSensorState_.setIcon("mdi:gamepad");
    haSensorAttempts_.setName("Tentativas");  haSensorAttempts_.setIcon("mdi:counter");
    haBtnReset_.setName("Resetar");          haBtnReset_.setIcon("mdi:restart");
    haBtnReset_.onCommand(haResetCallback);
    publishAllHA();
  }

  void publishAllHA() {
    const char* s = getState() == PuzzleState::Idle ? "idle" :
                    getState() == PuzzleState::Running ? "running" : "solved";
    haSensorState_.setValue(s);
    char buf[8]; snprintf(buf, sizeof(buf), "%d/%d", attempt_, config.maxAttempts);
    haSensorAttempts_.setValue(buf);
  }

  void update() override {
    if (getState() != PuzzleState::Running) return;
    taskManager.runLoop();
    char key = keypadListener.lastKey;
    if (key != 0) { keypadListener.lastKey = 0; handleKey(key); }
  }

  static void haResetCallback(HAButton*) {
    hw.digitalWrite(config.lockPin, HIGH);
    hw.setLightAll(0);
    puzzle.newGame();
    disp.print("CodE");
    puzzle.setState(PuzzleState::Running);
    puzzle.publishAllHA();
  }

private:
  uint8_t secret_[8] = {0};
  uint8_t guess_[8] = {0};
  int guessLen_ = 0;
  int attempt_ = 0;
  bool gameOver_ = false;

  void newGame() {
    attempt_ = 0; guessLen_ = 0; gameOver_ = false;
    memset(guess_, 0, sizeof(guess_));
    for (uint8_t i = 0; i < config.codeLength; i++)
      secret_[i] = random(1, config.maxColors + 1);
    showGuess();
    publishAllHA();
  }

  void handleKey(char key) {
    if (gameOver_) return;
    if (key == '*') {
      guessLen_ = 0; memset(guess_, 0, sizeof(guess_));
      hw.sound().playOnce(config.sfxCancel);
      showGuess();
    } else if (key == '#') {
      submitGuess();
    } else if (key >= '1' && key <= '6') {
      if (guessLen_ < config.codeLength) {
        guess_[guessLen_++] = key - '0';
        hw.sound().playOnce(config.sfxKeypress);
        showGuess();
      }
    }
  }

  void submitGuess() {
    if (guessLen_ < config.codeLength) return;
    attempt_++;

    uint8_t exact = 0, color = 0;
    uint8_t secretCopy[8], guessCopy[8];
    memcpy(secretCopy, secret_, config.codeLength);
    memcpy(guessCopy, guess_, config.codeLength);

    for (uint8_t i = 0; i < config.codeLength; i++) {
      if (guessCopy[i] == secretCopy[i]) { exact++; secretCopy[i] = guessCopy[i] = 0; }
    }
    for (uint8_t i = 0; i < config.codeLength; i++) {
      if (guessCopy[i] == 0) continue;
      for (uint8_t j = 0; j < config.codeLength; j++) {
        if (secretCopy[j] == 0 || i == j) continue;
        if (guessCopy[i] == secretCopy[j]) { color++; secretCopy[j] = 0; break; }
      }
    }

    showFeedback(guess_, exact, color);
    char buf[5]; snprintf(buf, sizeof(buf), "P%dE%d", exact, color);
    disp.print(buf);

    if (exact == config.codeLength) {
      handleSolve(); return;
    }
    if (attempt_ >= config.maxAttempts) {
      gameOver_ = true;
      hw.sound().playOnce(config.sfxDenied);
      hw.setLightAll(config.colorDenied);
      disp.print("noPE");
      publishAllHA();
      return;
    }
    guessLen_ = 0; memset(guess_, 0, sizeof(guess_));
  }

  void handleSolve() {
    hw.digitalWrite(config.lockPin, LOW);
    hw.sound().playOnce(config.sfxUnlock);
    hw.setLightAll(config.colorUnlock);
    disp.print("donE");
    gameOver_ = true;
    setState(PuzzleState::Solved);
    publishAllHA();
  }

  void showGuess() {
    if (!pixels()) return;
    auto* px = pixels();
    px->clear();
    for (int i = 0; i < guessLen_; i++) {
      uint8_t c = guess_[i] - 1;
      if (c < 6) px->setPixel(i, colorValues[c]);
    }
    for (int i = guessLen_; i < config.codeLength; i++)
      px->setPixel(i, px->Color(0, 0, 15));
    px->show();
  }

  void showFeedback(uint8_t* g, uint8_t exact, uint8_t color) {
    if (!pixels()) return;
    auto* px = pixels();
    px->clear();
    for (int i = 0; i < config.codeLength; i++) {
      uint8_t c = g[i] - 1;
      if (c < 6) px->setPixel(i, colorValues[c]);
    }
    int offset = config.codeLength;
    for (int i = 0; i < exact && offset < config.neoCount; i++)
      px->setPixel(offset++, px->Color(0, 50, 0));
    for (int i = 0; i < color && offset < config.neoCount; i++)
      px->setPixel(offset++, px->Color(50, 50, 0));
    px->show();
  }

  HASensor haSensorState_{"state"};
  HASensor haSensorAttempts_{"attempts"};
  HAButton haBtnReset_{"reset"};
};

// ============================================================================
// INSTANCIAS GLOBAIS
// ============================================================================
LogicManager master(17);
MastermindPuzzle puzzle;
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);

void setup() {
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);
  Serial.println(F("=== Mastermind - ESP32-S3 ==="));
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
}

void loop() {
  haMqtt.loop();
  netConfig.handleAP();
  master.update();
  taskManager.runLoop();
  delay(1);
}