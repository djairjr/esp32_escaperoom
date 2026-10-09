// InteriorPuzzles.h - Puzzles 6, 7, 8, 9 of the Nuclear Reactor
// Level 3: Lid (P6 SimonSays) - PARALLEL
// Level 4: Interior (P7 RadioRepair, P8 RotaryDial, P9 BombTimer) - SEQUENCE
//
// ALL puzzles are NON-BLOCKING (state machines in update())
// so they run under LogicManager without blocking the loop.

#pragma once

#include <ESP32EscapeRoom.h>

using namespace esp32_escaperoom;

// ====================================================================
// P6: SIMON SAYS ON THE LID - Pressurize pumps
// ====================================================================
// Hardware: 4 buttons + NeoPixels on the lid, servo to open
// Mechanics: repeat the light sequence. N rounds, the sequence grows.
// Non-blocking: state machine with millis(), no delay() or while().

class LidSimonSaysPuzzle : public PuzzleBase {
public:
    LidSimonSaysPuzzle(uint8_t pcfIndex, uint8_t servoChannel = 3)
        : PuzzleBase(ID_P6_LID_SIMONSAYS), pcfIndex_(pcfIndex), servoCh_(servoChannel) {}

    void setupHardware() override {
        setPixelRange(23, 6);  // LEDs 23-28 (4 buttons + 2 status)

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        // 4 buttons on pins 0-3
        for (uint8_t i = 0; i < 4; i++) {
            pcf->pinMode(i, INPUT_PULLUP);
        }

        // Lid relay (or PCA9685)
        lockPin_ = 8;
        pcf->pinMode(lockPin_, OUTPUT);
        pcf->digitalWrite(lockPin_, HIGH);

        // Generate a 5-round sequence
        generateRandomSequence(sequence_, ROUNDS, 3);
        Serial.printf("[P6 SimonSays] %d rounds: ", ROUNDS);
        for (uint8_t i = 0; i < ROUNDS; i++) Serial.printf("%d ", sequence_[i]);
        Serial.println();

        Serial.printf("[P6 SimonSays] Setup PCF[%d]\n", pcfIndex_);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        switch (gameState_) {
        case SS_IDLE:
            // Show "ready" on pixels
            if (pixels()) {
                for (uint8_t i = 0; i < 4; i++)
                    pixels()->setPixel(i, pixels()->Color(50, 0, 50));
                pixels()->show();
            }
            // Wait for any button to start
            if (anyButtonPressed()) {
                gameState_ = SS_SHOWING;
                showStep_ = 0;
                stateStart_ = millis();
                Serial.println("[P6] Starting sequence...");
            }
            break;

        case SS_SHOWING:
            // Show sequence one LED at a time with 600ms intervals
            if (millis() - stateStart_ >= 600) {
                stateStart_ = millis();
                if (showStep_ < ROUNDS) {
                    flashLed(sequence_[showStep_]);
                    showStep_++;
                } else {
                    // Done showing, now listen for player input
                    gameState_ = SS_LISTENING;
                    playerStep_ = 0;
                    Serial.println("[P6] Your turn!");
                }
            }
            break;

        case SS_LISTENING:
            if (millis() - lastDebounce_ < 200) break;
            lastDebounce_ = millis();

            for (uint8_t b = 0; b < 4; b++) {
                if (isButtonPressed(b)) {
                    flashLed(b);
                    if (b == sequence_[playerStep_]) {
                        // Correct!
                        playerStep_++;
                        if (playerStep_ >= ROUNDS) {
                            // ALL CORRECT! Solved
                            gameState_ = SS_SOLVED;
                            stateStart_ = millis();
                            Serial.println("[P6] ALL ROUNDS COMPLETE!");
                            if (pixels()) {
                                for (uint8_t i = 0; i < pixels()->count(); i++)
                                    pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                                pixels()->show();
                            }
                        }
                    } else {
                        // Wrong! Reset sequence
                        Serial.printf("[P6] Wrong button! Expected %d, got %d\n",
                                      sequence_[playerStep_], b);
                        gameState_ = SS_IDLE;
                        if (pixels()) {
                            for (uint8_t i = 0; i < 4; i++)
                                pixels()->setPixel(i, pixels()->Color(255, 0, 0));
                            pixels()->show();
                        }
                        delay(500);  // Brief error flash (acceptable - single puzzle in level)
                        if (pixels()) {
                            for (uint8_t i = 0; i < 4; i++)
                                pixels()->setPixel(i, 0);
                            pixels()->show();
                        }
                    }
                    break;
                }
            }
            break;

        case SS_SOLVED:
            if (millis() - stateStart_ > 2000) {
                openLid();
                hw.sound().playOnce("/sfx/lid_open.mp3");
                setState(PuzzleState::Solved);
            }
            break;
        }
    }

private:
    uint8_t pcfIndex_;
    uint8_t servoCh_;
    uint8_t lockPin_;

