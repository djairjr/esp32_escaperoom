/**
 * EXEMPLO: RADIOCLOCK - SINTONIZADOR DE RADIO
 * ---------------------------------------------
 * Jogador usa SELECT para percorrer frequencias no dial.
 * Audio toca estatica em frequencias erradas, sinal limpo na certa.
 * Display HT16K33 mostra a frequencia atual.
 * 3 acertos resolvem o puzzle.
 *
 * Hardware:
 *   - NeoPixel externo para o dial do radio
 *   - Botao SELECT (GPIO 4) para percorrer frequencias
 *   - Display HT16K33 para mostrar frequencia
 *   - Audio MP3 via I2S (LittleFS)
 *   - Rele/Trava no pino lockPin
 *
 * Configuracao via /radioClock.json no LittleFS.
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>
#include <TaskManager.h>
#include <SwitchInput.h>

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
// CONFIGURACAO JSON
// ============================================================================
struct RadioClockConfig {
  uint8_t lockPin = 10;
  uint8_t neoCount = 24;
  uint8_t frequencyCount = 12;      // quantas "frequencias" no dial
  uint8_t targetFrequencies[3] = {3, 8, 5}; // frequencias-alvo (3 rounds)
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
// PINOS
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
  int freq_ = 0;                // frequencia atual (0..frequencyCount-1)
  int round_ = 0;               // round atual
  unsigned long solvedAt_ = 0;
  unsigned long lastMove_ = 0;
  bool signalFound_ = false;    // true depois de tocar sinal no round atual

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
    Serial.printf("[RADIO] Round %d/%d. Frequencia alvo: %d\n",
                  round_ + 1, config.roundsToWin, config.targetFrequencies[round_]);
  }

  void nextFrequency() {
    freq_ = (freq_ + 1) % config.frequencyCount;
    lastMove_ = millis();
    signalFound_ = false;

    hw.sound().playOnce(config.sfxSelect);
    showFrequency();

    // Checa se acertou
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

    Serial.printf("[RADIO] Acertou! Round %d/%d\n", round_, config.roundsToWin);

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
    Serial.println("[RADIO] RESOLVIDO!");
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

    // Mapeia frequencia para posicao no anel
    int center = map(freq_, 0, config.frequencyCount - 1, 0, config.neoCount - 1);

    // Brilho branco no centro
    px->setPixel(center, 0x404040);

    // Trail sutil
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

    // Verde pulsante
    uint8_t g = (sin(millis() / 200.0f) * 0.5f + 0.5f) * 50 + 30;
    px->setAll(px->Color(0, g, 0));
    px->show();

    disp.print("GOOd");
  }

  void updateRunning() {
    // Retorna ao estado normal apos mostrar sinal
    if (signalFound_ && millis() - lastMove_ > 2000) {
      signalFound_ = false;

      if (getState() == PuzzleState::Running) {
        // Ainda jogando (nao resolveu), mostra nova frequencia alvo
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
