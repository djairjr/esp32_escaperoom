#include "ConfigManager.h"
#include <Adafruit_NeoPixel.h>

namespace universal_controller {

// Helper para converter RGB para uint32_t
inline uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

ConfigManager::ConfigManager() {
    strncpy(configPath_, "/config.json", sizeof(configPath_));
    loadDefaults();
}

ConfigManager::ConfigManager(const char* path) {
    strncpy(configPath_, path, sizeof(configPath_) - 1);
    configPath_[sizeof(configPath_) - 1] = '\0';
    loadDefaults();
}

bool ConfigManager::begin() {
    if (!LittleFS.begin()) {
        Serial.println("[ConfigManager] Falha ao montar LittleFS");
        return false;
    }
    return true;
}

bool ConfigManager::loadDefaults() {
    // Controller
    strncpy(controller_.name, "Puzzle Universal", sizeof(controller_.name));
    controller_.id = 1;
    controller_.enabled = true;

    // Inputs
    inputs_.count = 8;
    inputs_.pcfAddress = 0x20;
    for (uint8_t i = 0; i < 16; i++) {
        inputs_.pinMapping[i] = i;
    }
    inputs_.debounceMs = 50;

    // Logic
    logic_.mode = PuzzleMode::LINEAR_SEQUENCE;
    logic_.sequenceLength = 0;
    logic_.timeoutSeconds = 300;
    logic_.resetOnError = true;
    logic_.allowRepeat = false;

    // PWM
    pwm_.enabled = true;
    pwm_.channel = 0;
    pwm_.minValue = 0;
    pwm_.maxValue = 100;
    pwm_.steps = 5;

    // Audio
    audio_.enabled = true;
    audio_.inputSoundCount = 5;
    for (uint8_t i = 0; i < 5; i++) {
        snprintf(audio_.inputSounds[i], sizeof(audio_.inputSounds[i]), "/input%d.mp3", i + 1);
    }
    strncpy(audio_.unlockSound, "/unlock.mp3", sizeof(audio_.unlockSound));
    strncpy(audio_.errorSound, "/error.mp3", sizeof(audio_.errorSound));
    strncpy(audio_.resetSound, "/reset.mp3", sizeof(audio_.resetSound));

    // NeoPixel
    neopixel_.enabled = true;
    neopixel_.count = 16;
    neopixel_.mode = 0;  // PROGRESS_BAR
    neopixel_.colorIdle = Adafruit_NeoPixel::Color(0, 0, 255);
    neopixel_.colorActive = Adafruit_NeoPixel::Color(255, 255, 0);
    neopixel_.colorSolved = Adafruit_NeoPixel::Color(0, 255, 0);
    neopixel_.colorError = Adafruit_NeoPixel::Color(255, 0, 0);
    neopixel_.colorProgress = Adafruit_NeoPixel::Color(0, 255, 100);

    // RS485
    rs485_.enabled = false;
    rs485_.address = 1;
    rs485_.baudRate = 115200;

    // Output
    output_.lockPin = 10;
    output_.activeHigh = false;

    return true;
}

bool ConfigManager::loadConfig() {
    File file = LittleFS.open(configPath_, "r");
    if (!file) {
        Serial.printf("[ConfigManager] %s nao encontrado, usando defaults\n", configPath_);
        return false;
    }

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.printf("[ConfigManager] Erro ao parsear JSON: %s\n", error.c_str());
        return false;
    }

    bool success = true;
    if (doc.containsKey("controller")) {
        success &= parseControllerConfig(doc["controller"]);
    }
    if (doc.containsKey("inputs")) {
        success &= parseInputConfig(doc["inputs"]);
    }
    if (doc.containsKey("logic")) {
        success &= parseLogicConfig(doc["logic"]);
    }
    if (doc.containsKey("pwm")) {
        success &= parsePwmConfig(doc["pwm"]);
    }
    if (doc.containsKey("audio")) {
        success &= parseAudioConfig(doc["audio"]);
    }
    if (doc.containsKey("neopixel")) {
        success &= parseNeoPixelConfig(doc["neopixel"]);
    }
    if (doc.containsKey("rs485")) {
        success &= parseRs485Config(doc["rs485"]);
    }
    if (doc.containsKey("output")) {
        success &= parseOutputConfig(doc["output"]);
    }

    if (success) {
        Serial.println("[ConfigManager] Configuração carregada com sucesso");
    }

    return success;
}

