#pragma once

// ============================================================================
// CONFIG — Constantes de hardware e sistema do Cardputer H4CK-OS
// ============================================================================

// ─── Display ───────────────────────────────────────────────────────────────
// ST7789V2 240×135, rotation 3 (landscape, USB à direita)
// Usar M5.Lcd.width() e height() em vez de constantes fixas

// ─── Fonte FiraCode ────────────────────────────────────────────────────────
// Três tamanhos disponíveis: 8pt, 10pt, 20pt
// Incluir o necessário nos headers que usarem

// ─── SD Card ───────────────────────────────────────────────────────────────
#define SD_CS   12
#define SD_MOSI 14
#define SD_CLK  40
#define SD_MISO 39

// ─── RF433 (SYN531R TX no Grove G2) ───────────────────────────────────────
#define PIN_RF433_DATA  2   // Grove amarelo

// ─── I2S (NS4168) ─────────────────────────────────────────────────────────
#define I2S_BCLK   41
#define I2S_DATA   42
#define I2S_LRCLK  44

// ─── Wi-Fi / MQTT ─────────────────────────────────────────────────────────
// Carregados do config.json no LittleFS

// ─── Dispositivos (targets do invade) ──────────────────────────────────────
#define MAX_TARGETS     3

struct TargetCfg {
    char name[24];
    char topic[64];     // MQTT command topic (ex: "cmnd/luz_sala/POWER")
};

// ─── Constantes de sistema ─────────────────────────────────────────────────
#define APP_NAME        "H4CK-OS"
#define APP_VERSION     "v1.0"
#define DEFAULT_USER    "root"
#define DEFAULT_PASS    "kali"
#define CONFIG_FILE     "/config.json"

// ─── Estados da máquina ────────────────────────────────────────────────────
enum SystemState {
    STATE_BOOT,
    STATE_LOGIN,
    STATE_SHELL,
    STATE_DEFUSED
};

// ─── Tamanhos máximos ─────────────────────────────────────────────────────
#define MAX_USERNAME    32
#define MAX_PASSWORD    64
#define MAX_IP          16
#define MAX_WORDLIST    20
#define MAX_WORD_LEN    32
#define MAX_CMD_LINE    128
#define CMD_HISTORY     16
