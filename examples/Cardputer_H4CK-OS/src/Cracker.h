#pragma once

#include <M5Cardputer.h>
#include "ConfigManager.h"

// ============================================================================
// CRACKER — hydra (força bruta simulada)
// ============================================================================

extern ConfigManager configManager;

class Cracker {
public:
    Cracker() : cracked_(false), key_(0) {}

    void reset() {
        cracked_ = false;
        key_ = 0;
    }

    bool isCracked() { return cracked_; }
    int key() { return key_; }

    void hydra(const char* target, void (*print)(const char*)) {
        if (strlen(target) == 0) {
            print("Usage: hydra <ip>");
            return;
        }

        if (cracked_) {
            char buf[48];
            snprintf(buf, sizeof(buf), "[HACK] Already cracked! Key: 0x%04X", key_);
            print(buf);
            return;
        }

        print("[HACK] Brute forcing AUTH_SERVICE port 9090...");
        print("");

        char buf[80];
        for (int i = 0; i < configManager.wordlist.count; i++) {
            snprintf(buf, sizeof(buf), "[HACK] Trying password: %s... FAIL",
                configManager.wordlist.words[i]);
            print(buf);
            delay(300 + (rand() % 200));
        }

        // Última sempre acerta
        int last = configManager.wordlist.count - 1;
        if (last < 0) last = 0;
        snprintf(buf, sizeof(buf), "[HACK] Trying password: %s... SUCCESS!",
            configManager.wordlist.words[last]);
        print(buf);

        cracked_ = true;
        key_ = 0x7A3F;
        snprintf(buf, sizeof(buf), "[HACK] Key obtained: 0x%04X", key_);
        print(buf);
    }

private:
    bool cracked_;
    int key_;
};