bool ConfigManager::saveConfig() {
    File file = LittleFS.open(configPath_, "w");
    if (!file) {
        Serial.println("[ConfigManager] Falha ao abrir arquivo para escrita");
        return false;
    }

    StaticJsonDocument<4096> doc;

    // Controller
    JsonObject controller = doc.createNestedObject("controller");
    controller["name"] = controller_.name;
    controller["id"] = controller_.id;
    controller["enabled"] = controller_.enabled;

    // Inputs
    JsonObject inputs = doc.createNestedObject("inputs");
    inputs["count"] = inputs_.count;
    inputs["pcfAddress"] = inputs_.pcfAddress;
    JsonArray pinMapping = inputs.createNestedArray("pinMapping");
    for (uint8_t i = 0; i < inputs_.count; i++) {
        pinMapping.add(inputs_.pinMapping[i]);
    }
    inputs["debounceMs"] = inputs_.debounceMs;

    // Logic
    JsonObject logic = doc.createNestedObject("logic");
    logic["mode"] = modeToString(logic_.mode);
    if (logic_.sequenceLength > 0) {
        JsonArray sequence = logic.createNestedArray("sequence");
        for (uint8_t i = 0; i < logic_.sequenceLength; i++) {
            sequence.add(logic_.sequence[i]);
        }
    }
    logic["timeoutSeconds"] = logic_.timeoutSeconds;
    logic["resetOnError"] = logic_.resetOnError;
    logic["allowRepeat"] = logic_.allowRepeat;

    // PWM
    JsonObject pwm = doc.createNestedObject("pwm");
    pwm["enabled"] = pwm_.enabled;
    pwm["channel"] = pwm_.channel;
    pwm["minValue"] = pwm_.minValue;
    pwm["maxValue"] = pwm_.maxValue;
    pwm["steps"] = pwm_.steps;

    // Audio
    JsonObject audio = doc.createNestedObject("audio");
    audio["enabled"] = audio_.enabled;
    JsonArray inputSounds = audio.createNestedArray("inputSounds");
    for (uint8_t i = 0; i < audio_.inputSoundCount; i++) {
        inputSounds.add(audio_.inputSounds[i]);
    }
    audio["unlockSound"] = audio_.unlockSound;
    audio["errorSound"] = audio_.errorSound;
    audio["resetSound"] = audio_.resetSound;

    // NeoPixel
    JsonObject neopixel = doc.createNestedObject("neopixel");
    neopixel["enabled"] = neopixel_.enabled;
    neopixel["count"] = neopixel_.count;
    neopixel["mode"] = neopixel_.mode;
    JsonObject colors = neopixel.createNestedObject("colors");
    JsonArray idle = colors.createNestedArray("idle");
    idle.add((neopixel_.colorIdle >> 16) & 0xFF);
    idle.add((neopixel_.colorIdle >> 8) & 0xFF);
    idle.add(neopixel_.colorIdle & 0xFF);
    // ... outros cores

    // RS485
    JsonObject rs485 = doc.createNestedObject("rs485");
    rs485["enabled"] = rs485_.enabled;
    rs485["address"] = rs485_.address;
    rs485["baudRate"] = rs485_.baudRate;

    // Output
    JsonObject output = doc.createNestedObject("output");
    output["lockPin"] = output_.lockPin;
    output["activeHigh"] = output_.activeHigh;

    serializeJson(doc, file);
    file.close();

    return true;
}

bool ConfigManager::parseControllerConfig(const JsonObject& obj) {
    if (obj.containsKey("name")) {
        strncpy(controller_.name, obj["name"], sizeof(controller_.name));
    }
    if (obj.containsKey("id")) {
        controller_.id = obj["id"];
    }
    if (obj.containsKey("enabled")) {
        controller_.enabled = obj["enabled"];
    }
    return true;
}

bool ConfigManager::parseInputConfig(const JsonObject& obj) {
    if (obj.containsKey("count")) {
        inputs_.count = constrain(obj["count"], 1, 16);
    }
    if (obj.containsKey("pcfAddress")) {
        inputs_.pcfAddress = obj["pcfAddress"];
    }
    if (obj.containsKey("pinMapping")) {
        JsonArray arr = obj["pinMapping"];
        uint8_t len = min((size_t)inputs_.count, arr.size());
        for (uint8_t i = 0; i < len; i++) {
            inputs_.pinMapping[i] = arr[i];
        }
    }
    if (obj.containsKey("debounceMs")) {
        inputs_.debounceMs = obj["debounceMs"];
    }
    return true;
}

bool ConfigManager::parseLogicConfig(const JsonObject& obj) {
    if (obj.containsKey("mode")) {
        logic_.mode = parseMode(obj["mode"]);
    }
    if (obj.containsKey("sequence")) {
        JsonArray arr = obj["sequence"];
        logic_.sequenceLength = min((size_t)16, arr.size());
        for (uint8_t i = 0; i < logic_.sequenceLength; i++) {
            logic_.sequence[i] = arr[i];
        }
    }
    if (obj.containsKey("timeoutSeconds")) {
        logic_.timeoutSeconds = obj["timeoutSeconds"];
    }
    if (obj.containsKey("resetOnError")) {
        logic_.resetOnError = obj["resetOnError"];
    }
    if (obj.containsKey("allowRepeat")) {
        logic_.allowRepeat = obj["allowRepeat"];
    }
    return true;
}

