#include "Hardware.h"

namespace esp32_escaperoom {

void Hardware::begin() {
  if (begun_) return;

  Serial.println(F("\n[HARDWARE] ========================================="));
  Serial.println(F("[HARDWARE]    By   Nicolau dos Brinquedos - 2026"));
  Serial.println(F("[HARDWARE] ESP32 Escape Room Engine Initializing"));
  Serial.println(F("[HARDWARE] ========================================="));

  if (Serial) { /* already begun */ } else { Serial.begin(115200); }

  Serial.println(F("\n[HARDWARE] ========================================="));
  Serial.println(F("[HARDWARE]        Starting Diagnostic Boot"));
  Serial.println(F("[HARDWARE] ========================================="));

  // 0. I2C Scan - apenas detecta dispositivos
  Serial.println(F("[HARDWARE] Init I2C Bus"));
  Wire.begin();
  scanI2C();

  // Nota: Expansores PCF857x devem ser configurados manualmente após o begin()
  // Use: hw.addPcf8574(0x20) ou hw.addPcf8575(0x21)
  // Ou use initExpanders() para inicializar todos os detectados (fallback)

  // SPI Start - SD Card Setup
  Serial.println(F("[HARDWARE] Init SPI Bus"));
  SPI.begin();

  // Power Rail - liga imediatamente para áudio/servo/NeoPixel
  pinMode(PIN_EXTERNAL_POWER, OUTPUT);
  setPower5V(true);
  Serial.println(F("[HARDWARE] External Power: ON"));

  // Storage
  Serial.println(F("[HARDWARE] CHECKING FILE SYSTEMS:"));
  if (LittleFS.begin()) {
    Serial.println(F("[HARDWARE] LittleFS: MOUNTED"));
    listDirectory("LittleFS", LittleFS, "/");
    littleFsMounted_ = true;
  } else {
    Serial.println(F("[HARDWARE] LittleFS: FAIL"));
    littleFsMounted_ = false;
  }

  if (SD.begin(PIN_SD_CS)) {
    Serial.println(F("[HARDWARE] SD Card: MOUNTED"));
    listDirectory("SD Card", SD, "/");
    sdMounted_ = true;
  } else {
    Serial.println(F("[HARDWARE] SD Card: FAIL (Check card insertion)"));
    sdMounted_ = false;
  }

  // Audio Subsystem - INICIALIZA PRIMEIRO (antes do NeoPixel)
  Serial.print(F("[HARDWARE] Audio Amplifier (I2S): "));
  audio_.begin(sdMounted_, littleFsMounted_);
  Serial.println(audio_.mounted() ? F("READY") : F("FAIL"));
  Serial.printf("[AUDIO] Storage: SD=%s, LittleFS=%s\n", sdMounted_ ? "YES" : "NO", littleFsMounted_ ? "YES" : "NO");
  volumeFull_ = audio_.getVolume();
  

  // NeoPixel Interno
  pixelsInternal_.begin();
  pixelsInternal_.setBrightness(50);
  pixelBrightnessFull_ = 50;
  pixelsInternal_.clear();
  pixelsInternal_.show();
  // delay(500);
  Serial.println(F("[HARDWARE] NeoPixel Interno: Initialized"));

  // NeoPixel Externo
  pixelsExternal_.begin();
  pixelsExternal_.setBrightness(50);
  pixelsExternal_.clear();
  pixelsExternal_.show();
  // delay(500);
  Serial.println(F("[HARDWARE] NeoPixel Externo: Initialized"));

  // Servo
  servo_.attach(PIN_SERVO_PWM);
  Serial.println(F("[HARDWARE] Servo: ATTACHED")); 
  // ── Inicialização de periféricos onboard (condicional por placa) ──
  boardInit();

  // ── Relatório de periféricos onboard ──
#if HAS_ONBOARD_TFT
#if HAS_M5UNIFIED
  Serial.printf("[HARDWARE] Onboard TFT: %dx%d (M5Unified)\n", M5.Display.width(), M5.Display.height());
#else
  Serial.printf("[HARDWARE] Onboard TFT: %dx%d (pins MOSI=%d SCK=%d CS=%d DC=%d RST=%d BL=%d)\n", TFT_WIDTH, TFT_HEIGHT, PIN_TFT_MOSI, PIN_TFT_SCK, PIN_TFT_CS, PIN_TFT_RS, PIN_TFT_RST, PIN_TFT_BL);
#endif
#endif
#if HAS_ONBOARD_LED_MATRIX
  Serial.printf("[HARDWARE] Onboard LED Matrix: %dx%d\n", MATRIX_WIDTH, MATRIX_HEIGHT);
#endif
#if HAS_ONBOARD_CODEC
  Serial.printf("[HARDWARE] Onboard Audio Codec: ES8388 @0x%02X (I2C)\n", ES8388_I2C_ADDRESS);
#endif
#if HAS_ONBOARD_KEYBOARD
  Serial.println(F("[HARDWARE] Onboard Keyboard: present"));
#endif
#if HAS_ONBOARD_IR
  Serial.printf("[HARDWARE] Onboard IR TX: GPIO%d\n", PIN_IR_TX);
#endif
#if HAS_ONBOARD_GROVE
  Serial.printf("[HARDWARE] Onboard Grove: HY2.0-4P (SDA=GPIO%d SCL=GPIO%d)\n", PIN_GROVE_YELLOW, PIN_GROVE_WHITE);
#endif
#if HAS_ONBOARD_ACCEL
  Serial.printf("[HARDWARE] Onboard Accelerometer: LIS3DH @0x%02X\n", LIS3DH_I2C_ADDRESS);
#endif
#if HAS_ONBOARD_OPTO_IN
  Serial.println(F("[HARDWARE] Onboard Opto-Isolated Inputs: available"));
#endif
#if HAS_ONBOARD_RELAY
  Serial.println(F("[HARDWARE] Onboard Relays: PCF8574 I2C"));
#endif

  begun_ = true;
  Serial.println(F("[HARDWARE] -----------------------------------------"));
  Serial.println(F("[HARDWARE]       ENGINE INITIALIZATION COMPLETE"));
  Serial.println(F("[HARDWARE] =========================================\n"));
}

#ifndef ESP32_ESCAPEROOM_NO_IOABSTRACTION
uint8_t Hardware::detectPcfType(uint8_t address) {
  // PCF8575 tem 16 bits de I/O, PCF8574 tem apenas 8 bits
  // Método: ler sem escrever nada - PCF8575 retorna 2 bytes, PCF8574 retorna 1

  // Tenta ler 2 bytes diretamente (sem escrita prévia)
  Wire.requestFrom(address, (uint8_t)2);
  uint32_t start = millis();
  // Aguarda pelo menos 1 byte (ambos respondem)
  while (Wire.available() == 0 && (millis() - start) < 100) {
    delay(1);
  }
  uint8_t available = Wire.available();

  // Se leu pelo menos 1 byte, dispositivo presente
  if (available >= 1) {
    // PCF8575 responde com 2 bytes, PCF8574 responde com 1 byte
    if (available >= 2) return 2;  // PCF8575 (16 bits)
    return 1;  // PCF8574 (8 bits)
  }
  return 0;  // Não respondeu
}

PCF8574IoAbstraction* Hardware::getExpanderByAddress(uint8_t address) {
  for (uint8_t i = 0; i < expanderCount_; i++) {
    if (expanderAddresses_[i] == address) {
      return expanders_[i];
    }
  }
  return nullptr;
}

int Hardware::getExpanderIndexByAddress(uint8_t address) {
  for (uint8_t i = 0; i < expanderCount_; i++) {
    if (expanderAddresses_[i] == address) {
      return (int)i;
    }
  }
  return -1;
}

void Hardware::initAllExpanders() {
  Serial.println(F("[HARDWARE] Initializing all Port Expanders:"));

  for (uint8_t i = 0; i < expanderCount_; i++) {
    uint8_t addr = expanderAddresses_[i];
    Serial.print(F("  - Address 0x"));
    if (addr < 16) Serial.print("0");
    Serial.print(addr, HEX);
    Serial.print(F(": "));

    if (expanders_[i] != nullptr) {
      expanders_[i]->sync();
      Serial.println(isPcf8575_[i] ? F("PCF8575 (16 channels) - OK") : F("PCF8574 (8 channels) - OK"));
    } else {
      Serial.println(F("NULL"));
    }
  }

  Serial.printf("[HARDWARE] Total expanders: %d\n", expanderCount_);
}

// Manual expander configuration - preferred over auto-detection
void Hardware::addExpander(uint8_t address, bool is16bit) {
  if (expanderCount_ >= 8) {
    Serial.println(F("[HARDWARE] Max expanders reached (8)"));
    return;
  }

  expanders_[expanderCount_] = new PCF8574IoAbstraction(address, IO_PIN_NOT_DEFINED, &Wire, is16bit);
  expanders_[expanderCount_]->runLoop();  // Inicializa estado I/O
  isPcf8575_[expanderCount_] = is16bit;
  expanderAddresses_[expanderCount_] = address;
  expanderCount_++;
  hasPortExpander_ = true;

  Serial.print(F("[HARDWARE] Expander added at 0x"));
  if (address < 16) Serial.print("0");
  Serial.print(address, HEX);
  Serial.println(is16bit ? F(" (PCF8575 - 16 channels) [MANUAL]") : F(" (PCF8574 - 8 channels) [MANUAL]"));
}

bool Hardware::initPortExpander(uint8_t address) {
  // Auto-detection fallback - only used if manual configuration not provided
  // Manual configuration via addExpander() or addPcf8574/addPcf8575 is preferred
  if (expanderCount_ >= 8) {
    Serial.println(F("[HARDWARE] Max expanders reached (8)"));
    return false;
  }

  uint8_t type = detectPcfType(address);

  if (type == 2) {
    // PCF8575 (16 bits)
    expanders_[expanderCount_] = new PCF8574IoAbstraction(address, IO_PIN_NOT_DEFINED, &Wire, true);
    expanders_[expanderCount_]->runLoop();  // Inicializa estado I/O
    isPcf8575_[expanderCount_] = true;
    expanderAddresses_[expanderCount_] = address;
    expanderCount_++;
    hasPortExpander_ = true;
    return true;
  }

  if (type == 1) {
    // PCF8574 (8 bits)
    expanders_[expanderCount_] = new PCF8574IoAbstraction(address, IO_PIN_NOT_DEFINED, &Wire, false);
    expanders_[expanderCount_]->runLoop();  // Inicializa estado I/O
    isPcf8575_[expanderCount_] = false;
    expanderAddresses_[expanderCount_] = address;
    expanderCount_++;
    hasPortExpander_ = true;
    return true;
  }

  return false;
}
#endif // ESP32_ESCAPEROOM_NO_IOABSTRACTION

void Hardware::scanI2C() {
  Serial.println(F("[HARDWARE] Scanning I2C Bus and initializing devices..."));
  byte count = 0;

  for (byte address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("  - Found device at 0x"));
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);

