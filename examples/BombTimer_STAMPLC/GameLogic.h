#pragma once

#include <M5StamPLC.h>
#include <Preferences.h>
#include "Pins.h"

// ============================================================================
// GAME LOGIC — Timer, Wires, Detonation, Defusal, Menu, RTC Alarm
// ============================================================================

class GameLogic {
public:
    // ─── Main states ──────────────────────────────────────────────────
    enum State : uint8_t {
        STATE_IDLE       = 0,
        STATE_RUNNING    = 1,
        STATE_DEFUSED    = 2,
        STATE_DETONATED  = 3,
        STATE_MENU       = 4,
        STATE_ALARM      = 5
    };

    // ─── Menu ─────────────────────────────────────────────────────────
    enum MenuItem : uint8_t {
        MENU_COUNTDOWN     = 0,  // 5-5999s
        MENU_DEFUSE_MODE   = 1,  // CODE / WIRE / ORDER
        MENU_DEFUSE_CODE   = 2,  // 0000-9999
        MENU_DEFUSE_ORDER  = 3,  // wire order (e.g. 1234)
        MENU_DET_WIRE      = 4,  // RAND / 1-4 / NONE
        MENU_SPEED_WIRE    = 5,  // RAND / 1-4 / NONE
        MENU_PAUSE_WIRE    = 6,  // RAND / 1-4 / NONE
        MENU_BRIGHTNESS    = 7,  // 0-15
        MENU_TICK          = 8,  // ON / OFF
        MENU_RFID_ENABLE   = 9,  // ON / OFF
        MENU_RFID_DEFU     = 10, // learn defuse tag
        MENU_RFID_DETO     = 11, // learn detonate tag
        MENU_DET_TRIGGER   = 12, // 1-10s (TDET)
        MENU_DEFUSE_TRIGGER= 13, // 1-10s (TDEF)
        MENU_ALARM_H       = 14, // 0-23
        MENU_ALARM_M       = 15, // 0-59
        MENU_RFID_CLR_DEFU = 16, // clear defuse tags
        MENU_RFID_CLR_DETO = 17, // clear detonate tags
        MENU_COUNT         = 18
    };

    // ─── Defuse modes ─────────────────────────────────────────────────
    enum DefuseMode : uint8_t {
        DEFUSE_CODE   = 0,
        DEFUSE_WIRE   = 1,
        DEFUSE_ORDER  = 2
    };

    // ─── Configuration ───────────────────────────────────────────────
    struct Config {
        int countdownSeconds = 180;
        DefuseMode defuseMode = DEFUSE_CODE;
        int defuseCode = 1234;
        int defuseOrder = 1234;  // 4 digits, 1-4 each
        int detWire = 5;         // 5=NONE (default: no function)
        int defuseWire = 5;      // 5=NONE
        int speedupWire = 5;     // 5=NONE
        int pauseWire = 5;       // 5=NONE
        int brightness = 15;
        bool tickEnabled = true;
        bool rfidEnabled = false;
        int alarmHour = 0;
        int alarmMin = 0;
        bool alarmEnabled = false;
        int maxCodeAttempts = 3;
        int detTriggerSec = 5;     // time in s the detonation relays stay active
        int defuseTriggerSec = 5;  // time in s the defuse relays stay active

        // RFID tags (stored as uint32_t = bytes 0-3 of the UID)
        uint32_t defuseTags[4] = {0};
        uint8_t defuseTagCount = 0;
        uint32_t detonateTags[4] = {0};
        uint8_t detonateTagCount = 0;
    };

    Config config;

    void begin() {
        loadConfig();
        resetGame();
        state_ = STATE_IDLE;
    }

    void resetGame() {
        for (int i = 0; i < 4; i++) {
            wireCut_[i] = false;
            wireCutState_[i] = false;
        }
        wiresCut_ = 0;
        wireOrderIndex_ = 0;
        currentCodeAttempt_ = 0;
        doubleSpeed_ = false;
        pauseActive_ = false;
        pauseUntil_ = 0;
        countdownActive_ = false;
    }

    // ─── State ────────────────────────────────────────────────────────
    State getState() { return state_; }
    void setState(State s) { state_ = s; }

    // ─── Countdown getter/setter ───────────────────────────────────────
    int getCountdownSeconds() { return config.countdownSeconds; }
    void setCountdownSeconds(int s) {
        if (s < 5) s = 5;
        if (s > 5999) s = 5999;
        config.countdownSeconds = s;
    }