bool ConfigManager::parsePwmConfig(const JsonObject& obj) {
    if (obj.containsKey("enabled")) {
        pwm_.enabled = obj["enabled"];
    }
    if (obj.containsKey("channel")) {
        pwm_.channel = obj["channel"];
    }
    if (obj.containsKey("minValue")) {
        pwm_.minValue = obj["minValue"];
    }
    if (obj.containsKey("maxValue")) {
        pwm_.maxValue = obj["maxValue"];
    }
    if (obj.containsKey("steps")) {
        pwm_.steps = obj["steps"];
    }
    return true;
}

bool ConfigManager::parseAudioConfig(const JsonObject& obj) {
    if (obj.containsKey("enabled")) {
        audio_.enabled = obj["enabled"];
    }
    if (obj.containsKey("inputSounds")) {
        JsonArray arr = obj["inputSounds"];
        audio_.inputSoundCount = min((size_t)16, arr.size());
        for (uint8_t i = 0; i < audio_.inputSoundCount; i++) {
            strncpy(audio_.inputSounds[i], arr[i], sizeof(audio_.inputSounds[i]));
        }
    }
    if (obj.containsKey("unlockSound")) {
        strncpy(audio_.unlockSound, obj["unlockSound"], sizeof(audio_.unlockSound));
    }
    if (obj.containsKey("errorSound")) {
        strncpy(audio_.errorSound, obj["errorSound"], sizeof(audio_.errorSound));
    }
    if (obj.containsKey("resetSound")) {
        strncpy(audio_.resetSound, obj["resetSound"], sizeof(audio_.resetSound));
    }
    return true;
}

bool ConfigManager::parseNeoPixelConfig(const JsonObject& obj) {
    if (obj.containsKey("enabled")) {
        neopixel_.enabled = obj["enabled"];
    }
    if (obj.containsKey("count")) {
        neopixel_.count = obj["count"];
    }
    if (obj.containsKey("mode")) {
        neopixel_.mode = obj["mode"];
    }
    if (obj.containsKey("colors")) {
        JsonObject colors = obj["colors"];
        if (colors.containsKey("idle")) {
            neopixel_.colorIdle = parseColor(colors["idle"]);
        }
        if (colors.containsKey("active")) {
            neopixel_.colorActive = parseColor(colors["active"]);
        }
        if (colors.containsKey("solved")) {
            neopixel_.colorSolved = parseColor(colors["solved"]);
        }
        if (colors.containsKey("error")) {
            neopixel_.colorError = parseColor(colors["error"]);
        }
        if (colors.containsKey("progress")) {
            neopixel_.colorProgress = parseColor(colors["progress"]);
        }
    }
    return true;
}

bool ConfigManager::parseRs485Config(const JsonObject& obj) {
    if (obj.containsKey("enabled")) {
        rs485_.enabled = obj["enabled"];
    }
    if (obj.containsKey("address")) {
        rs485_.address = obj["address"];
    }
    if (obj.containsKey("baudRate")) {
        rs485_.baudRate = obj["baudRate"];
    }
    return true;
}

bool ConfigManager::parseOutputConfig(const JsonObject& obj) {
    if (obj.containsKey("lockPin")) {
        output_.lockPin = obj["lockPin"];
    }
    if (obj.containsKey("activeHigh")) {
        output_.activeHigh = obj["activeHigh"];
    }
    return true;
}

PuzzleMode ConfigManager::parseMode(const char* modeStr) {
    if (strcmp(modeStr, "CUSTOM_SEQUENCE") == 0) {
        return PuzzleMode::CUSTOM_SEQUENCE;
    } else if (strcmp(modeStr, "LINEAR_SEQUENCE") == 0) {
        return PuzzleMode::LINEAR_SEQUENCE;
    } else if (strcmp(modeStr, "ALL_CONNECTED") == 0) {
        return PuzzleMode::ALL_CONNECTED;
    } else if (strcmp(modeStr, "RANDOM") == 0) {
        return PuzzleMode::RANDOM;
    }
    return PuzzleMode::LINEAR_SEQUENCE;  // Default
}

const char* ConfigManager::modeToString(PuzzleMode mode) {
    switch (mode) {
        case PuzzleMode::CUSTOM_SEQUENCE:
            return "CUSTOM_SEQUENCE";
        case PuzzleMode::LINEAR_SEQUENCE:
            return "LINEAR_SEQUENCE";
        case PuzzleMode::ALL_CONNECTED:
            return "ALL_CONNECTED";
        case PuzzleMode::RANDOM:
            return "RANDOM";
        default:
            return "LINEAR_SEQUENCE";
    }
}

uint32_t ConfigManager::parseColor(const JsonArray& arr) {
    if (arr.size() >= 3) {
        uint8_t r = arr[0];
        uint8_t g = arr[1];
        uint8_t b = arr[2];
        return Color(r, g, b);
    }
    return Color(0, 0, 0);
}

} // namespace universal_controller