      // LIS3DH Accelerometer (0x18-0x19)
      if (address >= 0x18 && address <= 0x19) {
        Serial.print(F(" (LIS3DH Accelerometer)"));
        if (!hasAccelerometer_) {
          if (lis_.begin(address)) {
            lis_.setRange(LIS3DH_RANGE_4_G);
            lis_.setClick(2, 80);
            lisBegun_ = true;
            hasAccelerometer_ = true;
            Serial.print(F(" - INITIALIZED"));
          } else {
            Serial.print(F(" - INIT FAIL"));
          }
        }
      }

      // PCF8574/PCF8575 (0x20-0x27) - apenas detecta, tipo deve ser configurado manualmente
      if (address >= 0x20 && address <= 0x27) {
        Serial.print(F(" (PCF857x)"));
        hasPortExpander_ = true;
        Serial.print(F(" - DETECTED (use hw.addPcf8574(0x"));
        if (address < 16) Serial.print("0");
        Serial.print(address, HEX);
        Serial.print(F(") ou hw.addPcf8575)"));
      }

      // PCF8574A/PCF8575A (0x38-0x3F) - apenas detecta, tipo deve ser configurado manualmente
      if (address >= 0x38 && address <= 0x3F) {
        Serial.print(F(" (PCF857xA)"));
        hasPortExpander_ = true;
        Serial.print(F(" - DETECTED (use hw.addPcf8574(0x"));
        if (address < 16) Serial.print("0");
        Serial.print(address, HEX);
        Serial.print(F(") ou hw.addPcf8575)"));
      }

