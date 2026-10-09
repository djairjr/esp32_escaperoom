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
    setState(PuzzleState::Idle); // Keeps the idle pattern (Orchestration)
  }

  // Semantic shortcut to start the puzzle
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

  // State change callback (used by LogicManager for game levels)
  typedef void (*StateChangeCallback)(PuzzleBase* puzzle, PuzzleState oldState, PuzzleState newState, void* userData);

  void setStateChangeCallback(StateChangeCallback cb, void* userData = nullptr) {
    stateChangeCb_ = cb;
    stateChangeCbData_ = userData;
  }

  // Network Orchestration
  // Organicity & Effects Helpers
  float getSine(float period, float minVal = 0, float maxVal = 255) {
    return Hardware::sineWave(period, minVal, maxVal);
  }

  uint8_t getFlicker(uint8_t baseVal, uint8_t amount) {
    return (uint8_t)constrain(baseVal + random(-amount, amount), 0, 255);
  }

  // ===== NEW SEQUENCE METHODS =====

  /**
   * Generates a random sequence of size 'count' using inputs from 0 to 'maxInput'.
   * Repetitions may occur.
   */
  void generateRandomSequence(uint8_t* sequence, uint8_t count, uint8_t maxInput);

  /**
   * Generates a random sequence without repetitions.
   * Useful for puzzles where each input can appear only once.
   */
  void generateRandomSequenceNoRepeat(uint8_t* sequence, uint8_t count, uint8_t maxInput);

  /**
   * Generates random pairs of inputs (useful for ConnectWires-type puzzles).
   * 'pairCount' defines how many pairs will be generated.
   */
  void generateRandomPairs(uint8_t* pairs, uint8_t pairCount, uint8_t maxInput);

  /**
   * Checks whether ALL the specified inputs are active (logical AND).
   * Returns true only if all of them are active.
   */
  bool areInputsActive(uint8_t* inputs, uint8_t count);

  /**
   * Checks whether at least N of any inputs are active.
   * Useful for ALL_CONNECTED mode.
   */
  bool areNInputsActive(uint8_t n, uint8_t maxInput);

  /**
   * Checks whether the current input sequence matches the expected one.
   * Useful for validating progress in real time.
   */
  bool checkSequence(uint8_t* sequence, uint8_t sequenceLen, uint8_t* currentInputs, uint8_t currentLen);

  /**
   * Resets the index of the current sequence.
   */
  void resetSequence() { sequenceIndex_ = 0; }

  /**
   * Returns the current index of the sequence.
   */
  uint8_t getSequenceIndex() const { return sequenceIndex_; }

  /**
   * Sets the current index of the sequence.
   */
  void setSequenceIndex(uint8_t index) { sequenceIndex_ = index; }

  /**
   * Advances in the sequence if the input is correct.
   * Returns true if the input matches the expected one.
   */
  bool advanceSequence(uint8_t expectedInput, uint8_t actualInput);

  /**
   * Checks whether the sequence is complete.
   */
  bool isSequenceComplete(uint8_t sequenceLen) const { return sequenceIndex_ >= sequenceLen; }

  // ===== INPUT HELPERS =====

  /**
   * Reads an input with debounce (noise filter).
   * Returns true when the stabilized state is ACTIVE (LOW).
   */
  bool readInputDebounce(uint8_t pin, uint16_t debounceMs = 50);

  /**
   * Detects a rising edge (transition from inactive to active).
   * Returns true only at the moment of the transition.
   */
  bool detectRisingEdge(uint8_t pin);

  /**
   * Detects a falling edge (transition from active to inactive).
   * Returns true only at the moment of the transition.
   */
  bool detectFallingEdge(uint8_t pin);

  /**
   * Waits until one of the specified inputs is activated.
   * Returns the index of the activated input, or -1 on timeout.
   */
  int8_t waitForInput(uint8_t* inputs, uint8_t count, uint16_t timeoutMs = 0);

  /**
   * Waits until any input is activated.
   * Returns the number of the activated input, or -1 on timeout.
   */
  int8_t waitForAnyInput(uint8_t maxInput, uint16_t timeoutMs = 0);

  // ===== PROGRESS HELPERS =====

  /**
   * Computes the progress percentage (0-100).
   */
  static uint8_t calculateProgressPercent(uint8_t currentStep, uint8_t totalSteps);

  /**
   * Computes progress as a float (0.0 - 1.0).
   */
  static float calculateProgressFloat(uint8_t currentStep, uint8_t totalSteps);

  /**
   * Maps progress to a range of values.
   * Useful for computing a PWM value based on progress.
   */
  static uint16_t mapProgressToRange(uint8_t currentStep, uint8_t totalSteps, uint16_t minVal, uint16_t maxVal);

 public:
  // ===== PIXEL (NEOPIXEL) MANAGEMENT =====

  /**
   * @brief Defines the pixel range that this puzzle controls
   * @param offset Starting index in the physical strip (0-based)
   * @param count Number of LEDs allocated to this puzzle
   *
   * Must be called before registering with the LogicManager (usually in the constructor).
   * Example: setPixelRange(0, 10) → LEDs 0-9
   *          setPixelRange(11, 8) → LEDs 11-18
   */
  void setPixelRange(uint8_t offset, uint8_t count);

  /**
   * @brief Returns access to this puzzle's pixel view
   * @return Reference to PuzzlePixelView
   * @note Returns nullptr if setPixelRange() has not been called
   */
  PuzzlePixelView* pixels() { return pixelView_; }

  /**
   * @brief Checks whether this puzzle has a defined pixel range
   */
  bool hasPixels() const { return pixelView_ != nullptr; }

  /**
   * @brief Returns the starting offset of the pixel range
   */
  uint8_t pixelOffset() const { return pixelOffset_; }

  /**
   * @brief Returns the number of pixels in the range
   */
  uint8_t pixelCount() const { return pixelCount_; }

 protected:
  uint8_t id_;
  PuzzleState state_;
  uint8_t internalValue_ = 0;

  // State callback (optional, 8 bytes when used)
  StateChangeCallback stateChangeCb_ = nullptr;
  void* stateChangeCbData_ = nullptr;

  // Sequence attributes
  uint8_t sequenceIndex_ = 0;

  // Pixel management attributes
  uint8_t pixelOffset_ = 0;
  uint8_t pixelCount_ = 0;
  PuzzlePixelView* pixelView_ = nullptr;

  // Arrays for debounce and edge detection (max 16 inputs)
  uint8_t lastInputState_[16] = {0};
  uint8_t currentInputState_[16] = {0};
  uint8_t edgeLastState_[16] = {0};
  unsigned long lastDebounceTime_[16] = {0};
};

}  // namespace esp32_escaperoom
