#include "PuzzleBase.h"

namespace esp32_escaperoom {

// ===== PIXEL MANAGEMENT =====

void PuzzleBase::setPixelRange(uint8_t offset, uint8_t count) {
    // Clears previous view if it exists
    if (pixelView_ != nullptr) {
        delete pixelView_;
    }

    pixelOffset_ = offset;
    pixelCount_ = count;

    // Creates a new view over the external strip
    pixelView_ = new PuzzlePixelView(&hw.pixelsExternal(), offset, count);

    Serial.printf("[PUZZLE %d] Pixel range: offset=%d, count=%d (LEDs %d-%d)\n",
                  id_, offset, count, offset, offset + count - 1);
}

// ===== SEQUENCE METHODS =====

void PuzzleBase::generateRandomSequence(uint8_t* sequence, uint8_t count, uint8_t maxInput) {
    if (!sequence || count == 0 || maxInput == 0) return;

    for (uint8_t i = 0; i < count; i++) {
        sequence[i] = random(0, maxInput + 1);
    }

    Serial.printf("[PuzzleBase] Random sequence generated: ");
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf("%d ", sequence[i]);
    }
    Serial.println();
}

void PuzzleBase::generateRandomSequenceNoRepeat(uint8_t* sequence, uint8_t count, uint8_t maxInput) {
    if (!sequence || count == 0 || maxInput == 0) return;
    if (count > maxInput + 1) count = maxInput + 1;  // Clamps to the maximum possible

    // Temporary array allocated dynamically
    uint8_t* available = new uint8_t[maxInput + 1];
    if (!available) { Serial.println(F("[PuzzleBase] ERROR: alloc fail")); return; }

    for (uint8_t i = 0; i <= maxInput; i++) {
        available[i] = i;
    }

    // Partial Fisher-Yates shuffle
    for (uint8_t i = 0; i < count; i++) {
        uint8_t j = random(i, maxInput + 1);
        uint8_t temp = available[i];
        available[i] = available[j];
        available[j] = temp;
        sequence[i] = available[i];
    }

    delete[] available;

    Serial.printf("[PuzzleBase] Random sequence (no repetition): ");
    for (uint8_t i = 0; i < count; i++) {
        Serial.printf("%d ", sequence[i]);
    }
    Serial.println();
}

void PuzzleBase::generateRandomPairs(uint8_t* pairs, uint8_t pairCount, uint8_t maxInput) {
    if (!pairs || pairCount == 0 || maxInput == 0) return;
    if (pairCount * 2 > maxInput + 1) pairCount = (maxInput + 1) / 2;

    // Generates a sequence without repetition and groups it into pairs
    uint8_t* temp = new uint8_t[pairCount * 2];
    if (!temp) { Serial.println(F("[PuzzleBase] ERROR: alloc fail")); return; }

    generateRandomSequenceNoRepeat(temp, pairCount * 2, maxInput);

    for (uint8_t i = 0; i < pairCount; i++) {
        pairs[i * 2] = temp[i * 2];
        pairs[i * 2 + 1] = temp[i * 2 + 1];
    }

    delete[] temp;

    Serial.printf("[PuzzleBase] Random pairs: ");
    for (uint8_t i = 0; i < pairCount; i++) {
        Serial.printf("(%d-%d) ", pairs[i*2], pairs[i*2+1]);
    }
    Serial.println();
}

bool PuzzleBase::areInputsActive(uint8_t* inputs, uint8_t count) {
    if (!inputs || count == 0) return false;
    if (!hw.hasPortExpander()) return false;

    auto* io = hw.io();
    if (!io) return false;

    for (uint8_t i = 0; i < count; i++) {
        // Assumes INPUT_PULLUP: LOW = active
        if (io->digitalRead(inputs[i]) != LOW) {
            return false;
        }
    }
    return true;
}

bool PuzzleBase::areNInputsActive(uint8_t n, uint8_t maxInput) {
    if (n == 0) return true;
    if (!hw.hasPortExpander()) return false;

    auto* io = hw.io();
    if (!io) return false;

    uint8_t activeCount = 0;
    for (uint8_t i = 0; i <= maxInput; i++) {
        if (io->digitalRead(i) == LOW) {
            activeCount++;
            if (activeCount >= n) return true;
        }
    }
    return activeCount >= n;
}

