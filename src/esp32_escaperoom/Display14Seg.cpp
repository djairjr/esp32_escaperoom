#include "Display14Seg.h"

namespace esp32_escaperoom {

void Display14Seg::begin(uint8_t address) {
  // Testa se dispositivo responde no I2C
  Wire.beginTransmission(address);
  uint8_t error = Wire.endTransmission();
  if (error != 0) {
    Serial.printf("[HT16K33] Desconectado - Nao responde em 0x%02X (erro=%d)\n", address, error);
    initialized_ = false;
    return;
  }

  display14x4_.begin(address);
  display14x4_.setBrightness(15);
  display14x4_.blinkRate(0);
  display14x4_.clear();
  display14x4_.writeDisplay();
  initialized_ = true;
  Serial.printf("[HT16K33] Inicializado em 0x%02X\n", address);
}

void Display14Seg::setBrightness(uint8_t brightness) {
  if (!initialized_) {
    Serial.printf("[HT16K33] Desconectado - setBrightness(%d)\n", brightness);
    return;
  }
  if (brightness > 15) brightness = 15;
  display14x4_.setBrightness(brightness);
}

void Display14Seg::setBlinkRate(uint8_t rate) {
  if (!initialized_) {
    Serial.printf("[HT16K33] Desconectado - setBlinkRate(%d)\n", rate);
    return;
  }
  if (rate > 3) rate = 0;
  display14x4_.blinkRate(rate);
}

void Display14Seg::clear() {
  if (!initialized_) {
    Serial.println(F("[HT16K33] Desconectado - clear()"));
    return;
  }
  display14x4_.clear();
  display14x4_.writeDisplay();
}

void Display14Seg::setColon(bool on) {
  if (!initialized_) {
    Serial.printf("[HT16K33] Desconectado - setColon(%s)\n", on ? "true" : "false");
    return;
  }
  // Colon no HT16K33 alpha-numeric e o segmento DP
  // Nota: nao ha readDigitRaw na biblioteca Adafruit, entao sempre setamos
  if (on) {
    display14x4_.writeDigitRaw(1, ALPHANUM_SEG_DP);
  } else {
    display14x4_.writeDigitRaw(1, 0);
  }
  display14x4_.writeDisplay();
}

void Display14Seg::printNum(uint16_t value, bool leadingZeros) {
  if (!initialized_) {
    Serial.printf("[HT16K33] Desconectado - Print number %04d\n", value);
    return;
  }
  char buffer[5];
  snprintf(buffer, sizeof(buffer), leadingZeros ? "%04u" : "%4u", value);
  printString(buffer);
}

void Display14Seg::printString(const char *text) {
  if (!initialized_) {
    Serial.printf("[HT16K33] Desconectado - Print string '%s'\n", text);
    return;
  }
  for (uint8_t i = 0; i < 4; i++) {
    char c = text[i];
    if (c == '\0') break;
    display14x4_.writeDigitAscii(i, c);
  }
  display14x4_.writeDisplay();
}

void Display14Seg::printTime(uint16_t minutes, uint16_t seconds, bool blink) {
  if (!initialized_) {
    Serial.printf("[HT16K33] Desconectado - Print time %02d:%02d\n", minutes % 100, seconds % 60);
    return;
  }
  char buffer[5];
  snprintf(buffer, sizeof(buffer), "%02d%02d", minutes % 100, seconds % 60);

  // Escreve todos os 4 digitos
  // O segundo digito (indice 1) recebe o ponto se blink=true
  display14x4_.writeDigitAscii(0, buffer[0], false);
  display14x4_.writeDigitAscii(1, buffer[1], blink);  // Ponto no segundo digito
  display14x4_.writeDigitAscii(2, buffer[2], false);
  display14x4_.writeDigitAscii(3, buffer[3], false);

  display14x4_.writeDisplay();
}

void Display14Seg::writeDisplay() {
  if (!initialized_) {
    Serial.println(F("[HT16K33] Desconectado - writeDisplay()"));
    return;
  }
  display14x4_.writeDisplay();
}

void Display14Seg::writeDigitRaw(uint8_t digit, uint16_t value) {
  if (!initialized_) {
    Serial.printf("[HT16K33] Desconectado - writeDigitRaw(digit=%d, value=0x%04X)\n", digit, value);
    return;
  }
  if (digit >= 4) return;
  display14x4_.writeDigitRaw(digit, value);
  display14x4_.writeDisplay();
}

void Display14Seg::writeDigitAscii(uint8_t digit, char c) {
  if (!initialized_) {
    Serial.printf("[HT16K33] Desconectado - writeDigitAscii(digit=%d, char='%c')\n", digit, c);
    return;
  }
  if (digit >= 4) return;
  display14x4_.writeDigitAscii(digit, c);
  display14x4_.writeDisplay();
}

}  // namespace esp32_escaperoom
