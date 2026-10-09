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

    // Injeta o MicInput (modo MUSICAL_SEQUENCE). Construtor NÃO muda.
    void setMicInput(esp32_escaperoom::MicInput* mic) { mic_ = mic; }

private:
    ConfigManager& config_;
    esp32_escaperoom::MicInput* mic_ = nullptr;
    EngineState state_ = EngineState::IDLE;

    // Sequência
    uint8_t sequence_[16];
    uint8_t sequenceLength_ = 0;
    uint8_t currentStep_ = 0;
    uint8_t totalSteps_ = 0;
    uint8_t progress_ = 0;  // 0-100

    // Timer
    unsigned long stateStartTime_ = 0;
    unsigned long puzzleStartTime_ = 0;
    unsigned long lastInputTime_ = 0;

    // Estados de input
    uint8_t lastInputStates_[16];
    unsigned long debounceTimes_[16];

    // Métodos de processamento por modo
    void processCustomSequence();
    void processLinearSequence();
    void processAllConnected();
    void processRandomSequence();
    void processMusicalSequence();

    // Helpers
    void checkInputs();
    void onInputCorrect(uint8_t inputIndex);
    void onInputWrong(uint8_t inputIndex);
    void onNoteCorrect(uint8_t midi);
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

    // Geração de sequências
    void generateLinearSequence();
    void generateRandomSequence();

    // Visualização
    void showIdleEffect();
    void showRunningEffect();
    void showCorrectEffect();
    void showErrorEffect();
    void showSolvedEffect();
};

} // namespace universal_controller