#pragma once

#include <Arduino.h>

namespace esp32_escaperoom {

/**
 * Wrapper for the unified RotaryDial library (telephone rotary dial).
 * Provides a simple interface for reading dialed digits.
 *
 * Supported modes (defined in the library):
 *   ROTARYDIAL_UNIFIED_MECHANICAL (default) — metallic contacts
 *   ROTARYDIAL_UNIFIED_OPTICAL            — optical encoder
 *
 * External dependency:
 *   RotaryDial library v1.0.0+ in Documents/Arduino/libraries/RotaryDial
 *
 * Hardware wiring:
 *   - Connect the NC (Normal Closed) terminal of the dial to the GPIO pin
 *   - Connect the other terminal to GND
 *   - Use INPUT_PULLUP or INPUT (depending on the telephone model)
 */
class RotaryDialUnifiedReader {
 public:
  RotaryDialUnifiedReader() : initialized_(false), lastPin_(0) {}

  /**
   * Initializes the rotary dial reader in the default mode (MECHANICAL).
   * @param pin GPIO pin connected to the NC terminal of the dial
   * @return true if initialized successfully
   */
  bool begin(uint8_t pin);

  /**
   * Initializes the reader in optical mode, with calibrated timings.
   * @param pin       GPIO pin connected to the optical sensor
   * @param debounce  Floor (ms) — filters noise and return pulses (e.g. 50)
   * @param maxgap    Ceiling (ms) — 0 = no ceiling (recommended for the Djair dial)
   * @param digitend  Timeout (ms) to close the digit (e.g. 800)
   * @return true if initialized successfully
   */
  bool beginOptical(uint8_t pin, unsigned long debounce, unsigned long maxgap, unsigned long digitend);

  /**
   * Checks whether a digit is available for reading.
   * @return true if a digit was dialed and is ready
   */
  bool available();

  /**
   * Reads the dialed digit.
   * @return Dialed digit (0-9), or -1 if not available
   */
  int read();

  /** Clears the internal RotaryDial buffer (pulseCount, firstPulse, etc). */
  void reset();

  /**
   * Checks whether it was initialized.
   */
  bool isInitialized() const { return initialized_; }

  /**
   * Returns the configured pin.
   */
  uint8_t getPin() const { return lastPin_; }

 private:
  bool initialized_;
  uint8_t lastPin_;
};

}  // namespace esp32_escaperoom
