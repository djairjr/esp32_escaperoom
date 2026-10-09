#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

namespace esp32_escaperoom {

/**
 * @brief View of a NeoPixel strip range for a specific puzzle.
 *
 * Acts as a "window" over the physical NeoPixel strip,
 * allowing each puzzle to work with local indices (0 to count-1)
 * that are mapped to absolute positions on the physical strip.
 *
 * Example: PuzzlePixelView(strip, 11, 8) → maps indices 0-7
 * to LEDs 11-18 of the physical strip.
 */
class PuzzlePixelView {
 public:
  /**
   * @brief Constructor
   * @param strip Reference to the physical Adafruit_NeoPixel object
   * @param offset Starting index in the physical strip (0-based)
   * @param count Number of LEDs in this puzzle's range
   */
  PuzzlePixelView(Adafruit_NeoPixel* strip, uint8_t offset, uint8_t count)
      : strip_(strip), offset_(offset), count_(count) {}

  /**
   * @brief Sets the color of a specific LED (local index)
   * @param localIndex Local index of the LED (0 to count-1)
   * @param color Color in NeoPixel format (Color(r, g, b))
   * @return true if the index is valid, false otherwise
   */
  bool setPixel(uint8_t localIndex, uint32_t color) {
    if (!strip_ || localIndex >= count_) return false;
    strip_->setPixelColor(offset_ + localIndex, color);
    return true;
  }

  /**
   * @brief Fills the entire puzzle range with a color
   * @param color Color in NeoPixel format
   */
  void setAll(uint32_t color) {
    if (!strip_) return;
    for (uint8_t i = 0; i < count_; i++) {
      strip_->setPixelColor(offset_ + i, color);
    }
  }

  /**
   * @brief Turns off all LEDs in the range (black)
   */
  void clear() { setAll(0); }

  /**
   * @brief Sends the data to the physical strip (calls show() on the strip)
   * @return true if the strip exists, false otherwise
   */
  bool show() {
    if (!strip_) return false;
    strip_->show();
    return true;
  }

  /**
   * @brief Sets the color and calls show() in a single operation
   * @param localIndex Local index of the LED
   * @param color Color in NeoPixel format
   * @return true on success, false on an invalid index
   */
  bool setPixelAndShow(uint8_t localIndex, uint32_t color) {
    if (!setPixel(localIndex, color)) return false;
    return show();
  }

  /**
   * @brief Fills the range and calls show()
   * @param color Color in NeoPixel format
   */
  void setAllAndShow(uint32_t color) {
    setAll(color);
    show();
  }

  /**
   * @brief Returns the number of LEDs in this range
   */
  uint8_t count() const { return count_; }

  /**
   * @brief Returns the starting offset in the physical strip
   */
  uint8_t offset() const { return offset_; }

  /**
   * @brief Checks whether a local index is valid
   */
  bool isValidIndex(uint8_t localIndex) const { return localIndex < count_; }

  /**
   * @brief Returns access to the underlying physical strip
   * Useful for advanced operations not covered by this class
   */
  Adafruit_NeoPixel* strip() { return strip_; }

  /**
   * @brief Sets the global brightness of the strip (affects all puzzles!)
   * @param brightness Brightness from 0 to 255
   */
  void setBrightness(uint8_t brightness) {
    if (strip_) strip_->setBrightness(brightness);
  }

  /**
   * @brief Creates a color from RGB components
   * @return Color in NeoPixel format
   */
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }

  /**
   * @brief Creates a color from RGBW components
   * @return Color in NeoPixel format
   */
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
    return ((uint32_t)w << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }

 private:
  Adafruit_NeoPixel* strip_;
  uint8_t offset_;
  uint8_t count_;
};

}  // namespace esp32_escaperoom
