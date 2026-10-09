#include "PuzzleEngine.h"

namespace universal_controller {

PuzzleEngine::PuzzleEngine(ConfigManager& config) : config_(config) {
    memset(sequence_, 0, sizeof(sequence_));
    memset(lastInputStates_, 0, sizeof(lastInputStates_));
    memset(debounceTimes_, 0, sizeof(debounceTimes_));
}

void PuzzleEngine::begin() {
    const auto& logic = config_.getLogicConfig();
    const auto& inputs = config_.getInputConfig();

    // Initializes the sequence based on the mode
    switch (logic.mode) {
        case PuzzleMode::CUSTOM_SEQUENCE:
            // Uses the sequence defined in the JSON
            sequenceLength_ = logic.sequenceLength;
            for (uint8_t i = 0; i < sequenceLength_; i++) {
                sequence_[i] = logic.sequence[i] - 1;  // Converts to 0-based index
            }
            break;

        case PuzzleMode::LINEAR_SEQUENCE:
            generateLinearSequence();
            break;

        case PuzzleMode::ALL_CONNECTED:
            // All inputs must be activated
            sequenceLength_ = inputs.count;
            for (uint8_t i = 0; i < sequenceLength_; i++) {
                sequence_[i] = i;
            }
            break;

        case PuzzleMode::RANDOM:
            generateRandomSequence();
            break;

        case PuzzleMode::MUSICAL_SEQUENCE:
            // Sequence comes from the musical config (MIDI notes)
            sequenceLength_ = config_.getMusicalConfig().noteCount;
            for (uint8_t i = 0; i < sequenceLength_; i++) {
                sequence_[i] = config_.getMusicalConfig().notes[i].midi;
            }
            break;
    }

    totalSteps_ = sequenceLength_;
    currentStep_ = 0;
    progress_ = 0;

    // Sequence log
    Serial.printf("[PuzzleEngine] Mode: %s | Sequence: ", ConfigManager::modeToString(logic.mode));
    for (uint8_t i = 0; i < sequenceLength_; i++) {
        Serial.printf("%d ", sequence_[i] + 1);
    }
    Serial.printf("| Total steps: %d\n", totalSteps_);

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    // Configures NeoPixel if enabled (NOT in musical mode — no local feedback)
    const auto& np = config_.getNeoPixelConfig();
    if (logic.mode != PuzzleMode::MUSICAL_SEQUENCE && np.enabled && np.count > 0) {
        hw.pixelsExternal().updateLength(np.count);
        hw.pixelsExternal().begin();
        hw.pixelsExternal().setBrightness(128);
        hw.pixelsExternal().clear();
        hw.pixelsExternal().show();
    }
#endif

    state_ = EngineState::IDLE;
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    showIdleEffect();
#endif
}

void PuzzleEngine::update() {
    const auto& logic = config_.getLogicConfig();

    // GLOBAL start timeout — does NOT apply to musical mode.
    // The accordion is a PURE SENSOR: there is no start button, the game stays
    // armed while someone is playing. A clock running in the background would
    // end the game in the middle of the music (measured: TIMEOUT 30 s after
    // start, with the sequence intact).
    if (logic.mode != PuzzleMode::MUSICAL_SEQUENCE &&
        logic.timeoutSeconds > 0 && state_ == EngineState::RUNNING) {
        if (millis() - puzzleStartTime_ > logic.timeoutSeconds * 1000UL) {
            onTimeout();
            return;
        }
    }

    // Processes based on the state
    switch (state_) {
        case EngineState::IDLE:
            // Waits for a command to start
            break;

        case EngineState::RUNNING:
            switch (logic.mode) {
                case PuzzleMode::CUSTOM_SEQUENCE:
                    processCustomSequence();
                    break;
                case PuzzleMode::LINEAR_SEQUENCE:
                    processLinearSequence();
                    break;
                case PuzzleMode::ALL_CONNECTED:
                    processAllConnected();
                    break;
                case PuzzleMode::RANDOM:
                    processRandomSequence();
                    break;
                case PuzzleMode::MUSICAL_SEQUENCE:
                    processMusicalSequence();
                    break;
            }
            break;

        case EngineState::INPUT_CORRECT:
            // Waits a brief moment before continuing
            if (millis() - stateStartTime_ > 500) {
                state_ = EngineState::RUNNING;
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
                showRunningEffect();
#endif
            }
            break;

        case EngineState::INPUT_WRONG:
            // Waits a brief moment for feedback
            if (millis() - stateStartTime_ > 1000) {
                if (logic.resetOnError) {
                    reset();
                    start();
                } else {
                    state_ = EngineState::RUNNING;
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
                    showRunningEffect();
#endif
                }
            }
            break;

        case EngineState::ERROR:
        case EngineState::TIMEOUT:
            // Final error states
            break;

        case EngineState::SOLVED:
            // Keeps the solved effect
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
            showSolvedEffect();
#endif
            break;
    }

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    // Updates PWM
    updatePwmOutput();

    // Updates NeoPixel
    updateNeoPixel();
#endif
}

void PuzzleEngine::reset() {
    currentStep_ = 0;
    progress_ = 0;
    noteStreak_ = 0;
    wrongStreak_ = 0;
    awaitingNoteEnd_ = false;
    memset(lastInputStates_, 0, sizeof(lastInputStates_));
    memset(debounceTimes_, 0, sizeof(debounceTimes_));
    state_ = EngineState::IDLE;
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    showIdleEffect();
#endif
}

void PuzzleEngine::start() {
    puzzleStartTime_ = millis();
    state_ = EngineState::RUNNING;
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    showRunningEffect();
#endif
    Serial.println("[PuzzleEngine] Puzzle started");
}

// Resets the sequence PROGRESS and keeps the engine armed. Unlike start():
// it does not touch puzzleStartTime_, does not trigger a visual effect and
// PRINTS nothing. Used when the player pauses between phrases — frequent
// events in real use, where one log line per pause would clutter the serial
// without conveying anything.
void PuzzleEngine::restartProgress() {
    currentStep_ = 0;
    noteStreak_ = 0;
    wrongStreak_ = 0;
    awaitingNoteEnd_ = false;
    state_ = EngineState::RUNNING;
}

uint16_t PuzzleEngine::getRemainingTime() const {
    const auto& logic = config_.getLogicConfig();
    if (logic.timeoutSeconds == 0) return 0;

    unsigned long elapsed = (millis() - puzzleStartTime_) / 1000;
    if (elapsed >= logic.timeoutSeconds) return 0;
    return logic.timeoutSeconds - elapsed;
}

// ===== PROCESSING PER MODE =====

void PuzzleEngine::processCustomSequence() {
    // Checks whether there is a correct input in the sequence
    for (uint8_t i = 0; i < config_.getInputConfig().count; i++) {
        if (detectInputRisingEdge(i)) {
            uint8_t expectedInput = sequence_[currentStep_];

            if (i == expectedInput) {
                onInputCorrect(i);
            } else {
                onInputWrong(i);
            }
            break;  // Only processes one input per cycle
        }
    }
}

void PuzzleEngine::processLinearSequence() {
    // Same logic as custom sequence, but the sequence is 0,1,2,3...
    processCustomSequence();
}

void PuzzleEngine::processAllConnected() {
    // Checks whether all inputs are active
    uint8_t activeCount = 0;
    const auto& inputs = config_.getInputConfig();

    for (uint8_t i = 0; i < inputs.count; i++) {
        if (isInputActive(i)) {
            activeCount++;
        }
    }

    currentStep_ = activeCount;
    updateProgress();

    if (activeCount >= sequenceLength_) {
        onSequenceComplete();
    }
}

void PuzzleEngine::processRandomSequence() {
    // Same logic as custom sequence
    processCustomSequence();
}

void PuzzleEngine::playNote(int midi) {
    lastPlayedMidi_ = midi;
}

void PuzzleEngine::processMusicalSequence() {
    if (!mic_) return;

    const auto& musical = config_.getMusicalConfig();
    if (!musical.enabled || musical.noteCount == 0) return;

    // Frequency in Hz — NOT the rounded MIDI note. The tolerance is in cents,
    // and round() to an integer MIDI discards exactly the information it measures.
    float freq = mic_->detectFrequency();
    if (freq <= 0.0f) {
        // Silence: releases the note latch and resets the confirmations.
        // The latch is what prevents ONE note from advancing SEVERAL steps: without
        // it, the ~10 readings/s of a 400 ms note would confirm the new step
        // right after accepting the previous one.
        awaitingNoteEnd_ = false;
        noteStreak_ = 0;
        wrongStreak_ = 0;
        // Gap exceeded: resets the PROGRESS, not the game. It must stay in
        // RUNNING — reset() alone sets it to IDLE, and IDLE processes nothing
        // (the game would silently stop, which is the defect this fixes).
        if (lastInputTime_ > 0 && (millis() - lastInputTime_ > musical.maxGapMs)) {
            // Prolonged pause: the player stopped playing. Resets ONLY the
            // progress and keeps it armed — does NOT call start(): that would
            // restart the start clock, trigger the running visual effect and
            // print to serial. Called on every musical pause, that filled the
            // debug window and restarted state that should not be restarted.
            restartProgress();
        }

        // A new note can be flagged again: silence releases the error latch,
        // otherwise the SAME wrong note would never be reported again.
        wrongNoteLatched_ = false;
        return;
    }

    // Expected frequency of the target note (equal temperament, A4 = 440 Hz)
    uint8_t expectedMidi = sequence_[currentStep_];
    float expectedFreq = 440.0f * powf(2.0f, ((float)expectedMidi - 69.0f) / 12.0f);

    // Deviation in CENTS — the real unit of the tolerance
    float cents = 1200.0f * log2f(freq / expectedFreq);

    // Note closest to the read frequency (for diagnostics and for HA)
    int nearestMidi = (int)roundf(12.0f * log2f(freq / 440.0f) + 69.0f);
    playNote(nearestMidi);

    lastInputTime_ = millis();

    // Latch: while the SAME note keeps sounding, do not accept it again.
    // Released only by silence (above) or by a CLEARLY different note.
    if (awaitingNoteEnd_) {
        if (nearestMidi == lastPlayedMidi_) return;   // still the same note
        awaitingNoteEnd_ = false;                     // changed note -> release
    }

    if (fabsf(cents) <= (float)musical.toleranceCents) {
        // Correct — but requires CONFIRMATION: the note's attack generates
        // unstable readings, and deciding on the first frame counts that as an error.
        wrongStreak_ = 0;
        if (noteStreak_ < NOTE_CONFIRM_FRAMES) noteStreak_++;
        if (noteStreak_ >= NOTE_CONFIRM_FRAMES) {
            noteStreak_ = 0;
            awaitingNoteEnd_ = true;                  // latch until the note ends
            onNoteCorrect((uint8_t)nearestMidi);
        }
    } else {
        // Off target — also requires confirmation, otherwise an attack
        // transient (or a 1-reading error from the YIN) resets the progress.
        noteStreak_ = 0;
        if (wrongStreak_ < NOTE_CONFIRM_FRAMES) wrongStreak_++;
        if (wrongStreak_ >= NOTE_CONFIRM_FRAMES) {
            // Sustained wrong-note latch. Without this, onInputWrong() reset
            // the counter and flagged again every 3 readings (~120 ms): with the
            // wrong note sounding for 2 s, that was ~16 log lines and ~16
            // publications to HA for a SINGLE player mistake.
            // Flags ONCE and waits for the note to change (silence resets wrongStreak_).
            wrongStreak_ = 0;
            if (!wrongNoteLatched_ || lastWrongMidi_ != nearestMidi) {
                wrongNoteLatched_ = true;
                lastWrongMidi_ = (int)nearestMidi;
                onInputWrong(0);
            }
        }
    }
}


void PuzzleEngine::onNoteCorrect(uint8_t midi) {
    stateStartTime_ = millis();
    state_ = EngineState::INPUT_CORRECT;
    currentStep_++;
    updateProgress();

    Serial.printf("[PuzzleEngine] Note %d CORRECT | Progress: %d/%d\n",
                  midi, currentStep_, totalSteps_);

    if (currentStep_ >= totalSteps_) {
        onSequenceComplete();
    }
}

// ===== HELPERS =====

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
bool PuzzleEngine::detectInputRisingEdge(uint8_t pin) {
    const auto& inputs = config_.getInputConfig();
    if (pin >= inputs.count) return false;

    if (!hw.hasPortExpander()) return false;

    auto* io = hw.io();
    if (!io) return false;

    // Reads current state (LOW = active with PULLUP)
    uint8_t current = (io->digitalRead(inputs.pinMapping[pin]) == LOW) ? 1 : 0;
    uint8_t previous = lastInputStates_[pin];

    // Updates the state for the next call
    lastInputStates_[pin] = current;

    // Rising edge: inactive -> active
    return (previous == 0 && current == 1);
}

bool PuzzleEngine::isInputActive(uint8_t pin) {
    const auto& inputs = config_.getInputConfig();
    if (pin >= inputs.count) return false;

    if (!hw.hasPortExpander()) return false;

    auto* io = hw.io();
    if (!io) return false;

    return (io->digitalRead(inputs.pinMapping[pin]) == LOW);
}
#else
// ESP32-C3: no IoAbstraction — detectInputRisingEdge/isInputActive
// are never called (MUSICAL_SEQUENCE mode does not use physical inputs).
bool PuzzleEngine::detectInputRisingEdge(uint8_t) { return false; }
bool PuzzleEngine::isInputActive(uint8_t) { return false; }
#endif

void PuzzleEngine::onInputCorrect(uint8_t inputIndex) {
    stateStartTime_ = millis();
    state_ = EngineState::INPUT_CORRECT;
    currentStep_++;
    updateProgress();

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    playInputSound(currentStep_ - 1);
    showCorrectEffect();
#endif

    Serial.printf("[PuzzleEngine] Input %d CORRECT | Progress: %d/%d\n",
                  inputIndex + 1, currentStep_, totalSteps_);

    if (currentStep_ >= totalSteps_) {
        onSequenceComplete();
    }
}

void PuzzleEngine::onInputWrong(uint8_t inputIndex) {
    stateStartTime_ = millis();
    state_ = EngineState::INPUT_WRONG;

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    playErrorSound();
    showErrorEffect();
#endif

    // Musical mode: what exists is a NOTE, not "input 1". The old text
    // came from the INPUT puzzle model (buttons) and made the log look like a
    // wiring/button defect. Here it reports both notes in MIDI and by name.
    if (config_.getLogicConfig().mode == PuzzleMode::MUSICAL_SEQUENCE) {
        int esperada = getExpectedNote();
        Serial.printf("[PuzzleEngine] WRONG NOTE | played MIDI %d | expected MIDI %d (%s)\n",
                      (int)lastWrongMidi_, esperada,
                      esperada >= 0 ? ConfigManager::midiToNoteName((uint8_t)esperada) : "--");
    } else {
        Serial.printf("[PuzzleEngine] Input %d WRONG | Expected: %d\n",
                      inputIndex + 1, sequence_[currentStep_] + 1);
    }
}

void PuzzleEngine::onSequenceComplete() {
    state_ = EngineState::SOLVED;
    progress_ = 100;

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    playUnlockSound();
    showSolvedEffect();

    // Activates output (relay/lock)
    const auto& output = config_.getOutputConfig();
    hw.pinMode(output.lockPin, OUTPUT);
    hw.digitalWrite(output.lockPin, output.activeHigh ? HIGH : LOW);
#endif

    Serial.println("[PuzzleEngine] PUZZLE SOLVED!");
}

void PuzzleEngine::onTimeout() {
    state_ = EngineState::TIMEOUT;
    Serial.println("[PuzzleEngine] TIMEOUT!");
}

void PuzzleEngine::updateProgress() {
    if (totalSteps_ == 0) {
        progress_ = 0;
    } else {
        progress_ = (currentStep_ * 100) / totalSteps_;
    }
}

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
void PuzzleEngine::updatePwmOutput() {
    const auto& pwm = config_.getPwmConfig();
    if (!pwm.enabled) return;

    // Computes the PWM value based on the progress
    uint8_t value = map(progress_, 0, 100, pwm.minValue, pwm.maxValue);

    // Sets the PWM output
    hw.setAnalogOutput(pwm.channel, value);
}

void PuzzleEngine::updateNeoPixel() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    uint8_t ledsToLight = (progress_ * np.count) / 100;

