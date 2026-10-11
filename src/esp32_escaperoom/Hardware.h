#pragma once

#include <Arduino.h>

#include <Adafruit_LIS3DH.h> // Important to have the native accelerometer
#include <Adafruit_NeoPixel.h>
#include <Adafruit_Sensor.h>
#include <Arduino.h>
#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>
#include <ESP32Servo.h>
#include <Wire.h>

#include "Display14Seg.h"
#include "NfcReader.h"
#include "PlatformPins.h"
#include "RotaryEncoder.h"
#include "RotaryDialUnifiedReader.h"
#include "AudioPlayer.h"
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
#include <IoAbstractionWire.h>
#endif
#include <ADS1115_WE.h>
#include <PCA9685.h>

namespace esp32_escaperoom {
typedef uint8_t PinModeType;
typedef uint8_t PinStatusType;


class Hardware {
public:
  static Hardware &instance() {
    static Hardware instance;
    return instance;
  }

  void begin();

  // Hardware Detection Flags
  bool hasPortExpander() const { return hasPortExpander_; }
  bool hasAccelerometer() const { return hasAccelerometer_; }
  bool hasPwmDriver() const { return hasPwmDriver_; }
  bool hasDisplay() const { return hasDisplay_; }
  bool hasRtc() const { return hasRtc_; }
  bool hasOled() const { return hasOled_; }
  bool hasAdc() const { return hasAdc_; }
  bool hasNfc() const { return hasNfc_; }
  bool hasCodec() const { return hasCodec_; }
  bool isSystemHealthy() const {
    return hasPortExpander_ && hasAccelerometer_ && hasDisplay_;
  }

  // Port Expander Access - with nullptr check and error log
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
  PCF8574IoAbstraction* portExpander(uint8_t index = 0) {
    if (expanderCount_ == 0 || expanders_[0] == nullptr) {
      Serial.printf("[PCF857x] Disconnected - Cannot portExpander(index=%d) - call hw.addPcf8574/8575() in setup()\n", index);
      return nullptr;
    }
    if (index >= expanderCount_) {
      Serial.printf("[PCF857x] Invalid index - portExpander(%d), max=%d\n", index, expanderCount_);
      return expanders_[0];
    }
    if (expanders_[index] == nullptr) {
      Serial.printf("[PCF857x] nullptr - portExpander(index=%d)\n", index);
      return nullptr;
    }
    return expanders_[index];
  }
  PCF8574IoAbstraction* inputOutput(uint8_t index = 0) {
    return portExpander(index);
  } // Legacy alias
  PCF8574IoAbstraction* io(uint8_t index = 0) {
    return portExpander(index);
  } // Short alias

  // Get expander by I2C address
  PCF8574IoAbstraction* getExpanderByAddress(uint8_t address);
  int getExpanderIndexByAddress(uint8_t address);

  bool hasPcf8575(uint8_t index = 0) const {
    return (index < expanderCount_) ? isPcf8575_[index] : false;
  }
  int expanderCount() const { return expanderCount_; }

  // Manual Expander Configuration (preferred over auto-detection)
  // Use this to explicitly configure expanders instead of relying on auto-detection
  void addExpander(uint8_t address, bool is16bit = false);
  void addPcf8574(uint8_t address) { addExpander(address, false); }
  void addPcf8575(uint8_t address) { addExpander(address, true); }

  // Safe check before using expanders
  bool hasExpander() const { return expanderCount_ > 0 && expanders_[0] != nullptr; }
  bool hasExpander(uint8_t index) const {
    return index < expanderCount_ && expanders_[index] != nullptr;
  }
#endif

  // File System Status
  bool isLittleFSMounted() const { return littleFsMounted_; }
  bool isSDMounted() const { return sdMounted_; }
  
  // Power & Core
  void setPower5V(bool on);

  // Battery Monitoring
  void beginBatteryMonitor(uint8_t sensePin = PIN_BATTERY_SENSE);
  float readBatteryVoltage();
  float readBatteryPercent();
  bool isBatteryConnected();

  // Low Power Mode - dimming proportional to battery
  void setLowPowerMode(bool enabled);
  bool getLowPowerMode() const { return lowPowerEnabled_; }
  void updatePowerSave();
  void setPowerSaveThresholds(float warnV, float criticalV);

  // Sound (Audio)
  AudioPlayer &sound() { return audio_; }


  // Display - with nullptr check
  Display14Seg &display() {
    if (!hasDisplay_) {
      Serial.println(F("[HT16K33] Disconnected - Cannot display() - subsequent operations will fail"));
    }
    return display_;
  }

  // Network
  // Light (NeoPixel) - Direct access for testing
  Adafruit_NeoPixel &pixelsExternal() { return pixelsExternal_; }
  Adafruit_NeoPixel &pixelsInternal() { return pixelsInternal_; }

