/**
 * EXAMPLE: KEYPAD RFID LOCK - LOCK WITH PASSWORD + RFID
 * --------------------------------------------------------
 * Combines a 4x3 keypad (I2C/PCF8575) + PN532 reader (I2C) to
 * release a lock. Configuration via /lock.json in LittleFS.
 *
 * Ported to ESP32-S3 + Home Assistant (ArduinoHA).
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>
#include <KeyboardManager.h>

using namespace esp32_escaperoom;

// ============================================================================
// AUTHENTICATION MODES
// ============================================================================
enum class AuthMode : uint8_t {
    KEYPAD_ONLY = 0,
    RFID_ONLY = 1,
    KEYPAD_OR_RFID = 2,
    KEYPAD_AND_RFID = 3
};

// ============================================================================
// JSON CONFIGURATION
// ============================================================================
struct RfidTagBin {
    uint8_t bytes[7];
    uint8_t len;
    bool valid;
};

struct LockConfig {
    AuthMode mode = AuthMode::KEYPAD_OR_RFID;
    uint8_t lockPin = 10;
    uint16_t unlockDurationMs = 3000;
    char codes[4][5] = {"1234", "", "", ""};
    uint8_t codeCount = 1;
    RfidTagBin rfidTags[4] = {{{0}, 0, false}, {{0}, 0, false}, {{0}, 0, false}, {{0}, 0, false}};
    uint8_t rfidTagCount = 0;
    char sfxKeypress[24] = "/sfx/beep.mp3";
    char sfxUnlock[24] = "/sfx/magic.mp3";
    char sfxDenied[24] = "/sfx/error.mp3";
    char sfxReset[24] = "/sfx/cancel.mp3";
    uint8_t neoCount = 8;
    uint32_t colorIdle = 0x050514;
    uint32_t colorUnlock = 0x003200;
    uint32_t colorDenied = 0x320000;
    uint32_t colorRfidScan = 0x000032;
} config;

static uint8_t hexStringToBytes(const char* hexStr, uint8_t* bytes, uint8_t maxBytes) {
    uint8_t count = 0;
    const char* p = hexStr;
    while (*p && count < maxBytes) {
        while (*p && !isxdigit(*p)) p++;
        if (!*p) break;
        unsigned int b;
        if (sscanf(p, "%2x", &b) != 1) break;
        bytes[count++] = (uint8_t)b;
        p += 2;
    }
    return count;
}

static AuthMode parseAuthMode(const char* s) {
    if (!s) return AuthMode::KEYPAD_OR_RFID;
    if (!strcmp(s, "KEYPAD_ONLY"))     return AuthMode::KEYPAD_ONLY;
    if (!strcmp(s, "RFID_ONLY"))       return AuthMode::RFID_ONLY;
    if (!strcmp(s, "KEYPAD_AND_RFID")) return AuthMode::KEYPAD_AND_RFID;
    return AuthMode::KEYPAD_OR_RFID;
}

static void loadConfig() {
    if (!LittleFS.exists("/lock.json")) {
        Serial.println(F("[CONFIG] /lock.json does not exist, using defaults"));
        return;
    }
    File f = LittleFS.open("/lock.json", "r");
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) { Serial.printf("[CONFIG] JSON error: %s\n", err.c_str()); return; }

    config.mode = parseAuthMode(doc["mode"] | "");
    config.lockPin = doc["lockPin"] | config.lockPin;
    config.unlockDurationMs = doc["unlockDurationMs"] | config.unlockDurationMs;

    if (doc["codes"].is<JsonArray>()) {
        auto arr = doc["codes"].as<JsonArray>();
        config.codeCount = min((uint8_t)arr.size(), (uint8_t)4);
        for (uint8_t i = 0; i < config.codeCount; i++) {
            strncpy(config.codes[i], arr[i] | "", sizeof(config.codes[i]) - 1);
            config.codes[i][sizeof(config.codes[i]) - 1] = '\0';
        }
    }

    if (doc["rfidTags"].is<JsonArray>()) {
        auto arr = doc["rfidTags"].as<JsonArray>();
        config.rfidTagCount = min((uint8_t)arr.size(), (uint8_t)4);
        for (uint8_t i = 0; i < config.rfidTagCount; i++) {
            const char* hexStr = arr[i] | "";
            RfidTagBin& tag = config.rfidTags[i];
            tag.len = hexStringToBytes(hexStr, tag.bytes, 7);
            tag.valid = (tag.len > 0);
        }
    }

    strncpy(config.sfxKeypress, doc["audio"]["keypress"] | config.sfxKeypress, sizeof(config.sfxKeypress));
    strncpy(config.sfxUnlock,   doc["audio"]["unlock"]   | config.sfxUnlock,   sizeof(config.sfxUnlock));
    strncpy(config.sfxDenied,   doc["audio"]["denied"]   | config.sfxDenied,   sizeof(config.sfxDenied));
    strncpy(config.sfxReset,    doc["audio"]["reset"]    | config.sfxReset,    sizeof(config.sfxReset));

    config.neoCount = doc["neopixel"]["count"] | config.neoCount;
    if (doc["neopixel"]["colorIdle"].is<JsonArray>()) {
        auto a = doc["neopixel"]["colorIdle"].as<JsonArray>();
        config.colorIdle = hw.pixelsExternal().Color(a[0] | 5, a[1] | 5, a[2] | 20);
    }
    if (doc["neopixel"]["colorUnlock"].is<JsonArray>()) {
        auto a = doc["neopixel"]["colorUnlock"].as<JsonArray>();
        config.colorUnlock = hw.pixelsExternal().Color(a[0] | 0, a[1] | 50, a[2] | 0);
    }
    if (doc["neopixel"]["colorDenied"].is<JsonArray>()) {
        auto a = doc["neopixel"]["colorDenied"].as<JsonArray>();
        config.colorDenied = hw.pixelsExternal().Color(a[0] | 50, a[1] | 0, a[2] | 0);
    }
    if (doc["neopixel"]["colorRfidScan"].is<JsonArray>()) {
        auto a = doc["neopixel"]["colorRfidScan"].as<JsonArray>();
        config.colorRfidScan = hw.pixelsExternal().Color(a[0] | 0, a[1] | 0, a[2] | 50);
    }
}

// ============================================================================
// HARDWARE - KEYPAD
// ============================================================================
#define PCF_ADDRESS 0x20
static const char keymap[] PROGMEM = "123456789*0#";
byte rowPins[4] = {3, 2, 1, 0};
byte colPins[3] = {6, 5, 4};

KeyboardLayout keypadLayout(4, 3, keymap);
class KeypadListener : public KeyboardListener {
public:
    char lastKey = 0;
    void keyPressed(char key, bool held) override { if (!held) lastKey = key; }
    void keyReleased(char key) override { lastKey = 0; }
};
KeypadListener keypadListener;
MatrixKeyboardManager keypadManager;

// ============================================================================
// DISPLAY WRAPPER
// ============================================================================
class DisplayWrapper {
public:
  bool checkDisplay() { return hw.hasDisplay(); }
  void printString(const char* str) {
    if (checkDisplay()) { hw.display().clear(); hw.display().printString(str); }
    else Serial.printf("[DISPLAY] %s\n", str);
  }
  void writeDigitAscii(int digit, char c) {
    if (checkDisplay()) hw.display().writeDigitAscii(digit, c);
  }
};
DisplayWrapper display;

// ============================================================================
// PUZZLE
// ============================================================================
String enteredCode = "";
bool rfidAuthenticated = false;
unsigned long unlockUntil = 0;
bool lockOpen = false;

class KeypadRfidLock;
extern KeypadRfidLock puzzle;

class KeypadRfidLock : public PuzzleBase {
public:
    KeypadRfidLock() : PuzzleBase(13), nfc_(nullptr) {}

    void setupHardware() override {
        loadConfig();
        hw.pinMode(config.lockPin, OUTPUT);
        hw.digitalWrite(config.lockPin, HIGH);

        if (config.mode == AuthMode::RFID_ONLY) display.printString("rFId");
        else display.printString("____");

        nfc_ = hw.createNfcReader();
        if (nfc_) Serial.println(F("[RFID] PN532 ready"));
        else Serial.println(F("[RFID] PN532 not detected"));

        setVisualIdle();
        Serial.printf("[LOCK] Mode=%d\n", (int)config.mode);
    }

    void setupHA() {
        haSensorState_.setName("State");      haSensorState_.setIcon("mdi:lock");
        haSensorMode_.setName("Auth Mode");     haSensorMode_.setIcon("mdi:tune");
        haSensorLock_.setName("Lock");     haSensorLock_.setIcon("mdi:door");
        haBtnReset_.setName("Reset");         haBtnReset_.setIcon("mdi:restart");
        haBtnReset_.onCommand(haResetCallback);
        haBtnUnlock_.setName("Unlock");       haBtnUnlock_.setIcon("mdi:lock-open");
        haBtnUnlock_.onCommand(haUnlockCallback);
        publishAllHA();
    }

    void publishAllHA() {
        haSensorState_.setValue(PuzzleBase::stateName(getState()));
        const char* modes[] = {"KEYPAD_ONLY","RFID_ONLY","KEYPAD_OR_RFID","KEYPAD_AND_RFID"};
        haSensorMode_.setValue(modes[(int)config.mode]);
        haSensorLock_.setValue(lockOpen ? "open" : "locked");
    }

    void update() override {
        if (getState() != PuzzleState::Running && getState() != PuzzleState::Idle) return;
        if (getState() == PuzzleState::Idle) setState(PuzzleState::Running);
        taskManager.runLoop();

        if (lockOpen && millis() >= unlockUntil) {
            lockOpen = false;
            hw.digitalWrite(config.lockPin, HIGH);
            Serial.println(F("[LOCK] Lock engaged"));
            display.printString(config.mode == AuthMode::RFID_ONLY ? "rFId" : "____");
            setVisualIdle();
            publishAllHA();
        }

        if (nfc_ && config.mode != AuthMode::KEYPAD_ONLY) pollRfid();

        char key = keypadListener.lastKey;
        if (key != 0) { keypadListener.lastKey = 0; handleKey(key); }
    }

    static void haResetCallback(HAButton*) {
        hw.digitalWrite(config.lockPin, HIGH);
        enteredCode = ""; rfidAuthenticated = false; lockOpen = false;
        puzzle.setState(PuzzleState::Running);
        hw.setLightAll(0); hw.setLight(0, config.colorIdle);
        display.printString("____");
        puzzle.publishAllHA();
    }
    static void haUnlockCallback(HAButton*) {
        hw.digitalWrite(config.lockPin, LOW);
        lockOpen = true; unlockUntil = millis() + config.unlockDurationMs;
        hw.setLightAll(config.colorUnlock);
        display.printString("OPEn");
        puzzle.setState(PuzzleState::Solved);
        puzzle.publishAllHA();
    }

private:
    NfcReader* nfc_;
    unsigned long lastRfidPoll_ = 0;
    uint8_t lastUid_[7] = {0};
    uint8_t lastUidLen_ = 0;

    void pollRfid() {
        if (millis() - lastRfidPoll_ < 300) return;
        lastRfidPoll_ = millis();
        NfcReader::CardData card = nfc_->readCard(200);
        if (card.present) {
            if (card.uidLength != lastUidLen_ || memcmp(card.uid, lastUid_, lastUidLen_) != 0) {
                memcpy(lastUid_, card.uid, card.uidLength);
                lastUidLen_ = card.uidLength;
                handleRfid(card.uid, card.uidLength);
            }
        } else { lastUidLen_ = 0; }
    }

    void handleRfid(uint8_t* uid, uint8_t len) {
        for (uint8_t i = 0; i < config.rfidTagCount; i++) {
            RfidTagBin& tag = config.rfidTags[i];
            if (tag.valid && tag.len == len && memcmp(tag.bytes, uid, len) == 0) {
                Serial.println(F("[RFID] TAG authorized!"));
                onRfidAuthorized();
                return;
            }
        }
        Serial.println(F("[RFID] Unknown TAG"));
    }

    void onRfidAuthorized() {
        if (config.mode == AuthMode::KEYPAD_AND_RFID) {
            rfidAuthenticated = true;
            hw.sound().playOnce(config.sfxKeypress);
            display.printString("CodE");
        } else {
            unlockNow();
        }
    }

    void handleKey(char key) {
        if (key == '*') {
            enteredCode = ""; rfidAuthenticated = false;
            hw.sound().playOnce(config.sfxReset);
            display.printString("____");
            setVisualIdle();
        } else if (key == '#') {
            checkCode();
        } else {
            enteredCode += key;
            hw.sound().playOnce(config.sfxKeypress);
            display.writeDigitAscii(enteredCode.length() - 1, key);
            if (enteredCode.length() >= 4) checkCode();
        }
    }

    void checkCode() {
        if (config.mode == AuthMode::RFID_ONLY) { enteredCode = ""; return; }
        bool valid = false;
        for (uint8_t i = 0; i < config.codeCount; i++) {
            if (enteredCode == config.codes[i]) { valid = true; break; }
        }
        if (valid) {
            if (config.mode == AuthMode::KEYPAD_AND_RFID) {
                if (rfidAuthenticated) unlockNow();
                else { hw.sound().playOnce(config.sfxKeypress); display.printString("rFId"); }
            } else unlockNow();
        } else handleDenied();
        enteredCode = "";
    }

    void unlockNow() {
        if (lockOpen) return;
        lockOpen = true; unlockUntil = millis() + config.unlockDurationMs;
        hw.digitalWrite(config.lockPin, LOW);
        hw.sound().playOnce(config.sfxUnlock);
        display.printString("OPEn");
        hw.setLightAll(config.colorUnlock);
        Serial.printf("[LOCK] OPEN for %d ms\n", config.unlockDurationMs);
        publishAllHA();
    }

    void handleDenied() {
        Serial.println(F("[LOCK] ACCESS DENIED"));
        hw.sound().playOnce(config.sfxDenied);
        display.printString("no  ");
        hw.setLightAll(config.colorDenied);
        delay(800);
        display.printString("____");
        setVisualIdle();
    }

    void setVisualIdle() {
        rfidAuthenticated = false;
        hw.setLightAll(0); hw.setLight(0, config.colorIdle);
    }

    HASensor haSensorState_{"state"};
    HASensor haSensorMode_{"mode"};
    HASensor haSensorLock_{"lock"};
    HAButton haBtnReset_{"reset"};
    HAButton haBtnUnlock_{"unlock"};
};

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================
LogicManager master(13);
KeypadRfidLock puzzle;
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);

void setup() {
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);
    Serial.println(F("========================================"));
    Serial.println(F("  KeypadRfidLock - ESP32-S3"));
    Serial.println(F("========================================"));

    hw.begin();
    hw.addPcf8575(PCF_ADDRESS);

    netConfig.begin();
    if (netConfig.isAPMode()) Serial.println(F("[CFG] AP: http://192.168.4.1"));

    byte mac[6]; WiFi.macAddress(mac);
    haDevice.setUniqueId(mac, sizeof(mac));
    haDevice.setName(netConfig.devName());
    haDevice.setSoftwareVersion("1.0.0");

    puzzle.setupHA();
    haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());

    for (int i = 0; i < 4; i++) keypadLayout.setRowPin(i, rowPins[i]);
    for (int i = 0; i < 3; i++) keypadLayout.setColPin(i, colPins[i]);
    keypadManager.initialise(hw.portExpander(), &keypadLayout, &keypadListener, false);

    master.addPuzzle(&puzzle);
    master.begin();
    puzzle.run();
    Serial.println(F("[LOCK] Ready!"));
}

void loop() {
    haMqtt.loop();
    netConfig.handleAP();
    master.update();
    taskManager.runLoop();
    delay(1);
}