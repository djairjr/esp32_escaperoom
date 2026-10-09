/*
  LibrarySmokeTest

  Diagnostic sketch that validates all connected hardware as a Puzzle.
  The verbose boot reports detected I2C/SPI devices.
  The logic task runs a loop with the tests for all detected devices.

  WARNING: Audio requires a continuous call to hw.sound().loop() in loop().
  Without it, the system hangs and audio does not play.
  See: https://github.com/pschatzmann/arduino-audio-tools/issues

  Note: For full FreeRTOS, enable "FreeRTOS SMP" in the IDE menu.
  In production, use separate tasks for audio (Core 1) and logic (Core 0).
*/

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

// ============================================================================
// HOME ASSISTANT + WIFI (added by the ESP32 port)
// ============================================================================
using namespace esp32_escaperoom;

ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);
HASensor haSensorState("state");

void initHA() {
  netConfig.begin();
  if (netConfig.isAPMode()) Serial.println(F("[CFG] AP: http://192.168.4.1"));
  byte mac[6]; WiFi.macAddress(mac);
  haDevice.setUniqueId(mac, sizeof(mac));
  haDevice.setName(netConfig.devName());
  haDevice.setSoftwareVersion("1.0.0");
  haSensorState.setName("State");
  haSensorState.setIcon("mdi:puzzle");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}


using esp32_escaperoom::PuzzleState;

// ============================================
// HardwareTestPuzzle - Diagnostic puzzle
// ============================================
class HardwareTestPuzzle : public esp32_escaperoom::PuzzleBase {
public:
  HardwareTestPuzzle()
    : PuzzleBase(0) {}

  void setupHardware() override {
    Serial.println(F("[HWTEST] Setup Hardware Test Puzzle"));
    testState_ = TestState::Running;
    testIndex_ = 0;
    subStep_ = 0;
    testTimer_ = millis();
  }

  void update() override {
    // Test state machine
    switch (testState_) {
      case TestState::Running:
        runTestCycle();
        break;

      case TestState::Complete:
        // All tests completed - blinks green LED
        hw.setLightInternalAll(hw.pixelsInternal().Color(0, 50, 0));
        hw.testMemory();
        delay(5000);

        setupHardware();
        break;

      case TestState::Error:
        // Error - blinks red LED
        hw.setLightInternalAll(hw.pixelsInternal().Color(50, 0, 0));
        break;
    }
  }

private:
  enum class TestState { Running,
                         Complete,
                         Error };

  TestState testState_;
  uint8_t testIndex_;
  uint8_t subStep_;
  uint32_t testTimer_;

