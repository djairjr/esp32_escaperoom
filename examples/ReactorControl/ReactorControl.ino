/**
 * EXAMPLE: REACTOR CONTROL - NUCLEAR CONTROL PANEL
 * ----------------------------------------------------------
 * Analog adjustment puzzle with potentiometers, PWM gauges and buttons.
 *
 * NARRATIVE:
 * Nuclear reactor in a critical state. Three technicians operate a control panel
 * with 3 faces. On each face there is a valve (potentiometer) and a gauge (voltmeter).
 * The valve of one face NEVER controls the gauge of the same face — the mapping is
 * random (derangement). Furthermore, each gauge can respond in the normal or
 * reversed direction.
 *
 * If the players fail the confirmation (simultaneous buttons), the panel
 * reconfigures the mapping and the orientations, forcing them to rediscover everything.
 *
 * HARDWARE: Adafruit PropMaker Feather RP2040
 * - ADS1115 (I2C 0x48) — 3 channels for potentiometers
 * - PCA9685 (I2C 0x40) — 3 PWM channels for voltmeters
 * - PCF8575 (I2C 0x20) — 3 confirmation buttons
 * - External NeoPixel — 3 LEDs for button feedback + status bar
 * - Magnetic lock — digital pin via hw.digitalWrite()
 *
 * REQUIRED AUDIO (LittleFS or SD):
 * - ready.mp3       : Puzzle started
 * - alarm.mp3       : Gauge out of the green zone
 * - stabilizing.mp3 : Gauge entered the green zone
 * - warning.mp3     : All gauges in the zone (tension)
 * - button_press.mp3: Button pressed
 * - fail.mp3        : Confirmation failed (time out)
 * - reshuffle.mp3   : Reconfiguration after failure
 * - victory.mp3     : Puzzle solved
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
// CONSTANTS
// ============================================================================
#define PCF_ADDRESS         0x20

// Confirmation buttons (P0-P2 on the PCF8575)
#define BUTTON_1_PIN        0
#define BUTTON_2_PIN        1
#define BUTTON_3_PIN        2

// NeoPixel for each button (indices in the external strip)
#define BUTTON_1_PIXEL      0
#define BUTTON_2_PIXEL      1
#define BUTTON_3_PIXEL      2

// Magnetic lock pin
#define LOCK_PIN            20

// Game configuration
static constexpr uint8_t NUM_VALVES = 3;
static constexpr uint8_t ADS_CHANNELS[NUM_VALVES] = {0, 1, 2};
static constexpr uint8_t PCA_CHANNELS[NUM_VALVES] = {0, 1, 2};
static constexpr uint8_t BUTTON_PINS[NUM_VALVES] = {BUTTON_1_PIN, BUTTON_2_PIN, BUTTON_3_PIN};
static constexpr uint8_t BUTTON_PIXELS[NUM_VALVES] = {BUTTON_1_PIXEL, BUTTON_2_PIXEL, BUTTON_3_PIXEL};

// Green zone: targets on a 0-10 scale, with tolerance
static constexpr float TARGETS[NUM_VALVES] = {7.0f, 5.0f, 3.0f};
static constexpr float TOLERANCE = 0.5f;

// Times (ms)
static constexpr unsigned long CONFIRM_WINDOW_MS = 2000UL;
static constexpr unsigned long RESHUFFLE_DURATION_MS = 1500UL;
static constexpr unsigned long EFFECT_INTERVAL_MS = 200UL;

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
extern volatile bool btnPressed[NUM_VALVES];

// Forward declarations
void onButtonPressed(pinid_t pin, bool held);

// ============================================================================
// PUZZLE CLASS
// ============================================================================
class ReactorControlPuzzle : public PuzzleBase {
public:
    ReactorControlPuzzle() : PuzzleBase(31) {}  // ID 31 on the RS485 network

    void setupHardware() override {
        Serial.println(F("[REACTOR] Setup started"));

        // Initializes buttons via SwitchInput on the PCF8575
        if (hw.hasExpander()) {
            Serial.println(F("[REACTOR] Initializing SwitchInput on the PCF8575..."));
            switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);

            for (uint8_t i = 0; i < NUM_VALVES; i++) {
                switches.addSwitch(BUTTON_PINS[i], onButtonPressed, NO_REPEAT, false);
            }
            Serial.println(F("[REACTOR] SwitchInput OK: 3 buttons"));
        } else {
            Serial.println(F("[REACTOR] ERROR: PCF8575 not detected!"));
        }

        // Initializes lock
        hw.pinMode(LOCK_PIN, OUTPUT);
        hw.digitalWrite(LOCK_PIN, HIGH);  // Lock active on HIGH

        // Zeroes PCA9685 gauges
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            hw.setAnalogOutput(PCA_CHANNELS[i], 0.0f);
        }

        // Generates initial mapping
        randomizeMapping();

        // Initial state
        setState(PuzzleState::Idle);

        Serial.println(F("[REACTOR] Nuclear Control Panel Ready."));
    }

    void update() override {
        // Processes switch input (buttons)
        switches.runLoop();
        taskManager.runLoop();

        // Updates visual/audio effects (non-blocking)
        updateEffects();

        // State machine
        switch (getState()) {
            case PuzzleState::Idle:
                if (buttonPressed(BUTTON_1_PIN)) {
                    startPuzzle();
                }
                break;

            case PuzzleState::Running:
                switch (subState_) {
                    case SubState::Adjusting:
                        readValvesAndWriteGauges();
                        checkZones();
                        processButtonsAdjusting();
                        updateGaugeColors();
                        break;

                    case SubState::Confirming:
                        readValvesAndWriteGauges();
                        checkZones();
                        processButtonsConfirming();
                        updateConfirmationEffect();
                        break;

                    case SubState::Reshuffling:
                        updateReshuffleEffect();
                        break;
                }
                break;

            case PuzzleState::Solved:
                // Waits for manual reset
                if (buttonPressed(BUTTON_1_PIN)) {
                    resetPuzzle();
                }
                break;

            default:
                break;
        }
    }

private:
    enum class SubState { Adjusting, Confirming, Reshuffling };
    SubState subState_ = SubState::Adjusting;

    // Random mapping
    uint8_t valveToGauge_[NUM_VALVES];
    bool gaugeReversed_[NUM_VALVES];

    // Gauge state
    bool inZone_[NUM_VALVES] = {false};
    uint8_t zonesActive_ = 0;

    // Confirmation
    unsigned long confirmWindowStart_ = 0;
    bool buttonPressedInWindow_[NUM_VALVES] = {false};
    uint8_t buttonsConfirmed_ = 0;

    // Effects
    enum class EffectPhase { None, Alarm, Stabilizing, AllGreen, Victory, Reshuffle };
    EffectPhase effectPhase_ = EffectPhase::None;
    unsigned long effectTimer_ = 0;
    uint8_t effectStep_ = 0;

    // =======================================================================
    // MAPPING
    // =======================================================================
    void randomizeMapping() {
        // Derangement of 3 elements: only 2 possibilities
        // [1,2,0] or [2,0,1]
        uint8_t derangements[2][NUM_VALVES] = {
            {1, 2, 0},
            {2, 0, 1}
        };
        uint8_t choice = random(0, 2);
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            valveToGauge_[i] = derangements[choice][i];
        }

        // Normal/reversed orientation for each gauge
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            gaugeReversed_[i] = random(0, 2) == 1;
        }

        Serial.println(F("[REACTOR] Mapping reconfigured:"));
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            Serial.printf("  Valve %d -> Gauge %d (%s)\n",
                          i, valveToGauge_[i],
                          gaugeReversed_[valveToGauge_[i]] ? "REVERSED" : "NORMAL");
        }
    }

    // =======================================================================
    // ANALOG READ AND WRITE
    // =======================================================================
    void readValvesAndWriteGauges() {
        for (uint8_t valveIdx = 0; valveIdx < NUM_VALVES; valveIdx++) {
            uint8_t gaugeIdx = valveToGauge_[valveIdx];

            // Reads potentiometer via ADS1115 (0-65535 raw)
            uint16_t raw = hw.readAnalogRaw(ADS_CHANNELS[valveIdx]);

            // Converts to percentage 0.0 - 100.0
            float percent = map((long)raw, 0L, 65535L, 0L, 1000L) / 10.0f;
            percent = constrain(percent, 0.0f, 100.0f);

            // Applies inversion if necessary
            if (gaugeReversed_[gaugeIdx]) {
                percent = 100.0f - percent;
            }

            // Writes to the gauge via PCA9685
            hw.setAnalogOutput(PCA_CHANNELS[gaugeIdx], percent);
        }
    }

    // =======================================================================
    // ZONE CHECK
    // =======================================================================
    void checkZones() {
        zonesActive_ = 0;
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            float percent = getCurrentGaugePercent(i);
            float value = percent / 10.0f;  // 0-10 scale
            bool wasInZone = inZone_[i];
            inZone_[i] = (fabs(value - TARGETS[i]) <= TOLERANCE);

            if (inZone_[i]) {
                zonesActive_++;
            }

            // Transition sound (only when it enters the zone)
            if (!wasInZone && inZone_[i] && effectPhase_ != EffectPhase::Victory) {
                hw.sound().playOnce("/stabilizing.mp3");
            }
        }
    }

    float getCurrentGaugePercent(uint8_t gaugeIdx) {
        // Reconstructs the gauge percentage from the potentiometer that controls it
        uint8_t valveIdx = 255;
        for (uint8_t v = 0; v < NUM_VALVES; v++) {
            if (valveToGauge_[v] == gaugeIdx) {
                valveIdx = v;
                break;
            }
        }
        if (valveIdx == 255) return 0.0f;

        uint16_t raw = hw.readAnalogRaw(ADS_CHANNELS[valveIdx]);
        float percent = map((long)raw, 0L, 65535L, 0L, 1000L) / 10.0f;
        percent = constrain(percent, 0.0f, 100.0f);
        if (gaugeReversed_[gaugeIdx]) {
            percent = 100.0f - percent;
        }
        return percent;
    }

    bool isInZone(uint8_t gaugeIdx) {
        float percent = getCurrentGaugePercent(gaugeIdx);
        float value = percent / 10.0f;
        return fabs(value - TARGETS[gaugeIdx]) <= TOLERANCE;
    }

    // =======================================================================
    // BUTTONS
    // =======================================================================
    void processButtonsAdjusting() {
        // When all gauges are in the zone, starts the confirmation window
        if (zonesActive_ == NUM_VALVES && subState_ == SubState::Adjusting) {
            startConfirmationWindow();
        }
    }

    void processButtonsConfirming() {
        // Checks whether the confirmation window expired
        unsigned long elapsed = millis() - confirmWindowStart_;
        if (elapsed >= CONFIRM_WINDOW_MS) {
            // Time out
            if (buttonsConfirmed_ < NUM_VALVES) {
                handleConfirmationFail();
            }
            return;
        }

        // Processes buttons pressed in the window
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            if (!buttonPressedInWindow_[i] && buttonPressed(BUTTON_PINS[i])) {
                buttonPressedInWindow_[i] = true;
                buttonsConfirmed_++;
                hw.sound().playOnce("/button_press.mp3");

                // Turns on the button's NeoPixel
                hw.setLight(BUTTON_PIXELS[i], Adafruit_NeoPixel::Color(0, 255, 0));

                Serial.printf("[REACTOR] Button %d pressed (%d/%d)\n",
                              i + 1, buttonsConfirmed_, NUM_VALVES);
            }
        }

        // Checks victory
        if (buttonsConfirmed_ >= NUM_VALVES) {
            handleSolve();
        }
    }

    boolean buttonPressed(int button) {
        int idx = button - BUTTON_1_PIN;
        if (idx < 0 || idx >= (int)NUM_VALVES) return false;

        if (btnPressed[idx]) {
            btnPressed[idx] = false;  // Clears flag
            return true;
        }
        return false;
    }

    // =======================================================================
    // VISUAL EFFECTS
    // =======================================================================
    void updateGaugeColors() {
        // Colors per gauge: red=out, yellow=in the zone, green=all in the zone
        if (subState_ == SubState::Confirming || subState_ == SubState::Reshuffling) {
            return;
        }

        uint32_t colors[NUM_VALVES];
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            if (inZone_[i]) {
                colors[i] = Adafruit_NeoPixel::Color(0, 255, 0);  // Green
            } else {
                colors[i] = Adafruit_NeoPixel::Color(255, 0, 0);  // Red
            }
        }

        // If all in the zone, blink green
        if (zonesActive_ == NUM_VALVES) {
            unsigned long t = millis() % 1000;
            if (t < 500) {
                for (uint8_t i = 0; i < NUM_VALVES; i++) {
                    hw.setLight(BUTTON_PIXELS[i], colors[i]);
                }
            } else {
                hw.setLightAll(0);
            }
            return;
        }

        // Alarm if none in the zone
        if (zonesActive_ == 0) {
            unsigned long t = millis() % 600;
            if (t < 300) {
                for (uint8_t i = 0; i < NUM_VALVES; i++) {
                    hw.setLight(BUTTON_PIXELS[i], colors[i]);
                }
            } else {
                hw.setLightAll(0);
            }
            return;
        }

        // Partial state: turns on each button with the color of its gauge
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            hw.setLight(BUTTON_PIXELS[i], colors[i]);
        }
    }

    void updateConfirmationEffect() {
        // Blinks white on the buttons not pressed during confirmation
        unsigned long t = millis() % 400;
        uint32_t color = (t < 200) ? Adafruit_NeoPixel::Color(255, 255, 255) : 0;

        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            if (!buttonPressedInWindow_[i]) {
                hw.setLight(BUTTON_PIXELS[i], color);
            }
        }
    }

    void updateReshuffleEffect() {
        unsigned long elapsed = millis() - effectTimer_;
        if (elapsed >= RESHUFFLE_DURATION_MS) {
            // End of the reshuffle
            subState_ = SubState::Adjusting;
            effectPhase_ = EffectPhase::None;
            hw.setLightAll(0);
            Serial.println(F("[REACTOR] Reconfiguration complete. Keep adjusting."));
            return;
        }

        // Blinks white on all the LEDs during reconfiguration
        unsigned long t = millis() % 150;
        if (t < 75) {
            hw.setLightAll(Adafruit_NeoPixel::Color(255, 255, 255));
        } else {
            hw.setLightAll(0);
        }
    }

    void updateEffects() {
        switch (effectPhase_) {
            case EffectPhase::Alarm:
                if (millis() - effectTimer_ > EFFECT_INTERVAL_MS) {
                    effectPhase_ = EffectPhase::None;
                }
                break;

            case EffectPhase::Victory:
                updateVictoryEffect();
                break;

            case EffectPhase::Reshuffle:
                // Controlled by updateReshuffleEffect()
                break;

            default:
                break;
        }
    }

    void updateVictoryEffect() {
        unsigned long elapsed = millis() - effectTimer_;

        switch (effectStep_) {
            case 0:
                hw.setLightAll(Adafruit_NeoPixel::Color(0, 255, 0));
                if (elapsed >= 200) {
                    effectStep_ = 1;
                }
                break;
            case 1:
                hw.setLightAll(0);
                if (elapsed >= 400) {
                    hw.setLightAll(Adafruit_NeoPixel::Color(0, 255, 0));
                    effectStep_ = 2;
                }
                break;
            case 2:
                if (elapsed >= 800) {
                    hw.setLightAll(Adafruit_NeoPixel::Color(0, 255, 0));
                    effectPhase_ = EffectPhase::None;
                }
                break;
        }
    }

    // =======================================================================
    // PUZZLE STATES
    // =======================================================================
    void startPuzzle() {
        Serial.println(F("[REACTOR] Starting sequence..."));
        hw.sound().playOnce("/ready.mp3");
        subState_ = SubState::Adjusting;
        setState(PuzzleState::Running);
    }

    void startConfirmationWindow() {
        subState_ = SubState::Confirming;
        confirmWindowStart_ = millis();
        buttonsConfirmed_ = 0;
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            buttonPressedInWindow_[i] = false;
        }

        hw.sound().playOnce("/warning.mp3");
        Serial.println(F("[REACTOR] All gauges in the zone! Confirmation window open (2s)"));
    }

    void handleSolve() {
        Serial.println(F("[REACTOR] Puzzle solved! Reactor stabilized."));

        hw.sound().playOnce("/victory.mp3");

        // Opens lock
        hw.digitalWrite(LOCK_PIN, LOW);

        // Victory visual effect
        effectPhase_ = EffectPhase::Victory;
        effectTimer_ = millis();
        effectStep_ = 0;

        setState(PuzzleState::Solved);
    }

    void handleConfirmationFail() {
        Serial.println(F("[REACTOR] Confirmation failed! Reconfiguring panel..."));

        hw.sound().playOnce("/fail.mp3");

        // Reshuffles mapping and orientations
        randomizeMapping();

        // Resets confirmation state
        subState_ = SubState::Reshuffling;
        effectPhase_ = EffectPhase::Reshuffle;
        effectTimer_ = millis();
        buttonsConfirmed_ = 0;
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            buttonPressedInWindow_[i] = false;
        }

        // Reconfiguration sound after a brief non-blocking delay
        // (it will be played on the next updateEffects iteration)
        hw.sound().playOnce("/reshuffle.mp3");
    }

    void resetPuzzle() {
        Serial.println(F("[REACTOR] Resetting puzzle..."));

        // Lock
        hw.digitalWrite(LOCK_PIN, HIGH);

        // Zero gauges
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            hw.setAnalogOutput(PCA_CHANNELS[i], 0.0f);
        }

        // Clear LEDs
        hw.setLightAll(0);

        // Reset state
        subState_ = SubState::Adjusting;
        zonesActive_ = 0;
        buttonsConfirmed_ = 0;
        effectPhase_ = EffectPhase::None;
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            inZone_[i] = false;
            buttonPressedInWindow_[i] = false;
        }

        // Reshuffle for a new round
        randomizeMapping();

        setState(PuzzleState::Idle);
    }
};

// ============================================================================
// SwitchInput CALLBACKS (outside the class)
// ============================================================================
volatile bool btnPressed[NUM_VALVES] = {false, false, false};

void onButtonPressed(pinid_t pin, bool held) {
    if (held) return;

    int idx = pin - BUTTON_1_PIN;
    if (idx >= 0 && idx < NUM_VALVES) {
        btnPressed[idx] = true;
        Serial.printf("[BTN] Button %d (P%d) PRESSED\n", idx + 1, pin);
    }
}

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(31, true);  // ID 31, with RS485 network enabled
ReactorControlPuzzle puzzle;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    Serial.println(F("============================================"));
    Serial.println(F("  ReactorControl - PropMaker EscapeRoom"));
    Serial.println(F("  Nuclear Control Panel"));
    Serial.println(F("  April 2026 - Nicolau dos Brinquedos"));
    Serial.println(F("============================================"));

    hw.begin();
    hw.addPcf8575(PCF_ADDRESS);  // Adds PCF expander

    master.addPuzzle(&puzzle);
    master.begin();
    // RS485 is not available on the ESP32 port; remote control is via MQTT/Home Assistant.

    Serial.println(F("[Main] System ready"));
    Serial.println(F("[Main] Waiting for start command (network or button 1)"));
}

void loop() {
  loopHA();
    master.update();
    delay(1);
}
