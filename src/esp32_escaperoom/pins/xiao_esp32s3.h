#pragma once

/**
 * pins/xiao_esp32s3.h — Seeed XIAO ESP32-S3 / Sense / Plus
 *
 * Família de placas mini baseadas no ESP32-S3R8.
 * 8MB Flash, 8MB PSRAM. WiFi + BLE 5.0.
 *
 * Periféricos onboard:
 *   - NeoPixel (opcional, pino externo)
 *   - USB-C (UART + OTG)
 *   - Bateria LiPo com carregador
 *   - Microfone PDM na versão **Sense** (MSM261D3526H1CPM, GPIO42/41)
 *
 * Compatível com Expansion Board Base for XIAO:
 *   - OLED SSD1306 0.96" I2C (0x3C)
 *   - RTC PCF8563 I2C (0x51)
 *   - MicroSD SPI
 *   - Buzzer passivo (A3)
 *   - Botão usuário (D1)
 *   - Servo (D5)
 *   - Grove I2C/UART/A0D0
 *
 * Ref: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
 *      https://wiki.seeedstudio.com/xiao_esp32s3_sense_mic/  (mic PDM)
 */

#include <Arduino.h>

namespace esp32_escaperoom {

constexpr const char* BOARD_NAME         = "Seeed XIAO ESP32-S3";
constexpr const char* BOARD_MANUFACTURER = "Seeed Studio";
constexpr const char* BOARD_CHIP         = "ESP32-S3R8";

// ── Memória ──────────────────────────────────────────────────
#define BOARD_FLASH_MB  8
#define BOARD_PSRAM_MB  8
#define HAS_PSRAM       1

// ── Periféricos onboard ──────────────────────────────────────

#define HAS_ONBOARD_NEOPIXEL   0
#define HAS_ONBOARD_BATTERY    1    // LiPo charger

// ── I2C ──────────────────────────────────────────────────────
constexpr int PIN_I2C_SDA = 5;   // D4 (GPIO5)
constexpr int PIN_I2C_SCL = 6;   // D5 (GPIO6)

// ── I2S ──────────────────────────────────────────────────────
#define HAS_ONBOARD_I2S_AMP     0
constexpr int PIN_I2S_BCLK = 43;  // D6
constexpr int PIN_I2S_WS   = 44;  // D7
constexpr int PIN_I2S_DATA = 4;   // D3 (GPIO4)
constexpr bool HAS_I2S     = true;

// ── Microfone PDM onboard (versão Sense) ─────────────────────
// O XIAO ESP32S3 **Sense** traz um microfone MEMS PDM (MSM261D3526H1CPM) na
// placa de expansão. CLK/DATA ficam em GPIOs dedicados, fora do header D0-D10:
//   GPIO42 = PDM CLK  (D12)
//   GPIO41 = PDM DATA (D11)
// O driver lê PDM via modo PDM_RX (i2s_pdm.h), não pelo modo STD: o PDM RX
// aceita SOMENTE 16 bits e a conversão PDM->PCM é feita por hardware no S3.
// As versões não-Sense (XIAO ESP32S3 puro, Plus) NÃO têm este microfone.
#define HAS_ONBOARD_MIC         1
constexpr int PIN_I2S_PDM_CLK  = 42;   // D12 — mic PDM onboard (CLK)
constexpr int PIN_I2S_PDM_DATA = 41;   // D11 — mic PDM onboard (DATA)

// ── NeoPixel ─────────────────────────────────────────────────
// Sem NeoPixel onboard. Usar D2 (GPIO3) para NeoPixel externo.
constexpr int PIN_EXTERNAL_NEOPIXEL = 3;   // D2
constexpr int PIN_NEOPIXEL          = PIN_EXTERNAL_NEOPIXEL;
constexpr int PIN_NEOPIXEL_INTERNAL = 255;

// ── SPI ──────────────────────────────────────────────────────
// D8/D9/D10 — reservado para SD Card (Expansion Board)
constexpr int PIN_SPI_SCK  = 7;   // D8
constexpr int PIN_SPI_MISO = 8;   // D9
constexpr int PIN_SPI_MOSI = 9;   // D10

// ── SD Card ──────────────────────────────────────────────────
#define HAS_ONBOARD_SD         0    // requer Expansion Board
constexpr int PIN_SD_CS    = 3;   // D2 (GPIO3)
#define USE_SD_LIBRARY SD

// ── Atuadores ────────────────────────────────────────────────
#define HAS_ONBOARD_RELAY    0
constexpr int PIN_RELAY      = 1;   // D0 (GPIO1)
constexpr int PIN_SERVO_PWM  = 2;   // D1 (GPIO2)

// ── Expansion Board ──────────────────────────────────────────
// OLED SSD1306 0.96" 128×64 (I2C)
constexpr uint8_t OLED_I2C_ADDRESS = 0x3C;
// RTC PCF8563 (I2C, bateria CR1220)
constexpr uint8_t RTC_I2C_ADDRESS  = 0x51;
#define HAS_ONBOARD_RTC 1    // via Expansion Board
// Buzzer passivo
constexpr int PIN_BUZZER = 4;   // D3 (GPIO4) — via Expansion Board
// Botão usuário
constexpr int PIN_EXPANSION_BUTTON = 2;  // D1 (GPIO2)

// ── Botões ───────────────────────────────────────────────────
constexpr int PIN_BTN = 0;  // BOOT (GPIO0)

// ── Power ────────────────────────────────────────────────────
constexpr int PIN_EXTERNAL_POWER     = 21;
constexpr int PIN_BATTERY_SENSE      = 10;  // ADC_BAT (GPIO10)

// ── Display externo ──────────────────────────────────────────
#define HAS_ONBOARD_DISPLAY 1    // via Expansion Board
// OLED_I2C_ADDRESS já definido acima

// ── Flags ────────────────────────────────────────────────────
#define HAS_WIFI            1
#define HAS_BLUETOOTH       1
#define HAS_ONBOARD_LEDS    0
#define HAS_ONBOARD_BUZZER  0

// ── IO disponíveis ───────────────────────────────────────────
constexpr int PIN_IO0  = 1;    // D0
constexpr int PIN_IO1  = 2;    // D1
constexpr int PIN_IO2  = 3;    // D2
constexpr int PIN_IO3  = 4;    // D3
constexpr int PIN_IO4  = 5;    // D4 (I2C SDA)
constexpr int PIN_IO5  = 6;    // D5 (I2C SCL)
constexpr int PIN_IO6  = 43;   // D6
constexpr int PIN_IO7  = 44;   // D7
constexpr int PIN_IO8  = 7;    // D8
constexpr int PIN_IO9  = 8;    // D9
constexpr int PIN_IO10 = 9;    // D10
constexpr int PIN_IO11 = 42;   // D11 (sem ADC)
constexpr int PIN_IO12 = 41;   // D12 (sem ADC)

// ── Constantes ───────────────────────────────────────────────
constexpr uint8_t I2C_DEFAULT_ADDRESS = 0x20;
constexpr unsigned long DOOR_OPEN_MS  = 5000;

inline void boardInit() {
  // XIAO S3 não precisa de init especial
}

}  // namespace esp32_escaperoom
