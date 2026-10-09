#include "PuzzleEngine.h"

namespace universal_unique_rfid {

PuzzleEngine::PuzzleEngine(ConfigManager& config) : config_(config) {}

void PuzzleEngine::begin() {
    const auto& rfid = config_.getRfidConfig();
    const auto& np = config_.getNeoPixelConfig();

    reader_ = hw.createNfcReader();
    if (reader_ && reader_->isInitialized()) {
        Serial.println("[PuzzleEngine] NFC Reader OK");
    } else {
        Serial.println("[PuzzleEngine] The NFC Reader on NAO is initialized.");
    }

    if (rfid.mode == RfidMode::SEQUENCE && rfid.sequenceLength > 0) {
        totalSteps_ = rfid.sequenceLength;
        Serial.printf("[PuzzleEngine] Mode: SEQUENCE | Steps: %d\n", totalSteps_);
    } else {
        totalSteps_ = 1;
        Serial.printf("[PuzzleEngine] Modo: UNIQUE | Tags: %d\n", rfid.tagCount);
    }

    currentStep_ = 0;
    progress_ = 0;
    state_ = EngineState::IDLE;
    showIdleEffect();
}

void PuzzleEngine::update() {
    const auto& rfid = config_.getRfidConfig();

    if (rfid.timeoutSeconds > 0 && state_ == EngineState::RUNNING) {
        if (millis() - puzzleStartTime_ > rfid.timeoutSeconds * 1000UL) {
            onTimeout();
            return;
        }
    }

    if (millis() - lastPollTime_ < 250) return;
    lastPollTime_ = millis();

    switch (state_) {
        case EngineState::IDLE: break;
        case EngineState::RUNNING:
            if (!reader_ || !reader_->isInitialized()) break;
            if (rfid.mode == RfidMode::UNIQUE) processUniqueMode();
            else processSequenceMode();
            break;
        case EngineState::TAG_CORRECT:
            if (millis() - stateStartTime_ > 500) { state_ = EngineState::RUNNING; showRunningEffect(); }
            break;
        case EngineState::TAG_WRONG:
            if (millis() - stateStartTime_ > 1000) {
                if (rfid.resetOnError) { reset(); start(); }
                else { state_ = EngineState::RUNNING; showRunningEffect(); }
            }
            break;
        case EngineState::SOLVED: showSolvedEffect(); break;
        default: break;
    }
}

void PuzzleEngine::reset() {
    currentStep_ = 0; progress_ = 0;
    memset(lastUid_, 0, sizeof(lastUid_)); lastUidLen_ = 0;
    state_ = EngineState::IDLE;
    showIdleEffect();
}

void PuzzleEngine::start() {
    puzzleStartTime_ = millis();
    state_ = EngineState::RUNNING;
    showRunningEffect();
    Serial.println("[PuzzleEngine] Puzzle iniciado");
}

uint16_t PuzzleEngine::getRemainingTime() const {
    const auto& rfid = config_.getRfidConfig();
    if (rfid.timeoutSeconds == 0) return 0;
    unsigned long elapsed = (millis() - puzzleStartTime_) / 1000;
    if (elapsed >= rfid.timeoutSeconds) return 0;
    return rfid.timeoutSeconds - elapsed;
}

void PuzzleEngine::processUniqueMode() {
    if (!readTag()) return;
    const auto& rfid = config_.getRfidConfig();
    if (rfid.tagCount == 0) return;
    for (uint8_t i = 0; i < rfid.tagCount; i++) {
        uint8_t tagBytes[7], tagLen = 0;
        hexToBytes(rfid.tags[i].uid, tagBytes, tagLen);
        if (tagLen == lastUidLen_ && memcmp(lastUid_, tagBytes, tagLen) == 0) {
            onTagCorrect(i); return;
        }
    }
    onTagWrong();
}

void PuzzleEngine::processSequenceMode() {
    if (!readTag()) return;
    const auto& rfid = config_.getRfidConfig();
    if (rfid.tagCount == 0 || rfid.sequenceLength == 0) return;
    int tagIndex = findTagIndex(lastUid_, lastUidLen_);
    if (tagIndex < 0) { onTagWrong(); return; }
    uint8_t expected = rfid.sequence[currentStep_];
    if ((uint8_t)tagIndex == expected) onTagCorrect(tagIndex);
    else onTagWrong();
}

bool PuzzleEngine::readTag() {
    if (!reader_) return false;
    auto card = reader_->readCard(100);
    if (!card.present) { lastUidLen_ = 0; return false; }
    if (card.uidLength == lastUidLen_ && memcmp(card.uid, lastUid_, lastUidLen_) == 0) return false;
    lastUidLen_ = card.uidLength;
    memcpy(lastUid_, card.uid, lastUidLen_);
    char uidHex[15];
    bytesToHex(card.uid, card.uidLength, uidHex);
    Serial.printf("[PuzzleEngine] Tag: UID=%s (%d bytes)\n", uidHex, card.uidLength);
    playTagSound();
    return true;
}

void PuzzleEngine::hexToBytes(const char* hex, uint8_t* bytes, uint8_t& len) {
    len = 0;
    size_t hexLen = strlen(hex);
    for (size_t i = 0; i < hexLen && len < 7; i += 2) {
        char byteStr[3] = {hex[i], hex[i+1], '\0'};
        bytes[len++] = strtoul(byteStr, NULL, 16);
    }
}

void PuzzleEngine::bytesToHex(const uint8_t* bytes, uint8_t len, char* hex) {
    for (uint8_t i = 0; i < len; i++) sprintf(hex + (i * 2), "%02X", bytes[i]);
    hex[len * 2] = '\0';
}

int PuzzleEngine::findTagIndex(const uint8_t* uid, uint8_t uidLen) {
    const auto& rfid = config_.getRfidConfig();
    for (uint8_t i = 0; i < rfid.tagCount; i++) {
        uint8_t tagBytes[7], tagLen = 0;
        hexToBytes(rfid.tags[i].uid, tagBytes, tagLen);
        if (tagLen == uidLen && memcmp(uid, tagBytes, tagLen) == 0) return i;
    }
    return -1;
}

void PuzzleEngine::onTagCorrect(uint8_t tagIndex) {
    stateStartTime_ = millis();
    state_ = EngineState::TAG_CORRECT;
    if (config_.getRfidConfig().mode == RfidMode::SEQUENCE) {
        currentStep_++; updateProgress();
    } else {
        currentStep_ = 1; progress_ = 100;
    }
    playCorrectSound();
    showCorrectEffect();
    Serial.printf("[PuzzleEngine] Tag CORRECTED: %s\n", config_.getRfidConfig().tags[tagIndex].name);
    if (currentStep_ >= totalSteps_) onSequenceComplete();
}

void PuzzleEngine::onTagWrong() {
    stateStartTime_ = millis();
    state_ = EngineState::TAG_WRONG;
    playWrongSound();
    showWrongEffect();
    Serial.println("[PuzzleEngine] Tag ERRADA");
}

void PuzzleEngine::onSequenceComplete() {
    state_ = EngineState::SOLVED; progress_ = 100;
    playUnlockSound();
    showSolvedEffect();
    const auto& output = config_.getOutputConfig();
    hw.pinMode(output.lockPin, OUTPUT);
    hw.digitalWrite(output.lockPin, output.activeHigh ? HIGH : LOW);
    Serial.println("[PuzzleEngine] PUZZLE RESOLVIDO!");
}

void PuzzleEngine::onTimeout() {
    state_ = EngineState::TIMEOUT;
    Serial.println("[PuzzleEngine] TIMEOUT!");
}

void PuzzleEngine::updateProgress() {
    progress_ = (totalSteps_ == 0) ? 0 : (currentStep_ * 100) / totalSteps_;
}

void PuzzleEngine::playTagSound() { if (config_.getAudioConfig().enabled) hw.sound().playOnce(config_.getAudioConfig().tagSound); }
void PuzzleEngine::playCorrectSound() { if (config_.getAudioConfig().enabled) hw.sound().playOnce(config_.getAudioConfig().correctSound); }
void PuzzleEngine::playWrongSound() { if (config_.getAudioConfig().enabled) hw.sound().playOnce(config_.getAudioConfig().wrongSound); }
void PuzzleEngine::playUnlockSound() { if (config_.getAudioConfig().enabled) hw.sound().playOnce(config_.getAudioConfig().unlockSound); }
void PuzzleEngine::playErrorSound() { if (config_.getAudioConfig().enabled) hw.sound().playOnce(config_.getAudioConfig().errorSound); }
void PuzzleEngine::playResetSound() { if (config_.getAudioConfig().enabled) hw.sound().playOnce(config_.getAudioConfig().resetSound); }

void PuzzleEngine::showIdleEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;
    for (uint8_t i = 0; i < np.count; i++) hw.pixelsExternal().setPixelColor(i, np.colorIdle);
    hw.pixelsExternal().show();
}