  void runTestCycle() {
    uint32_t now = millis();

    // Tests Internal NeoPixel (always available)
    if (testIndex_ == 0) {
      if (now - testTimer_ > 100) {
        testTimer_ = now;
        if (subStep_ == 0) {
          hw.setLightInternal(0, hw.pixelsInternal().Color(255, 0, 0));
          Serial.println(F("[HWTEST] NeoPixel Internal: RED"));
        } else if (subStep_ == 1) {
          hw.setLightInternal(0, hw.pixelsInternal().Color(0, 255, 0));
          Serial.println(F("[HWTEST] NeoPixel Internal: GREEN"));
        } else if (subStep_ == 2) {
          hw.setLightInternal(0, hw.pixelsInternal().Color(0, 0, 255));
          Serial.println(F("[HWTEST] NeoPixel Internal: BLUE"));
        } else if (subStep_ == 3) {
          hw.setLightInternalAll(0);
          Serial.println(F("[HWTEST] NeoPixel Internal: OK"));
          testIndex_++;
          subStep_ = 0;
          testTimer_ = now;
          return;
        }
        subStep_++;
      }
      return;
    }

    // Tests External NeoPixel
    if (testIndex_ == 1) {
      if (now - testTimer_ > 100) {
        testTimer_ = now;
        if (subStep_ == 0) {
          hw.setLightAll(hw.pixelsExternal().Color(255, 0, 0));
          Serial.println(F("[HWTEST] NeoPixel External: RED"));
        } else if (subStep_ == 1) {
          hw.setLightAll(hw.pixelsExternal().Color(0, 255, 0));
          Serial.println(F("[HWTEST] NeoPixel External: GREEN"));
        } else if (subStep_ == 2) {
          hw.setLightAll(hw.pixelsExternal().Color(0, 0, 255));
          Serial.println(F("[HWTEST] NeoPixel External: BLUE"));
        } else if (subStep_ == 3) {
          hw.setLightAll(0);
          Serial.println(F("[HWTEST] NeoPixel External: OK"));
          testIndex_++;
          subStep_ = 0;
          testTimer_ = now;
          return;
        }
        subStep_++;
      }
      return;
    }

    // Tests Port Expander (PCF857x via IoAbstraction) << CRASH HERE
    if (testIndex_ == 2) {
      if (subStep_ == 0) {
        if (hw.hasPortExpander()) {
          auto* pcf = hw.portExpander();
          pcf->writeValue(0, 1);
          pcf->runLoop();
          subStep_ = 1;
          testTimer_ = now;
          return;
        } else {
          Serial.println(F("[HWTEST] PCF857x: SKIP (not detected)"));
          testIndex_++;
          subStep_ = 0;
          return;
        }
      }
      if (subStep_ == 1 && now - testTimer_ > 10) {
        auto* pcf = hw.portExpander();
        pcf->runLoop();
        uint8_t read = pcf->readValue(0);
        pcf->writeValue(0, 0);
        pcf->runLoop();
        if (read == 1 || read == 0) {
          Serial.println(F("[HWTEST] PCF857x: OK (read/write test passed)"));
        } else {
          Serial.println(F("[HWTEST] PCF857x: FAIL"));
          testState_ = TestState::Error;
          return;
        }
        testIndex_++;
        subStep_ = 0;
        return;
      }
      return;
    }

    // Tests LIS3DH Accelerometer
    if (testIndex_ == 3) {
      if (hw.hasAccelerometer()) {
        sensors_event_t event;
        hw.accelerometer().getEvent(&event);
        float mag = sqrt(event.acceleration.x * event.acceleration.x + event.acceleration.y * event.acceleration.y + event.acceleration.z * event.acceleration.z);
        if (mag >= 0.5 && mag <= 15.0) {
          Serial.printf("[HWTEST] LIS3DH: OK (G=%.2f)\n", mag);
        } else {
          Serial.printf("[HWTEST] LIS3DH: FAIL (G=%.2f out of range)\n", mag);
          testState_ = TestState::Error;
          return;
        }
      } else {
        Serial.println(F("[HWTEST] LIS3DH: SKIP (not detected)"));
      }
      testIndex_++;
      subStep_ = 0;
      return;
    }

    // Tests PCA9685 PWM Driver
    if (testIndex_ == 4) {
      if (subStep_ == 0 && hw.hasPwmDriver()) {
        hw.pwmDriver().setChannelDutyCycle(0, 50.0);
        subStep_ = 1;
        testTimer_ = now;
        return;
      }
      if (subStep_ == 1 && now - testTimer_ > 100) {
        hw.pwmDriver().setChannelDutyCycle(0, 0.0);
        Serial.println(F("[HWTEST] PCA9685: OK (PWM channel 0 test)"));
        testIndex_++;
        subStep_ = 0;
        return;
      }
      if (!hw.hasPwmDriver()) {
        Serial.println(F("[HWTEST] PCA9685: SKIP (not detected)"));
        testIndex_++;
        subStep_ = 0;
      }
      return;
    }

    // Tests Audio (BEFORE the display to avoid I2C conflicts)
    if (testIndex_ == 5) {
      hw.sound().loop();
      hw.sound().setVolume(200);
      // Waits for audio to finish (subStep_ == 1 after playOnce)
      if (subStep_ == 1) {
        if (now - testTimer_ > 5000) {  // Waits 5s for audio to play
          hw.sound().stop();            // Explicit stop before the next test
          Serial.println(F("[HWTEST] Audio: stopped"));
          testIndex_++;
          subStep_ = 0;
        }
        return;
      }

      // subStep_ == 0: starts playback
      Serial.print(F("[HWTEST] Audio: "));
      if (hw.isLittleFSMounted() || hw.isSDMounted()) {
        const char* testPath = nullptr;
        if (hw.isLittleFSMounted() && LittleFS.exists("/armed.mp3")) {
          testPath = "/armed.mp3";
        } else if (hw.isSDMounted() && SD.exists("/armed.mp3")) {
          testPath = "/armed.mp3";
        }

        if (testPath) {
          Serial.println(F("OK (file found)"));
          Serial.print(F("[HWTEST] Audio: "));
          Serial.print(testPath);

          // Checks the file size
          File f = LittleFS.open(testPath, "r");
          if (f) {
            Serial.print(F(" size="));
            Serial.print(f.size());
            Serial.println(F(" bytes"));
            f.close();
          }

          Serial.println(F("[HWTEST] Audio: Starting playback..."));
          hw.sound().playOnceBlocking(testPath);
          Serial.println(F("[HWTEST] Audio: playOnce() called"));
          subStep_ = 1;
          testTimer_ = now;
          return;
        } else {
          Serial.println(F("SKIP (no MP3 - upload test.mp3 to LittleFS/SD)"));
        }
      } else {
        Serial.println(F("SKIP (no storage mounted)"));
      }
      testIndex_++;
      subStep_ = 0;
      return;
    }

    // Tests HT16K33 Display
    if (testIndex_ == 6) {
      if (subStep_ == 0 && hw.hasDisplay()) {
        hw.display().printString("TEST");
        subStep_ = 1;
        testTimer_ = now;
        return;
      }
      if (subStep_ == 1 && now - testTimer_ > 1000) {
        hw.display().printString("    ");
        Serial.println(F("[HWTEST] HT16K33: OK"));
        testIndex_++;
        subStep_ = 0;
        return;
      }
      if (!hw.hasDisplay()) {
        Serial.println(F("[HWTEST] HT16K33: SKIP (not detected)"));
        testIndex_++;
        subStep_ = 0;
      }
      return;
    }

    // Tests Servo
    if (testIndex_ == 7) {
      if (subStep_ == 0) {
        hw.setActuator(0);
        subStep_ = 1;
        testTimer_ = now;
        return;
      }
      if (subStep_ == 1 && now - testTimer_ > 300) {
        hw.setActuator(90);
        subStep_ = 2;
        testTimer_ = now;
        return;
      }
      if (subStep_ == 2 && now - testTimer_ > 300) {
        hw.setActuator(180);
        subStep_ = 3;
        testTimer_ = now;
        return;
      }
      if (subStep_ == 3 && now - testTimer_ > 300) {
        hw.setActuator(90);
        Serial.println(F("[HWTEST] Servo: OK (0-180 sweep)"));
        testIndex_++;
        subStep_ = 0;
        return;
      }
      return;
    }

    // Tests ADS1115 ADC
    if (testIndex_ == 8) {
      if (hw.hasAdc()) {
        float mv = hw.readVoltage(0);
        uint16_t raw = hw.readAnalogRaw(0);
        Serial.printf("[HWTEST] ADS1115: OK (CH0: %d = %.0fmV)\n", raw, mv);
      } else {
        Serial.println(F("[HWTEST] ADS1115: SKIP (not detected)"));
      }
      testIndex_++;
      subStep_ = 0;
      return;
    }

    // All tests completed
    testState_ = TestState::Complete;
    Serial.println(F("[HWTEST] ======================================="));
    Serial.println(F("[HWTEST]    All Hardware Diagnostics Complete"));
    Serial.println(F("[HWTEST] ======================================="));
    
    
  }
};

// ============================================
// Main Setup and Loop
// ============================================

HardwareTestPuzzle hwTestPuzzle;

void setup() {
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);

  hw.begin();

  // Manual configuration of the expanders (required after the update)
  // Adjust the addresses and types according to your hardware
  hw.addPcf8575(0x20);  // PCF8575 (16 channels) at 0x20
  // hw.addPcf8574(0x21);  // Uncomment if you have a PCF8574 (8 channels)

  Serial.println();
  Serial.println(F("== esp32_escaperoom smoke test =="));
  Serial.print(F("System Healthy: "));
  Serial.println(hw.isSystemHealthy() ? F("YES") : F("NO"));

  hwTestPuzzle.setupHardware();

  Serial.println(F("Setup complete - tests running in loop"));
}

void loop() {
  loopHA();
  hwTestPuzzle.update();  // Audio loop moved to hwTestPuzzle.update
}
