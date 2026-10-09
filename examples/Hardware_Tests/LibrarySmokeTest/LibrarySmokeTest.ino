/*
  LibrarySmokeTest

  Sketch de diagnóstico que valida todo hardware conectado como um Puzzle.
  O boot verbose reporta dispositivos I2C/SPI detectados.
  A logic task roda um loop com os testes de todos os dispositivos encontrados.

  ATENÇÃO: O áudio requer chamada contínua ao hw.sound().loop() no loop().
  Sem isso, o sistema trava e o áudio não toca.
  Veja: https://github.com/pschatzmann/arduino-audio-tools/issues

  Nota: Para FreeRTOS completo, habilite "FreeRTOS SMP" no menu da IDE.
  Em produção, use tasks separadas para áudio (Core 1) e lógica (Core 0).
*/

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

// ============================================================================
// HOME ASSISTANT + WIFI (adicionado pelo port ESP32)
// ============================================================================
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
  haSensorState.setName("Estado");
  haSensorState.setIcon("mdi:puzzle");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}


using propmaker_escaperoom::PuzzleState;

// ============================================
// HardwareTestPuzzle - Puzzle de diagnóstico
// ============================================
class HardwareTestPuzzle : public propmaker_escaperoom::PuzzleBase {
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
    // Máquina de estados de teste
    switch (testState_) {
      case TestState::Running:
        runTestCycle();
        break;

      case TestState::Complete:
        // Todos testes completados - pisca LED verde
        hw.setLightInternalAll(hw.pixelsInternal().Color(0, 50, 0));
        hw.testMemory();
        delay(5000);

        setupHardware();
        break;

      case TestState::Error:
        // Erro - pisca LED vermelho
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

    // Testa NeoPixel Interno (sempre disponível)
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

    // Testa NeoPixel Externo
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

    // Testa Port Expander (PCF857x via IoAbstraction) << CRASH AQUI
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

    // Testa LIS3DH Accelerometer
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

    // Testa PCA9685 PWM Driver
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

    // Testa Audio (ANTES do display para evitar conflitos I2C)
    if (testIndex_ == 5) {
      hw.sound().loop();
      hw.sound().setVolume(200);
      // Aguarda áudio terminar (subStep_ == 1 após playOnce)
      if (subStep_ == 1) {
        if (now - testTimer_ > 5000) {  // Aguarda 5s para áudio tocar
          hw.sound().stop();            // Stop explícito antes de próximo teste
          Serial.println(F("[HWTEST] Audio: stopped"));
          testIndex_++;
          subStep_ = 0;
        }
        return;
      }

      // subStep_ == 0: inicia playback
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

          // Verifica tamanho do arquivo
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

    // Testa HT16K33 Display
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

    // Testa Servo
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

    // Testa ADS1115 ADC
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

    // Todos testes completados
    testState_ = TestState::Complete;
    Serial.println(F("[HWTEST] ======================================="));
    Serial.println(F("[HWTEST]    All Hardware Diagnostics Complete"));
    Serial.println(F("[HWTEST] ======================================="));
    
    
  }
};

// ============================================
// Setup e Loop principais
// ============================================

HardwareTestPuzzle hwTestPuzzle;

void setup() {
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);

  hw.begin();

  // Configuração manual dos expansores (obrigatório após atualização)
  // Ajuste os endereços e tipos conforme seu hardware
  hw.addPcf8575(0x20);  // PCF8575 (16 canais) em 0x20
  // hw.addPcf8574(0x21);  // Descomente se tiver PCF8574 (8 canais)

  Serial.println();
  Serial.println(F("== propmaker_escaperoom smoke test =="));
  Serial.print(F("System Healthy: "));
  Serial.println(hw.isSystemHealthy() ? F("YES") : F("NO"));

  hwTestPuzzle.setupHardware();

  Serial.println(F("Setup complete - tests running in loop"));
}

void loop() {
  loopHA();
  hwTestPuzzle.update();  // Audio loop movido para hwTestPuzzle.update
}
