/**
 * H4CK-OS — M5Stack Cardputer
 * ==============================
 * Miniature hacker terminal for escape room.
 * Boot → Login → Shell with simulated pentest tools.
 *
 * Display: ST7789V2 240×135, rotation 1
 * UI: LVGL v9.5
 * Prompt: cmd>
 */

// Disables the M5GFX LVGL shim and uses the real LVGL
#define M5GFX_USING_REAL_LVGL 1
#include <lvgl.h>

#include <M5Cardputer.h>
#include <M5GFX.h>
#include <LittleFS.h>
#include <SD.h>
#include <PubSubClient.h>

#include "src/LVGL_Display.h"
#include "src/LVGL_Terminal.h"
#include "src/LVGL_Login.h"
#include "src/nerd_icons.h"

#include "src/Config.h"
#include "src/ConfigManager.h"
#include "src/SoundManager.h"
#include "src/TapScanner.h"
#include "src/Scanner.h"
#include "src/Cracker.h"
#include "src/Exploit.h"
#include "src/Transmitter.h"
#include "src/Sniffer.h"
#include "src/BombStatus.h"
#include "src/HAClient.h"
#include "src/BootScreen.h"

// ============================================================================
// GLOBAL INSTANCES
// ============================================================================
ConfigManager configManager;
SoundManager sound;
TapScanner tapScanner;
Scanner scanner;
Cracker cracker;
Exploit exploit;
Transmitter transmitter;
Sniffer sniffer;
BombStatus bombStatus;
HAClient haClient;
BootScreen bootScreen;
LVGL_Login loginScreen;
LVGL_Terminal scroll;

// ============================================================================
// GLOBAL STATE
// ============================================================================
SystemState systemState = STATE_BOOT;

char cmdLine[MAX_CMD_LINE];
int cmdLineLen = 0;
unsigned long lastKeysMs = 0;

// ============================================================================
// PROTOTYPES
// ============================================================================
void updateBoot();
void updateLogin();
void updateShell();
void processShellCommand(const char* cmd);

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== H4CK-OS BOOT ===");

    M5Cardputer.begin();

    M5Cardputer.Display.setRotation(1);
    M5Cardputer.Display.setBrightness(100);
    M5Cardputer.Display.fillScreen(TFT_BLACK);

    lvgl_init();
    configManager.begin();

    if (!LittleFS.begin()) {
        Serial.println("[LFS] LittleFS mount FAILED — formatting...");
        LittleFS.format();
        LittleFS.begin();
    }

    Serial.println("[LFS] === LittleFS root contents ===");
    File root = LittleFS.open("/");
    if (root && root.isDirectory()) {
        File f = root.openNextFile();
        while (f) {
            Serial.printf("  %s  (%d bytes)%s\n",
                f.name(), f.size(), f.isDirectory() ? " [DIR]" : "");
            f = root.openNextFile();
        }
        root.close();
    }
    Serial.println("[LFS] === end of listing ===");

    SD.begin(SD_CS, SPI, 25000000);
    sound.begin();

    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS, SPI, 25000000))
        Serial.println("[SD] Card mount FAILED");
    else
        Serial.println("[SD] Card OK");

    tapScanner.begin(&sound);
    transmitter.begin();
    haClient.begin();
    bootScreen.begin();
    systemState = STATE_BOOT;
    Serial.println("[BOOT] H4CK-OS initialized");
}

// ============================================================================
// BOOT SCREEN
// ============================================================================

void updateBoot() {
    static bool bootPlayed = false;
    if (!bootPlayed) { bootPlayed = true; bootScreen.play(); }
    loginScreen.begin();
    systemState = STATE_LOGIN;
    Serial.println("[BOOT] Complete -> LOGIN");
}

// ============================================================================
// LOGIN SCREEN (LVGL 10pt)
// ============================================================================

void updateLogin() {
    loginScreen.tick();

    if (millis() - lastKeysMs < 100) return;
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return;

    M5Cardputer.Keyboard.updateKeysState();
    auto& ks = M5Cardputer.Keyboard.keysState();
    lastKeysMs = millis();

    char k = ks.word.empty() ? 0 : ks.word[0];

    if (ks.enter) {
        if (strcmp(loginScreen.getPassword(), configManager.login.pass) == 0) {
            loginScreen.clean();
            systemState = STATE_SHELL;
            scroll.begin();
            cmdLineLen = 0; memset(cmdLine, 0, sizeof(cmdLine));
            Serial.println("[LOGIN] Login OK");
        } else {
            loginScreen.setError(true);
            loginScreen.reset();
            Serial.println("[LOGIN] Login FAILED");
        }
        return;
    }
    if (ks.del) { loginScreen.backspace(); return; }
    if (k >= 32 && k <= 126) loginScreen.appendChar(k);
}

