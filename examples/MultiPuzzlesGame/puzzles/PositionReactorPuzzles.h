// PositionReactorPuzzles.h - Puzzles 4 and 5 of the Nuclear Reactor
// Level 1: Positioning (P4) - RFID on the platform bases
// Level 2: Reactor (P5) - ReactorControl with 3 pots + ADS1115

#pragma once

#include <ESP32EscapeRoom.h>

using namespace esp32_escaperoom;

// ====================================================================
// P4: RFID POSITIONING - Position technicians on the platforms
// ====================================================================
// Hardware: 3 CH552G modules + MFRC522 (one per base A, B, C)
// Connection: each CH552G becomes a digital sensor on the PCF (LOW = tag OK).
// Mechanics: place the right badge in the right place.
// When all 3 are in the correct position, the puzzle is solved.

class PositionRFIDPuzzle : public PuzzleBase {
public:
    PositionRFIDPuzzle(uint8_t pcfIndex)
        : PuzzleBase(ID_P4_POSITION_RFID), pcfIndex_(pcfIndex) {}

    void setupHardware() override {
        setPixelRange(13, 3);  // LEDs 13-15

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) {
            Serial.println("[P4] PCF NOT FOUND!");
            return;
        }

        // P0-P2: inputs from the CH552G modules (LOW = valid tag)
        for (uint8_t i = 0; i < 3; i++) {
            pcf->pinMode(i, INPUT_PULLUP);
        }

        Serial.printf("[P4 PositionRFID] Setup PCF[%d] bases A,B,C on P0-P2\n", pcfIndex_);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        if (millis() - lastPoll_ < 200) return;
        lastPoll_ = millis();

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        bool allCorrect = true;

        for (uint8_t i = 0; i < 3; i++) {
            if (positionCorrect_[i]) continue;

            // CH552G output: LOW = valid tag, HIGH = denied/nothing
            if (pcf->digitalRead(i) == LOW) {
                positionCorrect_[i] = true;
                Serial.printf("[P4] Tech positioned at platform %c!\n", 'A' + i);
                if (pixels()) {
                    pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                    pixels()->show();
                }
            } else {
                allCorrect = false;
            }
        }

        if (allCorrect && positionCorrect_[0] && positionCorrect_[1] && positionCorrect_[2]) {
            if (!solved_) {
                solved_ = true;
                solveStart_ = millis();
                Serial.println("[P4] ALL TECHS POSITIONED!");

                if (pixels()) {
                    for (uint8_t i = 0; i < pixels()->count(); i++)
                        pixels()->setPixel(i, pixels()->Color(0, 255, 255));
                    pixels()->show();
                }
            }

            if (millis() - solveStart_ > 1500) {
                setState(PuzzleState::Solved);
                Serial.println("[P4] SOLVED");
            }
        }
    }

private:
    uint8_t pcfIndex_;
    bool positionCorrect_[3] = {false, false, false};
    unsigned long lastPoll_ = 0;
    bool solved_ = false;
    unsigned long solveStart_ = 0;
};

// ====================================================================
// P5: REACTOR CONTROL - Stabilize the reactor core
// ====================================================================
// Hardware: 3 pots (ADS1115 channels A0, A1, A2), NeoPixel
// Mechanics: each pot controls a subsystem.
// Solved when all 3 stay within range for 5 continuous seconds.

class ReactorControlPuzzle : public PuzzleBase {
public:
    ReactorControlPuzzle() : PuzzleBase(ID_P5_REACTOR_CTRL) {}

    void setupHardware() override {
        setPixelRange(16, 7);  // LEDs 16-22

        if (hw.hasAdc()) {
            Serial.println("[P5] ADS1115 OK");
        } else {
            Serial.println("[P5] ADS1115 NOT FOUND!");
        }

        Serial.println("[P5 ReactorControl] Setup complete");
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        if (millis() - lastRead_ < 200) return;
        lastRead_ = millis();

        if (!hw.hasAdc()) return;

        int raw0 = hw.readAnalogRaw(0);
        int raw1 = hw.readAnalogRaw(1);
        int raw2 = hw.readAnalogRaw(2);

        float tempC = map(raw0, 0, 65535, 200, 1200);
        float pressure = map(raw1, 0, 65535, 0, 100);
        float rpm = map(raw2, 0, 65535, 500, 3000);

        // Simulated drift
        drift_ += 0.05;
        float driftTemp = sin(drift_ * 0.7) * 80;
        float driftPress = sin(drift_ * 0.5 + 1.0) * 12;
        float driftRpm = sin(drift_ * 0.6 + 2.0) * 300;

        float effTemp = tempC + driftTemp;
        float effPress = pressure + driftPress;
        float effRpm = rpm + driftRpm;

        bool tempOk = (effTemp < 750);
        bool pressOk = (effPress >= 40 && effPress <= 70);
        bool rpmOk = (effRpm >= 1400 && effRpm <= 1600);

        if (pixels()) {
            pixels()->setPixel(0, tempOk ? pixels()->Color(0, 255, 0) : pixels()->Color(255, 0, 0));
            pixels()->setPixel(2, pressOk ? pixels()->Color(0, 255, 255) : pixels()->Color(255, 0, 0));
            pixels()->setPixel(4, rpmOk ? pixels()->Color(255, 255, 0) : pixels()->Color(255, 0, 0));
            pixels()->setPixel(6, (tempOk && pressOk && rpmOk) ? pixels()->Color(0, 255, 0) : pixels()->Color(50, 0, 0));
            pixels()->show();
        }

        static unsigned long lastDebug = 0;
        if (millis() - lastDebug > 2000) {
            lastDebug = millis();
            Serial.printf("[P5] Temp=%.0fC %s | Press=%.0f%% %s | RPM=%.0f %s\n",
                          effTemp, tempOk ? "OK" : "HIGH",
                          effPress, pressOk ? "OK" : "BAD",
                          effRpm, rpmOk ? "OK" : "BAD");
        }

        if (tempOk && pressOk && rpmOk) {
            if (!stable_) {
                stable_ = true;
                stableStart_ = millis();
                Serial.println("[P5] All systems NOMINAL!");
            }
            if (millis() - stableStart_ > 5000) {
                if (!solved_) {
                    solved_ = true;
                    Serial.println("[P5] REACTOR STABILIZED!");
                    if (pixels()) {
                        for (uint8_t i = 0; i < pixels()->count(); i++)
                            pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                        pixels()->show();
                    }
                    hw.sound().playOnce("/sfx/reactor_stable.mp3");
                }
                setState(PuzzleState::Solved);
            }
        } else {
            stable_ = false;
            stableStart_ = 0;
            solved_ = false;
        }
    }

private:
    float drift_ = 0;
    unsigned long lastRead_ = 0;
    bool stable_ = false;
    unsigned long stableStart_ = 0;
    bool solved_ = false;
};