void PuzzleEngine::showRunningEffect() { showIdleEffect(); }
void PuzzleEngine::showCorrectEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;
    for (uint8_t i = 0; i < np.count; i++) hw.pixelsExternal().setPixelColor(i, np.colorCorrect);
    hw.pixelsExternal().show();
}

void PuzzleEngine::showWrongEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;
    for (uint8_t i = 0; i < np.count; i++) hw.pixelsExternal().setPixelColor(i, np.colorWrong);
    hw.pixelsExternal().show();
}

void PuzzleEngine::showSolvedEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;
    static unsigned long lastUpdate = 0;
    static uint8_t brightness = 0;
    static bool increasing = true;
    if (millis() - lastUpdate > 20) {
        lastUpdate = millis();
        if (increasing) { brightness += 5; if (brightness >= 255) { brightness = 255; increasing = false; } }
        else { brightness -= 5; if (brightness <= 100) { brightness = 100; increasing = true; } }
        uint32_t color = ((np.colorSolved >> 16) & 0xFF) * brightness / 255;
        color = (color << 8) | (((np.colorSolved >> 8) & 0xFF) * brightness / 255);
        color = (color << 8) | ((np.colorSolved & 0xFF) * brightness / 255);
        for (uint8_t i = 0; i < np.count; i++) hw.pixelsExternal().setPixelColor(i, color);
        hw.pixelsExternal().show();
    }
}

} // namespace universal_unique_rfid
