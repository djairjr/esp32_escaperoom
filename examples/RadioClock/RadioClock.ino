/**
 * EXAMPLE: RADIOCLOCK - RADIO TUNER
 * ----------------------------------
 * The player uses SELECT to scroll through frequencies on the dial.
 * Audio plays static on wrong frequencies, clean signal on the right one.
 * HT16K33 display shows the current frequency.
 * 3 correct hits solve the puzzle.
 *
 * Hardware:
 *   - External NeoPixel for the radio dial
 *   - SELECT button (GPIO 4) to scroll through frequencies
 *   - HT16K33 display to show the frequency
 *   - MP3 audio via I2S (LittleFS)
 *   - Relay/Lock on the lockPin
 *
 * Configuration via /radioClock.json on LittleFS.
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>
#include <TaskManager.h>
#include <SwitchInput.h>

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
// JSON CONFIGURATION
// ============================================================================
struct RadioClockConfig {
  uint8_t lockPin = 10;
  uint8_t neoCount = 24;
  uint8_t frequencyCount = 12;      // how many "frequencies" on the dial
  uint8_t targetFrequencies[3] = {3, 8, 5}; // target frequencies (3 rounds)
  uint8_t roundsToWin = 3;
  uint8_t volume = 128;
  char sfxStatic[24]   = "/sfx/static.mp3";
  char sfxSignal[24]   = "/sfx/signal.mp3";
  char sfxSolved[24]   = "/sfx/solved.mp3";
  char sfxSelect[24]   = "/sfx/select.mp3";
} config;

static void loadConfig() {
  if (!LittleFS.exists("/radioClock.json")) return;
  File f = LittleFS.open("/radioClock.json", "r");
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return;

  config.lockPin    = doc["lockPin"]    | config.lockPin;
  config.neoCount   = doc["neoCount"]   | config.neoCount;
  config.frequencyCount = doc["frequencyCount"] | config.frequencyCount;
  config.roundsToWin = doc["roundsToWin"] | config.roundsToWin;
  config.volume     = doc["volume"]     | config.volume;

  if (doc["targetFrequencies"].is<JsonArray>()) {
    JsonArray arr = doc["targetFrequencies"].as<JsonArray>();
    uint8_t n = min((int)arr.size(), 3);
    for (uint8_t i = 0; i < n; i++)
      config.targetFrequencies[i] = arr[i] | config.targetFrequencies[i];
  }

  strncpy(config.sfxStatic, doc["audio"]["static"] | config.sfxStatic, sizeof(config.sfxStatic));
  strncpy(config.sfxSignal, doc["audio"]["signal"] | config.sfxSignal, sizeof(config.sfxSignal));
  strncpy(config.sfxSolved, doc["audio"]["solved"] | config.sfxSolved, sizeof(config.sfxSolved));
  strncpy(config.sfxSelect, doc["audio"]["select"] | config.sfxSelect, sizeof(config.sfxSelect));
}

// ============================================================================
// PINS
// ============================================================================
#define BTN_SELECT 4

// ============================================================================
// DISPLAY WRAPPER
// ============================================================================
class DispHelper {
public:
  bool check() { return hw.hasDisplay(); }
  void print(const char* s) { if (check()) { hw.display().clear(); hw.display().printString(s); } }
  void bright(uint8_t b) { if (check()) hw.display().setBrightness(b); }
};
DispHelper disp;

// ============================================================================
// FORWARD
// ============================================================================
class RadioClockPuzzle;
static RadioClockPuzzle* radioInstance = nullptr;
static void onButtonPress(uint8_t pin, bool held);

// ============================================================================
// PUZZLE
// ============================================================================
class RadioClockPuzzle : public PuzzleBase {
public:
  RadioClockPuzzle() : PuzzleBase(18) { radioInstance = this; }

  void setupHardware() override {
    loadConfig();

    hw.pinMode(config.lockPin, OUTPUT);
    hw.digitalWrite(config.lockPin, HIGH);

    if (config.neoCount > 0) setPixelRange(0, config.neoCount);

    switches.addSwitch(BTN_SELECT, onButtonPress, NO_REPEAT, false);

    disp.bright(15);
    disp.print("rAdi");

    randomSeed(analogRead(28));
    newGame();
    setState(PuzzleState::Idle);
  }

  void update() override {
    hw.sound().loop();
    taskManager.runLoop();
    switches.runLoop();

    switch (getState()) {
      case PuzzleState::Idle:    renderIdle(); break;
      case PuzzleState::Running: updateRunning(); break;
      case PuzzleState::Solved:  renderSolved(); break;
      default: break;
    }
  }

  void handleSelect() {
    if (getState() == PuzzleState::Idle) {
      startGame();
    } else if (getState() == PuzzleState::Running) {
      nextFrequency();
    }
  }

private:
  int freq_ = 0;                // current frequency (0..frequencyCount-1)
  int round_ = 0;               // current round
  unsigned long solvedAt_ = 0;
  unsigned long lastMove_ = 0;
  bool signalFound_ = false;    // true after playing the signal in the current round

  void newGame() {
    round_ = 0;
    freq_ = 0;
    signalFound_ = false;
  }

  void startGame() {
    round_ = 0;
    freq_ = 0;
    signalFound_ = false;
    setState(PuzzleState::Running);
    showFrequency();
    Serial.printf("[RADIO] Round %d/%d. Target frequency: %d\n",
                  round_ + 1, config.roundsToWin, config.targetFrequencies[round_]);
  }

  void nextFrequency() {
    freq_ = (freq_ + 1) % config.frequencyCount;
    lastMove_ = millis();
    signalFound_ = false;

    hw.sound().playOnce(config.sfxSelect);
    showFrequency();

    // Check if it was correct
    if (freq_ == config.targetFrequencies[round_]) {
      onTargetHit();
    } else {
      hw.sound().playOnce(config.sfxStatic);
    }
  }

  void onTargetHit() {
    round_++;
    signalFound_ = true;

    hw.sound().playOnce(config.sfxSignal);
    showSignal();

    Serial.printf("[RADIO] Correct! Round %d/%d\n", round_, config.roundsToWin);

    if (round_ >= config.roundsToWin) {
      solve();
    }
  }

  void solve() {
    hw.digitalWrite(config.lockPin, LOW);
    hw.sound().playOnce(config.sfxSolved);
    solvedAt_ = millis();
    disp.print("donE");
    setState(PuzzleState::Solved);
    Serial.println("[RADIO] SOLVED!");
  }

  void showFrequency() {
    // Display: "F--X"
    char buf[5];
    snprintf(buf, sizeof(buf), "F%2d", freq_);
    disp.print(buf);

    // NeoPixel
    if (!pixels()) return;
    auto* px = pixels();
    px->clear();

    // Map frequency to position on the ring
    int center = map(freq_, 0, config.frequencyCount - 1, 0, config.neoCount - 1);

    // White brightness at the center
    px->setPixel(center, 0x404040);

    // Subtle trail
    for (int i = 1; i <= 2; i++) {
      int left = (center - i + config.neoCount) % config.neoCount;
      int right = (center + i) % config.neoCount;
      uint8_t b = 20 - i * 8;
      px->setPixel(left,  px->Color(b, b, b));
      px->setPixel(right, px->Color(b, b, b));
    }

    px->show();
  }

  void showSignal() {
    if (!pixels()) return;
    auto* px = pixels();
    px->clear();

    // Pulsing green
    uint8_t g = (sin(millis() / 200.0f) * 0.5f + 0.5f) * 50 + 30;
    px->setAll(px->Color(0, g, 0));
    px->show();

    disp.print("GOOd");
  }

  void updateRunning() {
    // Return to the normal state after showing the signal
    if (signalFound_ && millis() - lastMove_ > 2000) {
      signalFound_ = false;

      if (getState() == PuzzleState::Running) {
        // Still playing (not solved), show the new target frequency
        showFrequency();
        disp.bright(15);
        char buf[5];
        snprintf(buf, sizeof(buf), "F%2d", freq_);
        disp.print(buf);
      }
    }
  }

  void renderIdle() {
    if (!pixels()) return;
    auto* px = pixels();
    uint8_t b = (sin(millis() / 800.0f) * 0.5f + 0.5f) * 10 + 5;
    px->clear();
    px->setAll(px->Color(0, 0, b));
    px->show();
  }

  void renderSolved() {
    if (!pixels()) return;
    auto* px = pixels();
    unsigned long t = millis() - solvedAt_;
    uint8_t hue = (t / 40) % 256;
    for (uint8_t i = 0; i < px->count(); i++)
      px->setPixel(i, px->strip()->ColorHSV(((uint16_t)(hue + i * 12) % 256) << 8, 255, 70));
    px->show();

    if (t > 8000) {
      hw.digitalWrite(config.lockPin, HIGH);
      disp.print("rAdi");
      newGame();
      setState(PuzzleState::Idle);
    }
  }
};

// ============================================================================
// CALLBACK
// ============================================================================
static void onButtonPress(uint8_t pin, bool held) {
  if (held || !radioInstance) return;
  if (pin == BTN_SELECT) radioInstance->handleSelect();
}

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(18);
RadioClockPuzzle puzzle;

void setup() {
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);
  hw.begin();
  master.addPuzzle(&puzzle);
  master.begin();
}

void loop() {
  loopHA();
  master.update();
}
