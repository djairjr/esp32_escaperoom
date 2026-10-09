#pragma once

#include <Arduino.h>
#include "MicInput.h"
#include "ConfigManager.h"

// XIAO ESP32-C3: IoAbstraction não compila (sem DAC).
// Este define é visível a PuzzleEngine.cpp (que inclui este header).
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

    /// Zera o progresso da sequencia e mantem o motor armado. Sem efeito
    /// visual e sem log — use nas pausas do jogador. NAO e' start().
    void restartProgress();

    EngineState getState() const { return state_; }
    uint8_t getProgress() const { return progress_; }
    uint8_t getCurrentStep() const { return currentStep_; }
    uint8_t getTotalSteps() const { return totalSteps_; }
    uint16_t getRemainingTime() const;
    // Nota MIDI esperada no passo atual. Devolve -1 quando NAO ha passo valido
    // (sequencia vazia, jogo resolvido, ou passo fora da faixa). Sem isto, uma
    // sequencia vazia devolvia 0 — que e' MIDI 0 = C-1, uma nota REAL (8 Hz),
    // e a pagina/HA exibiam "C-1" como se fosse a nota esperada.
    // Tambem evita ler sequence_[16]: o array tem 16 posicoes e currentStep_
    // chega a totalSteps_ apos o SOLVED.
    int getExpectedNote() const {
        if (totalSteps_ == 0) return -1;
        if (currentStep_ >= totalSteps_ || currentStep_ >= 16) return -1;
        return (int)sequence_[currentStep_];
    }
    // Ultima nota efetivamente tocada (o que o motor consumiu)
    int getLastPlayed() const { return lastPlayedMidi_; }

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

    // Modo musical: confirmacao por repeticao.
    // O ATAQUE de uma nota produz leituras transitorias que nao batem com a
    // esperada; decidir no primeiro frame conta isso como erro. Exigir N
    // leituras CONSECUTIVAS antes de decidir elimina o falso erro de ataque.
    uint8_t  noteStreak_  = 0;             // leituras seguidas na nota certa
    uint8_t  wrongStreak_ = 0;             // leituras seguidas fora do alvo
    bool     awaitingNoteEnd_ = false;     // trava: mesma nota nao avanca 2x
    int      lastPlayedMidi_ = -1;         // ultima nota tocada (diagnostico/HA)
    static const uint8_t NOTE_CONFIRM_FRAMES = 3;

    // Trava de ERRO por nota: acusa uma vez por nota errada distinta, em vez de
    // a cada NOTE_CONFIRM_FRAMES leituras enquanto ela soar (era ~8 linhas/s).
    bool wrongNoteLatched_ = false;
    int  lastWrongMidi_ = -1;

    // Métodos de processamento por modo
    void processCustomSequence();
    void processLinearSequence();
    void processAllConnected();
    void processRandomSequence();
    void processMusicalSequence();
    void playNote(int midi);              // registra a nota tocada (para o HA)

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