// ============================================================================
// SHELL HELPERS
// ============================================================================

void shellResult(const char* text)  { scroll.result(text); }
void shellError(const char* text)   { scroll.error(text); }
void shellSuccess(const char* text) { scroll.success(text); }
void shellHack(const char* text)    { scroll.hack(text); }

// ============================================================================
// HELP
// ============================================================================

void showHelpFor(const char* topic) {
    if (strcmp(topic, "help") == 0 || strlen(topic) == 0) {
        scroll.result("");
        scroll.hack(ICON_TERMINAL " Commands:");
        scroll.result("scan, nmap, hydra, sniff, exploit, hackcomm, tapscan, invade, status, clear, logout");
        scroll.result("");
        scroll.result(ICON_INFO " Type 'help <command>' for details");
        return;
    }
    if (strcmp(topic, "scan") == 0) {
        scroll.hack("scan -- scan local network");
        scroll.result("Scans for reachable devices."); scroll.result("Usage: scan"); return;
    }
    if (strcmp(topic, "nmap") == 0) {
        scroll.hack("nmap <ip> -- port scan target");
        scroll.result("Scans open ports on target."); scroll.result("Usage: nmap 192.168.1.100"); return;
    }
    if (strcmp(topic, "hydra") == 0) {
        scroll.hack("hydra <ip> -- brute force auth");
        scroll.result("Password cracking on target."); scroll.result("Usage: hydra 192.168.1.100"); return;
    }
    if (strcmp(topic, "sniff") == 0) {
        scroll.hack("sniff <ip> -- capture traffic");
        scroll.result("Intercepts config data."); scroll.result("Usage: sniff 10.0.0.5"); return;
    }
    if (strcmp(topic, "exploit") == 0) {
        scroll.hack("exploit -k <key> -- gen RF payload");
        scroll.result("Generates RF433 code."); scroll.result("Usage: exploit -k 0x7A3F"); return;
    }
    if (strcmp(topic, "hackcomm") == 0) {
        scroll.hack("hackcomm <hex> -- tx RF433");
        scroll.result("Transmits 24-bit hex code.");
        scroll.result("Usage: hackcomm 0x456789"); return;
    }
    if (strcmp(topic, "tapscan") == 0) {
        scroll.hack("tapscan -- intercept lines");
        scroll.result("Lists/plays audio."); scroll.result("Usage: tapscan [name]"); return;
    }
    if (strcmp(topic, "invade") == 0) {
        scroll.hack("invade -- deploy payloads");
        scroll.result("Sends HA signals."); scroll.result("Usage: invade / invade <name>"); return;
    }
    if (strcmp(topic, "status") == 0) {
        scroll.hack("status -- check device");
        scroll.result("Shows bomb timer state."); scroll.result("Usage: status"); return;
    }
    if (strcmp(topic, "clear") == 0) {
        scroll.hack("clear -- wipe terminal"); scroll.result("Usage: clear"); return;
    }
    if (strcmp(topic, "logout") == 0) {
        scroll.hack("logout -- back to login"); scroll.result("Usage: logout"); return;
    }
    char errBuf[64];
    snprintf(errBuf, sizeof(errBuf), "No help for '%s'.", topic);
    shellError(errBuf);
}

// ============================================================================
// SHELL COMMAND DISPATCH
// ============================================================================

