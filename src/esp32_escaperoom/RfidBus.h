#pragma once

#include <Arduino.h>
#include <MFRC522.h>
#include <SPI.h>

#include "Pcf857x.h"

namespace esp32_escaperoom {

class RfidBus {
 public:
  RfidBus(const uint8_t *csPins, uint8_t readerCount, uint8_t rstPin, uint8_t ssDummyPin, SPIClass &spi = SPI)
      : csPins_(csPins), readerCount_(readerCount), rstPin_(rstPin), ssDummyPin_(ssDummyPin), spi_(spi) {}

  RfidBus(Pcf857x &expander, const uint8_t *csPins, uint8_t readerCount, uint8_t rstPin, uint8_t ssDummyPin,
          SPIClass &spi = SPI)
      : expander_(&expander),
        csPins_(csPins),
        readerCount_(readerCount),
        rstPin_(rstPin),
        ssDummyPin_(ssDummyPin),
        spi_(spi) {}

  bool begin(uint16_t expanderIdleMask = 0xFFFF) {
    pinMode(ssDummyPin_, OUTPUT);
    digitalWrite(ssDummyPin_, HIGH);
    pinMode(rstPin_, OUTPUT);
    digitalWrite(rstPin_, HIGH);
    spi_.begin();

    if (usesExpander()) {
      activeMask_ = expanderIdleMask;
      for (uint8_t i = 0; i < readerCount_; i++) {
        activeMask_ |= (1u << csPins_[i]);
      }
      if (!expander_->begin(activeMask_)) {
        return false;
      }
      return expander_->write(activeMask_);
    }

    for (uint8_t i = 0; i < readerCount_; i++) {
      pinMode(csPins_[i], OUTPUT);
      digitalWrite(csPins_[i], HIGH);
    }
    return true;
  }

  bool beginReader(MFRC522 &reader, uint8_t readerIndex, bool dumpVersion = true) {
    if (!select(readerIndex)) {
      return false;
    }
    delay(4);
    reader.PCD_Init();
    reader.PCD_SetAntennaGain(reader.RxGain_max);
    if (dumpVersion) {
      Serial.print(F("Reader "));
      Serial.print(readerIndex);
      Serial.print(F(": "));
      reader.PCD_DumpVersionToSerial();
    }
    release(readerIndex);
    return true;
  }

  bool readCard(MFRC522 &reader, uint8_t readerIndex) {
    if (!select(readerIndex)) {
      return false;
    }
    delayMicroseconds(selectDelayUs_);
    if (!reader.PICC_IsNewCardPresent()) {
      release(readerIndex);
      return false;
    }
    if (!reader.PICC_ReadCardSerial()) {
      release(readerIndex);
      return false;
    }
    return true;
  }

  void finishCard(MFRC522 &reader, uint8_t readerIndex) {
    reader.PICC_HaltA();
    reader.PCD_StopCrypto1();
    release(readerIndex);
  }

 private:
  bool usesExpander() const {
    return expander_ != nullptr;
  }

  void deselectAll() {
    if (usesExpander()) {
      expander_->write(activeMask_);
    } else {
      for (uint8_t i = 0; i < readerCount_; i++) {
        digitalWrite(csPins_[i], HIGH);
      }
    }
  }

  bool select(uint8_t readerIndex) {
    if (readerIndex >= readerCount_) {
      return false;
    }
    deselectAll();
    delayMicroseconds(5);
    if (usesExpander()) {
      uint16_t nextState = activeMask_ & ~(1u << csPins_[readerIndex]);
      expander_->write(nextState);
    } else {
      digitalWrite(csPins_[readerIndex], LOW);
    }
    delayMicroseconds(5);
    return true;
  }

  void release(uint8_t readerIndex) {
    if (readerIndex >= readerCount_) {
      return;
    }
    if (usesExpander()) {
      expander_->write(activeMask_);
    } else {
      digitalWrite(csPins_[readerIndex], HIGH);
    }
  }

  Pcf857x *expander_ = nullptr;
  const uint8_t *csPins_;
  uint8_t readerCount_;
  uint8_t rstPin_;
  uint8_t ssDummyPin_;
  SPIClass &spi_;
  uint16_t activeMask_ = 0xFFFF;
  uint16_t selectDelayUs_ = 20;
};

}  // namespace esp32_escaperoom
