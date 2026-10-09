// AcessoPuzzles.h - Puzzles 1, 2, 3 of the Nuclear Reactor
// Level 0: Access (PARALLEL) - Release the 3 trapped technicians
//
// P1 - KeypadRfidLock: RFID badge + password on the 4x3 keypad (two factors)
// P2 - CoolantValves: ALL_CONNECTED, 3 microswitches/valves
// P3 - ElectricalPanel: LINEAR_SEQUENCE, 3 toggle switches in order

#pragma once

#include <ESP32EscapeRoom.h>

using namespace esp32_escaperoom;

// ====================================================================
// P1: KEYPAD + RFID - Releases Alexei (Reactor Engineer)
// ====================================================================
// Hardware: PCF8575 0x20, 4x3 keypad, PN532 (I2C), NeoPixel, relay
// Mechanics: tap badge + type emergency password = door opens

class KeypadRfidPuzzle : public PuzzleBase {
public:
    KeypadRfidPuzzle(uint8_t pcfIndex) : PuzzleBase(ID_P1_KEYPAD_RFID), pcfIndex_(pcfIndex) {}

    void setupHardware() override {
        setPixelRange(0, 5);  // LEDs 0-4

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        // Keypad matrix: Rows P0-P3 (outputs), Cols P4-P6 (inputs)
        for (uint8_t r = 0; r < 4; r++) pcf->pinMode(r, OUTPUT);
        for (uint8_t c = 0; c < 3; c++) pcf->pinMode(c + 4, INPUT_PULLUP);

        // Lock relay on P8
        pcf->pinMode(8, OUTPUT);
        pcf->digitalWrite(8, HIGH);  // Normally locked

        // Create NFC reader for badge
        nfc_ = hw.createNfcReader();
        if (nfc_) {
            Serial.println("[P1] NFC reader created");
        }

        Serial.printf("[P1 KeypadRFID] Setup PCF[%d]\n", pcfIndex_);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        switch (subState_) {
        case 0: // Wait for RFID or keypad input
            checkKeypad();
            checkRfid();
            if (rfidOk_ && codeBufferLen_ == 4 && codeOk_) {
                subState_ = 1;
                subStateStart_ = millis();
                if (pixels()) {
                    for (uint8_t i = 0; i < pixels()->count(); i++)
                        pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                    pixels()->show();
                }
                Serial.println("[P1] ACCESS GRANTED! Alexei released.");
                hw.sound().playOnce("/sfx/alexei_free.mp3");
            }
            break;

        case 1: // Solved effect (2s delay)
            if (millis() - subStateStart_ > 2000) {
                unlockDoor();
                setState(PuzzleState::Solved);
            }
            break;
        }
    }

private:
    uint8_t pcfIndex_;
    NfcReader* nfc_ = nullptr;
    uint8_t subState_ = 0;
    unsigned long subStateStart_ = 0;
    unsigned long lastKeyScan_ = 0;
    unsigned long lastRfidScan_ = 0;

    char codeBuffer_[4] = {0};
    uint8_t codeBufferLen_ = 0;
    const char CORRECT_CODE_[4] = {'1', '2', '3', '4'};
    bool rfidOk_ = false;
    bool codeOk_ = false;
    unsigned long lastKeyPress_ = 0;

    const char keys_[4][3] = {
        {'1','2','3'},
        {'4','5','6'},
        {'7','8','9'},
        {'*','0','#'}
    };

    void checkKeypad() {
        if (millis() - lastKeyScan_ < 100) return;
        lastKeyScan_ = millis();

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        for (uint8_t r = 0; r < 4; r++) {
            for (uint8_t rr = 0; rr < 4; rr++) pcf->digitalWrite(rr, HIGH);
            pcf->digitalWrite(r, LOW);
            delay(1);

            for (uint8_t c = 0; c < 3; c++) {
                if (pcf->digitalRead(c + 4) == LOW) {
                    handleKeyPress(keys_[r][c]);
                    return;
                }
            }
        }
    }