  void setLight(uint16_t n, uint32_t c) {
    pixelsExternal_.setPixelColor(n, c);
    pixelsExternal_.show();
  }
  void setLightAll(uint32_t c) {
    pixelsExternal_.fill(c);
    pixelsExternal_.show();
  }
  void setLightBrightness(uint8_t b) {
    pixelsExternal_.setBrightness(b);
    pixelsExternal_.show();
  }
  void setLightInternal(uint16_t n, uint32_t c) {
    pixelsInternal_.setPixelColor(n, c);
    pixelsInternal_.show();
  }
  void setLightInternalAll(uint32_t c) {
    pixelsInternal_.fill(c);
    pixelsInternal_.show();
  }

  // Actuators (Servo)
  void setActuator(int angle) { servo_.write(angle); }

  // Analog Input (ADS1115) - Returns millivolts
  float readVoltage(uint8_t ch) {
    if (!hasAdc_) {
      Serial.printf("[ADS1115] Disconnected - Cannot readVoltage(ch=%d)\n", ch);
      return 0.0f;
    }
    setAdcChannel(ch);
    return ads_.getResult_mV();
  }
  uint16_t readAnalogRaw(uint8_t ch) {
    if (!hasAdc_) {
      Serial.printf("[ADS1115] Disconnected - Cannot readAnalogRaw(ch=%d)\n", ch);
      return 0;
    }
    setAdcChannel(ch);
    return (uint16_t)ads_.getRawResult();
  }
  void setAdcChannel(uint8_t ch) {
    if (!hasAdc_) {
      Serial.printf("[ADS1115] Disconnected - Cannot setAdcChannel(ch=%d)\n", ch);
      return;
    }
    ads_.setSingleChannel(ch);
    ads_.startSingleMeasurement();
    while (ads_.isBusy()) {}
  }

  // Analog Output (PCA9685) - Value as percentage 0.0 to 100.0
  void setAnalogOutput(uint8_t ch, float percent) {
    if (!hasPwmDriver_) {
      Serial.printf("[PCA9685] Disconnected - Cannot setAnalogOutput(ch=%d, %.1f%%)\n", ch, percent);
      return;
    }
    pca_.setChannelDutyCycle(ch, percent);
  }

  // Rotary Encoder Factory (quadrature encoder on two native GPIOs)
  EscapeRoomRotaryEncoder *createEncoder(uint8_t pinA, uint8_t pinB) {
    EscapeRoomRotaryEncoder *enc = new EscapeRoomRotaryEncoder(pinA, pinB);
    enc->begin();
    return enc;
  }

  // Rotary Dial Factory (telephone rotary dial)
  RotaryDialUnifiedReader *createRotaryDialUnified(uint8_t pin) {
    RotaryDialUnifiedReader *dial = new RotaryDialUnifiedReader();
    if (dial->begin(pin)) {
      return dial;
    }
    delete dial;
    return nullptr;
  }

  // Rotary Dial Factory — Optical Mode with calibrated timings
  RotaryDialUnifiedReader *createRotaryDialUnifiedOptical(uint8_t pin,
                                              unsigned long debounce = 50,
                                              unsigned long maxgap = 0,
                                              unsigned long digitend = 800) {
    RotaryDialUnifiedReader *dial = new RotaryDialUnifiedReader();
    if (dial->beginOptical(pin, debounce, maxgap, digitend)) {
      return dial;
    }
    delete dial;
    return nullptr;
  }

  // ============================================================================
  // FACTORIES FOR IOABSTRACTION DEVICES
  // ============================================================================

  // PN532 - NFC/RFID Reader via I2C
  NfcReader *createNfcReader(uint8_t irq = 255, uint8_t reset = 255) {
    NfcReader *nfc = new NfcReader(irq, reset, &Wire);
    if (nfc->begin()) {
      hasNfc_ = true;
      return nfc;
    }
    delete nfc;
    return nullptr;
  }

  // Standard Pin Management (Replicating Arduino API)
  void pinMode(uint8_t pin, uint8_t mode);
  int digitalRead(uint8_t pin);
  void digitalWrite(uint8_t pin, uint8_t val);

  // Hardware component access - for testing
  Adafruit_LIS3DH &accelerometer() { return lis_; }
  PCA9685 &pwmDriver() { return pca_; }
  ADS1115_WE &adc() { return ads_; }

  // Shake / Motion (LIS3DH)
  bool isShaking(float threshold_ms2 = 15.0f) {
    if (!hasAccelerometer_) {
      Serial.printf("[LIS3DH] Disconnected - Cannot isShaking(threshold=%.1f)\n", threshold_ms2);
      return false;
    }
    if (!lisBegun_) {
      Serial.println(F("[LIS3DH] Not initialized - Cannot isShaking()"));
      return false;
    }
    sensors_event_t event;
    lis_.getEvent(&event);
    float mag = sqrt(event.acceleration.x * event.acceleration.x +
                     event.acceleration.y * event.acceleration.y +
                     event.acceleration.z * event.acceleration.z);
    return mag > threshold_ms2;
  }

