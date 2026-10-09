/**
 * EXAMPLE: SIMON SAYS - GENIUS GAME
 * ------------------------------------------------------
 * Implementation of the classic Simon Says (Genius) game for Escape Room.
 * 4 colored buttons with NeoPixel rings, each with a distinct sound.
 *
 * MAIN FEATURE: No punishment for mistakes!
 * If the player makes a mistake, the same sequence is repeated until it is correct.
 *
 * HARDWARE: Adafruit PropMaker Feather RP2040 + PCF8574
 * - PCF8574 I2C address: 0x20
 * - Buttons: P0-P3 (active LOW, INPUT_PULLUP)
 * - NeoPixel: 48 LEDs (4 rings of 12 LEDs each)
 * - Lock: PIN_EXTERNAL_POWER
 *
 * REQUIRED AUDIO (LittleFS or SD):
 * - ready.mp3    : System ready
 * - begin.mp3    : Game start
 * - red.mp3      : Red button sound
 * - blue.mp3     : Blue button sound
 * - yellow.mp3   : Yellow button sound
 * - green.mp3    : Green button sound
 * - correct.mp3  : Correct sequence
 * - error.mp3    : Sequence error
 * - victory.mp3  : Victory (puzzle solved)
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

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
// CONFIGURATION
// ============================================================================
#define PCF_ADDRESS         0x20

// Buttons (P0-P3 on the PCF8574)
#define BUTTON_RED_PIN      0
#define BUTTON_BLUE_PIN     1
#define BUTTON_YELLOW_PIN   2
#define BUTTON_GREEN_PIN    3

// Game constants
#define BUTTONS             4
#define NUM_LEDS_PER_RING   12
#define TOTAL_LEDS          (NUM_LEDS_PER_RING * BUTTONS)
#define MIN_LEVELS          2
#define MAX_LEVELS          7
#define TIMEOUT             5000UL
#define MAX_INTERVAL        800
#define MIN_INTERVAL        300
#define RESET_HOLD_TIME     2000UL
#define RAINBOW_SPEED       15

// RGB colors
const uint32_t COLORS[BUTTONS] = {
    Adafruit_NeoPixel::Color(255, 0, 0),     // Red
    Adafruit_NeoPixel::Color(0, 0, 255),     // Blue
    Adafruit_NeoPixel::Color(255, 215, 0),   // Yellow/Gold
    Adafruit_NeoPixel::Color(0, 128, 0)      // Green
};

// Color names
const char* COLOR_NAMES[BUTTONS] = {"Red", "Blue", "Yellow", "Green"};

// Sound files
const char* COLOR_SOUNDS[BUTTONS] = {"red.mp3", "blue.mp3", "yellow.mp3", "green.mp3"};

// Game states
enum GameStatus {
    Initialising,
    WaitCommand,
    Overture,
    Running,
    Solved,
    WaitForReset
};

// Color enum
enum Color {
    COLOR_RED = 0,
    COLOR_BLUE = 1,
    COLOR_YELLOW = 2,
    COLOR_GREEN = 3
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================
GameStatus gameStatus = Initialising;
int gameSequence[MAX_LEVELS];
int currentLevel = MIN_LEVELS;
int gameSpeed = MAX_INTERVAL;
int correctMoves = 0;
bool rainbowActive = false;
uint8_t gHue = 0;
unsigned long lastRainbowUpdate = 0;

// Button callbacks
void onButtonPressed(pinid_t pin, bool held);

// ============================================================================
// SIMON SAYS PUZZLE CLASS
// ============================================================================
class SimonSaysPuzzle : public PuzzleBase {
public:
    SimonSaysPuzzle() : PuzzleBase(20) {}  // ID 20 on the RS485 network

    void setupHardware() override {
        Serial.println(F("\n========================================"));
        Serial.println(F("  Simon Says - Genius Game"));
        Serial.println(F("========================================"));

        // Initializes SwitchInput
        if (hw.hasPortExpander()) {
            Serial.println(F("[SIMON] Initializing SwitchInput..."));
            switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);

            // Adds buttons with callback
            switches.addSwitch(BUTTON_RED_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_BLUE_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_YELLOW_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_GREEN_PIN, onButtonPressed, NO_REPEAT, false);

            Serial.println(F("[SIMON] 4 buttons configured"));
        } else {
            Serial.println(F("[SIMON] ERROR: PCF8574 not detected!"));
        }

        // Initializes the external NeoPixels
        hw.pixelsExternal().updateLength(TOTAL_LEDS);
        hw.pixelsExternal().begin();
        hw.pixelsExternal().setBrightness(70);
        hw.pixelsExternal().clear();
        hw.pixelsExternal().show();

        // Configures the lock
        hw.pinMode(PIN_EXTERNAL_POWER, OUTPUT);
        hw.digitalWrite(PIN_EXTERNAL_POWER, LOW);  // Locked

        // Generates the game sequence
        generateGameSequence();

        // Plays the ready sound
        hw.sound().playOnce("ready.mp3");

        gameStatus = WaitCommand;
        Serial.println(F("[SIMON] Ready - press any button to start"));
    }

    void update() override {
        // Processes audio
        hw.sound().loop();

        // Processes inputs
        switches.runLoop();

        // State machine
        switch (gameStatus) {
            case WaitCommand:
                handleWaitCommand();
                break;
            case Overture:
                handleOverture();
                break;
            case Running:
                handleRunning();
                break;
            case Solved:
                handleSolved();
                break;
            case WaitForReset:
                handleWaitForReset();
                break;
            default:
                break;
        }

        // Updates the rainbow animation if active
        if (rainbowActive) {
            updateRainbow();
        }
    }

private:
    // ------------------------------------------------------------------------
    // STATE HANDLERS
    // ------------------------------------------------------------------------
    void handleWaitCommand() {
        hw.digitalWrite(PIN_EXTERNAL_POWER, LOW);  // Ensures locked

        // Waits for any button
        if (switches.isSwitchPressed(BUTTON_RED_PIN) ||
            switches.isSwitchPressed(BUTTON_BLUE_PIN) ||
            switches.isSwitchPressed(BUTTON_YELLOW_PIN) ||
            switches.isSwitchPressed(BUTTON_GREEN_PIN)) {
            delay(300);
            gameStatus = Overture;
        }
    }

    void handleOverture() {
        Serial.println(F("[SIMON] Starting game..."));
        hw.sound().playOnce("begin.mp3");

        // Lights all rings
        for (int i = 0; i < TOTAL_LEDS; i++) {
            int ring = i / NUM_LEDS_PER_RING;
            hw.pixelsExternal().setPixelColor(i, COLORS[ring]);
        }
        hw.pixelsExternal().show();
        delay(1000);

        // Turns everything off
        hw.pixelsExternal().clear();
        hw.pixelsExternal().show();
        delay(1000);

        gameStatus = Running;
    }

    void handleRunning() {
        hw.digitalWrite(PIN_EXTERNAL_POWER, LOW);  // Locked
        playGame();
    }

    void handleSolved() {
        Serial.println(F("[SIMON] PUZZLE SOLVED! Box open."));
        hw.sound().playOnce("victory.mp3");
        startRainbow();
        hw.digitalWrite(PIN_EXTERNAL_POWER, HIGH);  // Opens the lock

        delay(5000);  // Waits for stabilization
        gameStatus = WaitForReset;
    }

    void handleWaitForReset() {
        static unsigned long pressStart = 0;
        static bool wasPressed = false;

        bool anyPressed = switches.isSwitchPressed(BUTTON_RED_PIN) ||
                         switches.isSwitchPressed(BUTTON_BLUE_PIN) ||
                         switches.isSwitchPressed(BUTTON_YELLOW_PIN) ||
                         switches.isSwitchPressed(BUTTON_GREEN_PIN);

        if (anyPressed) {
            if (!wasPressed) {
                pressStart = millis();
                wasPressed = true;
            }
            if (millis() - pressStart >= RESET_HOLD_TIME) {
                stopRainbow();
                resetAnimation();
                resetGame();
                wasPressed = false;
            }
        } else {
            wasPressed = false;
            pressStart = 0;
        }
    }

    // ------------------------------------------------------------------------
    // GAME LOGIC
    // ------------------------------------------------------------------------
    void playGame() {
        while (gameStatus == Running) {
            hw.sound().loop();

            // Shows the sequence
            showSequence();
            delay(500);

            // Waits for the player to repeat it
            int result = checkPlayerInput();

            // If wrong, repeats the same sequence (no punishment!)
            while (result == 0 && gameStatus == Running) {
                hw.sound().playOnce("error.mp3");
                delay(1000);
                showSequence();
                delay(500);
                result = checkPlayerInput();
            }

            // If fully correct
            if (result == 1) {
                hw.sound().playOnce("correct.mp3");
                delay(500);

                if (currentLevel < MAX_LEVELS) {
                    currentLevel++;
                    gameSpeed = calculateSpeed(currentLevel);
                }

                if (currentLevel == MAX_LEVELS) {
                    gameStatus = Solved;
                }
            }
        }
    }

    void showSequence() {
        Serial.print(F("[SIMON] Sequence: "));
        for (int i = 0; i < currentLevel; i++) {
            Serial.print(COLOR_NAMES[gameSequence[i]]);
            Serial.print(F(" "));
            flashRing(gameSequence[i], gameSpeed);
        }
        Serial.println();
        delay(gameSpeed);
    }

    int checkPlayerInput() {
        for (int i = 0; i < currentLevel; i++) {
            int button = -1;
            unsigned long startTime = millis();

            // Waits for a button or timeout
            while (button == -1) {
                hw.sound().loop();
                switches.runLoop();

                if (switches.isSwitchPressed(BUTTON_RED_PIN)) {
                    button = COLOR_RED;
                } else if (switches.isSwitchPressed(BUTTON_BLUE_PIN)) {
                    button = COLOR_BLUE;
                } else if (switches.isSwitchPressed(BUTTON_YELLOW_PIN)) {
                    button = COLOR_YELLOW;
                } else if (switches.isSwitchPressed(BUTTON_GREEN_PIN)) {
                    button = COLOR_GREEN;
                }

                // Timeout
                if (millis() - startTime >= TIMEOUT) {
                    return 0;
                }
            }

            // Visual/sound feedback
            flashRing(button, 500);

            // Checks if it was correct
            if (button != gameSequence[i]) {
                return 0;  // Wrong
            }

            correctMoves++;
        }

        correctMoves = 0;
        return 1;  // All correct
    }

    // ------------------------------------------------------------------------
    // VISUAL EFFECTS
    // ------------------------------------------------------------------------
    void flashRing(int colorIdx, int duration) {
        // Lights the ring
        for (int i = colorIdx * NUM_LEDS_PER_RING;
             i < (colorIdx + 1) * NUM_LEDS_PER_RING;
             i++) {
            hw.pixelsExternal().setPixelColor(i, COLORS[colorIdx]);
        }
        hw.pixelsExternal().show();

        // Plays the sound
        hw.sound().playOnce(COLOR_SOUNDS[colorIdx]);
        delay(duration);

        // Turns the ring off
        for (int i = colorIdx * NUM_LEDS_PER_RING;
             i < (colorIdx + 1) * NUM_LEDS_PER_RING;
             i++) {
            hw.pixelsExternal().setPixelColor(i, 0);
        }
        hw.pixelsExternal().show();
        delay(duration);
    }

    void startRainbow() {
        rainbowActive = true;
        gHue = 0;
        lastRainbowUpdate = millis();
        Serial.println(F("[SIMON] Rainbow started"));
    }

    void stopRainbow() {
        rainbowActive = false;
        hw.pixelsExternal().clear();
        hw.pixelsExternal().show();
    }

    void updateRainbow() {
        if (!rainbowActive) return;

        unsigned long now = millis();
        if (now - lastRainbowUpdate < RAINBOW_SPEED) return;

        lastRainbowUpdate = now;

        // Turns everything off
        hw.pixelsExternal().clear();

        // For each ring, lights 1 LED with a rainbow color
        for (int ring = 0; ring < BUTTONS; ring++) {
            int ledPos = (ring * NUM_LEDS_PER_RING) + (gHue % NUM_LEDS_PER_RING);

            // Converts HSV to RGB
            uint8_t r, g, b;
            hsvToRgb(gHue + (ring * 64), 255, 255, r, g, b);
            hw.pixelsExternal().setPixelColor(ledPos, r, g, b);
        }

        hw.pixelsExternal().show();
        gHue++;
    }

    void resetAnimation() {
        // Individual cycle
        for (int cycle = 0; cycle < BUTTONS; cycle++) {
            for (int i = 0; i < NUM_LEDS_PER_RING; i++) {
                int ledIdx = (cycle * NUM_LEDS_PER_RING) + i;
                hw.pixelsExternal().setPixelColor(ledIdx, COLORS[cycle]);
                hw.pixelsExternal().show();
                delay(50);
                hw.pixelsExternal().setPixelColor(ledIdx, 0);
                hw.pixelsExternal().show();
                delay(50);
            }
        }

        // Two blinks
        for (int p = 0; p < 2; p++) {
            for (int i = 0; i < TOTAL_LEDS; i++) {
                int ring = i / NUM_LEDS_PER_RING;
                hw.pixelsExternal().setPixelColor(i, COLORS[ring]);
            }
            hw.pixelsExternal().show();
            delay(150);
            hw.pixelsExternal().clear();
            hw.pixelsExternal().show();
            delay(150);
        }
    }

    // ------------------------------------------------------------------------
    // GAME MANAGEMENT
    // ------------------------------------------------------------------------
    void resetGame() {
        currentLevel = MIN_LEVELS;
        gameSpeed = MAX_INTERVAL;
        generateGameSequence();
        delay(1000);
        hw.sound().playOnce("ready.mp3");
        delay(500);
        gameStatus = WaitCommand;
        Serial.println(F("[SIMON] Game reset"));
    }

    void generateGameSequence() {
        // Base sequence
        int baseSequence[BUTTONS] = {COLOR_RED, COLOR_BLUE, COLOR_YELLOW, COLOR_GREEN};

        // Fills the repetition
        for (int i = 0; i < MAX_LEVELS; i++) {
            gameSequence[i] = baseSequence[i % BUTTONS];
        }

        // Avoids consecutive repetitions
        for (int i = 1; i < MAX_LEVELS; i++) {
            if (gameSequence[i] == gameSequence[i - 1]) {
                for (int j = i + 1; j < MAX_LEVELS; j++) {
                    if (gameSequence[j] != gameSequence[i - 1]) {
                        int temp = gameSequence[i];
                        gameSequence[i] = gameSequence[j];
                        gameSequence[j] = temp;
                        break;
                    }
                }
            }
        }

        // Additional shuffle
        bubbleUnsort(gameSequence, MAX_LEVELS);

        // Final check
        for (int i = 1; i < MAX_LEVELS; i++) {
            if (gameSequence[i] == gameSequence[i - 1]) {
                for (int j = i + 1; j < MAX_LEVELS; j++) {
                    if (gameSequence[j] != gameSequence[i - 1]) {
                        int temp = gameSequence[i];
                        gameSequence[i] = gameSequence[j];
                        gameSequence[j] = temp;
                        break;
                    }
                }
            }
        }
    }

    int calculateSpeed(int level) {
        int levelsProgress = MAX_LEVELS - MIN_LEVELS;
        int totalReduction = MAX_INTERVAL - MIN_INTERVAL;
        float reductionPerLevel = (float)totalReduction / levelsProgress;
        int speed = MAX_INTERVAL - (int)((level - MIN_LEVELS) * reductionPerLevel);
        return constrain(speed, MIN_INTERVAL, MAX_INTERVAL);
    }

    // ------------------------------------------------------------------------
    // UTILITIES
    // ------------------------------------------------------------------------
    void bubbleUnsort(int* list, int elem) {
        for (int a = elem - 1; a > 0; a--) {
            int r = random(a + 1);
            if (r != a) {
                int temp = list[a];
                list[a] = list[r];
                list[r] = temp;
            }
        }
    }

    void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
        uint8_t region = h / 43;
        uint8_t remainder = (h - (region * 43)) * 6;
        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        switch (region) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }
};

// ============================================================================
// GLOBAL CALLBACKS
// ============================================================================
void onButtonPressed(pinid_t pin, bool held) {
    // Callback required for SwitchInput
    // The main logic is in checkPlayerInput
}

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(20);
SimonSaysPuzzle puzzle;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    hw.begin();
    hw.addPcf8574(PCF_ADDRESS);  // Initializes PCF8574 (8 pins, using P0-P3 for 4 buttons)

    master.addPuzzle(&puzzle);
    master.begin();
      initHA();
  puzzle.run();
}

void loop() {
  loopHA();
    master.update();
}