    static const uint8_t ROUNDS = 5;
    uint8_t sequence_[ROUNDS];
    uint8_t showStep_ = 0;
    uint8_t playerStep_ = 0;

    enum { SS_IDLE, SS_SHOWING, SS_LISTENING, SS_SOLVED } gameState_ = SS_IDLE;

    unsigned long stateStart_ = 0;
    unsigned long lastDebounce_ = 0;

    bool isButtonPressed(uint8_t b) {
        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return false;
        return (pcf->digitalRead(b) == LOW);
    }

    bool anyButtonPressed() {
        for (uint8_t i = 0; i < 4; i++) {
            if (isButtonPressed(i)) return true;
        }
        return false;
    }

    void flashLed(uint8_t idx) {
        if (!pixels()) return;
        for (uint8_t i = 0; i < 4; i++)
            pixels()->setPixel(i, 0);
        uint32_t c = 0;
        switch (idx) {
            case 0: c = pixels()->Color(255, 0, 0); break;   // Red
            case 1: c = pixels()->Color(0, 255, 0); break;   // Green
            case 2: c = pixels()->Color(0, 0, 255); break;   // Blue
            case 3: c = pixels()->Color(255, 255, 0); break; // Yellow
        }
        pixels()->setPixel(idx, c);
        pixels()->show();
    }

    void openLid() {
        Serial.println("[P6] Lid OPEN via servo");
        // If PCA9685 available, use it
        if (hw.hasPwmDriver()) {
            // Opening ramp (non-blocking, so just set target)
            for (uint8_t a = 0; a <= 180; a += 5) {
                uint16_t pulse = map(a, 0, 180, 1000, 2000);
                hw.pwmDriver().setChannelServoPulseDuration(servoCh_, pulse);
                delay(50);
            }
        }
    }
};

// ====================================================================
// P7: RADIO REPAIR - Repair the field communicator
// ====================================================================
// Hardware: 16x2 LCD, 2 buttons, 2 toggle switches, NeoPixel
// Mechanics: insert modules in order, set switches, press test
// 3 sequential steps. At the end, the radio is repaired.
//
// Note: simplified version - uses Serial for feedback.
// In production, replace with a 16x2 LCD.

class RadioRepairPuzzle : public PuzzleBase {
public:
    RadioRepairPuzzle(uint8_t pcfIndex)
        : PuzzleBase(ID_P7_RADIO_REPAIR), pcfIndex_(pcfIndex) {}

    void setupHardware() override {
        setPixelRange(29, 6);  // LEDs 29-34

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        // Buttons: P3, P4 | Switches: P5, P6 | Module microswitches: P7, P8
        for (uint8_t i = 3; i <= 8; i++) {
            pcf->pinMode(i, INPUT_PULLUP);
        }

        Serial.printf("[P7 RadioRepair] Setup PCF[%d]\n", pcfIndex_);
        Serial.println("[P7] Insert modules, set switches, press TEST.");
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        if (millis() - lastPoll_ < 250) return;
        lastPoll_ = millis();

        switch (repairStep_) {
        case 0:
            // Step 1: Insert module A (microswitch P7)
            if (pcf->digitalRead(7) == LOW) {
                repairStep_ = 1;
                Serial.println("[P7] Step 1: Module A inserted");
                if (pixels()) pixels()->setPixel(0, pixels()->Color(0, 255, 0));
            }
            break;

        case 1:
            // Step 2: Set switch 1 to correct position (P5 = LOW)
            if (pcf->digitalRead(5) == LOW) {
                repairStep_ = 2;
                Serial.println("[P7] Step 2: Switch 1 set");
                if (pixels()) pixels()->setPixel(1, pixels()->Color(0, 255, 0));
            }
            break;

        case 2:
            // Step 3: Insert module B (P8) + set switch 2 (P6) + press test (P3)
            if (pcf->digitalRead(8) == LOW && pcf->digitalRead(6) == LOW && pcf->digitalRead(3) == LOW) {
                repairStep_ = 3;
                Serial.println("[P7] ALL STEPS COMPLETE! Radio repaired!");
                if (pixels()) {
                    for (uint8_t i = 0; i < pixels()->count(); i++)
                        pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                    pixels()->show();
                }
                stateStart_ = millis();
            }
            break;

        case 3:
            // Solved - wait 2s then transition
            if (millis() - stateStart_ > 2000) {
                Serial.println("[P7] Puzzle SOLVED - meltdown countdown starts!");
                hw.sound().playOnce("/sfx/meltdown_start.mp3");
                setState(PuzzleState::Solved);
            }
            break;
        }
    }

private:
    uint8_t pcfIndex_;
    uint8_t repairStep_ = 0;
    unsigned long lastPoll_ = 0;
    unsigned long stateStart_ = 0;
};

