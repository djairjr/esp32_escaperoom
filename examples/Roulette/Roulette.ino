/**
 * EXEMPLO: ROULETTE - ROLETA DE SORTE
 * ------------------------------------
 * Roleta luminosa com NeoPixel. Jogador pressiona SELECT para girar.
 * A luz percorre o anel e desacelera ate parar em uma posicao.
 * Se parar no setor vencedor, o puzzle resolve. Se nao, tenta de novo.
 *
 * Port do RP2040 PropMaker para ESP32-S3 usando ESP32EscapeRoom
 *
 * Hardware:
 *   - NeoPixel externo para o anel da roleta
 *   - Botao SELECT (D3 / GPIO4) para girar
 *   - Audio MP3 via I2S (LittleFS)
 *   - Rele/Trava no D0 (GPIO1)
 *
 * Configuracao via /roulette.json no LittleFS.
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
struct RouletteConfig {
  uint8_t lockPin = D0;          // GPIO1 - Relé da trava
  uint8_t neoCount = 24;
  uint8_t winSectorCount = 1;
  float spinSpeed = 8.0f;
  float friction = 0.97f;
  float minSpeed = 0.1f;
  uint8_t volume = 128;
  char sfxSpin[24]   = "/sfx/spin.mp3";
  char sfxWin[24]    = "/sfx/win.mp3";
  char sfxLose[24]   = "/sfx/lose.mp3";
  char sfxSolved[24] = "/sfx/solved.mp3";
} config;

static void loadConfig() {
  if (!LittleFS.exists("/roulette.json")) return;
  File f = LittleFS.open("/roulette.json", "r");
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return;

  config.lockPin    = doc["lockPin"]    | config.lockPin;
  config.neoCount   = doc["neoCount"]   | config.neoCount;
  config.winSectorCount = doc["winSectorCount"] | config.winSectorCount;
  config.spinSpeed  = doc["spinSpeed"]  | config.spinSpeed;
  config.friction   = doc["friction"]   | config.friction;
  config.minSpeed   = doc["minSpeed"]   | config.minSpeed;
  config.volume     = doc["volume"]     | config.volume;

  strncpy(config.sfxSpin,   doc["audio"]["spin"]   | config.sfxSpin,   sizeof(config.sfxSpin));
  strncpy(config.sfxWin,    doc["audio"]["win"]    | config.sfxWin,    sizeof(config.sfxWin));
  strncpy(config.sfxLose,   doc["audio"]["lose"]   | config.sfxLose,   sizeof(config.sfxLose));
  strncpy(config.sfxSolved, doc["audio"]["solved"] | config.sfxSolved, sizeof(config.sfxSolved));
}

// ============================================================================
// PINOS
// ============================================================================
#define BTN_SELECT D3   // GPIO4

// ============================================================================
// FORWARD
// ============================================================================
class RoulettePuzzle;
static RoulettePuzzle* rouletteInstance = nullptr;
static void onButtonPress(pinid_t pin, bool held);

// ============================================================================
// PUZZLE
// ============================================================================
class RoulettePuzzle : public PuzzleBase {
public:
  RoulettePuzzle() : PuzzleBase(16) { rouletteInstance = this; }

  void setupHardware() override {
    loadConfig();

    hw.pinMode(config.lockPin, OUTPUT);
    hw.digitalWrite(config.lockPin, HIGH);

    if (config.neoCount > 0) setPixelRange(0, config.neoCount);

    // SwitchInput
    if (hw.hasExpander()) {
      switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);
    } else {
      switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);
    }
    switches.addSwitch(BTN_SELECT, onButtonPress, NO_REPEAT, false);

    randomSeed(esp_random());
    resetSpin();
    chooseWinSector();
    setState(PuzzleState::Idle);
    Serial.printf("[ROULETTE] SELECT para girar! Setor vencedor oculto\n");
  }

  void update() override {
    hw.sound().loop();
    taskManager.runLoop();
    switches.runLoop();

    switch (getState()) {
      case PuzzleState::Idle:    renderIdle(); break;
      case PuzzleState::Running: updateSpinning(); break;
      case PuzzleState::Solved:  renderSolved(); break;
      default: break;
    }
  }

  void handleSelect() {
    if (getState() == PuzzleState::Idle) {
      startSpin();
    }
  }

private:
  float pos_ = 0;
  float speed_ = 0;
  int winSector_ = 0;
  int spins_ = 0;
  unsigned long solvedAt_ = 0;
  static constexpr int SPINS_TO_WIN = 3;

  void chooseWinSector() {
    winSector_ = random(0, config.neoCount);
  }

  void startSpin() {
    pos_ = 0;
    speed_ = config.spinSpeed;
    spins_ = 0;
    hw.sound().playOnce(config.sfxSpin);
    setState(PuzzleState::Running);
    Serial.println(F("[ROULETTE] Girando..."));
  }

  void updateSpinning() {
    speed_ *= config.friction;

    pos_ += speed_;
    while (pos_ >= config.neoCount) {
      pos_ -= config.neoCount;
      spins_++;
    }

    renderSpin();

    if (speed_ < config.minSpeed) {
      stopSpin();
    }
  }

  void stopSpin() {
    int ipos = (int)pos_;
    bool won = false;

    if (ipos == winSector_) {
      won = true;
    }

    if (!won && config.winSectorCount > 1) {
      for (int i = 1; i < config.winSectorCount; i++) {
        int left = (winSector_ - i + config.neoCount) % config.neoCount;
        int right = (winSector_ + i) % config.neoCount;
        if (ipos == left || ipos == right) { won = true; break; }
      }
    }

    if (won) {
      Serial.printf("[ROULETTE] GANHOU! Parou em %d (vencedor=%d)\n", ipos, winSector_);
      if (spins_ >= SPINS_TO_WIN - 1) {
        solve();
      } else {
        hw.sound().playOnce(config.sfxWin);
        showResult(true);
        setState(PuzzleState::Idle);
      }
    } else {
      Serial.printf("[ROULETTE] Perdeu! Parou em %d (vencedor=%d)\n", ipos, winSector_);
      hw.sound().playOnce(config.sfxLose);
      showResult(false);
      setState(PuzzleState::Idle);
    }
  }

  void solve() {
    hw.digitalWrite(config.lockPin, LOW);
    hw.sound().playOnce(config.sfxSolved);
    solvedAt_ = millis();
    showResult(true);
    setState(PuzzleState::Solved);
    Serial.println(F("[ROULETTE] RESOLVIDO!"));
  }

  void showResult(bool win) {
    if (!pixels()) return;
    auto* px = pixels();
    uint32_t color = win ? px->Color(0, 50, 0) : px->Color(50, 0, 0);
    px->setAll(color);
    px->show();
  }

  void renderSpin() {
    if (!pixels()) return;
    auto* px = pixels();
    px->clear();
    int ipos = (int)pos_;

    px->setPixel(winSector_, px->Color(0, 20, 0));
    px->setPixel(ipos, px->Color(255, 255, 255));

    for (int i = 1; i <= 3; i++) {
      int idx = ((int)pos_ - i + config.neoCount * 2) % config.neoCount;
      uint8_t b = 80 - i * 20;
      px->setPixel(idx, px->Color(b, b, b));
    }

    px->show();
  }

  void renderIdle() {
    if (!pixels()) return;
    auto* px = pixels();
    uint8_t b = (sin(millis() / 600.0f) * 0.5f + 0.5f) * 20;
    px->clear();
    px->setPixel(winSector_, px->Color(0, b, 0));
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
      chooseWinSector();
      setState(PuzzleState::Idle);
    }
  }

  void resetSpin() {
    pos_ = 0; speed_ = 0;
  }
};

// ============================================================================
// CALLBACK
// ============================================================================
static void onButtonPress(pinid_t pin, bool held) {
  if (held || !rouletteInstance) return;
  if (pin == BTN_SELECT) rouletteInstance->handleSelect();
}

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(16);
RoulettePuzzle puzzle;

void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);

  hw.begin();

  master.addPuzzle(&puzzle);
    initHA();
  master.begin();

  Serial.println(F("[ROULETTE] Pronto!"));
}

void loop() {
  loopHA();
  master.update();
}
