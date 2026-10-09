#pragma once

#include <M5Cardputer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "Config.h"

// ============================================================================
// CONFIG MANAGER — loads/saves config.json in LittleFS
// ============================================================================

struct LoginCfg {
    char user[MAX_USERNAME];
    char pass[MAX_PASSWORD];
};

struct SniffCfg {
    char ip[MAX_IP];
    int defuseCode;
    int channel;
};

struct StageCfg {
    int current;
    bool scanEnabled;
    bool hydraEnabled;
    bool exploitEnabled;
    bool rf433Enabled;
};

struct HACfg {
    char wifiSsid[64];
    char wifiPass[64];
    char broker[MAX_IP];
    uint16_t port;
    char user[32];
    char pass[32];
};

struct WordlistCfg {
    int count;
    char words[MAX_WORDLIST][MAX_WORD_LEN];
};

class ConfigManager {
public:
    LoginCfg  login;
    SniffCfg  sniff;
    StageCfg  stages;
    HACfg     ha;
    WordlistCfg wordlist;
    TargetCfg targets[MAX_TARGETS];
    int targetCount = 0;

    bool begin() {
        if (!LittleFS.begin(false)) {
            Serial.println("[CFG] LittleFS mount failed, formatting...");
            if (!LittleFS.begin(true)) {
                Serial.println("[CFG] LittleFS format FAILED");
                setDefaults();
                return false;
            }
        }
        load();
        return true;
    }

    void setDefaults() {
        strlcpy(login.user, DEFAULT_USER, sizeof(login.user));
        strlcpy(login.pass, DEFAULT_PASS, sizeof(login.pass));
        strlcpy(sniff.ip, "10.0.0.5", sizeof(sniff.ip));
        sniff.defuseCode = 7391;
        sniff.channel = 1;
        stages.current = 0;
        stages.scanEnabled = true;
        stages.hydraEnabled = false;
        stages.exploitEnabled = false;
        stages.rf433Enabled = false;
        ha.wifiSsid[0] = '\0';
        ha.wifiPass[0] = '\0';
        strlcpy(ha.broker, "192.168.1.10", sizeof(ha.broker));
        ha.port = 1883;
        ha.user[0] = '\0';
        ha.pass[0] = '\0';
        targetCount = 0;
        // Default wordlist
        const char* defaults[] = {"kali", "toor", "root", "h4ck3r", "device", "disable"};
        wordlist.count = 6;
        for (int i = 0; i < 6 && i < MAX_WORDLIST; i++) {
            strlcpy(wordlist.words[i], defaults[i], MAX_WORD_LEN);
        }
    }

    bool load() {
        File f = LittleFS.open(CONFIG_FILE, "r");
        if (!f) {
            Serial.println("[CFG] No config.json, using defaults");
            setDefaults();
            save();
            return true;
        }
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, f);
        f.close();
        if (err) {
            Serial.printf("[CFG] JSON parse error: %s\n", err.c_str());
            setDefaults();
            save();
            return false;
        }

        // Login
        strlcpy(login.user, doc["login"]["user"] | DEFAULT_USER, sizeof(login.user));
        strlcpy(login.pass, doc["login"]["password"] | DEFAULT_PASS, sizeof(login.pass));

        // Sniff
        strlcpy(sniff.ip, doc["sniff"]["ip"] | "10.0.0.5", sizeof(sniff.ip));
        sniff.defuseCode = doc["sniff"]["defuse_code"] | 7391;
        sniff.channel = doc["sniff"]["channel"] | 1;

        // Stages
        stages.current = doc["stages"]["current"] | 0;
        stages.scanEnabled = doc["stages"]["scan_enabled"] | true;
        stages.hydraEnabled = doc["stages"]["hydra_enabled"] | false;
        stages.exploitEnabled = doc["stages"]["exploit_enabled"] | false;
        stages.rf433Enabled = doc["stages"]["rf433_enabled"] | false;

        // HA
        strlcpy(ha.wifiSsid, doc["ha"]["wifi_ssid"] | "", sizeof(ha.wifiSsid));
        strlcpy(ha.wifiPass, doc["ha"]["wifi_pass"] | "", sizeof(ha.wifiPass));
        strlcpy(ha.broker, doc["ha"]["broker"] | "192.168.1.100", sizeof(ha.broker));
        ha.port = doc["ha"]["port"] | 1883;
        strlcpy(ha.user, doc["ha"]["user"] | "", sizeof(ha.user));
        strlcpy(ha.pass, doc["ha"]["pass"] | "", sizeof(ha.pass));

        // Targets (hackable devices)
        targetCount = 0;
        JsonArray tgts = doc["targets"].as<JsonArray>();
        for (JsonVariant v : tgts) {
            if (targetCount >= MAX_TARGETS) break;
            strlcpy(targets[targetCount].name, v["name"] | "", sizeof(targets[targetCount].name));
            strlcpy(targets[targetCount].topic, v["topic"] | "", sizeof(targets[targetCount].topic));
            targetCount++;
        }

        // Wordlist
        JsonArray wl = doc["wordlist"].as<JsonArray>();
        wordlist.count = 0;
        for (JsonVariant v : wl) {
            if (wordlist.count >= MAX_WORDLIST) break;
            strlcpy(wordlist.words[wordlist.count], v.as<const char*>(), MAX_WORD_LEN);
            wordlist.count++;
        }
        if (wordlist.count == 0) {
            const char* defaults[] = {"kali", "toor", "root", "h4ck3r", "device", "disable"};
            wordlist.count = 6;
            for (int i = 0; i < 6; i++)
                strlcpy(wordlist.words[i], defaults[i], MAX_WORD_LEN);
        }

        Serial.printf("[CFG] Loaded: user=%s, sniff_ip=%s, code=%d, targets=%d\n",
            login.user, sniff.ip, sniff.defuseCode, targetCount);
        return true;
    }

    bool save() {
        JsonDocument doc;
        doc["login"]["user"] = login.user;
        doc["login"]["password"] = login.pass;
        doc["sniff"]["ip"] = sniff.ip;
        doc["sniff"]["defuse_code"] = sniff.defuseCode;
        doc["sniff"]["channel"] = sniff.channel;
        doc["stages"]["current"] = stages.current;
        doc["stages"]["scan_enabled"] = stages.scanEnabled;
        doc["stages"]["hydra_enabled"] = stages.hydraEnabled;
        doc["stages"]["exploit_enabled"] = stages.exploitEnabled;
        doc["stages"]["rf433_enabled"] = stages.rf433Enabled;
        doc["ha"]["wifi_ssid"] = ha.wifiSsid;
        doc["ha"]["wifi_pass"] = ha.wifiPass;
        doc["ha"]["broker"] = ha.broker;
        doc["ha"]["port"] = ha.port;
        doc["ha"]["user"] = ha.user;
        doc["ha"]["pass"] = ha.pass;

        // Targets
        JsonArray tgts = doc["targets"].to<JsonArray>();
        for (int i = 0; i < targetCount; i++) {
            JsonObject t = tgts.add<JsonObject>();
            t["name"] = targets[i].name;
            t["topic"] = targets[i].topic;
        }

        JsonArray wl = doc["wordlist"].to<JsonArray>();
        for (int i = 0; i < wordlist.count; i++)
            wl.add(wordlist.words[i]);

        File f = LittleFS.open(CONFIG_FILE, FILE_WRITE);
        if (!f) {
            Serial.println("[CFG] FAILED to write config.json");
            return false;
        }
        serializeJson(doc, f);
        f.close();
        Serial.println("[CFG] config.json saved");
        return true;
    }
};
