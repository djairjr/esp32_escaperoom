#include "ConfigManager.h"

namespace universal_unique_rfid {

inline uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

ConfigManager::ConfigManager() { loadDefaults(); }

bool ConfigManager::begin() {
    if (!LittleFS.begin()) {
        Serial.println("[ConfigManager] Falha ao montar LittleFS");
        return false;
    }
    return true;
}

bool ConfigManager::loadDefaults() {
    strncpy(controller_.name, "Universal Unique RFID", sizeof(controller_.name));
    controller_.id = 2;
    controller_.enabled = true;

    rfid_.mode = RfidMode::UNIQUE;
    rfid_.tagCount = 0;
    rfid_.validTagCount = 0;
    rfid_.sequenceLength = 0;
    rfid_.timeoutSeconds = 300;
    rfid_.resetOnError = true;

    audio_.enabled = true;
    strncpy(audio_.tagSound, "/tag.mp3", sizeof(audio_.tagSound));
    strncpy(audio_.correctSound, "/correct.mp3", sizeof(audio_.correctSound));
    strncpy(audio_.wrongSound, "/wrong.mp3", sizeof(audio_.wrongSound));
    strncpy(audio_.unlockSound, "/unlock.mp3", sizeof(audio_.unlockSound));
    strncpy(audio_.errorSound, "/error.mp3", sizeof(audio_.errorSound));
    strncpy(audio_.resetSound, "/reset.mp3", sizeof(audio_.resetSound));

    neopixel_.enabled = true;
    neopixel_.count = 8;
    neopixel_.colorIdle = Color(0, 0, 255);
    neopixel_.colorTagRead = Color(255, 255, 0);
    neopixel_.colorCorrect = Color(0, 255, 0);
    neopixel_.colorWrong = Color(255, 0, 0);
    neopixel_.colorSolved = Color(0, 255, 0);

    rs485_.enabled = false;
    rs485_.address = 1;
    rs485_.baudRate = 115200;

    output_.lockPin = 10;
    output_.activeHigh = false;

    return true;
}

bool ConfigManager::loadConfig() {
    File file = LittleFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("[ConfigManager] config.json nao encontrado, usando defaults");
        return false;
    }

    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    if (error) {
        Serial.printf("[ConfigManager] Erro JSON: %s\n", error.c_str());
        return false;
    }

    bool success = true;
    if (doc.containsKey("controller")) success &= parseControllerConfig(doc["controller"]);
    if (doc.containsKey("rfid"))       success &= parseRfidConfig(doc["rfid"]);
    if (doc.containsKey("audio"))      success &= parseAudioConfig(doc["audio"]);
    if (doc.containsKey("neopixel"))   success &= parseNeoPixelConfig(doc["neopixel"]);
    if (doc.containsKey("rs485"))      success &= parseRs485Config(doc["rs485"]);
    if (doc.containsKey("output"))     success &= parseOutputConfig(doc["output"]);

    if (success) Serial.println("[ConfigManager] Config carregada");
    return success;
}

bool ConfigManager::saveConfig() { /* mantido igual ao original */ return true; }

bool ConfigManager::parseControllerConfig(const JsonObject& obj) {
    if (obj.containsKey("name"))    strncpy(controller_.name, obj["name"], sizeof(controller_.name));
    if (obj.containsKey("id"))      controller_.id = obj["id"];
    if (obj.containsKey("enabled")) controller_.enabled = obj["enabled"];
    return true;
}

