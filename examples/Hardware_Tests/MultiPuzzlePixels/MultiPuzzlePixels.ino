/**
 * EXAMPLE: MULTI PUZZLE PIXELS
 * ------------------------------------------------------
 * Demonstrates the use of multiple puzzles on the same PropMaker board,
 * each one with its own defined NeoPixel address range.
 *
 * Configuration:
 * - Puzzle 1 (PuzzleOne): LEDs 0-9 (10 LEDs) - Rainbow fade effect
 * - Puzzle 2 (PuzzleTwo): LEDs 11-18 (8 LEDs) - Sequential chase effect
 * - Puzzle 3 (PuzzleThree): LEDs 20-25 (6 LEDs) - Breathing effect
 *
 * The ranges are contiguous: (0,10), (11,8), (20,6)
 * No overlap, each puzzle controls only its own range.
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

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
// PUZZLE 1: Rainbow Fade Effect
// ============================================================================
class PuzzleOne : public PuzzleBase {
    uint8_t hue_ = 0;
    unsigned long lastUpdate_ = 0;

public:
    PuzzleOne() : PuzzleBase(1) {  // ID 1
        // Defines the pixel range in the constructor
        setPixelRange(0, 10);  // LEDs 0-9
    }

    void setupHardware() override {
        Serial.println(F("[PUZZLE 1] Setup - Rainbow Fade"));
        setState(PuzzleState::Idle);
    }

    void update() override {
        // Only runs if it is Running
        if (getState() != PuzzleState::Running) return;

        // Updates every 50ms
        if (millis() - lastUpdate_ < 50) return;
        lastUpdate_ = millis();

        // Rotating rainbow effect using LOCAL indices (0-9)
        for (uint8_t i = 0; i < pixels()->count(); i++) {
            uint8_t pixelHue = hue_ + (i * 25);  // Color offset per pixel
            pixels()->setPixel(i, wheel(pixelHue));
        }
        pixels()->show();

        hue_++;  // Advances the hue
    }

    // Helper to generate rainbow colors
    uint32_t wheel(uint8_t wheelPos) {
        wheelPos = 255 - wheelPos;
        if (wheelPos < 85) {
            return pixels()->Color(255 - wheelPos * 3, 0, wheelPos * 3);
        } else if (wheelPos < 170) {
            wheelPos -= 85;
            return pixels()->Color(0, wheelPos * 3, 255 - wheelPos * 3);
        } else {
            wheelPos -= 170;
            return pixels()->Color(wheelPos * 3, 255 - wheelPos * 3, 0);
        }
    }
};

// ============================================================================
// PUZZLE 2: Sequential Chase Effect
// ============================================================================
class PuzzleTwo : public PuzzleBase {
    uint8_t position_ = 0;
    uint32_t colors_[3];
    unsigned long lastUpdate_ = 0;

public:
    PuzzleTwo() : PuzzleBase(2) {  // ID 2
        // Defines the pixel range - contiguous with Puzzle 1
        setPixelRange(11, 8);  // LEDs 11-18
    }

    void setupHardware() override {
        Serial.println(F("[PUZZLE 2] Setup - Chase Sequence"));

        // Colors for the chase effect
        colors_[0] = pixels()->Color(255, 0, 0);    // Red
        colors_[1] = pixels()->Color(0, 255, 0);    // Green
        colors_[2] = pixels()->Color(0, 0, 255);    // Blue

        setState(PuzzleState::Idle);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        // Updates every 100ms
        if (millis() - lastUpdate_ < 100) return;
        lastUpdate_ = millis();

        // Chase effect: lights 3 pixels at a time, shifting along
        pixels()->clear();

        for (uint8_t i = 0; i < 3; i++) {
            uint8_t pixelIdx = (position_ + i) % pixels()->count();
            pixels()->setPixel(pixelIdx, colors_[i]);
        }
        pixels()->show();

        position_ = (position_ + 1) % pixels()->count();
    }
};

// ============================================================================
// PUZZLE 3: Breathing Effect
// ============================================================================
class PuzzleThree : public PuzzleBase {
    uint8_t brightness_ = 0;
    int8_t direction_ = 1;
    unsigned long lastUpdate_ = 0;
    uint32_t breatheColor_;

public:
    PuzzleThree() : PuzzleBase(3) {  // ID 3
        // Defines the pixel range - contiguous with the previous ones
        setPixelRange(20, 6);  // LEDs 20-25
    }

    void setupHardware() override {
        Serial.println(F("[PUZZLE 3] Setup - Breathing Effect"));

        // Amber color for the breathing
        breatheColor_ = pixels()->Color(255, 100, 0);

        setState(PuzzleState::Idle);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        // Updates every 30ms for smoothness
        if (millis() - lastUpdate_ < 30) return;
        lastUpdate_ = millis();

        // Computes brightness with a sine curve
        float phase = (float)brightness_ / 255.0f * PI;
        uint8_t intensity = (uint8_t)(sin(phase) * 255);

        // Fills all pixels with the computed intensity
        uint32_t dimmedColor = dimColor(breatheColor_, intensity);
        pixels()->setAll(dimmedColor);
        pixels()->show();

        // Updates brightness for the next frame
        brightness_ += direction_ * 5;
        if (brightness_ >= 255 || brightness_ == 0) {
            direction_ *= -1;  // Reverses direction
        }
    }

    // Dims the intensity of a color
    uint32_t dimColor(uint32_t color, uint8_t intensity) {
        uint8_t r = (uint8_t)(color >> 16);
        uint8_t g = (uint8_t)(color >> 8);
        uint8_t b = (uint8_t)color;

        r = (r * intensity) / 255;
        g = (g * intensity) / 255;
        b = (b * intensity) / 255;

        return pixels()->Color(r, g, b);
    }
};

// ============================================================================
// MAIN
// ============================================================================

// Puzzle instances
PuzzleOne puzzle1;
PuzzleTwo puzzle2;
PuzzleThree puzzle3;

// LogicManager coordinates multiple puzzles (no RS485 in this example)
LogicManager master(0, false);

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    Serial.println(F("\n========================================"));
    Serial.println(F("   Multi Puzzle Pixels Demo"));
    Serial.println(F("========================================"));

    // Initializes hardware
    hw.begin();

    // Adds PCF8575 (if present)
    // hw.addPcf8575(0x20);

    // Registers puzzles with the LogicManager
    master.addPuzzle(&puzzle1);
    master.addPuzzle(&puzzle2);
    master.addPuzzle(&puzzle3);

    Serial.println(F("\nPixel Configuration:"));
    Serial.printf("  Puzzle 1: offset=%d, count=%d (LEDs %d-%d)\n",
                  puzzle1.pixelOffset(), puzzle1.pixelCount(),
                  puzzle1.pixelOffset(), puzzle1.pixelOffset() + puzzle1.pixelCount() - 1);
    Serial.printf("  Puzzle 2: offset=%d, count=%d (LEDs %d-%d)\n",
                  puzzle2.pixelOffset(), puzzle2.pixelCount(),
                  puzzle2.pixelOffset(), puzzle2.pixelOffset() + puzzle2.pixelCount() - 1);
    Serial.printf("  Puzzle 3: offset=%d, count=%d (LEDs %d-%d)\n",
                  puzzle3.pixelOffset(), puzzle3.pixelCount(),
                  puzzle3.pixelOffset(), puzzle3.pixelOffset() + puzzle3.pixelCount() - 1);

    // Initializes the LogicManager (calls begin() on each puzzle)
    master.begin();

    // Starts all puzzles automatically
    puzzle1.run();
    puzzle2.run();
    puzzle3.run();

    Serial.println(F("\nPuzzles started - Effects running!"));
    Serial.println(F("Each puzzle controls its own isolated range of LEDs."));
}

void loop() {
  loopHA();
    // Updates all puzzles via LogicManager
    master.update();

    // Small delay to avoid saturation
    delay(1);
}
