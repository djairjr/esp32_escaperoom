#pragma once

#include <Arduino.h>
#include <Adafruit_PN532.h>
#include <Wire.h>

namespace esp32_escaperoom {

class NfcReader {
 public:
  NfcReader(uint8_t irq = 255, uint8_t reset = 255, TwoWire *theWire = &Wire)
      : pn532_(irq, reset, theWire), initialized_(false) {}

  bool begin() {
    if (!pn532_.begin()) {
      Serial.println(F("[NFC] PN532 init FAIL"));
      return false;
    }

    uint32_t version = pn532_.getFirmwareVersion();
    if (version == 0) {
      Serial.println(F("[NFC] PN532 firmware version invalid"));
      return false;
    }

    Serial.printf("[NFC] PN532 found, firmware v%d.%d\n",
                  (version >> 16) & 0xFF, (version >> 8) & 0xFF);

    pn532_.SAMConfig();
    initialized_ = true;
    return true;
  }

  bool isInitialized() const { return initialized_; }

  struct CardData {
    bool present = false;
    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;
  };

  CardData readCard(uint16_t timeoutMs = 1000) {
    CardData data;
    if (!initialized_) return data;

    uint8_t uid[7];
    uint8_t uidLen;
    bool found = pn532_.readPassiveTargetID(
        PN532_MIFARE_ISO14443A, uid, &uidLen, timeoutMs);

    if (found && uidLen > 0) {
      data.present = true;
      data.uidLength = uidLen;
      memcpy(data.uid, uid, min(uidLen, (uint8_t)7));
    }

    return data;
  }

  uint32_t getFirmwareVersion() {
    if (!initialized_) return 0;
    return pn532_.getFirmwareVersion();
  }

 private:
  Adafruit_PN532 pn532_;
  bool initialized_;
};

}  // namespace esp32_escaperoom