      // RTC PCF8563 (0x51)
      if (address == 0x51) {
        Serial.print(F(" (RTC PCF8563)"));
        hasRtc_ = true;
        Serial.print(F(" - DETECTED"));
      }

      // OLED SSD1306 (0x3C-0x3D)
      if (address >= 0x3C && address <= 0x3D) {
        Serial.print(F(" (OLED SSD1306)"));
        hasOled_ = true;
        Serial.print(F(" - DETECTED"));
      }

      // PCA9685 PWM Driver (0x40-0x47, 0x4C-0x4F) - exclui 0x48-0x4B (ADS1115)
      if ((address >= 0x40 && address <= 0x47) || (address >= 0x4C && address <= 0x4F)) {
        Serial.print(F(" (PCA9685 PWM)"));
        if (!hasPwmDriver_) {
          pca_.setupSingleDevice(Wire, address);
          pca_.setToFrequency(60);
          hasPwmDriver_ = true;
          Serial.print(F(" - INITIALIZED"));
        }
      }

      // HT16K33 Alphanumeric Display (0x70-0x77)
      if (address >= 0x70 && address <= 0x77) {
        Serial.print(F(" (HT16K33 Display @0x"));
        Serial.print(address, HEX);
        Serial.print(F(")"));
        if (!hasDisplay_) {
          display_.begin(address);
          display_.setBrightness(15);
          displayBrightnessFull_ = 15;
          hasDisplay_ = true;
          Serial.print(F(" - INITIALIZED"));
        }
      }