void processShellCommand(const char* cmd) {
    char buf[MAX_CMD_LINE];
    strlcpy(buf, cmd, sizeof(buf));

    int len = strlen(buf);
    while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r' || buf[len-1] == ' '))
        buf[--len] = '\0';

    Serial.printf("[SHELL] cmd='%s'\n", buf);

    // help
    if (strncmp(buf, "help", 4) == 0) {
        sound.cmdBeep(); const char* t = buf + 4; while (*t == ' ') t++; showHelpFor(t); return;
    }
    if (strcmp(buf, "clear") == 0) { scroll.clear(); return; }
    if (strcmp(buf, "logout") == 0) {
        scroll.clean();
        systemState = STATE_LOGIN;
        loginScreen.begin();
        return;
    }

    // tapscan
    if (strncmp(buf, "tapscan", 7) == 0) {
        sound.cmdBeep(); const char* a = buf + 7; while (*a == ' ') a++;
        if (strlen(a) == 0) tapScanner.listChannels([](const char* l) { scroll.hack(l); });
        else { tapScanner.playByName(a, [](const char* l) { scroll.hack(l); }); sound.successBeep(); }
        return;
    }
    // scan
    if (strcmp(buf, "scan") == 0) {
        sound.cmdBeep(); scanner.scan([](const char* l) { scroll.hack(l); }); sound.successBeep(); return;
    }
    // nmap
    if (strncmp(buf, "nmap", 4) == 0) {
        sound.cmdBeep(); const char* t = buf + 4; while (*t == ' ') t++;
        scanner.nmap(t, [](const char* l) { scroll.result(l); }); sound.successBeep(); return;
    }
    // hydra
    if (strncmp(buf, "hydra", 5) == 0) {
        sound.cmdBeep(); const char* t = buf + 5; while (*t == ' ') t++;
        cracker.hydra(t, [](const char* l) { scroll.hack(l); }); sound.successBeep(); return;
    }
    // exploit
    if (strncmp(buf, "exploit", 7) == 0) {
        sound.cmdBeep(); const char* a = buf + 7; while (*a == ' ') a++;
        sound.exploitBeep(); exploit.generate(a, [](const char* l) { scroll.hack(l); }); sound.successBeep(); return;
    }
    // hackcomm (RF433) — hex code only
    if (strncmp(buf, "hackcomm", 8) == 0) {
        sound.cmdBeep(); const char* a = buf + 8; while (*a == ' ') a++;
        if (strlen(a) > 0) {
            unsigned long code = strtoul(a, nullptr, 16);
            if (code) {
                sound.transmitBeep();
                transmitter.send(code, [](const char* l) { scroll.hack(l); });
                sound.successBeep();
            } else {
                shellError("Invalid hex code");
            }
        } else {
            scroll.hack(ICON_BROADCAST " hackcomm <hex>");
        }
        return;
    }
    // sniff
    if (strncmp(buf, "sniff", 5) == 0) {
        sound.cmdBeep(); const char* a = buf + 5; while (*a == ' ') a++;
        if (strlen(a) == 0) shellError("Usage: sniff <ip>");
        else sniffer.sniff(a, [](const char* l) { scroll.hack(l); });
        sound.successBeep(); return;
    }
    // invade — no args = list; invade <name> = send TOGGLE
    if (strncmp(buf, "invade", 6) == 0) {
        sound.cmdBeep(); const char* a = buf + 6; while (*a == ' ') a++;
        if (strlen(a) == 0) {
            // invade → list targets
            if (configManager.targetCount == 0) scroll.result("[INVADE] No targets.");
            else {
                scroll.hack("[INVADE] Targets:");
                for (int i = 0; i < configManager.targetCount; i++) {
                    char l[48]; snprintf(l, sizeof(l), "  %s", configManager.targets[i].name); scroll.result(l);
                }
            }
        } else {
            // invade <name> → send TOGGLE
            if (!haClient.isMQTTOK()) shellError("[INVADE] No MQTT connection");
            else if (haClient.invadeTarget(a)) {
                char m[64]; snprintf(m, sizeof(m), "[INVADE] Injecting %s...", a);
                scroll.hack(m); delay(800);
                snprintf(m, sizeof(m), "[INVADE] Signal injected on %s!", a);
                scroll.success(m); haClient.lastPayload(a); sound.successBeep();
            } else { char m[64]; snprintf(m, sizeof(m), "[INVADE] Unknown: %s", a); shellError(m); }
        }
        return;
    }
    // status
    if (strcmp(buf, "status") == 0) { sound.cmdBeep(); bombStatus.fetch([](const char* l) { scroll.result(l); }); sound.successBeep(); return; }

    // Not found
    sound.errorBeep(); char e[80]; snprintf(e, sizeof(e), "Command not found: %s", buf); shellError(e);
}

// ============================================================================
// SHELL -- MAIN LOOP
// ============================================================================

void updateShell() {
    if (millis() - lastKeysMs < 100) return;
    M5Cardputer.update();
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return;

    M5Cardputer.Keyboard.updateKeysState();
    auto& ks = M5Cardputer.Keyboard.keysState();
    lastKeysMs = millis();

    char k = ks.word.empty() ? 0 : ks.word[0];
    bool fnHeld = ks.fn;

    // Scroll UP: Fn + ;
    if (fnHeld && M5Cardputer.Keyboard.isKeyPressed(';')) { scroll.scrollUp(); return; }
    // Scroll DOWN: Fn + .
    if (fnHeld && M5Cardputer.Keyboard.isKeyPressed('.')) { scroll.scrollDown(); return; }

    if (ks.enter) { processShellCommand(scroll.getCmdLine()); scroll.resetCmdLine(); return; }
    if (ks.del)  { scroll.backspace(); return; }
    if (k >= 32 && k <= 126) scroll.appendChar(k);
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
    haClient.loopHA();
    switch (systemState) {
        case STATE_BOOT:  updateBoot();  break;
        case STATE_LOGIN: updateLogin(); break;
        case STATE_SHELL: updateShell(); break;
        default: break;
    }
    lv_timer_handler();
    delay(5);
}