    // ─── Menu navigation ─────────────────────────────────────────────
    MenuItem currentMenuItem = MENU_COUNTDOWN;
    int currentEditValue = 0;
    bool inSubEdit = false;
    bool wiresSorted = false;

    // ─── Timer ────────────────────────────────────────────────────────
    void startTimer() {
        state_ = STATE_RUNNING;
        startTime_ = millis();
        countdownActive_ = true;
        doubleSpeed_ = false;
        pauseActive_ = false;
        currentCodeAttempt_ = 0;
        assignRandomWires();
        // Reads the wires' initial state: only processes a cut if it saw them connected first
        for (int i = 0; i < 4; i++) {
            wireSeenConnected_[i] = false;
        }
    }

    void stopTimer() { countdownActive_ = false; }

    // Alias for compatibility with existing code that calls resetWires
    void resetWires() { resetGame(); }

    void toggleDoubleSpeed() { doubleSpeed_ = !doubleSpeed_; }
    bool isDoubleSpeed() { return doubleSpeed_; }

    void setPause(unsigned long durationMs) {
        pauseActive_ = true;
        pauseUntil_ = millis() + durationMs;
    }

    bool isPaused() { return pauseActive_; }

    int getRemainingSeconds() {
        if (pauseActive_) {
            if (millis() >= pauseUntil_) {
                pauseActive_ = false;
                // Adjust start time to compensate for pause
                startTime_ += (pauseUntil_ - millis()) + durationOffset_;
            }
            return lastRemainingBeforePause_;
        }
        if (!countdownActive_) return config.countdownSeconds;
        unsigned long elapsed = millis() - startTime_;
        int rate = doubleSpeed_ ? 2 : 1;
        int consumed = (elapsed * rate) / 1000;
        int remaining = config.countdownSeconds - consumed;
        if (remaining < 0) remaining = 0;
        lastRemainingBeforePause_ = remaining;
        return remaining;
    }

    bool tickDue() {
        int r = getRemainingSeconds();
        if (r != lastTickSecond_) {
            lastTickSecond_ = r;
            return true;
        }
        return false;
    }

    // ─── Wires ────────────────────────────────────────────────────────
    struct WireConfig {
        uint8_t detWire;       // 0-3 (index), 4=RAND
        uint8_t defuseWire;    // 0-3 (index), 4=RAND, 5=CODE, 6=ORDER
        uint8_t speedupWire;   // 0-3, 4=RAND, 5=NONE
        uint8_t pauseWire;     // 0-3, 4=RAND, 5=NONE
    };

    WireConfig wires;

    void assignRandomWires() {
        // Draws the wire pins based on the config
        uint8_t wires_assigned[4] = {0};
        int assigned_count = 0;

        auto assignWire = [&](int cfg, uint8_t& out) {
            if (cfg == 0) { // RAND
                uint8_t w;
                do {
                    w = random(0, 4);
                } while (wires_assigned[w]);
                wires_assigned[w] = 1;
                out = w;
            } else if (cfg >= 1 && cfg <= 4) {
                out = cfg - 1;
            } else {
                out = 5; // NONE
            }
        };

        assignWire(config.detWire, wires.detWire);
        assignWire(config.defuseWire, wires.defuseWire);
        assignWire(config.speedupWire, wires.speedupWire);
        assignWire(config.pauseWire, wires.pauseWire);

        // Defuse wire: depends on the mode
        if (config.defuseMode == DEFUSE_ORDER) {
            wires.defuseWire = 6; // ORDER
            setDefuseOrder(config.defuseOrder);
        }
    }

    // Wires state
    bool wireCut_[4] = {false};
    bool wireCutState_[4] = {false};
    bool wireSeenConnected_[4] = {false};
    int wiresCut_ = 0;

    void setWireCut(int idx, bool cut) {
        if (idx < 0 || idx > 3) return;
        wireCutState_[idx] = cut;
        if (cut && !wireCut_[idx]) {
            wireCut_[idx] = true;
            wiresCut_++;
        }
    }

    bool isWireCut(int idx) {
        if (idx < 0 || idx > 3) return false;
        return wireCutState_[idx];
    }

    // Alias for compatibility with code that calls isWireCutState
    bool isWireCutState(int idx) {
        return isWireCut(idx);
    }

    bool isWirePermanentCut(int idx) {
        if (idx < 0 || idx > 3) return false;
        return wireCut_[idx];
    }