  uint8_t readTap() {
    if (!hasAccelerometer_) {
      Serial.println(F("[LIS3DH] Disconnected - Cannot readTap()"));
      return 0;
    }
    if (!lisBegun_) {
      Serial.println(F("[LIS3DH] Not initialized - Cannot readTap()"));
      return 0;
    }
    // getClick() returns 8-bit register.
    // bit 4 (0x10) = single click
    // bit 5 (0x20) = double click
    return lis_.getClick();
  }

  bool isSingleTapped() {
      return (readTap() & 0x10) != 0;
  }

  bool isDoubleTapped() {
      return (readTap() & 0x20) != 0;
  }

  // Utilities
  static uint8_t applyGamma(uint8_t val);
  static float sineWave(float periodSeconds, float minVal = 0, float maxVal = 1);

  // Memory Diagnostics
  void testMemory();

private:
  Hardware()
      : pixelsInternal_(1, PIN_NEOPIXEL_INTERNAL, NEO_GRB + NEO_KHZ800),
        pixelsExternal_(30, PIN_EXTERNAL_NEOPIXEL, NEO_GRB + NEO_KHZ800) {
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
    // Initialize expander arrays
    for (uint8_t i = 0; i < 8; i++) {
      expanders_[i] = nullptr;
      isPcf8575_[i] = false;
      expanderAddresses_[i] = 0;
    }
    expanderCount_ = 0;
#endif
  }

  AudioPlayer audio_;
#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
  PCF8574IoAbstraction* expanders_[8];   // Supports up to 8 expanders
  bool isPcf8575_[8];                     // Type of each expander
  uint8_t expanderAddresses_[8];          // I2C address of each expander
  uint8_t expanderCount_ = 0;             // Number of detected expanders
#endif
  Display14Seg display_;
  Adafruit_NeoPixel pixelsInternal_;
  Adafruit_NeoPixel pixelsExternal_;
  Servo servo_;
  ADS1115_WE ads_{0x48};
  PCA9685 pca_;
  Adafruit_LIS3DH lis_;
  bool littleFsMounted_ = false;
  bool sdMounted_ = false;

  bool initPortExpander(uint8_t address);
  void initAllExpanders();

  template <typename T>
  void listDirectory(const char* label, T &fs, const char* dirname) {
    Serial.printf("[HARDWARE] Scanning %s directory: %s\n", label, dirname);
    File root = fs.open(dirname, "r");
    if (!root) {
      Serial.println(F("  [!] FAIL: Unable to open directory track."));
      return;
    }
    if (!root.isDirectory()) {
      Serial.println(F("  [!] FAIL: Path is not a directory."));
      root.close();
      return;
    }

    int count = 0;
    File file = root.openNextFile();
    while (file) {
      if (file.isDirectory()) {
         Serial.printf("  [DIR]  /%-18s\n", file.name());
      } else {
         Serial.printf("  [FILE] %-20s  (%d bytes)\n", file.name(), (int)file.size());
      }
      count++;
      file = root.openNextFile();
    }
    if (count == 0) Serial.println(F("  (No files found)"));
    Serial.printf("[HARDWARE] Found %d entries in %s.\n", count, label);
    root.close();
  }

  void scanI2C();
  uint8_t detectPcfType(uint8_t address);

  bool begun_ = false;
  bool lisBegun_ = false;

  // Battery & Low Power
  uint8_t batterySensePin_ = 26;
  bool batteryMonitorBegun_ = false;
  float batteryVoltage_ = 0.0f;
  unsigned long lastBatteryRead_ = 0;
  unsigned long batteryReadInterval_ = 1000;

  bool lowPowerEnabled_ = false;
  bool powerWasCut_ = false;
  float warnVoltage_ = 3.8f;
  float criticalVoltage_ = 3.5f;

  uint8_t pixelBrightnessFull_ = 50;
  uint8_t displayBrightnessFull_ = 15;
  uint8_t volumeFull_ = 128;

  // Hardware detection flags
  bool hasPortExpander_ = false;
  bool hasAccelerometer_ = false;
  bool hasPwmDriver_ = false;
  bool hasDisplay_ = false;
  bool hasRtc_ = false;
  bool hasOled_ = false;
  bool hasAdc_ = false;
  bool hasNfc_ = false;
  bool hasCodec_ = false;
};

} // namespace esp32_escaperoom

// Global instance alias - must be outside namespace
#define hw esp32_escaperoom::Hardware::instance()