    void handleKeyPress(char key) {
        if (millis() - lastKeyPress_ < 300) return;
        lastKeyPress_ = millis();

        Serial.printf("[P1] Key: %c\n", key);

        if (key == '#') {
            if (codeBufferLen_ == 4) {
                codeOk_ = true;
                for (uint8_t i = 0; i < 4; i++) {
                    if (codeBuffer_[i] != CORRECT_CODE_[i]) { codeOk_ = false; break; }
                }
                if (codeOk_) {
                    Serial.println("[P1] Code OK");
                    if (pixels()) pixels()->setPixel(0, pixels()->Color(0, 255, 0));
                } else {
                    Serial.println("[P1] Wrong code");
                    if (pixels()) {
                        for (uint8_t i = 0; i < 3; i++)
                            pixels()->setPixel(i, pixels()->Color(255, 0, 0));
                    }
                    codeBufferLen_ = 0;
                }
                if (pixels()) pixels()->show();
            }
        } else if (key == '*') {
            codeBufferLen_ = 0;
            codeOk_ = false;
            if (pixels()) {
                for (uint8_t i = 0; i < pixels()->count(); i++) pixels()->setPixel(i, 0);
                pixels()->show();
            }
        } else if (codeBufferLen_ < 4) {
            codeBuffer_[codeBufferLen_++] = key;
            if (pixels()) {
                pixels()->setPixel(codeBufferLen_, pixels()->Color(0, 0, 255));
                pixels()->show();
            }
        }
    }

    void checkRfid() {
        if (millis() - lastRfidScan_ < 500) return;
        lastRfidScan_ = millis();

        if (!nfc_) return;

        auto card = nfc_->readCard(500);
        if (card.present && !rfidOk_) {
            rfidOk_ = true;
            Serial.println("[P1] RFID tag detected");
            if (pixels()) {
                pixels()->setPixel(4, pixels()->Color(0, 255, 255));
                pixels()->show();
            }
        }
    }

    void unlockDoor() {
        auto* pcf = hw.portExpander(pcfIndex_);
        if (pcf) {
            pcf->digitalWrite(8, LOW);
            Serial.println("[P1] Door UNLOCKED");
        }
    }
};

// ====================================================================
// P2: COOLANT VALVES (ALL_CONNECTED) - Releases Boris (Coolant Technician)
// ====================================================================

class CoolantValvesPuzzle : public PuzzleBase {
public:
    CoolantValvesPuzzle(uint8_t pcfIndex) : PuzzleBase(ID_P2_COOLANT_VALVES), pcfIndex_(pcfIndex) {}

    void setupHardware() override {
        setPixelRange(5, 4);  // LEDs 5-8

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        for (uint8_t i = 0; i < 3; i++) pcf->pinMode(i, INPUT_PULLUP);
        pcf->pinMode(8, OUTPUT);
        pcf->digitalWrite(8, HIGH);

        Serial.printf("[P2 Coolant] Setup PCF[%d]\n", pcfIndex_);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        bool v1 = (pcf->digitalRead(0) == LOW);
        bool v2 = (pcf->digitalRead(1) == LOW);
        bool v3 = (pcf->digitalRead(2) == LOW);

        if (pixels()) {
            pixels()->setPixel(0, v1 ? pixels()->Color(0, 255, 255) : pixels()->Color(255, 0, 0));
            pixels()->setPixel(1, v2 ? pixels()->Color(0, 255, 255) : pixels()->Color(255, 0, 0));
            pixels()->setPixel(2, v3 ? pixels()->Color(0, 255, 255) : pixels()->Color(255, 0, 0));
            pixels()->show();
        }

        if (v1 && v2 && v3) {
            if (!solved_) {
                solved_ = true;
                solveStart_ = millis();
                Serial.println("[P2] ALL VALVES CLOSED!");
                if (pixels()) {
                    for (uint8_t i = 0; i < pixels()->count(); i++)
                        pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                    pixels()->show();
                }
            }
            if (millis() - solveStart_ > 2000) {
                pcf->digitalWrite(8, LOW);
                Serial.println("[P2] Door UNLOCKED - Boris released!");
                hw.sound().playOnce("/sfx/boris_free.mp3");
                setState(PuzzleState::Solved);
            }
        } else {
            solved_ = false;
            solveStart_ = 0;
        }
    }

private:
    uint8_t pcfIndex_;
    bool solved_ = false;
    unsigned long solveStart_ = 0;
};