    int getWiresCut() { return wiresCut_; }

    // ─── Wire conflict detection ──────────────────────────────────────
// key: which config is being adjusted (1=DET, 2=SPEEDUP, 3=PAUSE, 4=DEFUSE)
// value: new wire value (0=RAND, 1-4=wire, 5=NONE)
// returns true if there is a conflict (wire already used by another function)
    bool wireConflict(int key, int value) {
        if (value == 0 || value == 5) return false; // RAND and NONE never conflict
        // Checks each of the other functions
        for (int k = 1; k <= 4; k++) {
            if (k == key) continue;
            int otherVal = 0;
            switch (k) {
                case 1: otherVal = config.detWire; break;
                case 2: otherVal = config.speedupWire; break;
                case 3: otherVal = config.pauseWire; break;
                case 4: otherVal = config.defuseWire; break;
                default: break;
            }
            if (otherVal == value) return true;
        }
        return false;
    }

    // ─── ORDER defusal ────────────────────────────────────────────────
    uint8_t defuseOrderSeq[4];
    uint8_t wireOrderIndex_ = 0;

    void setDefuseOrder(int order) {
        defuseOrderSeq[0] = (order / 1000) - 1;
        defuseOrderSeq[1] = ((order % 1000) / 100) - 1;
        defuseOrderSeq[2] = ((order % 100) / 10) - 1;
        defuseOrderSeq[3] = ((order % 10)) - 1;
        wireOrderIndex_ = 0;
    }

    bool checkOrderWire(uint8_t wireIdx) {
        if (wireOrderIndex_ >= 4) return false;
        if (wireIdx == defuseOrderSeq[wireOrderIndex_]) {
            wireOrderIndex_++;
            return true;
        }
        return false; // wrong order → detonate
    }

    uint8_t getOrderIndex() { return wireOrderIndex_; }

    // ─── Code ─────────────────────────────────────────────────────────
    int currentCodeAttempt_ = 0;

    bool checkDefuseCode(int code) {
        if (code == config.defuseCode) return true;
        currentCodeAttempt_++;
        return false;
    }

    bool isCodeExceeded() {
        return currentCodeAttempt_ >= config.maxCodeAttempts;
    }

    int getCodeAttempts() { return currentCodeAttempt_; }

    // ─── Relays ────────────────────────────────────────────────────────
    void triggerDetonate() {
        M5StamPLC.writePlcRelay(0, true);
        M5StamPLC.writePlcRelay(1, true);
        relayTriggeredAt_ = millis();
        relayActive_ = true;
        relayType_ = RELAY_DETONATE;
    }

    void triggerDefuse() {
        M5StamPLC.writePlcRelay(2, true);
        M5StamPLC.writePlcRelay(3, true);
        relayTriggeredAt_ = millis();
        relayActive_ = true;
        relayType_ = RELAY_DEFUSE;
    }

    void clearAllRelays() {
        M5StamPLC.writePlcAllRelay(0);
        relayActive_ = false;
    }

    // Returns true if still within the relay triggering period
    bool isRelayActive() { return relayActive_; }

    // Call in the loop — turns off automatically after the configured time
    void updateRelays() {
        if (!relayActive_) return;
        unsigned long elapsed = millis() - relayTriggeredAt_;
        int threshold = (relayType_ == RELAY_DETONATE)
            ? (config.detTriggerSec * 1000)
            : (config.defuseTriggerSec * 1000);
        if (elapsed >= (unsigned long)threshold) {
            clearAllRelays();
        }
    }

    // ─── Tick ─────────────────────────────────────────────────────────
    bool isTickEnabled() { return config.tickEnabled; }