    for (uint8_t i = 0; i < np.count; i++) {
        if (i < ledsToLight) {
            hw.pixelsExternal().setPixelColor(i, np.colorProgress);
        } else if (i == ledsToLight && state_ == EngineState::INPUT_CORRECT) {
            // Pulse effect on the current LED
            uint8_t brightness = 128 + 127 * sin(millis() / 200.0);
            uint32_t color = Adafruit_NeoPixel::Color(
                (np.colorProgress >> 16) & 0xFF * brightness / 255,
                (np.colorProgress >> 8) & 0xFF * brightness / 255,
                np.colorProgress & 0xFF * brightness / 255
            );
            hw.pixelsExternal().setPixelColor(i, color);
        } else {
            hw.pixelsExternal().setPixelColor(i, np.colorIdle);
        }
    }

    hw.pixelsExternal().show();
}

// ===== AUDIO =====

void PuzzleEngine::playInputSound(uint8_t index) {
    const auto& audio = config_.getAudioConfig();
    if (!audio.enabled) return;

    if (index < audio.inputSoundCount) {
        hw.sound().playOnce(audio.inputSounds[index]);
    }
}

void PuzzleEngine::playUnlockSound() {
    const auto& audio = config_.getAudioConfig();
    if (!audio.enabled) return;
    hw.sound().playOnce(audio.unlockSound);
}

