#pragma once

#include <Arduino.h>

namespace esp32_escaperoom {

/**
 * Wrapper para biblioteca RotaryDial unificada (disco rotativo de telefone).
 * Prove interface simples para leitura de digitos discados.
 *
 * Modos suportados (definidos na biblioteca):
 *   ROTARYDIAL_MECHANICAL (default) — contatos metalicos
 *   ROTARYDIAL_OPTICAL            — encoder otico
 *
 * Dependencia externa:
 *   Biblioteca RotaryDial v1.0.0+ em Documents/Arduino/libraries/RotaryDial
 *
 * Hardware wiring:
 *   - Conecte o terminal NC (Normal Closed) do disco ao pino GPIO
 *   - Conecte o outro terminal ao GND
 *   - Use INPUT_PULLUP ou INPUT (dependendo do modelo de telefone)
 */
class RotaryDialReader {
 public:
  RotaryDialReader() : initialized_(false), lastPin_(0) {}

  /**
   * Inicializa o leitor de disco rotativo no modo padrao (MECHANICAL).
   * @param pin Pino GPIO conectado ao terminal NC do disco
   * @return true se inicializado com sucesso
   */
  bool begin(uint8_t pin);

  /**
   * Inicializa o leitor no modo optico, com timings calibrados.
   * @param pin       Pino GPIO conectado ao sensor otico
   * @param debounce  Piso (ms) — filtra ruido e pulsos da volta (ex: 50)
   * @param maxgap    Teto (ms) — 0 = sem teto (recomendado para disco Djair)
   * @param digitend  Timeout (ms) para fechar o digito (ex: 800)
   * @return true se inicializado com sucesso
   */
  bool beginOptical(uint8_t pin, unsigned long debounce, unsigned long maxgap, unsigned long digitend);

  /**
   * Verifica se ha um digito disponivel para leitura.
   * @return true se digito foi discado e esta pronto
   */
  bool available();

  /**
   * Le o digito discado.
   * @return Digito discado (0-9), ou -1 se nao disponivel
   */
  int read();

  /** Zera buffer interno do RotaryDial (pulseCount, firstPulse, etc). */
  void reset();

  /**
   * Verifica se foi inicializado.
   */
  bool isInitialized() const { return initialized_; }

  /**
   * Retorna o pino configurado.
   */
  uint8_t getPin() const { return lastPin_; }

 private:
  bool initialized_;
  uint8_t lastPin_;
};

}  // namespace esp32_escaperoom