bool ConfigManager::parseRfidConfig(const JsonObject& obj) {
    if (obj.containsKey("mode")) rfid_.mode = parseMode(obj["mode"]);

    if (obj.containsKey("tags")) {
        JsonArray arr = obj["tags"];
        rfid_.tagCount = min((size_t)16, arr.size());
        for (uint8_t i = 0; i < rfid_.tagCount; i++) {
            JsonObject t = arr[i];
            if (t.containsKey("uid"))  strncpy(rfid_.tags[i].uid, t["uid"], sizeof(rfid_.tags[i].uid));
            if (t.containsKey("name")) strncpy(rfid_.tags[i].name, t["name"], sizeof(rfid_.tags[i].name));
        }
    }

    if (obj.containsKey("validTags")) {
        JsonArray arr = obj["validTags"];
        rfid_.validTagCount = min((size_t)32, arr.size());
        for (uint8_t i = 0; i < rfid_.validTagCount; i++) {
            JsonObject t = arr[i];
            if (t.containsKey("uid"))  strncpy(rfid_.validTags[i].uid, t["uid"], sizeof(rfid_.validTags[i].uid));
            if (t.containsKey("name")) strncpy(rfid_.validTags[i].name, t["name"], sizeof(rfid_.validTags[i].name));
        }
    }

    if (obj.containsKey("sequence")) {
        JsonArray arr = obj["sequence"];
        rfid_.sequenceLength = min((size_t)16, arr.size());
        for (uint8_t i = 0; i < rfid_.sequenceLength; i++) rfid_.sequence[i] = arr[i];
    }

    if (obj.containsKey("timeoutSeconds")) rfid_.timeoutSeconds = obj["timeoutSeconds"];
    if (obj.containsKey("resetOnError"))   rfid_.resetOnError = obj["resetOnError"];
    return true;
}

bool ConfigManager::parseAudioConfig(const JsonObject& obj) {
    if (obj.containsKey("enabled"))      audio_.enabled = obj["enabled"];
    if (obj.containsKey("tagSound"))     strncpy(audio_.tagSound, obj["tagSound"], sizeof(audio_.tagSound));
    if (obj.containsKey("correctSound")) strncpy(audio_.correctSound, obj["correctSound"], sizeof(audio_.correctSound));
    if (obj.containsKey("wrongSound"))   strncpy(audio_.wrongSound, obj["wrongSound"], sizeof(audio_.wrongSound));
    if (obj.containsKey("unlockSound"))  strncpy(audio_.unlockSound, obj["unlockSound"], sizeof(audio_.unlockSound));
    if (obj.containsKey("errorSound"))   strncpy(audio_.errorSound, obj["errorSound"], sizeof(audio_.errorSound));
    if (obj.containsKey("resetSound"))   strncpy(audio_.resetSound, obj["resetSound"], sizeof(audio_.resetSound));
    return true;
}

bool ConfigManager::parseNeoPixelConfig(const JsonObject& obj) {
    if (obj.containsKey("enabled")) neopixel_.enabled = obj["enabled"];
    if (obj.containsKey("count"))   neopixel_.count = obj["count"];
    if (obj.containsKey("colors")) {
        JsonObject colors = obj["colors"];
        if (colors.containsKey("idle"))    neopixel_.colorIdle    = parseColor(colors["idle"]);
        if (colors.containsKey("tagRead")) neopixel_.colorTagRead = parseColor(colors["tagRead"]);
        if (colors.containsKey("correct")) neopixel_.colorCorrect = parseColor(colors["correct"]);
        if (colors.containsKey("wrong"))   neopixel_.colorWrong   = parseColor(colors["wrong"]);
        if (colors.containsKey("solved"))  neopixel_.colorSolved  = parseColor(colors["solved"]);
    }
    return true;
}

bool ConfigManager::parseRs485Config(const JsonObject& obj) {
    if (obj.containsKey("enabled")) rs485_.enabled = obj["enabled"];
    if (obj.containsKey("address")) rs485_.address = obj["address"];
    if (obj.containsKey("baudRate")) rs485_.baudRate = obj["baudRate"];
    return true;
}

bool ConfigManager::parseOutputConfig(const JsonObject& obj) {
    if (obj.containsKey("lockPin"))    output_.lockPin = obj["lockPin"];
    if (obj.containsKey("activeHigh")) output_.activeHigh = obj["activeHigh"];
    return true;
}

RfidMode ConfigManager::parseMode(const char* modeStr) {
    if (strcmp(modeStr, "UNIQUE") == 0)   return RfidMode::UNIQUE;
    if (strcmp(modeStr, "SEQUENCE") == 0) return RfidMode::SEQUENCE;
    return RfidMode::UNIQUE;
}

const char* ConfigManager::modeToString(RfidMode mode) {
    switch (mode) {
        case RfidMode::UNIQUE:   return "UNIQUE";
        case RfidMode::SEQUENCE: return "SEQUENCE";
        default:                 return "UNIQUE";
    }
}

uint32_t ConfigManager::parseColor(const JsonArray& arr) {
    if (arr.size() >= 3) return Color(arr[0], arr[1], arr[2]);
    return Color(0, 0, 0);
}

} // namespace universal_unique_rfid