    // ─── Persistence via NVS ──────────────────────────────────────────
    void saveConfig() {
        Preferences prefs;
        prefs.begin("bomb", false);
        prefs.putInt("ctdn", config.countdownSeconds);
        prefs.putUChar("mode", (uint8_t)config.defuseMode);
        prefs.putInt("code", config.defuseCode);
        prefs.putInt("ordr", config.defuseOrder);
        prefs.putUChar("detW", (uint8_t)config.detWire);
        prefs.putUChar("dffW", (uint8_t)config.defuseWire);
        prefs.putUChar("spdW", (uint8_t)config.speedupWire);
        prefs.putUChar("psW", (uint8_t)config.pauseWire);
        prefs.putUChar("bri", (uint8_t)config.brightness);
        prefs.putBool("tick", config.tickEnabled);
        prefs.putBool("rfid", config.rfidEnabled);
        prefs.putUChar("alrmH", (uint8_t)config.alarmHour);
        prefs.putUChar("alrmM", (uint8_t)config.alarmMin);
        prefs.putBool("alrmE", config.alarmEnabled);
        prefs.putUChar("maxAt", (uint8_t)config.maxCodeAttempts);
        prefs.putUChar("dtTrg", (uint8_t)config.detTriggerSec);
        prefs.putUChar("dfTrg", (uint8_t)config.defuseTriggerSec);
        // RFID tags
        prefs.putUChar("dtCnt", config.detonateTagCount);
        prefs.putUChar("dfCnt", config.defuseTagCount);
        char key[8];
        for (int i = 0; i < 4 && i < config.detonateTagCount; i++) {
            snprintf(key, sizeof(key), "det%02d", i);
            prefs.putUInt(key, config.detonateTags[i]);
        }
        for (int i = 0; i < 4 && i < config.defuseTagCount; i++) {
            snprintf(key, sizeof(key), "def%02d", i);
            prefs.putUInt(key, config.defuseTags[i]);
        }
        prefs.end();
        Serial.printf("[NVS] Config saved: %ds, mode=%d, code=%04d\n",
            config.countdownSeconds, config.defuseMode, config.defuseCode);
    }

    void loadConfig() {
        Preferences prefs;
        prefs.begin("bomb", true);
        config.countdownSeconds = prefs.getInt("ctdn", 180);
        config.defuseMode = (DefuseMode)prefs.getUChar("mode", DEFUSE_CODE);
        config.defuseCode = prefs.getInt("code", 1234);
        config.defuseOrder = prefs.getInt("ordr", 1234);
        config.detWire = (int)prefs.getUChar("detW", 5);
        config.defuseWire = (int)prefs.getUChar("dffW", 5);
        config.speedupWire = (int)prefs.getUChar("spdW", 5);
        config.pauseWire = (int)prefs.getUChar("psW", 5);
        config.brightness = (int)prefs.getUChar("bri", 15);
        config.tickEnabled = prefs.getBool("tick", true);
        config.rfidEnabled = prefs.getBool("rfid", false);
        config.alarmHour = (int)prefs.getUChar("alrmH", 0);
        config.alarmMin = (int)prefs.getUChar("alrmM", 0);
        config.alarmEnabled = prefs.getBool("alrmE", false);
        config.maxCodeAttempts = (int)prefs.getUChar("maxAt", 3);
        config.detTriggerSec = (int)prefs.getUChar("dtTrg", 5);
        config.defuseTriggerSec = (int)prefs.getUChar("dfTrg", 5);
        // RFID tags
        config.detonateTagCount = prefs.getUChar("dtCnt", 0);
        config.defuseTagCount = prefs.getUChar("dfCnt", 0);
        char key[8];
        for (int i = 0; i < config.detonateTagCount && i < 4; i++) {
            snprintf(key, sizeof(key), "det%02d", i);
            config.detonateTags[i] = prefs.getUInt(key, 0);
        }
        for (int i = 0; i < config.defuseTagCount && i < 4; i++) {
            snprintf(key, sizeof(key), "def%02d", i);
            config.defuseTags[i] = prefs.getUInt(key, 0);
        }
        prefs.end();
        Serial.printf("[NVS] Config loaded: %ds, mode=%d, code=%04d, detW=%d, spdW=%d, psW=%d, bri=%d\n",
            config.countdownSeconds, config.defuseMode, config.defuseCode,
            config.detWire, config.speedupWire, config.pauseWire, config.brightness);
    }

    // ─── Alarm ───────────────────────────────────────────────────────
    bool checkAlarm(int hour, int min) {
        if (!config.alarmEnabled) return false;
        return (hour == config.alarmHour && min == config.alarmMin);
    }

private:
    State state_ = STATE_IDLE;
    unsigned long startTime_ = 0;
    bool countdownActive_ = false;
    bool doubleSpeed_ = false;
    bool pauseActive_ = false;
    unsigned long pauseUntil_ = 0;
    unsigned long durationOffset_ = 0;
    int lastRemainingBeforePause_ = 0;
    int lastTickSecond_ = -1;

    // Timed relay release
    unsigned long relayTriggeredAt_ = 0;
    bool relayActive_ = false;
    enum RelayType { RELAY_DETONATE, RELAY_DEFUSE } relayType_ = RELAY_DETONATE;
};
