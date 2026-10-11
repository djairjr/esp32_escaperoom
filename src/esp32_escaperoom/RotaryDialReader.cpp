#include "RotaryDialReader.h"
#include <RotaryDialUnified.h>

namespace esp32_escaperoom {

bool RotaryDialReader::begin(uint8_t pin) {
    RotaryDialUnified::begin(pin, ROTARYDIAL_UNIFIED_MECHANICAL);   // Unified library (default = MECHANICAL)
    lastPin_ = pin;
    initialized_ = true;
    Serial.printf("[ROTARY] Initialized at pin %d (MECHANICAL)\n", pin);
    Serial.printf("[ROTARY]   Debounce=15ms  MaxGap=0  DigitEnd=600ms\n");
    return true;
}

bool RotaryDialReader::beginOptical(uint8_t pin, unsigned long debounce, unsigned long maxgap, unsigned long digitend) {
    RotaryDialUnified::begin(pin, ROTARYDIAL_UNIFIED_OPTICAL);
    if (debounce != 50)  RotaryDialUnified::setDebounce(debounce);
    if (maxgap != 0)     RotaryDialUnified::setMaxGap(maxgap);
    if (digitend != 800) RotaryDialUnified::setDigitEnd(digitend);

    lastPin_ = pin;
    initialized_ = true;
    Serial.printf("[ROTARY] Initialized at pin %d (OPTICAL)\n", pin);
    Serial.printf("[ROTARY]   Debounce=%lums  MaxGap=%lu  DigitEnd=%lums\n", debounce, maxgap, digitend);
    return true;
}

bool RotaryDialReader::available() {
    if (!initialized_) return false;
    return RotaryDialUnified::available();
}

int RotaryDialReader::read() {
    if (!initialized_) return -1;
    return RotaryDialUnified::read();
}

void RotaryDialReader::reset() {
    if (!initialized_) return;
    RotaryDialUnified::reset();
}

}  // namespace esp32_escaperoom
