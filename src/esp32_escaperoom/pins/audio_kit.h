#pragma once

/**
 * pins/audio_kit.h — Ai-Thinker ESP32-A1S Audio Kit v2.2 (A202)
 *
 * Placa de áudio com codec ES8388 + amplificador headphone.
 * ESP32 dual-core, 4MB Flash, 4MB PSRAM.
 *
 * Periféricos onboard:
 *   - ES8388 codec I2S (DAC + ADC)
 *   - Amplificador headphone 3.5mm
 *   - Microfones onboard (analógico + digital)
 *   - Slot microSD SPI
 *   - Botões de volume e play
 *
 * ⚠️ GPIOs 0,18,19,21,22,23,25,26,27 disponíveis. Os demais são do codec.
 */

#include <Arduino.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "Ai-Thinker ESP32-A1S Audio Kit v2.2";
constexpr const char* BOARD_MANUFACTURER = "Ai-Thinker";
constexpr const char* BOARD_CHIP         = "ESP32";

// ── Memória ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  4
#define BOARD_PSRAM_MB  4
#define HAS_PSRAM       1

// ── Periféricos onboard ──────────────────────────────────────

#define HAS_ONBOARD_CODEC      1    // ES8388 I2S codec
#define HAS_ONBOARD_HP_AMP     1    // amplificador headphone
#define HAS_ONBOARD_MIC        1    // microfone onboard
#define HAS_ONBOARD_SD         1    // slot microSD SPI

// I2C — compartilhado com codec ES8388 (GPIO18=SDA, GPIO23=SCL)
constexpr int PIN_I2C_SDA = 18;
constexpr int PIN_I2C_SCL = 23;

// ── ES8388 Audio Codec ───────────────────────────────────────
// Codec I2C: 0x10 (ES8388)
constexpr uint8_t ES8388_I2C_ADDRESS = 0x10;

// I2S — GPIO25=BCLK, GPIO26=WS, GPIO22=DOUT, GPIO21=SDIN
constexpr int PIN_I2S_BCLK = 25;
constexpr int PIN_I2S_WS   = 26;
constexpr int PIN_I2S_DATA = 22;   // DOUT → codec
constexpr int PIN_I2S_SDIN = 21;   // SDIN ← codec (mic)
constexpr bool HAS_I2S     = true;

// ── Headphone / Line Out ─────────────────────────────────────
constexpr int PIN_HP_DETECT = 19;  // jack detect (LOW = inserido)
constexpr int PIN_HP_SEL    = -1;  // auto pelo codec

// ── NeoPixel ─────────────────────────────────────────────────
#define HAS_ONBOARD_NEOPIXEL 0
constexpr int PIN_EXTERNAL_NEOPIXEL = 27;
constexpr int PIN_NEOPIXEL          = 27;
constexpr int PIN_NEOPIXEL_INTERNAL = 255;

// ── SPI (VSPI) ───────────────────────────────────────────────
constexpr int PIN_SPI_SCK  = 14;
constexpr int PIN_SPI_MISO = 12;
constexpr int PIN_SPI_MOSI = 13;

// ── SD Card ──────────────────────────────────────────────────
constexpr int PIN_SD_CS    = 15;
#define USE_SD_LIBRARY SD

// ── Atuadores ────────────────────────────────────────────────
#define HAS_ONBOARD_RELAY    0
constexpr int PIN_RELAY      = 19;  // GPIO19
constexpr int PIN_SERVO_PWM  = 0;   // GPIO0 (BOOT — evitar usar como servo)

// ── Botões ───────────────────────────────────────────────────
constexpr int PIN_BTN              = 0;    // BOOT
constexpr int PIN_EXPANSION_BUTTON = 19;   // GPIO19
// Audio Kit tem botões dedicados via matriz de teclas (K1-K6)
// mas esses não são mapeados como GPIOs individuais

// ── Power ────────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = -1;
constexpr int PIN_BATTERY_SENSE      = -1;

// ── Display externo ──────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 0
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;

// ── RTC ──────────────────────────────────────────────────────
#define HAS_ONBOARD_RTC 0
constexpr uint8_t RTC_I2C_ADDRESS = 0x51;

// ── Flags ────────────────────────────────────────────────────
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1
#define HAS_ONBOARD_LEDS    0
#define HAS_ONBOARD_BUZZER  0

constexpr int PIN_BUZZER = -1;

// ── IO disponíveis ───────────────────────────────────────────
constexpr int PIN_IO0  = 0;    // GPIO0  (BOOT)
constexpr int PIN_IO1  = 19;   // GPIO19
constexpr int PIN_IO2  = 21;   // GPIO21
constexpr int PIN_IO3  = 27;   // GPIO27
constexpr int PIN_IO4  = 255;  // indisponível
constexpr int PIN_IO5  = 255;
constexpr int PIN_IO6  = 255;
constexpr int PIN_IO7  = 255;
constexpr int PIN_IO8  = 255;
constexpr int PIN_IO9  = 255;
constexpr int PIN_IO10 = 255;
constexpr int PIN_IO11 = 255;
constexpr int PIN_IO12 = 255;

// ── Constantes ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  // ES8388 codec — configurar via I2C para modo de áudio
  // O codec é inicializado com defaults aceitáveis para MP3 playback
  #if HAS_ONBOARD_CODEC
  // Volume padrão: 70% (-12dB no ES8388)
  // O Arduino framework já configura I2S pins automaticamente
  #endif
}

}  // namespace esp32_escaperoom