bool PuzzleBase::checkSequence(uint8_t* sequence, uint8_t sequenceLen, uint8_t* currentInputs, uint8_t currentLen) {
    if (!sequence || !currentInputs) return false;
    if (currentLen == 0) return true;  // An empty sequence is still valid
    if (currentLen > sequenceLen) return false;

    for (uint8_t i = 0; i < currentLen; i++) {
        if (currentInputs[i] != sequence[i]) {
            return false;
        }
    }
    return true;
}

bool PuzzleBase::advanceSequence(uint8_t expectedInput, uint8_t actualInput) {
    if (actualInput != expectedInput) {
        return false;
    }
    sequenceIndex_++;
    return true;
}

// ===== INPUT HELPERS =====

bool PuzzleBase::readInputDebounce(uint8_t pin, uint16_t debounceMs) {
    if (!hw.hasPortExpander()) return false;

    auto* io = hw.io();
    if (!io) return false;

    uint8_t reading = (io->digitalRead(pin) == LOW) ? 1 : 0;  // LOW = active with PULLUP

    if (reading != lastInputState_[pin]) {
        lastDebounceTime_[pin] = millis();
    }

    if ((millis() - lastDebounceTime_[pin]) > debounceMs) {
        // Stabilized state
        if (reading != currentInputState_[pin]) {
            currentInputState_[pin] = reading;
            return (reading == 1);
        }
    }

    lastInputState_[pin] = reading;
    return false;
}

bool PuzzleBase::detectRisingEdge(uint8_t pin) {
    if (!hw.hasPortExpander()) return false;

    auto* io = hw.io();
    if (!io) return false;

    uint8_t current = (io->digitalRead(pin) == LOW) ? 1 : 0;
    uint8_t previous = edgeLastState_[pin];
    edgeLastState_[pin] = current;

    // Rising edge: 0 -> 1 (inactive -> active, considering PULLUP)
    return (previous == 0 && current == 1);
}

bool PuzzleBase::detectFallingEdge(uint8_t pin) {
    if (!hw.hasPortExpander()) return false;

    auto* io = hw.io();
    if (!io) return false;

    uint8_t current = (io->digitalRead(pin) == LOW) ? 1 : 0;
    uint8_t previous = edgeLastState_[pin];
    edgeLastState_[pin] = current;

    // Falling edge: 1 -> 0 (active -> inactive)
    return (previous == 1 && current == 0);
}

int8_t PuzzleBase::waitForInput(uint8_t* inputs, uint8_t count, uint16_t timeoutMs) {
    if (!inputs || count == 0) return -1;
    if (!hw.hasPortExpander()) return -1;

    auto* io = hw.io();
    if (!io) return -1;

    unsigned long startTime = millis();

    while (timeoutMs == 0 || (millis() - startTime) < timeoutMs) {
        for (uint8_t i = 0; i < count; i++) {
            if (io->digitalRead(inputs[i]) == LOW) {
                return i;  // Returns the index of the activated input
            }
        }
        delay(10);  // Small delay to avoid saturation
    }

    return -1;  // Timeout
}

int8_t PuzzleBase::waitForAnyInput(uint8_t maxInput, uint16_t timeoutMs) {
    if (!hw.hasPortExpander()) return -1;

    auto* io = hw.io();
    if (!io) return -1;

    unsigned long startTime = millis();

    while (timeoutMs == 0 || (millis() - startTime) < timeoutMs) {
        for (uint8_t i = 0; i <= maxInput; i++) {
            if (io->digitalRead(i) == LOW) {
                return i;  // Returns the number of the activated input
            }
        }
        delay(10);
    }

    return -1;  // Timeout
}

// ===== PROGRESS HELPERS =====

uint8_t PuzzleBase::calculateProgressPercent(uint8_t currentStep, uint8_t totalSteps) {
    if (totalSteps == 0) return 100;
    if (currentStep >= totalSteps) return 100;
    return (currentStep * 100) / totalSteps;
}

float PuzzleBase::calculateProgressFloat(uint8_t currentStep, uint8_t totalSteps) {
    if (totalSteps == 0) return 1.0f;
    if (currentStep >= totalSteps) return 1.0f;
    return (float)currentStep / (float)totalSteps;
}

uint16_t PuzzleBase::mapProgressToRange(uint8_t currentStep, uint8_t totalSteps, uint16_t minVal, uint16_t maxVal) {
    if (totalSteps == 0) return maxVal;
    if (currentStep >= totalSteps) return maxVal;
    return minVal + ((maxVal - minVal) * currentStep) / totalSteps;
}

}  // namespace esp32_escaperoom
