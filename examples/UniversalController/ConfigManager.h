#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESP32EscapeRoom.h>

namespace universal_controller {

// Enum dos modos de puzzle
enum class PuzzleMode {
    CUSTOM_SEQUENCE,   // Sequência customizada definida no JSON
    LINEAR_SEQUENCE,   // Ordem linear (0,1,2,3...)
    ALL_CONNECTED,     // Todas entradas ativadas, ordem não importa
    RANDOM,            // Sequência aleatória gerada no boot
    MUSICAL_SEQUENCE   // Sequência de notas musicais via MicInput (INMP441)
};

// Configuração do controlador
struct ControllerConfig {
    char name[32];
    uint8_t id;
    bool enabled;
};

// Configuração de entradas
struct InputConfig {
    uint8_t count;
    uint8_t pcfAddress;
    uint8_t pinMapping[16];
    uint16_t debounceMs;
};

// Configuração da lógica do puzzle
struct LogicConfig {
    PuzzleMode mode;
    uint8_t sequence[16];
    uint8_t sequenceLength;
    uint16_t timeoutSeconds;
    bool resetOnError;
    bool allowRepeat;
};

// Configuração de PWM
struct PwmConfig {
    bool enabled;
    uint8_t channel;
    uint8_t minValue;
    uint8_t maxValue;
    uint8_t steps;
};

// Configuração de áudio
struct AudioConfig {
    bool enabled;
    char inputSounds[16][32];  // Até 16 sons de input
    uint8_t inputSoundCount;
    char unlockSound[32];
    char errorSound[32];
    char resetSound[32];
};

// Configuração de cores NeoPixel
struct NeoPixelConfig {
    bool enabled;
    uint8_t count;
    uint8_t mode;  // 0=PROGRESS_BAR, 1=CHASE, 2=INDIVIDUAL
    uint32_t colorIdle;
    uint32_t colorActive;
    uint32_t colorSolved;
    uint32_t colorError;
    uint32_t colorProgress;
};

// Configuração RS485
struct Rs485Config {
    bool enabled;
    uint8_t address;
    uint32_t baudRate;
};

// Configuração de saída
struct OutputConfig {
    uint8_t lockPin;
    bool activeHigh;
};

// ── Configuração musical (MUSICAL_SEQUENCE) ─────────────────
struct MusicalNote {
    uint8_t midi;          // nota MIDI (ex: 60 = C4)
    uint16_t durationMs;   // duração esperada da nota (referência)
    uint16_t gapMs;        // gap esperado após a nota (referência)
};

struct MusicalSequenceConfig {
    bool enabled;                 // modo musical ativo
    MusicalNote notes[16];        // até 16 notas
    uint8_t noteCount;            // número de notas na sequência
    uint8_t toleranceCents;       // tolerância de afinação (± cents)
    uint16_t maxGapMs;            // gap máximo entre notas (se exceder → TIMEOUT)
    uint16_t maxNoteDurationMs;   // duração máxima de uma nota
    uint16_t totalTimeoutMs;      // timeout total da sequência
    uint16_t minAmplitude;        // limiar de amplitude (silêncio)
};

class ConfigManager {
public:
    ConfigManager();                          // path = "/config.json"
    ConfigManager(const char* path);          // path customizado

    const char* configPath() const { return configPath_; }

    bool begin();
    bool loadConfig();
    bool saveConfig();
    bool loadDefaults();

    // Getters
    const ControllerConfig& getControllerConfig() const { return controller_; }
    const InputConfig& getInputConfig() const { return inputs_; }
    const LogicConfig& getLogicConfig() const { return logic_; }
    const PwmConfig& getPwmConfig() const { return pwm_; }
    const AudioConfig& getAudioConfig() const { return audio_; }
    const NeoPixelConfig& getNeoPixelConfig() const { return neopixel_; }
    const Rs485Config& getRs485Config() const { return rs485_; }
    const OutputConfig& getOutputConfig() const { return output_; }
    const MusicalSequenceConfig& getMusicalConfig() const { return musical_; }

    // Helpers
    static PuzzleMode parseMode(const char* modeStr);
    static const char* modeToString(PuzzleMode mode);
    static uint32_t parseColor(const JsonArray& arr);

private:
    ControllerConfig controller_;
    InputConfig inputs_;
    LogicConfig logic_;
    PwmConfig pwm_;
    AudioConfig audio_;
    NeoPixelConfig neopixel_;
    Rs485Config rs485_;
    OutputConfig output_;
    MusicalSequenceConfig musical_;

    char configPath_[32];

    bool parseControllerConfig(const JsonObject& obj);
    bool parseInputConfig(const JsonObject& obj);
    bool parseLogicConfig(const JsonObject& obj);
    bool parsePwmConfig(const JsonObject& obj);
    bool parseAudioConfig(const JsonObject& obj);
    bool parseNeoPixelConfig(const JsonObject& obj);
    bool parseRs485Config(const JsonObject& obj);
    bool parseOutputConfig(const JsonObject& obj);
    bool parseMusicalConfig(const JsonObject& obj);
};

} // namespace universal_controller