void PuzzleEngine::playErrorSound() {
    const auto& audio = config_.getAudioConfig();
    if (!audio.enabled) return;
    hw.sound().playOnce(audio.errorSound);
}

void PuzzleEngine::playResetSound() {
    const auto& audio = config_.getAudioConfig();
    if (!audio.enabled) return;
    hw.sound().playOnce(audio.resetSound);
}

// ===== VISUAL EFFECTS =====

void PuzzleEngine::showIdleEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // All LEDs in the idle color, breathing effect
    for (uint8_t i = 0; i < np.count; i++) {
        hw.pixelsExternal().setPixelColor(i, np.colorIdle);
    }
    hw.pixelsExternal().show();
}

void PuzzleEngine::showRunningEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Empty progress bar
    for (uint8_t i = 0; i < np.count; i++) {
        hw.pixelsExternal().setPixelColor(i, np.colorIdle);
    }
    hw.pixelsExternal().show();
}

void PuzzleEngine::showCorrectEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Brief green flash
    for (uint8_t i = 0; i < np.count; i++) {
        hw.pixelsExternal().setPixelColor(i, np.colorSolved);
    }
    hw.pixelsExternal().show();
}

void PuzzleEngine::showErrorEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Red flash
    for (uint8_t i = 0; i < np.count; i++) {
        hw.pixelsExternal().setPixelColor(i, np.colorError);
    }
    hw.pixelsExternal().show();
}

