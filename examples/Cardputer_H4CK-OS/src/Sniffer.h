#pragma once

#include <M5Cardputer.h>
#include "ConfigManager.h"

// ============================================================================
// SNIFFER — Captura configuração do alvo (simulado)
// ============================================================================
// sniff <ip>    → mostra "interceptação" com dados do config
// sniff code    → mostra o código atual
// ============================================================================

extern ConfigManager configManager;

class Sniffer {
public:
    void sniff(const char* ip, void (*print)(const char*)) {
        if (strlen(ip) == 0) {
            print("Usage: sniff <ip>");
            print("       sniff code");
            return;
        }

        char buf[64];

        snprintf(buf, sizeof(buf), "[HACK] Connecting to %s:8080...", ip);
        print(buf);
        delay(1500);

        print("[HACK] Connection established.");
        print("[HACK] Intercepted remote configuration:");
        delay(300);

        snprintf(buf, sizeof(buf), "  → auth_token: %d", configManager.sniff.defuseCode);
        print(buf);
        delay(100);
        snprintf(buf, sizeof(buf), "  → timeout: %ds", configManager.sniff.defuseCode > 0 ? 180 : 300);
        print(buf);
        delay(100);
        print("  → protocol: KEY");
        print("");
        print("[HACK] Use 'sniff code' to display token again.");
    }

    void showCode(void (*print)(const char*)) {
        char buf[48];
        snprintf(buf, sizeof(buf), "[HACK] Token: %d",
            configManager.sniff.defuseCode);
        print(buf);
    }
};
