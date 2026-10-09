#pragma once

#include <Arduino.h>
#include "Hardware.h"
#include "PuzzleBase.h"
#include "GameLevelConfig.h"

namespace esp32_escaperoom {

class LogicManager {
 public:
  LogicManager(uint8_t hwId, bool isMaster = false) : hwId_(hwId), isMaster_(isMaster) {}

  void addPuzzle(PuzzleBase* p) {
    if (puzzleCount_ < MAX_PUZZLES) {
      puzzles_[puzzleCount_++] = p;
    }
  }

  void begin();
  void update();

  // --- Game Level Orchestration ---
  void setGameLevel(const GameLevelConfig* levels, uint8_t count);
  void startGame();
  uint8_t getGameLevel() const { return currentLevel_; }
  bool isGameComplete() const { return gameComplete_; }

  // Callbacks (override via subclass)
  virtual void onGameLevelEnter(uint8_t levelIndex) {}
  virtual void onGameLevelExit(uint8_t levelIndex) {}
  virtual void onGameComplete() {}
  virtual void onPuzzleSolved(uint8_t puzzleIndex) {}

  // Timer Control
  void startTimer(uint16_t seconds) {
    countdownSeconds_ = seconds;
    timerRunning_ = true;
    lastTickAt_ = millis();
    hw.display().printTime(countdownSeconds_ / 60, countdownSeconds_ % 60);
  }

  void stopTimer() {
    timerRunning_ = false;
  }

  uint16_t getRemainingTime() const { return countdownSeconds_; }

  // Status Broadcast

 private:
  void updateTimer();

  // Game Level internals
  static void onPuzzleStateChanged(PuzzleBase* puzzle, PuzzleState oldState, PuzzleState newState, void* userData);
  void handlePuzzleStateChange(PuzzleBase* puzzle, PuzzleState newState);
  void buildLevelMap();
  bool isLevelComplete(uint8_t levelIdx);
  void activateLevel(uint8_t levelIdx);
  void activateSequentialPuzzle(uint8_t levelIdx, uint8_t seqPos);
  uint8_t findPuzzleIndex(uint8_t puzzleId) const;

  uint8_t hwId_;
  bool isMaster_;
  static const uint8_t MAX_PUZZLES = 8;
  PuzzleBase* puzzles_[MAX_PUZZLES];
  uint8_t puzzleCount_ = 0;

  // Game Level State
  static const uint8_t MAX_LEVELS = 8;
  uint8_t levelCount_ = 0;
  GameLevelConfig levels_[MAX_LEVELS];
  uint8_t levelPosition_[MAX_LEVELS];          // Active puzzle within the level (SEQUENCE)
  uint8_t puzzleLevelMap_[MAX_PUZZLES];        // puzzleIndex -> levelIndex (0xFF for no level)
  uint8_t currentLevel_ = 0;
  bool gameStarted_ = false;
  bool gameComplete_ = false;

  // Poll fallback for state transitions
  PuzzleState previousStates_[MAX_PUZZLES];

  // Timer State
  uint16_t countdownSeconds_ = 0;
  bool timerRunning_ = false;
  unsigned long lastTickAt_ = 0;
};

}  // namespace esp32_escaperoom
