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

    // Inicializa sequência baseada no modo
    switch (logic.mode) {
        case PuzzleMode::CUSTOM_SEQUENCE:
            // Usa sequência definida no JSON
            sequenceLength_ = logic.sequenceLength;
            for (uint8_t i = 0; i < sequenceLength_; i++) {
                sequence_[i] = logic.sequence[i] - 1;  // Converte para índice 0-based
            }
            break;

        case PuzzleMode::LINEAR_SEQUENCE:
            generateLinearSequence();
            break;

        case PuzzleMode::ALL_CONNECTED:
            // Todas as entradas devem ser ativadas
            sequenceLength_ = inputs.count;
            for (uint8_t i = 0; i < sequenceLength_; i++) {
                sequence_[i] = i;
            }
            break;

        case PuzzleMode::RANDOM:
            generateRandomSequence();
            break;

        case PuzzleMode::MUSICAL_SEQUENCE:
            // Sequência vem do config musical (notas MIDI)
            sequenceLength_ = config_.getMusicalConfig().noteCount;
            for (uint8_t i = 0; i < sequenceLength_; i++) {
                sequence_[i] = config_.getMusicalConfig().notes[i].midi;
            }
            break;
    }

    totalSteps_ = sequenceLength_;
    currentStep_ = 0;
    progress_ = 0;

    // Log da sequência
    Serial.printf("[PuzzleEngine] Modo: %s | Sequência: ", ConfigManager::modeToString(logic.mode));
    for (uint8_t i = 0; i < sequenceLength_; i++) {
        Serial.printf("%d ", sequence_[i] + 1);
    }
    Serial.printf("| Total steps: %d\n", totalSteps_);

    // Configura NeoPixel se habilitado (NÃO no modo musical — sem feedback local)
    const auto& np = config_.getNeoPixelConfig();
    if (logic.mode != PuzzleMode::MUSICAL_SEQUENCE && np.enabled && np.count > 0) {
        hw.pixelsExternal().updateLength(np.count);
        hw.pixelsExternal().begin();
        hw.pixelsExternal().setBrightness(128);
        hw.pixelsExternal().clear();
        hw.pixelsExternal().show();
    }

    state_ = EngineState::IDLE;
    showIdleEffect();
}

void PuzzleEngine::update() {
    const auto& logic = config_.getLogicConfig();

    // Verifica timeout
    if (logic.timeoutSeconds > 0 && state_ == EngineState::RUNNING) {
        if (millis() - puzzleStartTime_ > logic.timeoutSeconds * 1000UL) {
            onTimeout();
            return;
        }
    }

    // Processa baseado no estado
    switch (state_) {
        case EngineState::IDLE:
            // Aguarda comando para iniciar
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
            // Aguarda breve momento antes de continuar
            if (millis() - stateStartTime_ > 500) {
                state_ = EngineState::RUNNING;
                showRunningEffect();
            }
            break;

        case EngineState::INPUT_WRONG:
            // Aguarda breve momento para feedback
            if (millis() - stateStartTime_ > 1000) {
                if (logic.resetOnError) {
                    reset();
                    start();
                } else {
                    state_ = EngineState::RUNNING;
                    showRunningEffect();
                }
            }
            break;

        case EngineState::ERROR:
        case EngineState::TIMEOUT:
            // Estados finais de erro
            break;

        case EngineState::SOLVED:
            // Mantém efeito de solução
            showSolvedEffect();
            break;
    }

    // Atualiza PWM
    updatePwmOutput();

    // Atualiza NeoPixel
    updateNeoPixel();
}

void PuzzleEngine::reset() {
    currentStep_ = 0;
    progress_ = 0;
    memset(lastInputStates_, 0, sizeof(lastInputStates_));
    memset(debounceTimes_, 0, sizeof(debounceTimes_));
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
    const auto& logic = config_.getLogicConfig();
    if (logic.timeoutSeconds == 0) return 0;

    unsigned long elapsed = (millis() - puzzleStartTime_) / 1000;
    if (elapsed >= logic.timeoutSeconds) return 0;
    return logic.timeoutSeconds - elapsed;
}

// ===== PROCESSAMENTO POR MODO =====

void PuzzleEngine::processCustomSequence() {
    // Verifica se há input correto na sequência
    for (uint8_t i = 0; i < config_.getInputConfig().count; i++) {
        if (detectInputRisingEdge(i)) {
            uint8_t expectedInput = sequence_[currentStep_];

            if (i == expectedInput) {
                onInputCorrect(i);
            } else {
                onInputWrong(i);
            }
            break;  // Só processa um input por ciclo
        }
    }
}

void PuzzleEngine::processLinearSequence() {
    // Mesma lógica de custom sequence, mas sequência é 0,1,2,3...
    processCustomSequence();
}

void PuzzleEngine::processAllConnected() {
    // Verifica se todas as entradas estão ativas
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
    // Mesma lógica de custom sequence
    processCustomSequence();
}