void PuzzleEngine::showSolvedEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Celebration effect (green fade in/out)
    static unsigned long lastUpdate = 0;
    static uint8_t brightness = 0;
    static bool increasing = true;

    if (millis() - lastUpdate > 20) {
        lastUpdate = millis();

        if (increasing) {
            brightness += 5;
            if (brightness >= 255) {
                brightness = 255;
                increasing = false;
            }
        } else {
            brightness -= 5;
            if (brightness <= 100) {
                brightness = 100;
                increasing = true;
            }
        }

        uint32_t color = Adafruit_NeoPixel::Color(
            ((np.colorSolved >> 16) & 0xFF) * brightness / 255,
            ((np.colorSolved >> 8) & 0xFF) * brightness / 255,
            (np.colorSolved & 0xFF) * brightness / 255
        );

        for (uint8_t i = 0; i < np.count; i++) {
            hw.pixelsExternal().setPixelColor(i, color);
        }
        hw.pixelsExternal().show();
    }
}
#endif // ESP32_ESCAPEROOM_NO_IOABSTRACTION

// ===== SEQUENCE GENERATION =====

void PuzzleEngine::generateLinearSequence() {
    const auto& inputs = config_.getInputConfig();
    sequenceLength_ = inputs.count;
    for (uint8_t i = 0; i < sequenceLength_; i++) {
        sequence_[i] = i;
    }
}

void PuzzleEngine::generateRandomSequence() {
    const auto& inputs = config_.getInputConfig();
    sequenceLength_ = inputs.count;

    // Fills the array with all inputs
    uint8_t temp[16];
    for (uint8_t i = 0; i < sequenceLength_; i++) {
        temp[i] = i;
    }

    // Shuffle (Fisher-Yates)
    for (uint8_t i = sequenceLength_ - 1; i > 0; i--) {
        uint8_t j = random(0, i + 1);
        uint8_t swap = temp[i];
        temp[i] = temp[j];
        temp[j] = swap;
    }

    // Copies to the sequence
    for (uint8_t i = 0; i < sequenceLength_; i++) {
        sequence_[i] = temp[i];
    }
}

} // namespace universal_controller
