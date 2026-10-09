#pragma once

#include <Arduino.h>
#include <ESP32EscapeRoom.h>
#include "ConfigManager.h"

using namespace esp32_escaperoom;

namespace universal_controller {

enum class EngineState {
    IDLE,
    RUNNING,
    INPUT_CORRECT,
    INPUT_WRONG,
    SOLVED,
    ERROR,
    TIMEOUT
};

class PuzzleEngine {
public:
    PuzzleEngine(ConfigManager& config);

    void begin();
    void update();
    void reset();
    void start();

    EngineState getState() const { return state_; }
    uint8_t getProgress() const { return progress_; }
    uint8_t getCurrentStep() const { return currentStep_; }
    uint8_t getTotalSteps() const { return totalSteps_; }
    uint16_t getRemainingTime() const;

    bool isSolved() const { return state_ == EngineState::SOLVED; }
    bool hasError() const { return state_ == EngineState::ERROR; }

private:
    ConfigManager& config_;
    EngineState state_ = EngineState::IDLE;

    // Sequence
    uint8_t sequence_[16];
    uint8_t sequenceLength_ = 0;
    uint8_t currentStep_ = 0;
    uint8_t totalSteps_ = 0;
    uint8_t progress_ = 0;  // 0-100

    // Timer
    unsigned long stateStartTime_ = 0;
    unsigned long puzzleStartTime_ = 0;
    unsigned long lastInputTime_ = 0;

    // Input states
    uint8_t lastInputStates_[16];
    unsigned long debounceTimes_[16];

    // Processing methods per mode
    void processCustomSequence();
    void processLinearSequence();
    void processAllConnected();
    void processRandomSequence();

    // Helpers
    void checkInputs();
    void onInputCorrect(uint8_t inputIndex);
    void onInputWrong(uint8_t inputIndex);
    void onSequenceComplete();
    void onTimeout();
    void updateProgress();
    void updatePwmOutput();
    void updateNeoPixel();

    bool detectInputRisingEdge(uint8_t pin);
    bool isInputActive(uint8_t pin);
    void playInputSound(uint8_t index);
    void playUnlockSound();
    void playErrorSound();
    void playResetSound();

    // Sequence generation
    void generateLinearSequence();
    void generateRandomSequence();

    // Visualization
    void showIdleEffect();
    void showRunningEffect();
    void showCorrectEffect();
    void showErrorEffect();
    void showSolvedEffect();
};

} // namespace universal_controller
