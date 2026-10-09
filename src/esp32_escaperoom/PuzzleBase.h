#pragma once

#include <Arduino.h>
#include "Hardware.h"
#include "PuzzlePixelView.h"
#include "PuzzleState.h"

namespace esp32_escaperoom {

class PuzzleBase {
 public:
  PuzzleBase(uint8_t id) : id_(id), state_(PuzzleState::Initialising) {}
  virtual ~PuzzleBase() {}

  virtual void begin() {
    setupHardware();
    setState(PuzzleState::Idle); // Mantém o padrão de espera (Orquestração)
  }

  // Atalho semântico para iniciar o puzzle
  void run() {
    Serial.printf("[PUZZLE %d] Manual Start Sequence (run() call)\n", id_);
    setState(PuzzleState::Running);
  }

  virtual void update() = 0;
  virtual void setupHardware() = 0;

  virtual uint8_t getProperty() { return internalValue_; }

  // State Management
  void setState(PuzzleState s) {
    if (state_ == s) return;
    PuzzleState old = state_;
    state_ = s;
    Serial.printf("[PUZZLE %d] State Change: %s -> %s\n", id_, stateName(old), stateName(s));
    if (stateChangeCb_) {
      stateChangeCb_(this, old, state_, stateChangeCbData_);
    }
  }
  PuzzleState getState() const { return state_; }
  uint8_t getId() const { return id_; }

  static const char* stateName(PuzzleState s) {
    switch(s) {
      case PuzzleState::Initialising: return "Initialising";
      case PuzzleState::Idle:         return "Idle";
      case PuzzleState::Running:      return "Running";
      case PuzzleState::Solved:       return "Solved";
      case PuzzleState::Inactive:     return "Inactive";
      case PuzzleState::Error:        return "Error";
      case PuzzleState::Disabled:     return "Disabled";
      default:                        return "Unknown";
    }
  }

  // Callback de mudanca de estado (usado pelo LogicManager para game levels)
  typedef void (*StateChangeCallback)(PuzzleBase* puzzle, PuzzleState oldState, PuzzleState newState, void* userData);

  void setStateChangeCallback(StateChangeCallback cb, void* userData = nullptr) {
    stateChangeCb_ = cb;
    stateChangeCbData_ = userData;
  }

  // Orquestração de Rede
  // Organicity & Effects Helpers
  float getSine(float period, float minVal = 0, float maxVal = 255) {
    return Hardware::sineWave(period, minVal, maxVal);
  }

  uint8_t getFlicker(uint8_t baseVal, uint8_t amount) {
    return (uint8_t)constrain(baseVal + random(-amount, amount), 0, 255);
  }

  // ===== NOVOS MÉTODOS DE SEQUÊNCIA =====

  /**
   * Gera uma sequência aleatória de tamanho 'count' usando entradas de 0 a 'maxInput'.
   * Pode haver repetições.
   */
  void generateRandomSequence(uint8_t* sequence, uint8_t count, uint8_t maxInput);

  /**
   * Gera uma sequência aleatória sem repetições.
   * Útil para puzzles onde cada entrada só pode aparecer uma vez.
   */
  void generateRandomSequenceNoRepeat(uint8_t* sequence, uint8_t count, uint8_t maxInput);

  /**
   * Gera pares aleatórios de entradas (útil para puzzles tipo ConnectWires).
   * 'pairCount' define quantos pares serão gerados.
   */
  void generateRandomPairs(uint8_t* pairs, uint8_t pairCount, uint8_t maxInput);

  /**
   * Verifica se TODAS as entradas especificadas estão ativas (AND lógico).
   * Retorna true apenas se todas estiverem ativas.
   */
  bool areInputsActive(uint8_t* inputs, uint8_t count);

  /**
   * Verifica se pelo menos N entradas quaisquer estão ativas.
   * Útil para modo ALL_CONNECTED.
   */
  bool areNInputsActive(uint8_t n, uint8_t maxInput);

  /**
   * Verifica se a sequência de inputs atual corresponde ao esperado.
   * Útil para validar progresso em tempo real.
   */
  bool checkSequence(uint8_t* sequence, uint8_t sequenceLen, uint8_t* currentInputs, uint8_t currentLen);

  /**
   * Reseta o índice da sequência atual.
   */
  void resetSequence() { sequenceIndex_ = 0; }

  /**
   * Retorna o índice atual da sequência.
   */
  uint8_t getSequenceIndex() const { return sequenceIndex_; }

