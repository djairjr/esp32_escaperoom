#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <ESP32EscapeRoom.h>

namespace universal_unique_rfid {

enum class RfidMode {
    UNIQUE,
    SEQUENCE
};

struct ControllerConfig {
    char name[32];
    uint8_t id;
    bool enabled;
};

struct RfidTag {
    char uid[16];
    char name[24];
};

struct ValidTag {
    char uid[16];
    char name[24];
};

struct RfidConfig {
    RfidMode mode;
    RfidTag tags[16];
    uint8_t tagCount;
    ValidTag validTags[32];
    uint8_t validTagCount;
    uint8_t sequence[16];
    uint8_t sequenceLength;
    uint16_t timeoutSeconds;
    bool resetOnError;
};

struct AudioConfig {
    bool enabled;
    char tagSound[32];
    char correctSound[32];
    char wrongSound[32];
    char unlockSound[32];
    char errorSound[32];
    char resetSound[32];
};

struct NeoPixelConfig {
    bool enabled;
    uint8_t count;
    uint32_t colorIdle;
    uint32_t colorTagRead;
    uint32_t colorCorrect;
    uint32_t colorWrong;
    uint32_t colorSolved;
};

struct Rs485Config {
    bool enabled;
    uint8_t address;
    uint32_t baudRate;
};

struct OutputConfig {
    uint8_t lockPin;
    bool activeHigh;
};

class ConfigManager {
public:
    ConfigManager();
    bool begin();
    bool loadConfig();
    bool saveConfig();
    bool loadDefaults();

    const ControllerConfig& getControllerConfig() const { return controller_; }
    const RfidConfig& getRfidConfig() const { return rfid_; }
    const AudioConfig& getAudioConfig() const { return audio_; }
    const NeoPixelConfig& getNeoPixelConfig() const { return neopixel_; }
    const Rs485Config& getRs485Config() const { return rs485_; }
    const OutputConfig& getOutputConfig() const { return output_; }

    static RfidMode parseMode(const char* modeStr);
    static const char* modeToString(RfidMode mode);
    static uint32_t parseColor(const JsonArray& arr);

private:
    static constexpr const char* CONFIG_FILE = "/config.json";
    ControllerConfig controller_;
    RfidConfig rfid_;
    AudioConfig audio_;
    NeoPixelConfig neopixel_;
    Rs485Config rs485_;
    OutputConfig output_;

    bool parseControllerConfig(const JsonObject& obj);
    bool parseRfidConfig(const JsonObject& obj);
    bool parseAudioConfig(const JsonObject& obj);
    bool parseNeoPixelConfig(const JsonObject& obj);
    bool parseRs485Config(const JsonObject& obj);
    bool parseOutputConfig(const JsonObject& obj);
};

} // namespace universal_unique_rfid
