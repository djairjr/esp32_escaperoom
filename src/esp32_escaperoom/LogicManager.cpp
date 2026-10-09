#include "LogicManager.h"

namespace esp32_escaperoom {

void LogicManager::begin() {
  for (uint8_t i = 0; i < puzzleCount_; i++) {
    puzzles_[i]->begin();
    puzzles_[i]->setStateChangeCallback(onPuzzleStateChanged, this);
    previousStates_[i] = puzzles_[i]->getState();
  }
  buildLevelMap();
}

void LogicManager::update() {
  updateTimer();
  if (levelCount_ == 0) {
    for (uint8_t i = 0; i < puzzleCount_; i++) puzzles_[i]->update();
    return;
  }
  if (!gameStarted_ || gameComplete_ || currentLevel_ >= levelCount_) return;
  const GameLevelConfig& level = levels_[currentLevel_];
  for (uint8_t i = 0; i < puzzleCount_; i++) {
    if (puzzleLevelMap_[i] != currentLevel_) continue;
    if (level.mode == GameLevelMode::SEQUENCE) {
      uint8_t seqId = level.puzzleIds[levelPosition_[currentLevel_]];
      if (puzzles_[i]->getId() == seqId) puzzles_[i]->update();
    } else {
      puzzles_[i]->update();
    }
  }
  for (uint8_t i = 0; i < puzzleCount_; i++) {
    PuzzleState cur = puzzles_[i]->getState();
    if (cur != previousStates_[i]) {
      previousStates_[i] = cur;
      if (cur == PuzzleState::Solved) handlePuzzleStateChange(puzzles_[i], cur);
    }
  }
}

void LogicManager::setGameLevel(const GameLevelConfig* levels, uint8_t count) {
  if (count > MAX_LEVELS) count = MAX_LEVELS;
  levelCount_ = count;
  for (uint8_t i = 0; i < count; i++) { levels_[i] = levels[i]; levelPosition_[i] = 0; }
}

void LogicManager::startGame() {
  if (levelCount_ == 0) return;
  gameStarted_ = true; gameComplete_ = false; currentLevel_ = 0;
  activateLevel(0); onGameLevelEnter(0);
}

void LogicManager::buildLevelMap() {
  for (uint8_t i = 0; i < MAX_PUZZLES; i++) puzzleLevelMap_[i] = 0xFF;
  for (uint8_t l = 0; l < levelCount_; l++)
    for (uint8_t j = 0; j < levels_[l].puzzleCount; j++) {
      uint8_t idx = findPuzzleIndex(levels_[l].puzzleIds[j]);
      if (idx < MAX_PUZZLES) puzzleLevelMap_[idx] = l;
    }
}

bool LogicManager::isLevelComplete(uint8_t levelIdx) {
  if (levelIdx >= levelCount_) return true;
  if (levels_[levelIdx].mode == GameLevelMode::SEQUENCE)
    return levelPosition_[levelIdx] >= levels_[levelIdx].puzzleCount;
  for (uint8_t i = 0; i < puzzleCount_; i++)
    if (puzzleLevelMap_[i] == levelIdx && puzzles_[i]->getState() != PuzzleState::Solved) return false;
  return true;
}

void LogicManager::activateLevel(uint8_t levelIdx) {
  if (levelIdx >= levelCount_) return;
  levelPosition_[levelIdx] = 0;
  if (levels_[levelIdx].mode == GameLevelMode::PARALLEL) {
    for (uint8_t i = 0; i < puzzleCount_; i++)
      if (puzzleLevelMap_[i] == levelIdx) puzzles_[i]->run();
  } else {
    activateSequentialPuzzle(levelIdx, 0);
  }
}

void LogicManager::activateSequentialPuzzle(uint8_t levelIdx, uint8_t seqPos) {
  if (levelIdx >= levelCount_) return;
  if (seqPos >= levels_[levelIdx].puzzleCount) return;
  uint8_t pid = levels_[levelIdx].puzzleIds[seqPos];
  for (uint8_t i = 0; i < puzzleCount_; i++)
    if (puzzles_[i]->getId() == pid) { puzzles_[i]->run(); break; }
}

void LogicManager::onPuzzleStateChanged(PuzzleBase* puzzle, PuzzleState, PuzzleState newState, void* userData) {
  if (newState == PuzzleState::Solved)
    static_cast<LogicManager*>(userData)->handlePuzzleStateChange(puzzle, newState);
}

void LogicManager::handlePuzzleStateChange(PuzzleBase* puzzle, PuzzleState) {
  if (!gameStarted_ || levelCount_ == 0 || gameComplete_) return;
  uint8_t pIdx = findPuzzleIndex(puzzle->getId());
  if (pIdx < MAX_PUZZLES) { previousStates_[pIdx] = PuzzleState::Solved; onPuzzleSolved(pIdx); }
  if (!isLevelComplete(currentLevel_) && levels_[currentLevel_].mode == GameLevelMode::SEQUENCE) {
    levelPosition_[currentLevel_]++;
    if (!isLevelComplete(currentLevel_)) activateSequentialPuzzle(currentLevel_, levelPosition_[currentLevel_]);
  }
  if (isLevelComplete(currentLevel_)) {
    onGameLevelExit(currentLevel_); currentLevel_++;
    if (currentLevel_ >= levelCount_) { gameComplete_ = true; onGameComplete(); }
    else { activateLevel(currentLevel_); onGameLevelEnter(currentLevel_); }
  }
}

uint8_t LogicManager::findPuzzleIndex(uint8_t puzzleId) const {
  for (uint8_t i = 0; i < puzzleCount_; i++)
    if (puzzles_[i]->getId() == puzzleId) return i;
  return 0xFF;
}

void LogicManager::updateTimer() {
  if (!timerRunning_) return;
  if (millis() - lastTickAt_ >= 1000) {
    lastTickAt_ += 1000;
    if (countdownSeconds_ > 0) {
      countdownSeconds_--;
      hw.display().printTime(countdownSeconds_ / 60, countdownSeconds_ % 60);
      if (countdownSeconds_ == 0) { Serial.println("TIME UP!"); timerRunning_ = false; }
    }
  }
}


}
