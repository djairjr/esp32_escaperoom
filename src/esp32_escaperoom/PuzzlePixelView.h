#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

namespace esp32_escaperoom {

/**
 * @brief View de uma faixa de NeoPixels para um puzzle específico.
 *
 * Atua como uma "janela" sobre o strip físico de NeoPixels,
 * permitindo que cada puzzle trabalhe com índices locais (0 a count-1)
 * que são mapeados para posições absolutas no strip físico.
 *
 * Exemplo: PuzzlePixelView(strip, 11, 8) → mapeia índices 0-7
 * para LEDs 11-18 do strip físico.
 */
class PuzzlePixelView {
 public:
  /**
   * @brief Construtor
   * @param strip Referência ao objeto Adafruit_NeoPixel físico
   * @param offset Índice inicial no strip físico (0-based)
   * @param count Número de LEDs na faixa deste puzzle
   */
  PuzzlePixelView(Adafruit_NeoPixel* strip, uint8_t offset, uint8_t count)
      : strip_(strip), offset_(offset), count_(count) {}

  /**
   * @brief Define a cor de um LED específico (índice local)
   * @param localIndex Índice local do LED (0 a count-1)
   * @param color Cor no formato NeoPixel (Color(r, g, b))
   * @return true se o índice é válido, false caso contrário
   */
  bool setPixel(uint8_t localIndex, uint32_t color) {
    if (!strip_ || localIndex >= count_) return false;
    strip_->setPixelColor(offset_ + localIndex, color);
    return true;
  }

  /**
   * @brief Preenche toda a faixa do puzzle com uma cor
   * @param color Cor no formato NeoPixel
   */
  void setAll(uint32_t color) {
    if (!strip_) return;
    for (uint8_t i = 0; i < count_; i++) {
      strip_->setPixelColor(offset_ + i, color);
    }
  }

  /**
   * @brief Apaga todos os LEDs da faixa (preto)
   */
  void clear() { setAll(0); }

  /**
   * @brief Envia os dados para o strip físico (chama show() no strip)
   * @return true se strip existe, false caso contrário
   */
  bool show() {
    if (!strip_) return false;
    strip_->show();
    return true;
  }

  /**
   * @brief Define cor e chama show() em uma operação
   * @param localIndex Índice local do LED
   * @param color Cor no formato NeoPixel
   * @return true se sucesso, false se índice inválido
   */
  bool setPixelAndShow(uint8_t localIndex, uint32_t color) {
    if (!setPixel(localIndex, color)) return false;
    return show();
  }

  /**
   * @brief Preenche faixa e chama show()
   * @param color Cor no formato NeoPixel
   */
  void setAllAndShow(uint32_t color) {
    setAll(color);
    show();
  }

  /**
   * @brief Retorna o número de LEDs nesta faixa
   */
  uint8_t count() const { return count_; }

  /**
   * @brief Retorna o offset inicial no strip físico
   */
  uint8_t offset() const { return offset_; }

  /**
   * @brief Verifica se um índice local é válido
   */
  bool isValidIndex(uint8_t localIndex) const { return localIndex < count_; }

  /**
   * @brief Retorna acesso ao strip físico subjacente
   * Útil para operações avançadas não cobertas por esta classe
   */
  Adafruit_NeoPixel* strip() { return strip_; }

  /**
   * @brief Configura o brilho global do strip (afeta todos os puzzles!)
   * @param brightness Brilho de 0 a 255
   */
  void setBrightness(uint8_t brightness) {
    if (strip_) strip_->setBrightness(brightness);
  }

  /**
   * @brief Cria uma cor a partir de componentes RGB
   * @return Cor no formato NeoPixel
   */
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }

  /**
   * @brief Cria uma cor a partir de componentes RGBW
   * @return Cor no formato NeoPixel
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
