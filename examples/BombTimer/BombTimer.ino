/**
 * BombTimerPro v. 2.0 - Portado para ESP32-S3 (XIAO)
 * Por Djair Guilherme - (Nicolau dos Brinquedos)
 * do original Game Timer Pro, da nootropic design
 *
 * Hardware: Seeed XIAO ESP32-S3 + Expansion Board
 * - Engine ESP32EscapeRoom + ArduinoHA + ConfigManager
 * - Configuracao via LittleFS (JSON) - arquivo bomb.json
 * - Entradas via PCF8575 (I2C) usando IoAbstraction (Fios, Botoes e Keypad)
 * - Display HT16K33 14 segmentos e 4 Digitos
 * - Audio MP3 via Arduino-Audio-Tools (I2S) com Codec Helix MP3
 * - NeoPixel para efeitos visuais (BOM 48 Leds Neopixel)
 * - Leitor NFC/RFID PN532 (I2C) para desarme ou detonacao via tag
 * - Comunicacao com Home Assistant via MQTT (ArduinoHA)
 * - RS485 removido, Acelerometro removido, Servo removido
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

using namespace esp32_escaperoom;

// ============================================================================
// FORWARD DECLARATION + HA ENTITY GLOBALS
// ============================================================================
class GameTimerPuzzle;
extern GameTimerPuzzle puzzle;

// Sensors (8)
HASensor haSensorState("state");
HASensor haSensorTime("time_remaining");
HASensor haSensorWires("wires_cut");
HASensor haSensorMode("defuse_mode");
HASensor haSensorCodeAttempts("code_attempts");
HASensor haSensorBrightness("brightness");
HASensor haSensorVolume("volume");
HASensor haSensorTick("tick_status");

// Lock (1)
HALock haLockRelay("relay");

// Buttons (4)
HAButton haBtnStart("start");
HAButton haBtnReset("reset");
HAButton haBtnDetonate("detonate");
HAButton haBtnDefuse("defuse");

// Select (1)
HASelect haSelectMode("mode");

// Numbers (2)
HANumber haNumCountdown("countdown");
HANumber haNumDefuseCode("defuse_code");

// Switch (1)
HASwitch haSwitchTick("tick_sound");

// ============================================================================
// DEFINES
// ============================================================================

// Modos de desarme
#define DEFUSE_WIRE_CODE    0
#define DEFUSE_WIRE_ORDER   1
#define DEFUSE_WIRE_RAND    6
#define DET_WIRE_NONE       0
#define DET_WIRE_RAND       1
#define WIRE1               2
#define WIRE2               3
#define WIRE3               4
#define WIRE4               5

// RFID ON/OFF
#define RFID_OFF            0
#define RFID_ON             1

// Modos do acelerometro (REMOVIDO no ESP32, mantidos p/ compilacao)
#define ACCEL_OFF           0
#define ACCEL_START         1
#define ACCEL_SPEED2X       2
#define ACCEL_PAUSE         3
#define ACCEL_DETONATE      4

// Efeitos NeoPixel
#define NEOFX_OFF           0
#define NEOFX_NORMAL        1
#define NEOFX_COUNTING      2
#define NEOFX_DOUBLE_SPEED  3
#define NEOFX_DEFUSED       4
#define NEOFX_EXPLOSION     5
#define NEOFX_RADIATION     6

// Pinos no PCF8575 (IoAbstraction: 0-7=P0-P7, 8-15=P10-P17)
#define BUTTON_LEFT_PIN     8
#define BUTTON_SELECT_PIN   9
#define BUTTON_RIGHT_PIN    10
#define BUTTON_DET_PIN      11
#define WIRE1_PIN           12
#define WIRE2_PIN           13
#define WIRE3_PIN           14
#define WIRE4_PIN           15

// Configuracao JSON
#define CONFIG_FILE         "/bomb.json"

// ============================================================================
// WRAPPER ROBUSTO DO DISPLAY
// Se display nao detectado, redireciona para Serial.print
// ============================================================================
class DisplayWrapper {
public:
  bool checkDisplay() {
    return hw.hasDisplay();
  }

  void setDisplayColon(bool on) {
    if (checkDisplay()) {
      hw.display().setColon(on);
    }
  }

  void printNumber(int num) {
    if (checkDisplay()) {
      hw.display().printNum(num);
    } else {
      Serial.printf("[DISPLAY] Numero: %d\n", num);
    }
  }

  void printTime(int min, int sec) {
    if (checkDisplay()) {
      hw.display().printTime(min, sec);
    } else {
      Serial.printf("[DISPLAY] Tempo: %02d:%02d\n", min, sec);
    }
  }

  void printString(const char* str) {
    if (checkDisplay()) {
      hw.display().printString(str);
    } else {
      Serial.printf("[DISPLAY] Texto: %s\n", str);
    }
  }

  void writeDigitAscii(int digit, char c) {
    if (checkDisplay()) {
      hw.display().writeDigitAscii(digit, c);
    } else {
      Serial.printf("[DISPLAY] Digito %d: %c\n", digit, c);
    }
  }

  void clear() {
    if (checkDisplay()) {
      hw.display().clear();
    } else {
      Serial.println(F("[DISPLAY] Limpar"));
    }
  }

  void setBrightness(uint8_t b) {
    if (checkDisplay()) {
      hw.display().setBrightness(b);
    } else {
      Serial.printf("[DISPLAY] Brilho: %d (display nao detectado)\n", b);
    }
  }
};
DisplayWrapper display;

// Tag RFID em formato binário (UID armazenado como bytes)
struct RfidTagBin {
  uint8_t bytes[7];
  uint8_t len;
  bool valid;
};

// Configuração (estrutura igual ao original, removidos rs485/accel/servo)
struct Config {
  int countdownSeconds = 600;
  int defuseMode = DEFUSE_WIRE_CODE;
  int defuseCode = 1234;
  int defuseOrder = 1234;
  int detWire = DET_WIRE_RAND;
  int defuseWire = DEFUSE_WIRE_RAND;
  int speedupWire = 0;
  int pauseWire = 0;
  int brightness = 15;
  int volume = 128;
  int detTriggerSec = 5;
  int defuseTriggerSec = 5;
  int tickEnabled = 1;

  // RFID
  int rfidEnabled = RFID_OFF;
  RfidTagBin defuseTags[4] = {};
  RfidTagBin detonateTags[4] = {};
  int defuseTagCount = 0;
  int detonateTagCount = 0;
} config;

// Estado do timer
bool countdownRunning = false;
bool doubleSpeed = false;
bool pcfPresent = false;       // estado do PCF8575 para hot-plug
unsigned long lastPcfPoll = 0; // timestamp da última verificação
int countdownSeconds = 600;
bool isDefused = false;
bool updateDisplayFlag = false;

// Configuração dos fios (sorteados ou fixos)
int activeDetWire = 0;
int activeDefuseWire = 0;
int activeSpeedupWire = 0;
int activePauseWire = 0;

// Estado dos fios
bool wireCutState[4] = {false, false, false, false};
bool pauseDone = false;

// Modo ORDER
int wireOrderPos = 0;
int nextWire = 0;
int sequence[4];

// Estado do keypad
int digits[4];
int currentDigit = 0;
bool enteringCode = false;
int currentCode = 1; // Tentativa atual
int maxCode = 3; // Máximo de tentativas

// Timing
unsigned long lastTickAt = 0;
unsigned long lastSecondAt = 0;
unsigned long relayReleaseAt = 0;

// NeoPixel
int neoEffect = NEOFX_NORMAL;
unsigned long neoLastUpdate = 0;
int neoStep = 0;

// Efeitos
enum class EffectState { IDLE, RUNNING, FINISHED };
EffectState explosionEffect = EffectState::IDLE;
EffectState defuseEffect = EffectState::IDLE;
unsigned long effectStartTime = 0;

// Menu principal — 7 itens (SHAK e NET removidos)
int menuItem = 0;
bool inMenu = false;
bool inAdjustMode = false;
unsigned long lastBtnCheck = 0;
unsigned long selectPressStart = 0;
bool selectWasPressed = false;
#define SELECT_LONG_PRESS_MS 800
#define MENU_ITEMS_COUNT 7

// Submenu WIRE (5 itens: ORDR, DET, DEF, SPD, PAUS)
bool inWireSubmenu = false;
int wireSubItem = 0;
#define WIRE_SUBITEMS_COUNT 5

// Submenu RELE (2 itens: TDET, TDEF)
bool inReleSubmenu = false;
int releSubItem = 0;
#define RELE_SUBITEMS_COUNT 2

// Submenu RFID (2 itens: DEFU, DETO)
bool inRfidSubmenu = false;
int rfidSubItem = 0;
#define RFID_SUBITEMS_COUNT 2
unsigned long rfidFeedbackUntil = 0;
unsigned long lastRfidDisplayUpdate = 0;
char rfidFeedbackMsg[5] = "";

// Keypad via IoAbstraction KeyboardManager
static const char keymap[] PROGMEM = "123456789*0#";
static KeyboardLayout keypadLayout(4, 3, keymap);
static byte rowPins[4] = {3, 2, 1, 0};
static byte colPins[3] = {6, 5, 4};

class KeypadListener : public KeyboardListener {
public:
  char lastKey = 0;
  void keyPressed(char key, bool held) override { if (!held) lastKey = key; }
  void keyReleased(char key) override { lastKey = 0; }
};
static KeypadListener keypadListener;
static MatrixKeyboardManager keypadManager;

// Variaveis de estado dos botoes e fios
extern volatile bool btnPressed[4];
extern volatile bool wireCutFlag[4];

// Declaracoes forward
void onButtonPressed(pinid_t pin, bool held);
void onWireCut(pinid_t pin, bool held);

// Puzzle wrapper
class GameTimerPuzzle : public PuzzleBase {
public:
  GameTimerPuzzle() : PuzzleBase(1) {}

  void setupHardware() override {
    Serial.println(F("[GameTimer] Setup iniciado"));

    // Carrega configuracao
    loadConfig();
    Serial.printf("[GameTimer] Config: %ds, modo=%d, code=%04d\n",
                  config.countdownSeconds, config.defuseMode, config.defuseCode);

    // Aplica configuracoes
    display.setBrightness(config.brightness);
    hw.sound().setVolume(config.volume);

    // Inicializa SwitchInput para botoes e fios via PCF8575
    if (hw.hasExpander()) {
      Serial.println(F("[GameTimer] Inicializando SwitchInput via IoAbstraction..."));
      switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);

      switches.addSwitch(BUTTON_LEFT_PIN, onButtonPressed, NO_REPEAT, false);
      switches.addSwitch(BUTTON_SELECT_PIN, onButtonPressed, NO_REPEAT, false);
      switches.addSwitch(BUTTON_RIGHT_PIN, onButtonPressed, NO_REPEAT, false);
      switches.addSwitch(BUTTON_DET_PIN, onButtonPressed, NO_REPEAT, false);

      switches.addSwitch(WIRE1_PIN, onWireCut, NO_REPEAT, true);
      switches.addSwitch(WIRE2_PIN, onWireCut, NO_REPEAT, true);
      switches.addSwitch(WIRE3_PIN, onWireCut, NO_REPEAT, true);
      switches.addSwitch(WIRE4_PIN, onWireCut, NO_REPEAT, true);

      Serial.println(F("[GameTimer] SwitchInput OK: 4 botoes + 4 fios"));
    } else {
      Serial.println(F("[GameTimer] ERRO: PCF8575 nao detectado!"));
    }

    // Inicializa keypad
    for (int i = 0; i < 4; i++) keypadLayout.setRowPin(i, rowPins[i]);
    for (int i = 0; i < 3; i++) keypadLayout.setColPin(i, colPins[i]);
    keypadManager.initialise(hw.io(), &keypadLayout, &keypadListener, false);

    // Inicializa NFC Reader
    if (hw.hasNfc()) {
      nfc_ = hw.createNfcReader();
      if (nfc_) {
        Serial.println(F("[RFID] PN532 reader ready"));
      } else {
        Serial.println(F("[RFID] PN532 init failed"));
      }
    }
    if (!nfc_) {
      Serial.println(F("[RFID] No NFC hardware"));
    }

    // Configura saidas
    pinMode(PUZZLE_PIN_RELAY, OUTPUT);
    digitalWrite(PUZZLE_PIN_RELAY, HIGH);
    pinMode(PIN_EXTERNAL_POWER, OUTPUT);
    digitalWrite(PIN_EXTERNAL_POWER, HIGH);

    // Estado inicial
    countdownSeconds = config.countdownSeconds;
    if (hw.hasDisplay()) {
      display.printTime(countdownSeconds / 60, countdownSeconds % 60);
    }
    setNeoEffect(NEOFX_NORMAL);

    // Som de inicializacao
    hw.sound().playOnceBlocking("/armed.mp3");
    Serial.println(F("[GameTimer] Self Destruction Initiated"));

    setState(PuzzleState::Idle);
  }


  void update() override {
    hw.sound().loop();
    taskManager.runLoop();
    switches.runLoop();
    updateNeoEffect();

    // Keypad hot-plug: verifica PCF8575 a cada 2s
    if (millis() - lastPcfPoll > 2000) {
      lastPcfPoll = millis();
      bool nowPresent = hw.hasExpander();
      if (nowPresent && !pcfPresent) {
        pcfPresent = true;
        Serial.println(F("[KEYPAD] PCF8575 CONNECTED (hot-plug)"));
      } else if (!nowPresent && pcfPresent) {
        pcfPresent = false;
        Serial.println(F("[KEYPAD] PCF8575 DISCONNECTED"));
      }
    }

    switch (getState()) {
      case PuzzleState::Idle:
        if (!countdownRunning) getTimerInput();
        else startCountdown();
        break;
      case PuzzleState::Running:
        countdown();
        break;
      case PuzzleState::Solved:
        if (buttonPressed(BUTTON_DET_PIN)) {
          while (buttonPressed(BUTTON_DET_PIN)) delay(10);
          resetSystem();
        }
        break;
      case PuzzleState::Inactive:
        if (buttonPressed(BUTTON_DET_PIN)) {
          while (buttonPressed(BUTTON_DET_PIN)) delay(10);
          resetSystem();
        }
        break;
      default:
        break;
    }
  }

  // ========================================================================
  // HOME ASSISTANT SETUP
  // ========================================================================
  void setupHA() {
    // Sensors
    haSensorState.setName("Estado da Bomba");
    haSensorState.setIcon("mdi:bomb");
    haSensorTime.setName("Tempo Restante");
    haSensorTime.setUnitOfMeasurement("s");
    haSensorTime.setIcon("mdi:timer");
    haSensorWires.setName("Fios Cortados");
    haSensorWires.setIcon("mdi:cable-data");
    haSensorMode.setName("Modo de Desarme");
    haSensorMode.setIcon("mdi:shield-key");
    haSensorCodeAttempts.setName("Tentativas de Código");
    haSensorCodeAttempts.setIcon("mdi:counter");
    haSensorBrightness.setName("Brilho");
    haSensorBrightness.setUnitOfMeasurement("");

    haSensorVolume.setName("Volume");
    haSensorVolume.setUnitOfMeasurement("%");
    haSensorVolume.setIcon("mdi:volume-high");
    haSensorTick.setName("Som Tick");
    haSensorTick.setIcon("mdi:music-note");

    // Lock
    haLockRelay.setName("Relé de Explosão");
    haLockRelay.setIcon("mdi:lightning-bolt");
    haLockRelay.onCommand(onRelayCmd);

    // Buttons
    haBtnStart.setName("Iniciar Contagem");
    haBtnStart.setIcon("mdi:play");
    haBtnStart.onCommand(onStartBtn);
    haBtnReset.setName("Resetar");
    haBtnReset.setIcon("mdi:restart");
    haBtnReset.onCommand(onResetBtn);
    haBtnDetonate.setName("Explodir");
    haBtnDetonate.setIcon("mdi:alert-circle");
    haBtnDetonate.onCommand(onDetonateBtn);
    haBtnDefuse.setName("Desarmar");
    haBtnDefuse.setIcon("mdi:shield-check");
    haBtnDefuse.onCommand(onDefuseBtn);

    // Select
    haSelectMode.setName("Modo de Desarme");
    haSelectMode.setIcon("mdi:tune");
    haSelectMode.setOptions("CODE;WIRE;ORDER");
    haSelectMode.onCommand(onModeSelect);

    // Numbers
    haNumCountdown.setName("Tempo (segundos)");
    haNumCountdown.setIcon("mdi:timer-outline");
    haNumCountdown.setMin(10);
    haNumCountdown.setMax(5999);
    haNumCountdown.setStep(1);
    haNumCountdown.setUnitOfMeasurement("s");
    haNumCountdown.onCommand(onCountdownNum);
    haNumCountdown.setState((int32_t)config.countdownSeconds);

    haNumDefuseCode.setName("Código de Desarme");
    haNumDefuseCode.setIcon("mdi:lock");
    haNumDefuseCode.setMin(0);
    haNumDefuseCode.setMax(9999);
    haNumDefuseCode.setStep(1);
    haNumDefuseCode.onCommand(onDefuseCodeNum);
    haNumDefuseCode.setState((int32_t)config.defuseCode);

    // Switch
    haSwitchTick.setName("Som Tick");
    haSwitchTick.setIcon("mdi:volume-mute");
    haSwitchTick.onCommand(onTickSwitch);

    // Estado inicial
    publishAllHA();
  }

  void publishAllHA() {
    haSensorState.setValue(stateName(getState()));
    haSensorTime.setValue(String(countdownSeconds).c_str());
    updateWiresSensor();
    updateModeSensor();
    haSensorCodeAttempts.setValue(String(currentCode).c_str());
    haSensorBrightness.setValue(String(config.brightness).c_str());
    haSensorVolume.setValue(String(config.volume).c_str());
    haSensorTick.setValue(config.tickEnabled ? "ON" : "OFF");
  }

  void updateWiresSensor() {
    String w = "";
    for (int i = 0; i < 4; i++) {
      if (wireCutState[i] || (!countdownRunning && switches.isSwitchPressed(WIRE1_PIN + i))) {
        if (w.length() > 0) w += ",";
        w += String(i + 1);
      }
    }
    if (w.length() == 0) w = "none";
    haSensorWires.setValue(w.c_str());
  }

  void updateModeSensor() {
    const char* modes[] = {"CODE", "WIRE", "ORDER"};
    int m = config.defuseMode;
    if (m >= 0 && m <= 2) haSensorMode.setValue(modes[m]);
    else haSensorMode.setValue("UNKNOWN");
  }

  void haPublishRelay() {
    haLockRelay.setState(getState() == PuzzleState::Solved || getState() == PuzzleState::Inactive ?
      HALock::StateLocked : HALock::StateUnlocked);
  }

  // ========================================================================
  // CALLBACKS HA (estáticos)
  // ========================================================================
  static void onRelayCmd(HALock::LockCommand cmd, HALock*) {
    if (cmd == HALock::CommandLock) puzzle.detonate();
  }
  static void onStartBtn(HAButton*) { puzzle.checkDetButton(0); }
  static void onResetBtn(HAButton*) { puzzle.resetSystem(); puzzle.publishAllHA(); }
  static void onDetonateBtn(HAButton*) { extern int countdownSeconds; countdownSeconds = 0; puzzle.detonate(); }
  static void onDefuseBtn(HAButton*) { extern bool isDefused; isDefused = true; }
  static void onModeSelect(int8_t idx, HASelect*) {
    if (idx >= 0 && idx <= 2) { config.defuseMode = idx; puzzle.updateModeSensor(); }
  }
  static void onCountdownNum(HANumeric val, HANumber*) {
    int v = (int)val.toInt32();
    if (v >= 10 && v <= 5999) { config.countdownSeconds = v; countdownSeconds = v; }
    haSensorTime.setValue(String(v).c_str());
  }
  static void onDefuseCodeNum(HANumeric val, HANumber*) {
    int v = (int)val.toInt32();
    if (v >= 0 && v <= 9999) config.defuseCode = v;
  }
  static void onTickSwitch(bool state, HASwitch*) {
    config.tickEnabled = state ? 1 : 0;
    haSensorTick.setValue(state ? "ON" : "OFF");
  }

private:
  NfcReader* nfc_ = nullptr;
  unsigned long lastRfidPoll_ = 0;
  uint8_t lastRfidUid_[7] = {0};
  uint8_t lastRfidUidLen_ = 0;

  void getTimerInput() {
    static unsigned long lastAdjCheck = 0;
    static unsigned long selectEnterStart = 0;
    static bool selectEnterPressed = false;

    if (inMenu) { configure(); return; }

    if (buttonPressed(BUTTON_SELECT_PIN) && !selectEnterPressed) {
      selectEnterPressed = true;
      selectEnterStart = millis();
      return;
    }

    if (!switches.isSwitchPressed(BUTTON_SELECT_PIN) && selectEnterPressed) {
      selectEnterPressed = false;
      unsigned long pressDuration = millis() - selectEnterStart;

      if (pressDuration < SELECT_LONG_PRESS_MS) {
        Serial.println(F("[INPUT] SELECT CURTO -> entrando no menu"));
        inMenu = true; menuItem = 0; inAdjustMode = false;
        showMenuItem();
      } else {
        Serial.println(F("[INPUT] SELECT LONGO -> salvando configuracao"));
        saveConfig();
        display.printString("-OK-");
        hw.sound().playOnce("/keypress.mp3");
        Serial.printf("[INPUT] Tempo salvo: %d segundos\n", config.countdownSeconds);
        delay(800);
      }
      return;
    }

    if (millis() - lastBtnCheck < 150) return;

    if (buttonPressed(BUTTON_DET_PIN)) {
      lastBtnCheck = millis();
      Serial.println(F("[INPUT] DET -> iniciando countdown"));
      checkDetButton(0);
      return;
    }

    handleTimeAdjustment();
  }

  unsigned long leftPressStart = 0;
  unsigned long rightPressStart = 0;
  bool leftWasPressed = false;
  bool rightWasPressed = false;
  unsigned long lastAdjTime = 0;
  bool timeChanged = false;
  #define TIME_ADJUST_INTERVAL_MS 150
  #define TIME_LONG_PRESS_MS 600
  #define TIME_AUTO_SAVE_MS 2000

  void handleTimeAdjustment() {
    unsigned long now = millis();

    if (buttonPressed(BUTTON_LEFT_PIN) && !leftWasPressed) {
      leftWasPressed = true; leftPressStart = now;
      if (countdownSeconds > 10) {
        countdownSeconds--; config.countdownSeconds = countdownSeconds;
        updateDisplay(); Serial.printf("[TIME] LEFT: -1s = %d\n", countdownSeconds);
        timeChanged = true;
      } lastAdjTime = now;
    }

    if (buttonPressed(BUTTON_RIGHT_PIN) && !rightWasPressed) {
      rightWasPressed = true; rightPressStart = now;
      if (countdownSeconds < 5999) {
        countdownSeconds++; config.countdownSeconds = countdownSeconds;
        updateDisplay(); Serial.printf("[TIME] RIGHT: +1s = %d\n", countdownSeconds);
        timeChanged = true;
      } lastAdjTime = now;
    }

    if (leftWasPressed && switches.isSwitchPressed(BUTTON_LEFT_PIN)) {
      unsigned long heldTime = now - leftPressStart;
      if (heldTime >= TIME_LONG_PRESS_MS && now - lastAdjTime >= TIME_ADJUST_INTERVAL_MS) {
        if (countdownSeconds > 15) { countdownSeconds -= 5; }
        else if (countdownSeconds > 10) { countdownSeconds = 10; }
        config.countdownSeconds = countdownSeconds; updateDisplay();
        timeChanged = true; lastAdjTime = now;
      }
    }

    if (rightWasPressed && switches.isSwitchPressed(BUTTON_RIGHT_PIN)) {
      unsigned long heldTime = now - rightPressStart;
      if (heldTime >= TIME_LONG_PRESS_MS && now - lastAdjTime >= TIME_ADJUST_INTERVAL_MS) {
        if (countdownSeconds < 5994) { countdownSeconds += 5; }
        else if (countdownSeconds < 5999) { countdownSeconds = 5999; }
        config.countdownSeconds = countdownSeconds; updateDisplay();
        timeChanged = true; lastAdjTime = now;
      }
    }

    if (!switches.isSwitchPressed(BUTTON_LEFT_PIN) && leftWasPressed) leftWasPressed = false;
    if (!switches.isSwitchPressed(BUTTON_RIGHT_PIN) && rightWasPressed) rightWasPressed = false;

    if (timeChanged && (now - lastAdjTime >= TIME_AUTO_SAVE_MS)) {
      saveConfig(); timeChanged = false;
      Serial.printf("[TIME] Auto-salvo: %d segundos\n", config.countdownSeconds);
    }
  }

  void checkDetButton(int trigger) {
    if (trigger == 0x10 || countdownRunning) return;
    Serial.println(F("[GameTimer] Iniciando countdown"));
    countdownRunning = true;

    for (int i = 0; i < 3; i++) {
      hw.setLightAll(neoColor(255, 0, 0)); delay(100);
      hw.setLightAll(0); delay(100);
    }
    setState(PuzzleState::Running);
  }

  void countdown() {
    if (!countdownRunning) return;

    unsigned long interval = doubleSpeed ? 500 : 1000;
    if (millis() - lastSecondAt >= interval) {
      lastSecondAt = millis();
      if (countdownSeconds > 0) {
        countdownSeconds--;
        if (config.tickEnabled) tick();
        updateDisplayFlag = true;
        if (doubleSpeed) setNeoEffect(NEOFX_DOUBLE_SPEED);
        else setNeoEffect(NEOFX_COUNTING);
      }
    }

    if (updateDisplayFlag && !enteringCode) { updateDisplay(); updateDisplayFlag = false; }
    // HA: publish time every second
    if (millis() - lastSecondAt < 200) { haSensorTime.setValue(String(countdownSeconds).c_str()); }

    if (config.defuseMode != DEFUSE_WIRE_ORDER) checkWiresNormal();
    else checkWiresOrder();

    if (config.defuseMode == DEFUSE_WIRE_CODE) checkKeypadCode();

    checkRfidDuringCountdown();

    if (countdownSeconds == 0) { countdownRunning = false; detonate(); }
    else if (isDefused) { countdownRunning = false; defused(); }
  }

  void checkWiresNormal() {
    static bool lastDetWireState = false;

    if (activeDetWire >= WIRE1) {
      bool cut = wireCut(activeDetWire);
      if (cut && !lastDetWireState) { countdownSeconds = 0; }
      lastDetWireState = cut; if (cut) return;
    }

    if (config.defuseMode == 1 && activeDefuseWire >= WIRE1) {
      if (wireCut(activeDefuseWire)) { isDefused = true; return; }
    }

    if (activePauseWire >= WIRE1 && !pauseDone) {
      if (wireCut(activePauseWire)) pauseCountdown();
    }

    if (activeSpeedupWire >= WIRE1) {
      if (wireCut(activeSpeedupWire)) doubleSpeed = true;
    }

    bool allCut = true;
    for (int i = 0; i < 4; i++) { if (!wireCut(WIRE1 + i)) { allCut = false; break; } }
    if (allCut) countdownSeconds = 0;
  }

  void pauseCountdown() {
    pauseDone = true;
    hw.sound().playOnce("/keypress.mp3");
    setNeoEffect(NEOFX_OFF); delay(2000);
    setNeoEffect(NEOFX_COUNTING);
  }

  void checkWiresOrder() {
    for (int w = 0; w < 4; w++) {
      if (countdownSeconds == 0) break;
      if (!wireCutState[w] && wireCut(WIRE1 + w)) {
        wireCutState[w] = true;
        if ((w + 1) != nextWire) { countdownSeconds = 0; break; }
        else {
          wireOrderPos++;
          if (wireOrderPos >= 4) { isDefused = true; break; }
          nextWire = sequence[wireOrderPos];
        }
      }
    }
  }

  void checkKeypadCode() {
    char key = keypadListener.lastKey;
    if (key == 0) return;
    keypadListener.lastKey = 0;

    if (!enteringCode && key >= '0' && key <= '9') {
      enteringCode = true; currentDigit = 0;
      memset(digits, 0, sizeof(digits));
      display.clear(); display.setDisplayColon(false);
      display.printString("____");
    }

    if (enteringCode && key >= '0' && key <= '9' && currentDigit < 4) {
      digits[currentDigit] = key - '0';
      display.writeDigitAscii(currentDigit, key);
      hw.sound().playOnce("/keypress.mp3");
      currentDigit++;

      if (currentDigit >= 4) {
        int entered = digits[0]*1000 + digits[1]*100 + digits[2]*10 + digits[3];
        hw.sound().playOnce("/keypress.mp3");
        delay(800);
        enteringCode = false;
        if (entered == config.defuseCode) { isDefused = true; }
        else {
          currentCode++;
          haSensorCodeAttempts.setValue(String(currentCode).c_str());
          if (currentCode <= maxCode) {
            display.clear(); display.printString("_Err");
            enteringCode = true; currentDigit = 0;
          } else { countdownSeconds = 0; }
        }
      }
    }

    if ((key == '*' || key == '#') && enteringCode) {
      enteringCode = false; currentDigit = 0;
      updateDisplayFlag = true;
      hw.sound().playOnce("/keypress.mp3");
    }
  }

  int wireToPin(int wire) {
    switch(wire) {
      case WIRE1: return WIRE1_PIN; case WIRE2: return WIRE2_PIN;
      case WIRE3: return WIRE3_PIN; case WIRE4: return WIRE4_PIN;
      default: return -1;
    }
  }

  bool wireCut(int wire) {
    if (wire < WIRE1 || wire > WIRE4) return false;
    return switches.isSwitchPressed(wireToPin(wire));
  }

  void detonate() {
    Serial.println(F("\n[detonate] ========== BOOM! =========="));
    setState(PuzzleState::Inactive);
    haSensorState.setValue("inactive");
    haPublishRelay();

    setNeoEffect(NEOFX_EXPLOSION);
    display.printString("boom");
    hw.sound().playOnce("/boom.mp3");

    digitalWrite(PUZZLE_PIN_RELAY, LOW);
    effectStartTime = millis();
    relayReleaseAt = effectStartTime + (config.detTriggerSec * 1000UL);
    explosionEffect = EffectState::RUNNING;

    while (millis() < relayReleaseAt) {
      hw.sound().loop(); switches.runLoop(); updateNeoEffect();
      if (buttonPressed(BUTTON_DET_PIN)) {
        while (buttonPressed(BUTTON_DET_PIN)) { taskManager.runLoop(); switches.runLoop(); delay(10); }
        break;
      }
      delay(10);
    }

    digitalWrite(PUZZLE_PIN_RELAY, HIGH);
    explosionEffect = EffectState::FINISHED;
  }

  void defused() {
    Serial.println(F("\n[defused] ========== BOMBA DESARMADA =========="));
    setState(PuzzleState::Solved);
    haSensorState.setValue("solved");
    haSensorTime.setValue("0");
    haPublishRelay();

    setNeoEffect(NEOFX_DEFUSED);
    display.printString("SAFE");
    hw.sound().playOnce("/sucess.mp3");

    digitalWrite(PUZZLE_PIN_RELAY, LOW);
    effectStartTime = millis();
    relayReleaseAt = effectStartTime + (config.defuseTriggerSec * 1000UL);
    defuseEffect = EffectState::RUNNING;

    while (millis() < relayReleaseAt) {
      hw.sound().loop(); switches.runLoop(); updateNeoEffect();
      if (buttonPressed(BUTTON_DET_PIN)) {
        while (buttonPressed(BUTTON_DET_PIN)) { taskManager.runLoop(); switches.runLoop(); delay(10); }
        break;
      }
      delay(10);
    }

    digitalWrite(PUZZLE_PIN_RELAY, HIGH);
    defuseEffect = EffectState::FINISHED;
  }

  void resetSystem() {
    Serial.println(F("[GameTimer] Reset"));
    countdownRunning = false; doubleSpeed = false; isDefused = false;
    enteringCode = false; currentCode = 1;
    inMenu = false; inAdjustMode = false;
    inWireSubmenu = false; inReleSubmenu = false; inRfidSubmenu = false;
    wireSubItem = 0; releSubItem = 0; rfidSubItem = 0;
    selectWasPressed = false; selectPressStart = 0;
    memset(wireCutState, 0, sizeof(wireCutState));
    lastRfidUidLen_ = 0;
    countdownSeconds = config.countdownSeconds;
    setNeoEffect(NEOFX_NORMAL);
    display.setDisplayColon(false);
    updateDisplay();
    setState(PuzzleState::Idle);
    haSensorState.setValue("idle");
    haSensorTime.setValue(String(countdownSeconds).c_str());
    haSensorCodeAttempts.setValue("1");
    updateWiresSensor();
    haPublishRelay();
  }

  void startCountdown() {
    Serial.printf("[startCountdown] Tempo: %ds, modo: %d\n", config.countdownSeconds, config.defuseMode);
    getRandomWire();
    lastRfidPoll_ = 0; lastRfidUidLen_ = 0;
    countdownSeconds = config.countdownSeconds;
    isDefused = false; doubleSpeed = false; pauseDone = false;
    enteringCode = false; currentDigit = 0; currentCode = 1;
    memset(wireCutState, 0, sizeof(wireCutState));

    if (config.defuseMode == DEFUSE_WIRE_ORDER) {
      sequence[0] = config.defuseOrder / 1000;
      sequence[1] = (config.defuseOrder % 1000) / 100;
      sequence[2] = (config.defuseOrder % 100) / 10;
      sequence[3] = config.defuseOrder % 10;
      wireOrderPos = 0; nextWire = sequence[0];
    }

    hw.sound().playOnceBlocking("/armed.mp3");
    setNeoEffect(NEOFX_COUNTING);
    display.setDisplayColon(true);
    lastSecondAt = millis(); updateDisplayFlag = true;
  }

  void getRandomWire() {
    activeDetWire = (config.detWire == DET_WIRE_RAND) ? random(WIRE1, WIRE4 + 1) : config.detWire;
    if (config.defuseMode == 1) {
      do { activeDefuseWire = (config.defuseWire == DEFUSE_WIRE_RAND) ? random(WIRE1, WIRE4 + 1) : config.defuseWire;
      } while (activeDefuseWire == activeDetWire);
    }
    if (config.speedupWire > 0) {
      do { activeSpeedupWire = (config.speedupWire == 1) ? random(WIRE1, WIRE4 + 1) : config.speedupWire;
      } while (activeSpeedupWire == activeDetWire || activeSpeedupWire == activeDefuseWire);
    }
    if (config.pauseWire > 0) {
      do { activePauseWire = (config.pauseWire == 1) ? random(WIRE1, WIRE4 + 1) : config.pauseWire;
      } while (activePauseWire == activeDetWire || activePauseWire == activeDefuseWire || activePauseWire == activeSpeedupWire);
    }
    Serial.printf("[WIRES] Det=%d Def=%d Spd=%d Paus=%d\n", activeDetWire, activeDefuseWire, activeSpeedupWire, activePauseWire);
  }

  bool wireConflict(int key, int value) {
    if (value < WIRE1 || value > WIRE4) return false;
    if (key != 2 && activeDetWire == value) return true;
    if (key != 3 && activeDefuseWire == value) return true;
    if (key != 4 && activeSpeedupWire == value) return true;
    if (key != 5 && activePauseWire == value) return true;
    return false;
  }

  void configure() {
    if (buttonPressed(BUTTON_SELECT_PIN) && !selectWasPressed) {
      selectWasPressed = true; selectPressStart = millis(); return;
    }

    if (!switches.isSwitchPressed(BUTTON_SELECT_PIN) && selectWasPressed) {
      selectWasPressed = false;
      unsigned long pressDuration = millis() - selectPressStart;
      if (pressDuration >= SELECT_LONG_PRESS_MS) { Serial.println(F("[CONFIG] SELECT LONGO")); handleSelectLong(); }
      else { handleSelectShort(); }
      lastBtnCheck = millis(); return;
    }

    if (millis() - lastBtnCheck < 150) return;

    if (buttonPressed(BUTTON_DET_PIN)) {
      lastBtnCheck = millis();
      Serial.println(F("[CONFIG] DET -> saindo e iniciando countdown"));
      exitAllMenus(); checkDetButton(0); return;
    }

    if (inAdjustMode) handleAdjustMode();
    else if (inWireSubmenu) handleWireSubmenuNavigation();
    else if (inReleSubmenu) handleReleSubmenuNavigation();
    else if (inRfidSubmenu) {
      handleRfidSubmenuNavigation();
      if (nfc_) pollRfidTag();
      if (millis() - lastRfidDisplayUpdate >= 200) {
        if (rfidFeedbackUntil != 0 && millis() < rfidFeedbackUntil && rfidFeedbackMsg[0]) {
          display.clear(); display.setDisplayColon(false); display.printString(rfidFeedbackMsg);
        } else { showRfidSubmenuItem(rfidSubItem); rfidFeedbackUntil = 0; rfidFeedbackMsg[0] = '\0'; }
        lastRfidDisplayUpdate = millis();
      }
    } else handleMainMenuNavigation();
  }

  void exitAllMenus() { inMenu = false; inAdjustMode = false; inWireSubmenu = false; inReleSubmenu = false; inRfidSubmenu = false; Serial.println(F("[CONFIG] Menu fechado")); }

  void handleSelectShort() {
    if (inAdjustMode) {
      if (menuItem == 6 && config.rfidEnabled == RFID_ON) {
        inAdjustMode = false; inRfidSubmenu = true; rfidSubItem = 0; showRfidSubmenuItem(rfidSubItem);
      } else {
        inAdjustMode = false;
        if (inWireSubmenu) showWireSubmenuItem(wireSubItem);
        else if (inReleSubmenu) showReleSubmenuItem(releSubItem);
        else if (inRfidSubmenu) showRfidSubmenuItem(rfidSubItem);
        else showMenuItem();
      }
    } else if (inWireSubmenu) {
      if (wireSubItem == 0) { inputWireOrder(); showWireSubmenuItem(wireSubItem); }
      else { inAdjustMode = true; showCurrentWireValue(wireSubItem); }
    } else if (inReleSubmenu) { inAdjustMode = true; showCurrentReleValue(releSubItem); }
    else if (inRfidSubmenu) { inRfidSubmenu = false; hw.sound().playOnce("/keypress.mp3"); showMenuItem(); }
    else {
      if (menuItem == 3) { inWireSubmenu = true; wireSubItem = 0; showWireSubmenuItem(wireSubItem); }
      else if (menuItem == 5) { inReleSubmenu = true; releSubItem = 0; showReleSubmenuItem(releSubItem); }
      else { inAdjustMode = true; showCurrentValue(); }
    }
  }

  void handleSelectLong() {
    if (inAdjustMode) {
      inAdjustMode = false; saveConfig(); hw.sound().playOnce("/keypress.mp3");
      if (inWireSubmenu) showWireSubmenuItem(wireSubItem);
      else if (inReleSubmenu) showReleSubmenuItem(releSubItem);
      else if (inRfidSubmenu) showRfidSubmenuItem(rfidSubItem);
      else showMenuItem();
    } else if (inWireSubmenu) { inWireSubmenu = false; saveConfig(); hw.sound().playOnce("/keypress.mp3"); showMenuItem(); }
    else if (inReleSubmenu) { inReleSubmenu = false; saveConfig(); hw.sound().playOnce("/keypress.mp3"); showMenuItem(); }
    else if (inRfidSubmenu) {
      int& tagCount = (rfidSubItem == 0) ? config.defuseTagCount : config.detonateTagCount;
      RfidTagBin* tags = (rfidSubItem == 0) ? config.defuseTags : config.detonateTags;
      memset(tags, 0, sizeof(RfidTagBin) * 4); tagCount = 0;
      display.clear(); display.setDisplayColon(false); display.printString("CLR ");
      hw.sound().playOnce("/keypress.mp3"); delay(500);
      inRfidSubmenu = false; saveConfig(); showMenuItem();
    } else { exitAllMenus(); saveConfig(); updateDisplay(); hw.sound().playOnce("/keypress.mp3"); }
  }

  void handleMainMenuNavigation() {
    if (buttonPressed(BUTTON_LEFT_PIN)) { lastBtnCheck = millis(); menuItem = (menuItem == 0) ? (MENU_ITEMS_COUNT - 1) : menuItem - 1; showMenuItem(); }
    if (buttonPressed(BUTTON_RIGHT_PIN)) { lastBtnCheck = millis(); menuItem = (menuItem >= MENU_ITEMS_COUNT - 1) ? 0 : menuItem + 1; showMenuItem(); }
  }

  void handleWireSubmenuNavigation() {
    if (buttonPressed(BUTTON_LEFT_PIN)) { lastBtnCheck = millis(); wireSubItem = (wireSubItem == 0) ? (WIRE_SUBITEMS_COUNT - 1) : wireSubItem - 1; showWireSubmenuItem(wireSubItem); }
    if (buttonPressed(BUTTON_RIGHT_PIN)) { lastBtnCheck = millis(); wireSubItem = (wireSubItem >= WIRE_SUBITEMS_COUNT - 1) ? 0 : wireSubItem + 1; showWireSubmenuItem(wireSubItem); }
  }

  void handleReleSubmenuNavigation() {
    if (buttonPressed(BUTTON_LEFT_PIN)) { lastBtnCheck = millis(); releSubItem = (releSubItem == 0) ? (RELE_SUBITEMS_COUNT - 1) : releSubItem - 1; showReleSubmenuItem(releSubItem); }
    if (buttonPressed(BUTTON_RIGHT_PIN)) { lastBtnCheck = millis(); releSubItem = (releSubItem >= RELE_SUBITEMS_COUNT - 1) ? 0 : releSubItem + 1; showReleSubmenuItem(releSubItem); }
  }

  void handleRfidSubmenuNavigation() {
    if (buttonPressed(BUTTON_LEFT_PIN)) { lastBtnCheck = millis(); rfidSubItem = (rfidSubItem == 0) ? (RFID_SUBITEMS_COUNT - 1) : rfidSubItem - 1; rfidFeedbackUntil = 0; rfidFeedbackMsg[0] = '\0'; showRfidSubmenuItem(rfidSubItem); }
    if (buttonPressed(BUTTON_RIGHT_PIN)) { lastBtnCheck = millis(); rfidSubItem = (rfidSubItem >= RFID_SUBITEMS_COUNT - 1) ? 0 : rfidSubItem + 1; rfidFeedbackUntil = 0; rfidFeedbackMsg[0] = '\0'; showRfidSubmenuItem(rfidSubItem); }
  }

  void handleAdjustMode() {
    if (inWireSubmenu) {
      if (buttonPressed(BUTTON_LEFT_PIN)) { lastBtnCheck = millis(); adjustWireSubmenuItem(-1); }
      if (buttonPressed(BUTTON_RIGHT_PIN)) { lastBtnCheck = millis(); adjustWireSubmenuItem(1); }
    } else if (inReleSubmenu) {
      if (buttonPressed(BUTTON_LEFT_PIN)) { lastBtnCheck = millis(); adjustReleSubmenuItem(-1); }
      if (buttonPressed(BUTTON_RIGHT_PIN)) { lastBtnCheck = millis(); adjustReleSubmenuItem(1); }
    } else {
      if (buttonPressed(BUTTON_LEFT_PIN)) { lastBtnCheck = millis(); adjustMenuItem(-1); }
      if (buttonPressed(BUTTON_RIGHT_PIN)) { lastBtnCheck = millis(); adjustMenuItem(1); }
    }
  }

  // Menu 7 itens: 0=BRIGHT, 1=VOL, 2=TICK, 3=WIRE, 4=CODE, 5=RELE, 6=RFID
  void showMenuItem() { display.clear(); display.setDisplayColon(false);
    switch (menuItem) {
      case 0: display.printString("BRIG"); break; case 1: display.printString("VOL "); break;
      case 2: display.printString("TICK"); break; case 3: display.printString("WIRE"); break;
      case 4: display.printString("CODE"); break; case 5: display.printString("RELE"); break;
      case 6: display.printString("RFID"); break; default: menuItem = 0; showMenuItem(); return;
    }
  }

  void showWireSubmenuItem(int subItem) { display.clear(); display.setDisplayColon(false);
    switch (subItem) {
      case 0: display.printString("ORDR"); break; case 1: display.printString("DET "); break;
      case 2: display.printString("DEF "); break; case 3: display.printString("SPD "); break;
      case 4: display.printString("PAUS"); break; default: subItem = 0; showWireSubmenuItem(subItem); return;
    }
  }

  void showReleSubmenuItem(int subItem) { display.clear(); display.setDisplayColon(false);
    switch (subItem) {
      case 0: display.printString("TDET"); break; case 1: display.printString("TDEF"); break;
      default: subItem = 0; showReleSubmenuItem(subItem); return;
    }
  }

  void adjustMenuItem(int direction) {
    switch (menuItem) {
      case 0:
        config.brightness = constrainValue(config.brightness + direction, 0, 15);
        Serial.printf("[ADJ] brightness=%d\n", config.brightness);
        display.setBrightness(config.brightness); showNum(config.brightness); break;
      case 1:
        config.volume = constrainValue(config.volume + (direction * 16), 0, 255);
        Serial.printf("[ADJ] volume=%d\n", config.volume);
        hw.sound().setVolume(config.volume); showNum(config.volume / 16); break;
      case 2:
        config.tickEnabled = !config.tickEnabled; showBool(config.tickEnabled); break;
      case 3: break; // WIRE - submenu
      case 4: if (direction > 0) { inputCode(); inAdjustMode = false; showMenuItem(); } break;
      case 5: break; // RELE - submenu
      case 6: config.rfidEnabled = !config.rfidEnabled; showBool(config.rfidEnabled); break;
    }
  }

  void adjustWireSubmenuItem(int direction) {
    switch (wireSubItem) {
      case 0: break; // ORDR
      case 1: { int nv = constrainValue(config.detWire + direction, 0, 4);
        if (nv > 0 && wireConflict(1, nv)) { display.printString("err "); hw.sound().playOnce("/error.mp3"); delay(500); showWireValue(config.detWire); }
        else { config.detWire = nv; showWireValue(config.detWire); } } break;
      case 2: { int nv = constrainValue(config.defuseWire + direction, 0, 4);
        if (nv > 0 && wireConflict(2, nv)) { display.printString("err "); hw.sound().playOnce("/error.mp3"); delay(500); showWireValue(config.defuseWire); }
        else { config.defuseWire = nv; showWireValue(config.defuseWire); } } break;
      case 3: { int nv = constrainValue(config.speedupWire + direction, 0, 4);
        if (nv > 0 && wireConflict(3, nv)) { display.printString("err "); hw.sound().playOnce("/error.mp3"); delay(500); showWireValue(config.speedupWire); }
        else { config.speedupWire = nv; showWireValue(config.speedupWire); } } break;
      case 4: { int nv = constrainValue(config.pauseWire + direction, 0, 4);
        if (nv > 0 && wireConflict(4, nv)) { display.printString("err "); hw.sound().playOnce("/error.mp3"); delay(500); showWireValue(config.pauseWire); }
        else { config.pauseWire = nv; showWireValue(config.pauseWire); } } break;
    }
  }

  void adjustReleSubmenuItem(int direction) {
    switch (releSubItem) {
      case 0: config.detTriggerSec = constrainValue(config.detTriggerSec + direction, 1, 10); showNum(config.detTriggerSec); break;
      case 1: config.defuseTriggerSec = constrainValue(config.defuseTriggerSec + direction, 1, 10); showNum(config.defuseTriggerSec); break;
    }
  }

  int constrainValue(int value, int min, int max) { if (value < min) return max; if (value > max) return min; return value; }

  void showCurrentValue() {
    switch (menuItem) {
      case 0: showNum(config.brightness); break; case 1: showNum(config.volume / 16); break;
      case 2: showBool(config.tickEnabled); break; case 3: showWireSubmenuItem(0); break;
      case 4: display.printString("CODE"); break; case 5: showReleSubmenuItem(0); break;
      case 6: showBool(config.rfidEnabled); break;
    }
  }

  void showCurrentWireValue(int subItem) {
    switch (subItem) { case 0: display.printString("ORDR"); break; case 1: showWireValue(config.detWire); break;
      case 2: showWireValue(config.defuseWire); break; case 3: showWireValue(config.speedupWire); break;
      case 4: showWireValue(config.pauseWire); break; }
  }

  void showCurrentReleValue(int subItem) { switch (subItem) { case 0: showNum(config.detTriggerSec); break; case 1: showNum(config.defuseTriggerSec); break; } }

  void showWireValue(int val) {
    if (val == 0) display.printString("NONE"); else { char buf[2] = {'0' + val, 0}; display.printString(buf); }
    delay(500); showWireSubmenuItem(wireSubItem);
  }

  void showNum(int val) { display.printNumber(val); delay(500); showMenuItem(); }
  void showBool(bool val) { display.printString(val ? " on " : " off"); delay(500); showMenuItem(); }

  void inputCode() {
    display.printString("____"); int pos = 0; int tempCode = 0;
    while (true) {
      taskManager.runLoop(); switches.runLoop(); hw.sound().loop();
      char key = keypadListener.lastKey;
      if (key != 0) {
        keypadListener.lastKey = 0;
        if (key >= '0' && key <= '9' && pos < 4) {
          tempCode = tempCode * 10 + (key - '0');
          display.writeDigitAscii(pos, key); pos++;
          hw.sound().playOnce("/keypress.mp3");
        }
        if (pos >= 4) { config.defuseCode = tempCode; hw.sound().playOnce("/keypress.mp3"); btnPressed[3] = false; break; }
      }
      if (buttonPressed(BUTTON_DET_PIN)) { btnPressed[3] = false; break; }
      delay(10);
    }
    showMenuItem();
  }

  void inputWireOrder() {
    display.printString("____"); int pos = 0; int used[4] = {0,0,0,0}; int tempOrder = 0;
    while (true) {
      taskManager.runLoop(); switches.runLoop(); hw.sound().loop();
      char key = keypadListener.lastKey;
      if (key != 0) {
        keypadListener.lastKey = 0;
        if (key >= '1' && key <= '4' && pos < 4) {
          int wire = key - '0';
          if (!used[wire-1]) { used[wire-1] = 1; tempOrder = tempOrder * 10 + wire; display.writeDigitAscii(pos, key); pos++; hw.sound().playOnce("/keypress.mp3"); }
        }
        if (pos >= 4) { config.defuseOrder = tempOrder; hw.sound().playOnce("/keypress.mp3"); btnPressed[3] = false; break; }
      }
      if (buttonPressed(BUTTON_DET_PIN)) { btnPressed[3] = false; break; }
      delay(10);
    }
    showMenuItem();
  }

  static uint8_t hexStringToBytes(const char* hexStr, uint8_t* bytes, uint8_t maxBytes) {
    uint8_t count = 0; const char* p = hexStr;
    while (*p && count < maxBytes) {
      while (*p && !isxdigit(*p)) p++;
      if (!*p) break; unsigned int b;
      if (sscanf(p, "%2x", &b) != 1) break;
      bytes[count++] = (uint8_t)b; p += 2;
    }
    return count;
  }

  static void bytesToHexString(const uint8_t* bytes, uint8_t len, char* out) {
    char* p = out;
    for (uint8_t i = 0; i < len; i++) { if (i > 0) *p++ = ':'; p += sprintf(p, "%02X", bytes[i]); }
    *p = '\0';
  }

  void showRfidSubmenuItem(int subItem) { display.clear(); display.setDisplayColon(false);
    switch (subItem) { case 0: display.printString("DEFU"); break; case 1: display.printString("DETO"); break; }
  }

  void pollRfidTag() {
    if (!nfc_ || millis() - lastRfidPoll_ < 250) return; lastRfidPoll_ = millis();
    bool isDefuse = (rfidSubItem == 0);
    int& tagCount = isDefuse ? config.defuseTagCount : config.detonateTagCount;
    RfidTagBin* tags = isDefuse ? config.defuseTags : config.detonateTags;
    RfidTagBin* otherTags = isDefuse ? config.detonateTags : config.defuseTags;
    int otherTagCount = isDefuse ? config.detonateTagCount : config.defuseTagCount;

    NfcReader::CardData card = nfc_->readCard(100);
    if (!card.present || card.uidLength == 0) return;
    if (card.uidLength == lastRfidUidLen_ && memcmp(card.uid, lastRfidUid_, lastRfidUidLen_) == 0) return;
    memcpy(lastRfidUid_, card.uid, card.uidLength); lastRfidUidLen_ = card.uidLength;

    for (int i = 0; i < tagCount; i++) {
      if (tags[i].valid && tags[i].len == card.uidLength && memcmp(tags[i].bytes, card.uid, card.uidLength) == 0) {
        strcpy(rfidFeedbackMsg, "dup "); rfidFeedbackUntil = millis() + 400; return;
      }
    }
    for (int i = 0; i < otherTagCount; i++) {
      if (otherTags[i].valid && otherTags[i].len == card.uidLength && memcmp(otherTags[i].bytes, card.uid, card.uidLength) == 0) {
        strcpy(rfidFeedbackMsg, "dup "); rfidFeedbackUntil = millis() + 400; return;
      }
    }
    if (tagCount >= 4) { hw.sound().playOnce("/error.mp3"); strcpy(rfidFeedbackMsg, "FULL"); rfidFeedbackUntil = millis() + 500; return; }

    tags[tagCount].len = card.uidLength; memcpy(tags[tagCount].bytes, card.uid, card.uidLength);
    tags[tagCount].valid = true; tagCount++;
    hw.sound().playOnce("/keypress.mp3"); strcpy(rfidFeedbackMsg, "GOOD"); rfidFeedbackUntil = millis() + 500;
  }

  void checkRfidDuringCountdown() {
    if (!config.rfidEnabled || !nfc_ || millis() - lastRfidPoll_ < 300) return; lastRfidPoll_ = millis();
    NfcReader::CardData card = nfc_->readCard(100);
    if (!card.present) { lastRfidUidLen_ = 0; return; }
    if (card.uidLength == lastRfidUidLen_ && memcmp(card.uid, lastRfidUid_, lastRfidUidLen_) == 0) return;
    memcpy(lastRfidUid_, card.uid, card.uidLength); lastRfidUidLen_ = card.uidLength;

    if (config.defuseTagCount > 0) {
      for (int i = 0; i < config.defuseTagCount; i++) {
        if (config.defuseTags[i].valid && config.defuseTags[i].len == card.uidLength && memcmp(config.defuseTags[i].bytes, card.uid, card.uidLength) == 0) { isDefused = true; return; }
      }
    }
    if (config.detonateTagCount > 0) {
      for (int i = 0; i < config.detonateTagCount; i++) {
        if (config.detonateTags[i].valid && config.detonateTags[i].len == card.uidLength && memcmp(config.detonateTags[i].bytes, card.uid, card.uidLength) == 0) { countdownSeconds = 0; return; }
      }
    }
  }

  void updateDisplay() { display.printTime(countdownSeconds / 60, countdownSeconds % 60); }
  void tick() { if (config.tickEnabled) hw.sound().playOnce("/tick.mp3"); }

  boolean buttonPressed(int button) {
    int idx = button - BUTTON_LEFT_PIN;
    if (idx < 0 || idx > 3) return false;
    if (btnPressed[idx]) { btnPressed[idx] = false; return true; }
    return false;
  }

  uint32_t neoColor(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t scale = constrain(config.brightness, 0, 15) * 17;
    return Adafruit_NeoPixel::Color((uint16_t)r * scale / 255, (uint16_t)g * scale / 255, (uint16_t)b * scale / 255);
  }

  void setNeoEffect(int effect) { neoEffect = effect; neoStep = 0; }

  void updateNeoEffect() {
    if (millis() - neoLastUpdate < 50) return; neoLastUpdate = millis();
    switch (neoEffect) {
      case NEOFX_OFF: hw.setLightAll(0); break;
      case NEOFX_NORMAL: { uint8_t b = 50 + (uint8_t)(50 * sin(millis() / 1000.0)); hw.setLightAll(neoColor(b, b, b)); break; }
      case NEOFX_COUNTING: { uint8_t b = 100 + (uint8_t)(100 * sin(millis() / 500.0)); hw.setLightAll(neoColor(b, 0, 0)); break; }
      case NEOFX_DOUBLE_SPEED: { uint8_t b = (millis() % 250 < 125) ? 255 : 50; hw.setLightAll(neoColor(b, 0, 0)); break; }
      case NEOFX_DEFUSED: { uint8_t b = 100 + (uint8_t)(155 * sin(millis() / 800.0)); hw.setLightAll(neoColor(0, b, 0)); break; }
      case NEOFX_EXPLOSION: doExplosionEffect(); break;
      case NEOFX_RADIATION: { uint8_t flicker = random(50, 255); hw.setLightAll(neoColor(random(20), flicker, random(50))); break; }
    }
  }

  void doExplosionEffect() {
    uint32_t elapsed = millis() % 3000;
    if (elapsed < 200) hw.setLightAll(neoColor(255, 255, 255));
    else if (elapsed < 500) { uint8_t fade = 255 - ((elapsed - 200) * 255 / 300); hw.setLightAll(neoColor(255, 255, fade)); }
    else if (elapsed < 1000) { uint8_t green = 150 - ((elapsed - 500) * 150 / 500); hw.setLightAll(neoColor(255, green, 0)); }
    else if (elapsed < 2000) { uint8_t bri = 255 - ((elapsed - 1000) * 200 / 1000); hw.setLightAll(neoColor(bri, 0, 0)); }
    else { if (random(100) < 30) hw.setLightAll(neoColor(255, random(100), 0)); else hw.setLightAll(neoColor(random(50), 0, 0)); }
  }

  void loadConfig() {
    if (!LittleFS.exists(CONFIG_FILE)) {
      Serial.println(F("[GameTimer] Arquivo de config nao existe, criando padrao..."));
      saveConfig(); return;
    }
    File f = LittleFS.open(CONFIG_FILE, "r");
    JsonDocument doc; deserializeJson(doc, f);

    config.countdownSeconds = doc["ctdn"] | 600;
    config.defuseMode = doc["mode"] | 0;
    config.defuseCode = doc["code"] | 1234;
    config.defuseOrder = doc["ordr"] | 1234;
    config.detWire = doc["detW"] | 1;
    config.defuseWire = doc["dffW"] | 1;
    config.speedupWire = doc["spdW"] | 0;
    config.pauseWire = doc["psW"] | 0;
    config.volume = doc["vol"] | 128;
    config.brightness = doc["bri"] | 15;
    config.detTriggerSec = doc["tdEt"] | 5;
    config.defuseTriggerSec = doc["tdEF"] | 5;
    config.tickEnabled = doc["tic"] | 1;

    config.rfidEnabled = doc["rFId"] | 0;
    config.defuseTagCount = 0; config.detonateTagCount = 0;
    memset(config.defuseTags, 0, sizeof(config.defuseTags));
    memset(config.detonateTags, 0, sizeof(config.detonateTags));
    if (doc["dTAG"].is<JsonArray>()) {
      auto arr = doc["dTAG"].as<JsonArray>(); int count = min((int)arr.size(), 4);
      for (int i = 0; i < count; i++) { config.defuseTags[i].len = hexStringToBytes(arr[i] | "", config.defuseTags[i].bytes, 7); config.defuseTags[i].valid = (config.defuseTags[i].len > 0); if (config.defuseTags[i].valid) config.defuseTagCount++; }
    }
    if (doc["oTAG"].is<JsonArray>()) {
      auto arr = doc["oTAG"].as<JsonArray>(); int count = min((int)arr.size(), 4);
      for (int i = 0; i < count; i++) { config.detonateTags[i].len = hexStringToBytes(arr[i] | "", config.detonateTags[i].bytes, 7); config.detonateTags[i].valid = (config.detonateTags[i].len > 0); if (config.detonateTags[i].valid) config.detonateTagCount++; }
    }
    f.close();

    Serial.println(F("[GameTimer] CONFIGURACAO CARREGADA:"));
    Serial.printf("  Countdown: %ds, Modo: %d, Codigo: %04d\n", config.countdownSeconds, config.defuseMode, config.defuseCode);
    Serial.printf("  Fios: Det=%d Def=%d Spd=%d Paus=%d\n", config.detWire, config.defuseWire, config.speedupWire, config.pauseWire);
    Serial.printf("  Brilho: %d/15, Volume: %d/255\n", config.brightness, config.volume);
    Serial.printf("  Trigger TDet=%ds TDef=%ds\n", config.detTriggerSec, config.defuseTriggerSec);
    Serial.printf("  Tick: %s, RFID: %s\n", config.tickEnabled ? "ON" : "OFF", config.rfidEnabled ? "ON" : "OFF");
  }

  void saveConfig() {
    File f = LittleFS.open(CONFIG_FILE, "w");
    JsonDocument doc;
    doc["ctdn"] = config.countdownSeconds; doc["mode"] = config.defuseMode;
    doc["code"] = config.defuseCode; doc["ordr"] = config.defuseOrder;
    doc["detW"] = config.detWire; doc["dffW"] = config.defuseWire;
    doc["spdW"] = config.speedupWire; doc["psW"] = config.pauseWire;
    doc["vol"] = config.volume; doc["bri"] = config.brightness;
    doc["tdEt"] = config.detTriggerSec; doc["tdEF"] = config.defuseTriggerSec;
    doc["tic"] = config.tickEnabled; doc["rFId"] = config.rfidEnabled;

    JsonArray dTagArr = doc["dTAG"].to<JsonArray>();
    for (int i = 0; i < config.defuseTagCount; i++) {
      if (config.defuseTags[i].valid) { char hexBuf[22]; bytesToHexString(config.defuseTags[i].bytes, config.defuseTags[i].len, hexBuf); dTagArr.add(hexBuf); }
    }
    JsonArray oTagArr = doc["oTAG"].to<JsonArray>();
    for (int i = 0; i < config.detonateTagCount; i++) {
      if (config.detonateTags[i].valid) { char hexBuf[22]; bytesToHexString(config.detonateTags[i].bytes, config.detonateTags[i].len, hexBuf); oTagArr.add(hexBuf); }
    }
    serializeJson(doc, f); f.close();
    Serial.println(F("[GameTimer] Config salva"));
  }
};

// ============================================================================
// CALLBACKS SwitchInput (fora da classe)
// ============================================================================

volatile bool btnPressed[4] = {false, false, false, false};
volatile bool wireCutFlag[4] = {false, false, false, false};

void onButtonPressed(pinid_t pin, bool held) {
  if (held) return;
  int idx = pin - BUTTON_LEFT_PIN;
  if (idx >= 0 && idx < 4) {
    btnPressed[idx] = true;
    const char* btnName = (idx == 0) ? "LEFT" : (idx == 1) ? "SELECT" : (idx == 2) ? "RIGHT" : "DET";
    Serial.printf("[BTN] %s (P%d) PRESSED\n", btnName, pin);
  }
}

int wirePinToIndex(pinid_t pin) {
  switch(pin) { case WIRE1_PIN: return 0; case WIRE2_PIN: return 1; case WIRE3_PIN: return 2; case WIRE4_PIN: return 3; default: return -1; }
}

void onWireCut(pinid_t pin, bool held) {
  int wireNum = wirePinToIndex(pin);
  if (wireNum >= 0 && wireNum < 4) { wireCutFlag[wireNum] = true; Serial.printf("[WIRE] P%d (Fio %d) CORTADO\n", pin, wireNum + 1); }
}

// ============================================================================
// MAIN
// ============================================================================

WiFiClient wifiClient;
ConfigManager netConfig;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);
GameTimerPuzzle puzzle;

void setup() {
  Serial.begin(115200);
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);

  Serial.println(F("============================================"));
  Serial.println(F("  BombTimerPro 2.0 - ESP32-S3 (XIAO)"));
  Serial.println(F("  Nicolau dos Brinquedos"));
  Serial.println(F("============================================"));

  // Hardware scan
  hw.begin();
  hw.addPcf8575(0x20);

  // ConfigManager: SD > SPIFFS > AP mode
  netConfig.begin();
  if (netConfig.isAPMode()) {
    Serial.println(F("[CONFIG] AP mode - http://192.168.4.1"));
  }

  // ArduinoHA
  byte mac[6];
  WiFi.macAddress(mac);
  haDevice.setUniqueId(mac, sizeof(mac));
  haDevice.setName("bomb-timer");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());

  // Inicializa puzzle
  puzzle.setupHardware();
  puzzle.setupHA();

  Serial.println(F("[Main] Sistema pronto"));
}

void loop() {
  haMqtt.loop();
  netConfig.handleAP();
  puzzle.update();
  delay(1);
}
