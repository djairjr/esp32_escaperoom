/**
 * CONNECT WIRES - ESP32-S3 (Seeed XIAO)
 * Port from RP2040 PropMaker to ESP32-S3 using ESP32EscapeRoom
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

using namespace esp32_escaperoom;

// ============================================================================
// D-PIN → GPIO MAPPING
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
// PINOUT
// ============================================================================
#define LOCK_PIN            D0   // Lock relay (native)

// Pins on the PCF8575 (IoAbstraction: 0-7=P0-P7, 8-15=P10-P17)
#define BUTTON_RESET_PIN    8    // P10
#define BUTTON_UNLOCK_PIN   9    // P11

#define NUM_SOCKETS         9
#define NUM_CONNECTIONS     4
const uint8_t SOCKET_PINS[NUM_SOCKETS] = {0, 1, 2, 3, 4, 5, 6, 7, 8};

#define VOLTMETER_CHANNEL   0    // PCA9685
#define GAUGE_LEDS          8    // NeoPixel for the progress bar

#define LOCK_ACTIVE         HIGH
#define LOCK_INACTIVE       LOW

// ============================================================================
// FORWARD DECLARATIONS + GLOBALS
// ============================================================================
class ConnectWiresPuzzle;
extern ConnectWiresPuzzle puzzle;
extern volatile bool btnPressed[2];

void onButtonPressed(pinid_t pin, bool held);

// Sensors (3)
HASensor haSensorState("state");
HASensor haSensorConnections("connections");
HASensor haSensorPairs("pairs");

// Lock (1)
HALock haLock("lock");

// Buttons (2)
HAButton haBtnStart("start");
HAButton haBtnUnlock("unlock");

// Number (1)
HANumber haNumPairs("pair_count");

// Select (1)
HASelect haSelectMode("pair_mode");

// ============================================================================
// PUZZLE CLASS
// ============================================================================
class ConnectWiresPuzzle : public PuzzleBase {
public:
    ConnectWiresPuzzle() : PuzzleBase(11) {}

    void setupHardware() override {
        Serial.println(F("[WIRES] Setup started"));

        // Lock relay (native)
        hw.pinMode(LOCK_PIN, OUTPUT);
        hw.digitalWrite(LOCK_PIN, LOCK_ACTIVE);

        // SwitchInput via PCF8575
        if (hw.hasExpander()) {
            switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);
            switches.addSwitch(BUTTON_RESET_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_UNLOCK_PIN, onButtonPressed, NO_REPEAT, false);
            Serial.println(F("[WIRES] SwitchInput OK"));
        } else {
            Serial.println(F("[WIRES] ERROR: PCF8575 not detected!"));
        }

        // Sockets on the PCF
        for (int i = 0; i < NUM_SOCKETS; i++) {
            hw.io()->pinMode(SOCKET_PINS[i], INPUT_PULLUP);
        }

        // Reset PCA9685 voltmeter
        hw.setAnalogOutput(VOLTMETER_CHANNEL, 0);

        // Generate connections
        randomizeConnections();
        setState(PuzzleState::Idle);
        Serial.println(F("[WIRES] Ready."));
    }

    void update() override {
        switches.runLoop();
        taskManager.runLoop();

        if (solveEffectActive_) updateSolveEffect();

        switch (getState()) {
            case PuzzleState::Idle:
                if (buttonPressed(BUTTON_RESET_PIN)) resetPuzzle();
                break;
            case PuzzleState::Running:
                processConnections();
                if (buttonPressed(BUTTON_RESET_PIN)) resetPuzzle();
                break;
            case PuzzleState::Solved:
                handleManualRelease();
                if (buttonPressed(BUTTON_RESET_PIN)) resetPuzzle();
                break;
            default: break;
        }
    }

    // ========================================================================
    // HA SETUP
    // ========================================================================
    void setupHA() {
        haSensorState.setName("State");  haSensorState.setIcon("mdi:cable-data");
        haSensorConnections.setName("Active Connections"); haSensorConnections.setIcon("mdi:connection");
        haSensorPairs.setName("Connected Pairs"); haSensorPairs.setIcon("mdi:electric-switch");

        haLock.setName("Lock"); haLock.setIcon("mdi:lock");
        haLock.onCommand(haLockCallback);

        haBtnStart.setName("Start / Reset"); haBtnStart.setIcon("mdi:play");
        haBtnStart.onCommand(haStartCallback);
        haBtnUnlock.setName("Release Lock"); haBtnUnlock.setIcon("mdi:lock-open");
        haBtnUnlock.onCommand(haUnlockCallback);

        haNumPairs.setName("Pairs"); haNumPairs.setIcon("mdi:numeric");
        haNumPairs.setMin(1); haNumPairs.setMax(4); haNumPairs.setStep(1);
        haNumPairs.onCommand(haPairsCallback);
        haNumPairs.setState((int32_t)NUM_CONNECTIONS);

        haSelectMode.setName("Pair Mode"); haSelectMode.setIcon("mdi:tune");
        haSelectMode.setOptions("Fixed;Random");
        haSelectMode.onCommand(haModeCallback);

        publishAllHA();
    }

    void publishAllHA() {
        const char* s = getState() == PuzzleState::Idle ? "idle" :
                        getState() == PuzzleState::Running ? "running" : "solved";
        haSensorState.setValue(s);
        char buf[8]; snprintf(buf, sizeof(buf), "%d/%d", activeConnections_, NUM_CONNECTIONS);
        haSensorConnections.setValue(buf);
        updatePairsSensor();
        haLock.setState(getState() == PuzzleState::Solved ? HALock::StateLocked : HALock::StateUnlocked);
    }

    void updatePairsSensor() {
        String p = "";
        for (int i = 0; i < NUM_CONNECTIONS; i++) {
            if (lastState_[i]) {
                if (p.length() > 0) p += ",";
                p += String(i + 1);
            }
        }
        if (p.length() == 0) p = "none";
        haSensorPairs.setValue(p.c_str());
    }

    // HA callbacks
    static void haLockCallback(HALock::LockCommand cmd, HALock*) {
        auto& _hw = esp32_escaperoom::Hardware::instance();
        _hw.digitalWrite(LOCK_PIN, cmd == HALock::CommandLock ? LOCK_ACTIVE : LOCK_INACTIVE);
    }
    static void haStartCallback(HAButton*) {
        if (puzzle.getState() == PuzzleState::Idle) {
            puzzle.setState(PuzzleState::Running);
            puzzle.publishAllHA();
        } else {
            puzzle.resetPuzzle();
        }
    }
    static void haUnlockCallback(HAButton*) {
        esp32_escaperoom::Hardware::instance().digitalWrite(LOCK_PIN, LOCK_INACTIVE);
    }
    static void haPairsCallback(HANumeric val, HANumber*) {
        // Only stores — reset applies
        (void)val;
    }
    static void haModeCallback(int8_t idx, HASelect*) {
        // 0=Fixed, 1=Random — reset applies
        (void)idx;
    }

private:
    uint8_t connections_[NUM_CONNECTIONS][2];
    bool lastState_[NUM_CONNECTIONS] = {false};
    int activeConnections_ = 0;

    uint8_t solveEffectStep_ = 0;
    unsigned long solveEffectStartTime_ = 0;
    bool solveEffectActive_ = false;

    void processConnections() {
        activeConnections_ = 0;
        bool allWiresCorrect = true;
        bool stateChanged = false;

        for (int i = 0; i < NUM_CONNECTIONS; i++) {
            uint8_t pin1 = connections_[i][0];
            uint8_t pin2 = connections_[i][1];
            bool currentState = checkPair(pin1, pin2);

            if (currentState != lastState_[i]) {
                stateChanged = true;
                lastState_[i] = currentState;
                if (currentState) {
                    Serial.printf("[WIRES] Connection %d: P%d <-> P%d\n", i+1, pin1, pin2);
                    hw.sound().playOnce("/connected.mp3");
                } else {
                    Serial.printf("[WIRES] Connection %d removed\n", i+1);
                }
                updatePairsSensor();
            }

            if (currentState) { activeConnections_++; }
            else { allWiresCorrect = false; }
        }

        float percent = map(activeConnections_, 0, NUM_CONNECTIONS, 0, 100) / 100.0f;
        hw.setAnalogOutput(VOLTMETER_CHANNEL, percent);
        updateGauge(activeConnections_);

        char buf[8]; snprintf(buf, sizeof(buf), "%d/%d", activeConnections_, NUM_CONNECTIONS);
        haSensorConnections.setValue(buf);

        if (allWiresCorrect && getState() == PuzzleState::Running) handleSolve();
        else if (!allWiresCorrect && getState() == PuzzleState::Solved) handleUnsolve();
    }

    bool checkPair(uint8_t outPin, uint8_t inPin) {
        auto* io = hw.io();
        io->pinMode(outPin, OUTPUT);
        io->digitalWrite(outPin, LOW);
        delayMicroseconds(100);
        io->pinMode(inPin, INPUT_PULLUP);
        bool connected = (io->digitalRead(inPin) == LOW);
        io->digitalWrite(outPin, HIGH);
        io->pinMode(outPin, INPUT_PULLUP);
        return connected;
    }

    void randomizeConnections() {
        uint8_t pins[NUM_SOCKETS];
        for (int i = 0; i < NUM_SOCKETS; i++) pins[i] = SOCKET_PINS[i];
        for (int i = NUM_SOCKETS - 1; i > 0; i--) {
            int j = random(i + 1);
            uint8_t t = pins[i]; pins[i] = pins[j]; pins[j] = t;
        }
        for (int i = 0; i < NUM_CONNECTIONS; i++) {
            connections_[i][0] = pins[i * 2];
            connections_[i][1] = pins[i * 2 + 1];
        }
        Serial.println(F("[WIRES] New connections generated:"));
        for (int i = 0; i < NUM_CONNECTIONS; i++) {
            Serial.printf("  %d: Pin %d <-> Pin %d\n",
                          i + 1, connections_[i][0], connections_[i][1]);
        }
    }

    void updateGauge(int count) {
        const uint32_t colors[] = {
            Adafruit_NeoPixel::Color(0,0,0), Adafruit_NeoPixel::Color(255,0,0),
            Adafruit_NeoPixel::Color(255,69,0), Adafruit_NeoPixel::Color(255,165,0),
            Adafruit_NeoPixel::Color(255,255,0), Adafruit_NeoPixel::Color(0,255,0)
        };
        hw.setLightAll(0);
        int ledsPerConnection = GAUGE_LEDS / NUM_CONNECTIONS;
        for (int i = 0; i < count * ledsPerConnection && i < GAUGE_LEDS; i++) {
            uint8_t ci = map(i, 0, GAUGE_LEDS, 0, 5);
            hw.setLight(i, colors[ci]);
        }
    }

    void handleSolve() {
        Serial.println(F("[WIRES] Puzzle solved!"));
        startSolveEffect();
        hw.sound().playOnce("/success.mp3");
        setState(PuzzleState::Solved);
        haSensorState.setValue("solved");
        haLock.setState(HALock::StateLocked);
    }

    void startSolveEffect() {
        solveEffectActive_ = true; solveEffectStep_ = 0; solveEffectStartTime_ = millis();
        hw.setLightAll(Adafruit_NeoPixel::Color(0,255,0));
    }

    void updateSolveEffect() {
        unsigned long e = millis() - solveEffectStartTime_;
        switch (solveEffectStep_) {
            case 0: if (e >= 100) { hw.setLightAll(0); solveEffectStep_ = 1; } break;
            case 1: if (e >= 200) { hw.setLightAll(Adafruit_NeoPixel::Color(0,255,0)); solveEffectStep_ = 2; } break;
            case 2: if (e >= 500) solveEffectActive_ = false; break;
        }
    }

    void handleUnsolve() {
        Serial.println(F("[WIRES] Puzzle unsolved!"));
        hw.digitalWrite(LOCK_PIN, LOCK_ACTIVE);
        setState(PuzzleState::Running);
        haSensorState.setValue("running");
        haLock.setState(HALock::StateUnlocked);
    }

    void handleManualRelease() {
        if (buttonPressed(BUTTON_UNLOCK_PIN)) {
            hw.digitalWrite(LOCK_PIN, LOCK_INACTIVE);
            hw.sound().playOnce("/lock_open.mp3");
        }
    }

    void resetPuzzle() {
        activeConnections_ = 0;
        for (int i = 0; i < NUM_CONNECTIONS; i++) lastState_[i] = false;
        solveEffectActive_ = false;
        hw.digitalWrite(LOCK_PIN, LOCK_ACTIVE);
        hw.setAnalogOutput(VOLTMETER_CHANNEL, 0);
        hw.setLightAll(0);
        randomizeConnections();
        setState(PuzzleState::Idle);
        publishAllHA();
    }

    boolean buttonPressed(int button) {
        int idx = button - BUTTON_RESET_PIN;
        if (idx < 0 || idx > 1) return false;
        if (btnPressed[idx]) { btnPressed[idx] = false; return true; }
        return false;
    }
};

// ============================================================================
// CALLBACKS + GLOBALS
// ============================================================================
volatile bool btnPressed[2] = {false, false};

void onButtonPressed(pinid_t pin, bool held) {
    if (held) return;
    int idx = pin - BUTTON_RESET_PIN;
    if (idx >= 0 && idx < 2) {
        btnPressed[idx] = true;
        Serial.printf("[BTN] %s\n", idx == 0 ? "RESET" : "UNLOCK");
    }
}

ConnectWiresPuzzle puzzle;
LogicManager master(11);
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    Serial.println(F("========================================"));
    Serial.println(F("  ConnectWires - ESP32-S3"));
    Serial.println(F("========================================"));

    hw.begin();
    hw.addPcf8575(0x20);

    netConfig.begin();
    if (netConfig.isAPMode()) Serial.println(F("[CFG] AP: http://192.168.4.1"));

    byte mac[6]; WiFi.macAddress(mac);
    haDevice.setUniqueId(mac, sizeof(mac));
    haDevice.setName(netConfig.devName());
    haDevice.setSoftwareVersion("1.0.0");

    puzzle.setupHardware();
    puzzle.setupHA();
    haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());

    master.addPuzzle(&puzzle);
    master.begin();
    puzzle.run();
    Serial.println(F("[WIRES] Ready!"));
}

// ============================================================================
// LOOP
// ============================================================================
void loop() {
    haMqtt.loop();
    netConfig.handleAP();
    master.update();
    delay(1);
}