      // PN532 NFC/RFID (0x24)
      if (address == 0x24) {
        Serial.print(F(" (PN532 NFC/RFID)"));
        hasNfc_ = true;
        Serial.print(F(" - DETECTED (use hw.createNfcReader())"));
      }

      // ADS1115 ADC (0x48-0x4B)
      if (address >= 0x48 && address <= 0x4B) {
        Serial.print(F(" (ADS1115 I2C)"));
        if (!hasAdc_) {
          hasAdc_ = true;
          Serial.print(F(" - DETECTED"));
        }
      }

      Serial.println();
      count++;
    }
  }

  // ADS1115 via I2C
  if (hasAdc_) {
    Serial.print(F("  - ADS1115 (I2C): "));
    if (ads_.init()) {
      ads_.setVoltageRange_mV(ADS1115_RANGE_6144);
      ads_.setConvRate(ADS1115_128_SPS);
      Serial.println(F("INITIALIZED"));
    } else {
      hasAdc_ = false;
      Serial.println(F("INIT FAIL"));
    }
  }

  // Relatório final
  Serial.println(F("[HARDWARE] ========================================="));
  Serial.println(F("[HARDWARE] Hardware Detection Summary:"));
  Serial.printf("  - Port Expander (PCF857x): %s\n", hasPortExpander_ ? "FOUND" : "NOT FOUND");
  Serial.printf("  - LIS3DH Accelerometer: %s\n", hasAccelerometer_ ? "FOUND" : "NOT FOUND");
  Serial.printf("  - PCA9685 PWM Driver: %s\n", hasPwmDriver_ ? "FOUND" : "NOT FOUND");
  Serial.printf("  - HT16K33 Display: %s\n", hasDisplay_ ? "FOUND" : "NOT FOUND");
  Serial.printf("  - RTC PCF8563: %s\n", hasRtc_ ? "FOUND" : "NOT FOUND");
  Serial.printf("  - OLED SSD1306: %s\n", hasOled_ ? "FOUND" : "NOT FOUND");
  Serial.printf("  - ADC (ADS1115): %s\n", hasAdc_ ? "FOUND" : "NOT FOUND");
  Serial.printf("  - PN532 (NFC/RFID): %s\n", hasNfc_ ? "FOUND" : "NOT FOUND");
  Serial.printf("  - System Healthy: %s\n", isSystemHealthy() ? "YES" : "NO");
  Serial.println(F("[HARDWARE] ========================================="));
}