// ====================================================================
// P8: ROTARY DIAL - Dial the emergency number
// ====================================================================
// Hardware: RotaryDial (pulse dial, 2 wires + GND), NeoPixel, MP3 audio
// Mechanics: old telephone rotary dial. Each pulse = one digit.
// Dial 4 correct digits -> plays the message with the wire order.
//
// Uses the library's RotaryDialReader (already included in PropMakerEscapeRoom.h)

class RotaryDialPuzzle : public PuzzleBase {
public:
    RotaryDialPuzzle(uint8_t dialDataPin, uint8_t dialFlickPin)
        : PuzzleBase(ID_P8_ROTARY_DIAL), dataPin_(dialDataPin), flickPin_(dialFlickPin) {}

    void setupHardware() override {
        setPixelRange(35, 6);  // LEDs 35-40

        // RotaryDial reader on GPIO pins
        pinMode(dataPin_, INPUT_PULLUP);
        pinMode(flickPin_, INPUT_PULLUP);

        Serial.printf("[P8 RotaryDial] Setup data=%d, flick=%d\n", dataPin_, flickPin_);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        // Read rotary dial pulses
        bool data = (digitalRead(dataPin_) == LOW);
        bool flick = (digitalRead(flickPin_) == LOW);

        // Detect pulse (flick transitions LOW->HIGH)
        if (flick && !lastFlick_) {
            pulseCount_++;
            lastPulseTime_ = millis();
            Serial.printf("[P8] Pulse %d\n", pulseCount_);
        }
        lastFlick_ = flick;

        // After 1.5s of no pulses, consider digit complete
        if (pulseCount_ > 0 && millis() - lastPulseTime_ > 1500) {
            uint8_t digit = pulseCount_;
            pulseCount_ = 0;
            handleDigit(digit);
        }

        // Update pixel feedback (show current digit position)
        if (pixels()) {
            for (uint8_t i = 0; i < pixels()->count(); i++) {
                if (i < dialIndex_) pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                else if (i == dialIndex_) pixels()->setPixel(i, pixels()->Color(255, 255, 0));
                else pixels()->setPixel(i, 0);
            }
            pixels()->show();
        }
    }

private:
    uint8_t dataPin_;
    uint8_t flickPin_;
    bool lastFlick_ = false;
    uint8_t pulseCount_ = 0;
    unsigned long lastPulseTime_ = 0;

    // Correct number: 5731 (or whatever we configure)
    const uint8_t CORRECT_NUMBER_[4] = {5, 7, 3, 1};
    uint8_t dialIndex_ = 0;

    void handleDigit(uint8_t digit) {
        Serial.printf("[P8] Digit received: %d\n", digit);

        if (digit == CORRECT_NUMBER_[dialIndex_]) {
            dialIndex_++;
            Serial.printf("[P8] Correct! %d/4\n", dialIndex_);

            if (dialIndex_ >= 4) {
                // All digits correct!
                Serial.println("[P8] EMERGENCY NUMBER CORRECT!");
                if (pixels()) {
                    for (uint8_t i = 0; i < pixels()->count(); i++)
                        pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                    pixels()->show();
                }
                // Play hint audio with wire order
                hw.sound().playOnce("/sfx/hint_wires.mp3");
                setState(PuzzleState::Solved);
            }
        } else {
            // Wrong digit, reset
            Serial.printf("[P8] Wrong digit. Expected %d, got %d. Resetting.\n",
                          CORRECT_NUMBER_[dialIndex_], digit);
            dialIndex_ = 0;
            if (pixels()) {
                for (uint8_t i = 0; i < pixels()->count(); i++)
                    pixels()->setPixel(i, pixels()->Color(255, 0, 0));
                pixels()->show();
            }
        }
    }
};

// ====================================================================
// P9: BOMB TIMER - Cut wires in the correct order
// ====================================================================
// Hardware: 6 terminals (PCF), TM1637 display, LIS3DH, NeoPixel, audio
// Mechanics: 6 wires on terminals. Cut in the order revealed by P8.
// Shake (LIS3DH) freezes 3s and counts as an error.
//
// Non-blocking: state machine with millis().

class BombTimerPuzzle : public PuzzleBase {
public:
    BombTimerPuzzle(uint8_t pcfIndex)
        : PuzzleBase(ID_P9_BOMB_TIMER), pcfIndex_(pcfIndex) {}