void PuzzleEngine::processMusicalSequence() {
    if (!mic_) return;

    const auto& musical = config_.getMusicalConfig();
    if (!musical.enabled || musical.noteCount == 0) return;

    // Lê a nota detectada pelo MicInput (MIDI ou -1 se silêncio)
    int detected = mic_->detectNote();
    if (detected < 0) {
        // Silêncio — verifica gap excedido
        if (lastInputTime_ > 0 && (millis() - lastInputTime_ > musical.maxGapMs)) {
            onTimeout();
        }
        return;
    }

    // Nota detectada — verifica se é a esperada (dentro da tolerância)
    uint8_t expected = sequence_[currentStep_];
    int diff = detected - expected;
    // Tolerância em cents: 1 semitom = 100 cents. Converte diff de semitons para cents.
    int diffCents = diff * 100;
    if (abs(diffCents) <= musical.toleranceCents) {
        onNoteCorrect((uint8_t)detected);
    } else {
        onInputWrong(0);
    }
    lastInputTime_ = millis();
}

void PuzzleEngine::onNoteCorrect(uint8_t midi) {
    stateStartTime_ = millis();
    state_ = EngineState::INPUT_CORRECT;
    currentStep_++;
    updateProgress();

    Serial.printf("[PuzzleEngine] Nota %d CORRETA | Progresso: %d/%d\n",
                  midi, currentStep_, totalSteps_);

    if (currentStep_ >= totalSteps_) {
        onSequenceComplete();
    }
}

// ===== HELPERS =====

bool PuzzleEngine::detectInputRisingEdge(uint8_t pin) {
    const auto& inputs = config_.getInputConfig();
    if (pin >= inputs.count) return false;

    if (!hw.hasPortExpander()) return false;

    auto* io = hw.io();
    if (!io) return false;

    // Lê estado atual (LOW = ativo com PULLUP)
    uint8_t current = (io->digitalRead(inputs.pinMapping[pin]) == LOW) ? 1 : 0;
    uint8_t previous = lastInputStates_[pin];

    // Atualiza estado para próxima chamada
    lastInputStates_[pin] = current;

    // Borda de subida: inativo -> ativo
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

void PuzzleEngine::onInputCorrect(uint8_t inputIndex) {
    stateStartTime_ = millis();
    state_ = EngineState::INPUT_CORRECT;
    currentStep_++;
    updateProgress();

    playInputSound(currentStep_ - 1);
    showCorrectEffect();

    Serial.printf("[PuzzleEngine] Input %d CORRETO | Progresso: %d/%d\n",
                  inputIndex + 1, currentStep_, totalSteps_);

    if (currentStep_ >= totalSteps_) {
        onSequenceComplete();
    }
}

void PuzzleEngine::onInputWrong(uint8_t inputIndex) {
    stateStartTime_ = millis();
    state_ = EngineState::INPUT_WRONG;

    playErrorSound();
    showErrorEffect();

    Serial.printf("[PuzzleEngine] Input %d ERRADO | Esperado: %d\n",
                  inputIndex + 1, sequence_[currentStep_] + 1);
}

void PuzzleEngine::onSequenceComplete() {
    state_ = EngineState::SOLVED;
    progress_ = 100;

    playUnlockSound();
    showSolvedEffect();

    // Ativa saída (relê/trava)
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
    if (totalSteps_ == 0) {
        progress_ = 0;
    } else {
        progress_ = (currentStep_ * 100) / totalSteps_;
    }
}

void PuzzleEngine::updatePwmOutput() {
    const auto& pwm = config_.getPwmConfig();
    if (!pwm.enabled) return;

    // Calcula valor PWM baseado no progresso
    uint8_t value = map(progress_, 0, 100, pwm.minValue, pwm.maxValue);

    // Define saída PWM
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
            // Efeito de pulso no LED atual
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

// ===== ÁUDIO =====

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

// ===== GERAÇÃO DE SEQUÊNCIAS =====

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

    // Preenche array com todas as entradas
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

    // Copia para sequência
    for (uint8_t i = 0; i < sequenceLength_; i++) {
        sequence_[i] = temp[i];
    }
}

// ===== EFEITOS VISUAIS =====

void PuzzleEngine::showIdleEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Todos os LEDs na cor idle, efeito de respiração
    for (uint8_t i = 0; i < np.count; i++) {
        hw.pixelsExternal().setPixelColor(i, np.colorIdle);
    }
    hw.pixelsExternal().show();
}

void PuzzleEngine::showRunningEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Barra de progresso vazia
    for (uint8_t i = 0; i < np.count; i++) {
        hw.pixelsExternal().setPixelColor(i, np.colorIdle);
    }
    hw.pixelsExternal().show();
}

void PuzzleEngine::showCorrectEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Flash verde breve
    for (uint8_t i = 0; i < np.count; i++) {
        hw.pixelsExternal().setPixelColor(i, np.colorSolved);
    }
    hw.pixelsExternal().show();
}

void PuzzleEngine::showErrorEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Flash vermelho
    for (uint8_t i = 0; i < np.count; i++) {
        hw.pixelsExternal().setPixelColor(i, np.colorError);
    }
    hw.pixelsExternal().show();
}

void PuzzleEngine::showSolvedEffect() {
    const auto& np = config_.getNeoPixelConfig();
    if (!np.enabled || np.count == 0) return;

    // Efeito de celebração (fade in/out verde)
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

} // namespace universal_controller