void Hardware::setPower5V(bool on) {
  digitalWrite(PIN_EXTERNAL_POWER, on ? HIGH : LOW);
}

void Hardware::pinMode(uint8_t pin, uint8_t mode) {
  ::pinMode(pin, (esp32_escaperoom::PinModeType)mode);
}

int Hardware::digitalRead(uint8_t pin) {
  return ::digitalRead(pin);
}

void Hardware::digitalWrite(uint8_t pin, uint8_t val) {
  ::digitalWrite(pin, (esp32_escaperoom::PinStatusType)val);
}

uint8_t Hardware::applyGamma(uint8_t val) {
  static const uint8_t gammaLUT[32] = {
    0, 1, 2, 4, 7, 10, 15, 20, 26, 34, 43, 53, 64, 76, 90, 105,
    121, 138, 157, 177, 198, 221, 245, 255, 255, 255, 255, 255, 255, 255, 255, 255
  };
  return gammaLUT[map(val, 0, 255, 0, 31)];
}

float Hardware::sineWave(float periodSeconds, float minVal, float maxVal) {
  float phase = (millis() % (uint32_t)(periodSeconds * 1000)) / (periodSeconds * 1000.0f);
  float s = sin(phase * 2.0f * PI);
  return minVal + (maxVal - minVal) * (s + 1.0f) / 2.0f;
}

void Hardware::testMemory() {
  Serial.println(F("[MEMORY] ========================================"));
  Serial.println(F("[MEMORY] System Memory Diagnostic"));
  Serial.println(F("[MEMORY] ========================================"));

  #ifdef ARDUINO_ARCH_RP2040
    Serial.println(F("[MEMORY] Platform: RP2040"));
    Serial.println(F("[MEMORY] RP2040 SRAM: 264 KB total"));

    int freeHeap = rp2040.getFreeHeap();
    Serial.print(F("[MEMORY] Free heap: "));
    Serial.print(freeHeap);
    Serial.println(F(" bytes"));

  #elif defined(ARDUINO_ARCH_ESP32)
    Serial.println(F("[MEMORY] Platform: ESP32"));
    Serial.print(F("[MEMORY] Free heap: "));
    Serial.print(ESP.getFreeHeap());
    Serial.println(F(" bytes"));
    Serial.print(F("[MEMORY] PSRAM: "));
    Serial.print(ESP.getPsramSize());
    Serial.println(F(" bytes"));
    Serial.print(F("[MEMORY] Free PSRAM: "));
    Serial.print(ESP.getFreePsram());
    Serial.println(F(" bytes"));
  #else
    Serial.println(F("[MEMORY] Platform: Unknown"));
  #endif

  #ifdef __FREERTOS
    Serial.println(F("[MEMORY] FreeRTOS: ENABLED"));
  #else
    Serial.println(F("[MEMORY] FreeRTOS: DISABLED"));
  #endif

  Serial.print(F("[MEMORY] LittleFS: "));
  Serial.println(littleFsMounted_ ? F("MOUNTED") : F("NOT MOUNTED"));
  Serial.print(F("[MEMORY] SD Card: "));
  Serial.println(sdMounted_ ? F("MOUNTED") : F("NOT MOUNTED"));

  Serial.println(F("[MEMORY] ========================================"));
}