// ====================================================================
// P3: ELECTRICAL PANEL (LINEAR_SEQUENCE) - Releases Vladislav (Electrician)
// ====================================================================

class ElectricalPanelPuzzle : public PuzzleBase {
public:
    ElectricalPanelPuzzle(uint8_t pcfIndex) : PuzzleBase(ID_P3_ELECTRICAL_PNL), pcfIndex_(pcfIndex) {}

    void setupHardware() override {
        setPixelRange(9, 4);  // LEDs 9-12

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        for (uint8_t i = 0; i < 3; i++) pcf->pinMode(i, INPUT_PULLUP);
        pcf->pinMode(8, OUTPUT);   // Door lock
        pcf->pinMode(9, OUTPUT);   // Drawer lock
        pcf->digitalWrite(8, HIGH);
        pcf->digitalWrite(9, HIGH);

        generateRandomSequenceNoRepeat(sequence_, 3, 2);
        Serial.printf("[P3 Electrical] Sequence: %d -> %d -> %d\n",
                      sequence_[0], sequence_[1], sequence_[2]);
        Serial.printf("[P3] Setup PCF[%d]\n", pcfIndex_);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        auto* pcf = hw.portExpander(pcfIndex_);
        if (!pcf) return;

        // Check reset timeout
        if (inError_ && millis() - errorStart_ > 1000) {
            inError_ = false;
        }

        if (inError_) return;

        // Read switches with edge detection
        for (uint8_t i = 0; i < 3; i++) {
            bool active = (pcf->digitalRead(i) == LOW);
            if (active && !lastSwitchState_[i]) {
                handleSwitchFlip(i);
            }
            lastSwitchState_[i] = active;
        }

        if (pixels()) {
            for (uint8_t i = 0; i < 3; i++) {
                if (i < sequenceIndex_) pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                else if (i == sequenceIndex_) pixels()->setPixel(i, pixels()->Color(255, 255, 0));
                else pixels()->setPixel(i, pixels()->Color(100, 0, 0));
            }
            pixels()->show();
        }
    }

private:
    uint8_t pcfIndex_;
    uint8_t sequence_[3];
    bool lastSwitchState_[3] = {false, false, false};
    uint8_t sequenceIndex_ = 0;
    unsigned long errorStart_ = 0;
    bool inError_ = false;

    void handleSwitchFlip(uint8_t switchId) {
        Serial.printf("[P3] Switch %d flipped\n", switchId);

        if (switchId == sequence_[sequenceIndex_]) {
            sequenceIndex_++;
            Serial.printf("[P3] Correct! Step %d/3\n", sequenceIndex_);
            if (sequenceIndex_ >= 3) {
                Serial.println("[P3] SEQUENCE COMPLETE! Vladislav released.");
                if (pixels()) {
                    for (uint8_t i = 0; i < pixels()->count(); i++)
                        pixels()->setPixel(i, pixels()->Color(0, 255, 0));
                    pixels()->show();
                }
                auto* pcf = hw.portExpander(pcfIndex_);
                if (pcf) {
                    pcf->digitalWrite(8, LOW);  // Door
                    pcf->digitalWrite(9, LOW);  // Drawer
                }
                hw.sound().playOnce("/sfx/vlad_free.mp3");
                setState(PuzzleState::Solved);
            }
        } else {
            Serial.println("[P3] WRONG! Resetting.");
            sequenceIndex_ = 0;
            inError_ = true;
            errorStart_ = millis();
            if (pixels()) {
                for (uint8_t i = 0; i < pixels()->count(); i++)
                    pixels()->setPixel(i, pixels()->Color(255, 0, 0));
                pixels()->show();
            }
        }
    }
};