  /**
   * Define o índice atual da sequência.
   */
  void setSequenceIndex(uint8_t index) { sequenceIndex_ = index; }

  /**
   * Avança na sequência se o input for correto.
   * Retorna true se o input corresponde ao esperado.
   */
  bool advanceSequence(uint8_t expectedInput, uint8_t actualInput);

  /**
   * Verifica se a sequência está completa.
   */
  bool isSequenceComplete(uint8_t sequenceLen) const { return sequenceIndex_ >= sequenceLen; }

  // ===== HELPERS DE INPUT =====

  /**
   * Lê uma entrada com debounce (filtro de ruído).
   * Retorna true quando o estado estabilizado é ATIVO (LOW).
   */
  bool readInputDebounce(uint8_t pin, uint16_t debounceMs = 50);

  /**
   * Detecta borda de subida (transição de inativo para ativo).
   * Retorna true apenas no momento da transição.
   */
  bool detectRisingEdge(uint8_t pin);

  /**
   * Detecta borda de descida (transição de ativo para inativo).
   * Retorna true apenas no momento da transição.
   */
  bool detectFallingEdge(uint8_t pin);

  /**
   * Aguarda até que uma das entradas especificadas seja ativada.
   * Retorna o índice da entrada ativada, ou -1 se timeout.
   */
  int8_t waitForInput(uint8_t* inputs, uint8_t count, uint16_t timeoutMs = 0);

  /**
   * Aguarda até que qualquer entrada seja ativada.
   * Retorna o número da entrada ativada, ou -1 se timeout.
   */
  int8_t waitForAnyInput(uint8_t maxInput, uint16_t timeoutMs = 0);

  // ===== HELPERS DE PROGRESSO =====

  /**
   * Calcula percentual de progresso (0-100).
   */
  static uint8_t calculateProgressPercent(uint8_t currentStep, uint8_t totalSteps);

  /**
   * Calcula progresso como float (0.0 - 1.0).
   */
  static float calculateProgressFloat(uint8_t currentStep, uint8_t totalSteps);

  /**
   * Mapeia progresso para um intervalo de valores.
   * Útil para calcular valor PWM baseado no progresso.
   */
  static uint16_t mapProgressToRange(uint8_t currentStep, uint8_t totalSteps, uint16_t minVal, uint16_t maxVal);

 public:
  // ===== GESTÃO DE PIXELS (NEOPIXEL) =====

  /**
   * @brief Define a faixa de pixels que este puzzle controla
   * @param offset Índice inicial no strip físico (0-based)
   * @param count Número de LEDs alocados para este puzzle
   *
   * Deve ser chamado antes do registro no LogicManager (geralmente no construtor).
   * Exemplo: setPixelRange(0, 10) → LEDs 0-9
   *          setPixelRange(11, 8) → LEDs 11-18
   */
  void setPixelRange(uint8_t offset, uint8_t count);

  /**
   * @brief Retorna acesso à view de pixels deste puzzle
   * @return Referência para PuzzlePixelView
   * @note Retorna nullptr se setPixelRange() não foi chamado
   */
  PuzzlePixelView* pixels() { return pixelView_; }

  /**
   * @brief Verifica se este puzzle tem uma faixa de pixels definida
   */
  bool hasPixels() const { return pixelView_ != nullptr; }

  /**
   * @brief Retorna o offset inicial da faixa de pixels
   */
  uint8_t pixelOffset() const { return pixelOffset_; }

  /**
   * @brief Retorna o número de pixels na faixa
   */
  uint8_t pixelCount() const { return pixelCount_; }

 protected:
  uint8_t id_;
  PuzzleState state_;
  uint8_t internalValue_ = 0;

  // Callback de estado (opcional, 8 bytes quando usado)
  StateChangeCallback stateChangeCb_ = nullptr;
  void* stateChangeCbData_ = nullptr;

  // Atributos para sequência
  uint8_t sequenceIndex_ = 0;

  // Atributos para gestão de pixels
  uint8_t pixelOffset_ = 0;
  uint8_t pixelCount_ = 0;
  PuzzlePixelView* pixelView_ = nullptr;

  // Arrays para debounce e detecção de borda (máx 16 entradas)
  uint8_t lastInputState_[16] = {0};
  uint8_t currentInputState_[16] = {0};
  uint8_t edgeLastState_[16] = {0};
  unsigned long lastDebounceTime_[16] = {0};
};

}  // namespace esp32_escaperoom