// ============================================================================
// BATTERY MONITORING
// ============================================================================
void Hardware::beginBatteryMonitor(uint8_t sensePin) {
  batterySensePin_ = sensePin;
  pinMode(batterySensePin_, INPUT);
  batteryMonitorBegun_ = true;
  lastBatteryRead_ = 0;

  // Leitura inicial
  int raw = analogRead(batterySensePin_);
  batteryVoltage_ = (raw / 1023.0f) * 3.3f * 2.0f;

  Serial.printf("[BATTERY] Monitor started on pin %d (GPIO%d)\n",
    sensePin == 26 ? 0 : sensePin == 27 ? 1 : sensePin == 28 ? 2 : 3, sensePin);
  Serial.printf("[BATTERY] Initial reading: %.2fV (raw=%d)\n", batteryVoltage_, raw);
  if (!isBatteryConnected()) {
    Serial.println(F("[BATTERY] No battery detected (USB power assumed)"));
  }
}

float Hardware::readBatteryVoltage() {
  if (!batteryMonitorBegun_) {
    Serial.println(F("[BATTERY] Not initialized - call hw.beginBatteryMonitor()"));
    return 0.0f;
  }

  unsigned long now = millis();
  if (now - lastBatteryRead_ >= batteryReadInterval_) {
    int raw = analogRead(batterySensePin_);
    batteryVoltage_ = (raw / 1023.0f) * 3.3f * 2.0f;
    lastBatteryRead_ = now;
  }
  return batteryVoltage_;
}

float Hardware::readBatteryPercent() {
  float v = readBatteryVoltage();
  if (v <= 3.3f) return 0.0f;
  if (v >= 4.2f) return 100.0f;
  return (v - 3.3f) / (4.2f - 3.3f) * 100.0f;
}

bool Hardware::isBatteryConnected() {
  return readBatteryVoltage() > 3.0f;
}

// ============================================================================
// LOW POWER MODE
// ============================================================================
void Hardware::setLowPowerMode(bool enabled) {
  lowPowerEnabled_ = enabled;
  Serial.printf("[POWER] LowPower mode: %s\n", enabled ? "ENABLED" : "DISABLED");
}

void Hardware::setPowerSaveThresholds(float warnV, float criticalV) {
  warnVoltage_ = warnV;
  criticalVoltage_ = criticalV;
  if (criticalVoltage_ >= warnVoltage_) {
    criticalVoltage_ = warnVoltage_ - 0.2f;
  }
  Serial.printf("[POWER] Thresholds: warn=%.2fV, critical=%.2fV\n",
    warnVoltage_, criticalVoltage_);
}

void Hardware::updatePowerSave() {
  if (!lowPowerEnabled_ || !batteryMonitorBegun_) return;

  float v = readBatteryVoltage();

  // Só atua se bateria estiver conectada
  if (v <= 3.0f) return;

  // Ratio linear entre warn e critical
  float ratio = 1.0f;
  if (v <= criticalVoltage_) {
    ratio = 0.0f;
  } else if (v < warnVoltage_) {
    ratio = (v - criticalVoltage_) / (warnVoltage_ - criticalVoltage_);
  }

  // Aplica dimming proporcional
  uint8_t neoB = (uint8_t)(pixelBrightnessFull_ * ratio);
  uint8_t dispB = hasDisplay_ ? (uint8_t)(displayBrightnessFull_ * ratio) : 0;
  uint8_t vol = (uint8_t)(volumeFull_ * ratio);

  pixelsExternal_.setBrightness(neoB);
  pixelsInternal_.setBrightness(neoB);
  if (hasDisplay_) display_.setBrightness(dispB);
  audio_.setVolume(vol);

  // Corte critico - desliga EXTERNAL_POWER
  if (ratio == 0.0f && !powerWasCut_) {
    setPower5V(false);
    powerWasCut_ = true;
    Serial.println(F("[POWER] CRITICAL - External power cut"));
  } else if (ratio > 0.0f && powerWasCut_) {
    setPower5V(true);
    powerWasCut_ = false;
    Serial.println(F("[POWER] Restored external power"));
  }

  // Debug a cada 5 segundos
  static unsigned long lastLog = 0;
  if (millis() - lastLog > 5000) {
    Serial.printf("[POWER] Battery=%.2fV (%d%%) ratio=%.2f neo=%d disp=%d vol=%d\n",
      v, (int)readBatteryPercent(), ratio, neoB, dispB, vol);
    lastLog = millis();
  }
}

}  // namespace esp32_escaperoom
