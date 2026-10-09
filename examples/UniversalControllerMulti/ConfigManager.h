#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

namespace universal_controller {

// Puzzle modes enum
enum class PuzzleMode {
    CUSTOM_SEQUENCE,   // Custom sequence defined in JSON
    LINEAR_SEQUENCE,   // Linear order (0,1,2,3...)
    ALL_CONNECTED,     // All inputs activated, order does not matter
    RANDOM             // Random sequence generated at boot
};

// Controller configuration
struct ControllerConfig {
    char name[32];
    uint8_t id;
    bool enabled;
};

// Input configuration
struct InputConfig {
    uint8_t count;
    uint8_t pcfAddress;
    uint8_t pinMapping[16];
    uint16_t debounceMs;
};

// Puzzle logic configuration
struct LogicConfig {
    PuzzleMode mode;
    uint8_t sequence[16];
    uint8_t sequenceLength;
    uint16_t timeoutSeconds;
    bool resetOnError;
    bool allowRepeat;
};

// PWM configuration
struct PwmConfig {
    bool enabled;
    uint8_t channel;
    uint8_t minValue;
    uint8_t maxValue;
    uint8_t steps;
};

// Audio configuration
struct AudioConfig {
    bool enabled;
    char inputSounds[16][32];  // Up to 16 input sounds
    uint8_t inputSoundCount;
    char unlockSound[32];
    char errorSound[32];
    char resetSound[32];
};

// NeoPixel color configuration
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

// RS485 configuration
struct Rs485Config {
    bool enabled;
    uint8_t address;
    uint32_t baudRate;
};

// Output configuration
struct OutputConfig {
    uint8_t lockPin;
    bool activeHigh;
};

class ConfigManager {
public:
    ConfigManager();                          // path = "/config.json"
    ConfigManager(const char* path);          // custom path

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

    char configPath_[32];

    bool parseControllerConfig(const JsonObject& obj);
    bool parseInputConfig(const JsonObject& obj);
    bool parseLogicConfig(const JsonObject& obj);
    bool parsePwmConfig(const JsonObject& obj);
    bool parseAudioConfig(const JsonObject& obj);
    bool parseNeoPixelConfig(const JsonObject& obj);
    bool parseRs485Config(const JsonObject& obj);
    bool parseOutputConfig(const JsonObject& obj);
};

} // namespace universal_controller
