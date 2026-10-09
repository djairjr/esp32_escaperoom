#pragma once

#include <Arduino.h>
#include "MicInput.h"
#include "ConfigManager.h"

// XIAO ESP32-C3: IoAbstraction does not compile (no DAC).
// This define is visible to PuzzleEngine.cpp (which includes this header).
#if defined(ARDUINO_XIAO_ESP32C3)
#define ESP32_ESCAPEROOM_NO_IOABSTRACTION
#endif

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

    /// Resets the sequence progress and keeps the engine armed. No visual
    /// effect and no log — use it during player pauses. It is NOT start().
    void restartProgress();

    EngineState getState() const { return state_; }
    uint8_t getProgress() const { return progress_; }
    uint8_t getCurrentStep() const { return currentStep_; }
    uint8_t getTotalSteps() const { return totalSteps_; }
    uint16_t getRemainingTime() const;
    // Expected MIDI note at the current step. Returns -1 when there is no valid
    // step (empty sequence, puzzle solved, or step out of range). Without this,
    // an empty sequence returned 0 — which is MIDI 0 = C-1, a REAL note (8 Hz),
    // and the page/HA displayed "C-1" as if it were the expected note.
    // It also avoids reading sequence_[16]: the array has 16 positions and
    // currentStep_ reaches totalSteps_ after SOLVED.
    int getExpectedNote() const {
        if (totalSteps_ == 0) return -1;
        if (currentStep_ >= totalSteps_ || currentStep_ >= 16) return -1;
        return (int)sequence_[currentStep_];
    }
    // Last note actually played (what the engine consumed)
    int getLastPlayed() const { return lastPlayedMidi_; }

    bool isSolved() const { return state_ == EngineState::SOLVED; }
    bool hasError() const { return state_ == EngineState::ERROR; }

    // Injects the MicInput (MUSICAL_SEQUENCE mode). Constructor does NOT change.
    void setMicInput(esp32_escaperoom::MicInput* mic) { mic_ = mic; }

private:
    ConfigManager& config_;
    esp32_escaperoom::MicInput* mic_ = nullptr;
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

    // Musical mode: confirmation by repetition.
    // A note's ATTACK produces transient readings that do not match the
    // expected one; deciding on the first frame counts that as an error.
    // Requiring N CONSECUTIVE readings before deciding eliminates the false
    // attack error.
    uint8_t  noteStreak_  = 0;             // consecutive readings on the right note
    uint8_t  wrongStreak_ = 0;             // consecutive readings off target
    bool     awaitingNoteEnd_ = false;     // latch: same note does not advance twice
    int      lastPlayedMidi_ = -1;         // last note played (diagnostics/HA)
    static const uint8_t NOTE_CONFIRM_FRAMES = 3;

    // Per-note ERROR latch: reports once per distinct wrong note, instead of
    // on every NOTE_CONFIRM_FRAMES readings while it sounds (was ~8 lines/s).
    bool wrongNoteLatched_ = false;
    int  lastWrongMidi_ = -1;

    // Processing methods per mode
    void processCustomSequence();
    void processLinearSequence();
    void processAllConnected();
    void processRandomSequence();
    void processMusicalSequence();
    void playNote(int midi);              // records the note played (for HA)

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