    void setupHardware() override {
        setPixelRange(41, 6);  // LEDs 41-46

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        // 6 wire inputs (P9-P14)
        for (uint8_t i = 9; i <= 14; i++) {
            pcf->pinMode(i, INPUT_PULLUP);
        }

        // Generate random wire order
        generateRandomSequenceNoRepeat(wireOrder_, 6, 5);
        Serial.printf("[P9 BombTimer] Wire order: ");
        for (uint8_t i = 0; i < 6; i++) Serial.printf("%d ", wireOrder_[i]);
        Serial.println();

        // Reset wire states (all connected = LOW with PULLUP)
        for (uint8_t i = 0; i < 6; i++) {
            wireCut_[i] = false;
            lastWireState_[i] = false;
        }

        Serial.printf("[P9 BombTimer] Setup PCF[%d]\n", pcfIndex_);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        if (millis() - lastTick_ < 100) return;  // 100ms cycle
        lastTick_ = millis();

        // Handle shake penalty
        if (hw.isShaking()) {
            if (!shaking_) {
                shaking_ = true;
                shakeStart_ = millis();
                errors_++;
                Serial.printf("[P9] SHAKE! Error %d/3\n", errors_);
                if (pixels()) {
                    for (uint8_t i = 0; i < pixels()->count(); i++)
                        pixels()->setPixel(i, pixels()->Color(255, 0, 0));
                    pixels()->show();
                }
            }

            // Timer frozen during shake (3s penalty)
            if (millis() - shakeStart_ > 3000) {
                shaking_ = false;
                Serial.println("[P9] Shake penalty over");
            }

            // Update display during shake
            hw.display().printTime(timerSeconds_ / 60, timerSeconds_ % 60);
            return;  // Skip wire checking during shake
        }

        // Check each wire for cut (transition from connected to disconnected)
        for (uint8_t i = 0; i < 6; i++) {
            bool connected = (pcf->digitalRead(i + 9) == LOW);  // P9-P14, LOW = connected
            if (!connected && lastWireState_[i]) {
                // Wire was just cut
                handleWireCut(i);
            }
            lastWireState_[i] = connected;
        }

        // Update timer
        if (timerRunning_) {
            timerAccum_ += 100;
            if (timerAccum_ >= 1000) {
                timerAccum_ -= 1000;
                timerSeconds_--;
                hw.display().printTime(timerSeconds_ / 60, timerSeconds_ % 60);

                if (timerSeconds_ <= 0) {
                    // BOOM!
                    Serial.println("[P9] MELTDOWN! Time's up!");
                    setState(PuzzleState::Error);
                    hw.sound().playOnce("/sfx/meltdown_explosion.mp3");
                    return;
                }
            }
        }

        // Update pixel feedback
        if (pixels()) {
            for (uint8_t i = 0; i < 6; i++) {
                if (wireCut_[i]) {
                    pixels()->setPixel(i, pixels()->Color(50, 0, 0));  // Cut
                } else if (i == wireOrder_[wireIndex_]) {
                    pixels()->setPixel(i, pixels()->Color(255, 255, 0));  // Next to cut
                } else {
                    pixels()->setPixel(i, pixels()->Color(255, 0, 0));  // Active
                }
            }
            pixels()->show();
        }

        // Check win
        if (wireIndex_ >= 6) {
            Serial.println("[P9] ALL WIRES CUT CORRECTLY! DEFUSED!");
            setState(PuzzleState::Solved);
            hw.sound().playOnce("/sfx/victory.mp3");
        }
    }

private:
    uint8_t pcfIndex_;
    uint8_t wireOrder_[6];
    bool wireCut_[6] = {false};
    bool lastWireState_[6] = {false};
    uint8_t wireIndex_ = 0;
    uint8_t errors_ = 0;

    // Timer
    uint16_t timerSeconds_ = 300;  // 5 minutes
    unsigned long timerAccum_ = 0;
    bool timerRunning_ = true;

    // Shake
    bool shaking_ = false;
    unsigned long shakeStart_ = 0;

    // General
    unsigned long lastTick_ = 0;

    void handleWireCut(uint8_t wireId) {
        Serial.printf("[P9] Wire %d cut\n", wireId);

        if (wireId == wireOrder_[wireIndex_]) {
            // Correct wire!
            wireCut_[wireId] = true;
            wireIndex_++;
            Serial.printf("[P9] Correct wire! %d/6\n", wireIndex_);
        } else {
            // Wrong wire! Accelerate timer
            errors_++;
            Serial.printf("[P9] WRONG WIRE! Error %d/3. Timer accelerated!\n", errors_);
            // Reduce remaining time by 30s as penalty
            if (timerSeconds_ > 30) {
                timerSeconds_ -= 30;
            } else {
                timerSeconds_ = 10;
            }
            hw.display().printTime(timerSeconds_ / 60, timerSeconds_ % 60);

            if (errors_ >= 3) {
                Serial.println("[P9] Too many errors! MELTDOWN!");
                setState(PuzzleState::Error);
                hw.sound().playOnce("/sfx/meltdown_explosion.mp3");
            }
        }
    }
};
