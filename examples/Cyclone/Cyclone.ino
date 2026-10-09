/**
 * EXEMPLO: CYCLONE - JOGO DE TIMING
 * -----------------------------------
 * Luz percorre um anel de NeoPixels em velocidade crescente.
 * Jogador pressiona SELECT para parar a luz no setor verde (alvo).
 * Se acertar, o puzzle resolve. Se errar, tenta de novo.
 *
 * Port do RP2040 PropMaker para ESP32-S3 usando ESP32EscapeRoom
 *
 * Hardware:
 *   - NeoPixel externo para o anel de luzes
 *   - Botao SELECT (GPIO 4 / D3) para parar
 *   - Botao DET (GPIO 10 / D10) para resetar apos solucao
 *   - Audio MP3 via I2S (LittleFS)
 *   - Rele/Trava no pino D0 (GPIO1)
 *
 * Configuracao via /cyclone.json no LittleFS.
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
// MAPEAMENTO D-PIN -> GPIO
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
// CONFIGURACAO JSON
// ============================================================================
struct CycloneConfig {
  uint8_t lockPin = D0;          // GPIO1 - Relé da trava
  uint8_t neoCount = 16;
  uint8_t targetSize = 3;
  float startSpeed = 0.05f;
  float speedIncrement = 0.02f;
  float maxSpeed = 0.4f;
  uint8_t roundsToWin = 3;
  uint8_t volume = 128;
  char sfxTick[24]   = "/sfx/tick.mp3";
  char sfxHit[24]    = "/sfx/hit.mp3";
  char sfxMiss[24]   = "/sfx/miss.mp3";
  char sfxSolved[24] = "/sfx/solved.mp3";
} config;

static void loadConfig() {
  if (!LittleFS.exists("/cyclone.json")) return;
  File f = LittleFS.open("/cyclone.json", "r");
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return;

  config.lockPin    = doc["lockPin"]    | config.lockPin;
  config.neoCount   = doc["neoCount"]   | config.neoCount;
  config.targetSize = doc["targetSize"] | config.targetSize;
  config.startSpeed = doc["startSpeed"] | config.startSpeed;
  config.speedIncrement = doc["speedIncrement"] | config.speedIncrement;
  config.maxSpeed   = doc["maxSpeed"]   | config.maxSpeed;
  config.roundsToWin = doc["roundsToWin"] | config.roundsToWin;
  config.volume     = doc["volume"]     | config.volume;

  strncpy(config.sfxTick,   doc["audio"]["tick"]   | config.sfxTick,   sizeof(config.sfxTick));
  strncpy(config.sfxHit,    doc["audio"]["hit"]    | config.sfxHit,    sizeof(config.sfxHit));
  strncpy(config.sfxMiss,   doc["audio"]["miss"]   | config.sfxMiss,   sizeof(config.sfxMiss));
  strncpy(config.sfxSolved, doc["audio"]["solved"] | config.sfxSolved, sizeof(config.sfxSolved));
}

// ============================================================================
// PINOS
// ============================================================================
#define BTN_SELECT D3   // GPIO4
#define BTN_DET    D10  // GPIO9

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
class CyclonePuzzle;
static CyclonePuzzle* cycloneInstance = nullptr;
static void onButtonPress(pinid_t pin, bool held);

// ============================================================================
// PUZZLE
// ============================================================================
class CyclonePuzzle : public PuzzleBase {
public:
  CyclonePuzzle() : PuzzleBase(15) { cycloneInstance = this; }

  void setupHardware() override {
    loadConfig();

    // Trava
    hw.pinMode(config.lockPin, OUTPUT);
    hw.digitalWrite(config.lockPin, HIGH);

    // Faixa de NeoPixel
    if (config.neoCount > 0) setPixelRange(0, config.neoCount);

    // SwitchInput via PCF (ou GPIO nativo se sem expander)
    if (hw.hasExpander()) {
      switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);
    } else {
      switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);
    }
    switches.addSwitch(BTN_SELECT, onButtonPress, NO_REPEAT, false);
    switches.addSwitch(BTN_DET, onButtonPress, NO_REPEAT, false);

    randomSeed(esp_random());
    resetGame();
    setState(PuzzleState::Idle);
    Serial.printf("[CYCLONE] SELECT para comecar (%d acertos para vencer)\n", config.roundsToWin);
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
      stopAttempt();
    }
  }

  void handleDet() {
    if (getState() == PuzzleState::Solved) {
      hw.digitalWrite(config.lockPin, HIGH);
      resetGame();
      setState(PuzzleState::Idle);
    }
  }

private:
  float pos_ = 0;
  int targetStart_ = 0;
  float speed_ = 0.05f;
  int round_ = 0;
  unsigned long solvedAt_ = 0;
  unsigned long lastStep_ = 0;

  void startGame() {
    round_ = 0;
    speed_ = config.startSpeed;
    nextRound();
    setState(PuzzleState::Running);
    Serial.println(F("[CYCLONE] INICIO! SELECT para parar"));
  }

  void nextRound() {
    pos_ = 0;
    targetStart_ = random(0, config.neoCount - config.targetSize);
    lastStep_ = 0;
    renderAll();
  }

  void updateRunning() {
    unsigned long now = millis();
    unsigned long stepDelay = (unsigned long)(100.0f / speed_);
    if (now - lastStep_ < stepDelay) return;
    lastStep_ = now;

    pos_ += 1;
    if (pos_ >= config.neoCount) pos_ = 0;
    renderAll();
  }

  void stopAttempt() {
    int ipos = (int)pos_;
    if (ipos >= targetStart_ && ipos < targetStart_ + config.targetSize) {
      round_++;
      Serial.printf("[CYCLONE] ACERTOU! Round %d/%d\n", round_, config.roundsToWin);
      hw.sound().playOnce(config.sfxHit);

      if (round_ >= config.roundsToWin) {
        solve();
      } else {
        speed_ = min(speed_ + config.speedIncrement, config.maxSpeed);
        nextRound();
      }
    } else {
      Serial.println(F("[CYCLONE] ERROU!"));
      hw.sound().playOnce(config.sfxMiss);
      nextRound();
    }
  }

  void solve() {
    hw.digitalWrite(config.lockPin, LOW);
    hw.sound().playOnce(config.sfxSolved);
    solvedAt_ = millis();
    setState(PuzzleState::Solved);
    Serial.println(F("[CYCLONE] RESOLVIDO!"));
  }

  void renderAll() {
    if (!pixels()) return;
    auto* px = pixels();
    px->clear();
    int ipos = (int)pos_;

    for (int i = 0; i < config.targetSize; i++)
      px->setPixel((targetStart_ + i) % config.neoCount, px->Color(0, 40, 0));

    px->setPixel(ipos, px->Color(255, 255, 255));
    px->show();
  }

  void renderIdle() {
    if (!pixels()) return;
    auto* px = pixels();
    uint8_t b = (sin(millis() / 500.0f) * 0.5f + 0.5f) * 30;
    px->setAll(px->Color(0, 0, b));
    px->show();
  }

  void renderSolved() {
    if (!pixels()) return;
    auto* px = pixels();
    unsigned long t = millis() - solvedAt_;
    uint8_t hue = (t / 50) % 256;
    for (uint8_t i = 0; i < px->count(); i++)
      px->setPixel(i, px->strip()->ColorHSV(((uint16_t)(hue + i * 10) % 256) << 8, 255, 60));
    px->show();

    if (t > 8000) {
      hw.digitalWrite(config.lockPin, HIGH);
      resetGame();
      setState(PuzzleState::Idle);
    }
  }

  void resetGame() {
    pos_ = 0; round_ = 0; speed_ = config.startSpeed;
    targetStart_ = 0; solvedAt_ = 0; lastStep_ = 0;
  }
};

// ============================================================================
// CALLBACK GLOBAL
// ============================================================================
static void onButtonPress(pinid_t pin, bool held) {
  if (held || !cycloneInstance) return;
  if (pin == BTN_SELECT) cycloneInstance->handleSelect();
  if (pin == BTN_DET)    cycloneInstance->handleDet();
}

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(15);
CyclonePuzzle puzzle;

void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);

  hw.begin();

  master.addPuzzle(&puzzle);
    initHA();
  master.begin();

  Serial.println(F("[CYCLONE] Pronto!"));
}

void loop() {
  loopHA();
  master.update();
}
