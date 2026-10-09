#pragma once

#include <M5Cardputer.h>

// ============================================================================
// SCANNER — scan + nmap (simulados)
// ============================================================================
// Lista dispositivos por IP + MAC + hostname.
// Apenas a bomba (192.168.1.100) aparece como "unknown".
// Todos os demais são estações de trabalho da empresa —
// o jogador deduz que o desconhecido é o alvo.
// ============================================================================

class Scanner {
public:
    void scan(void (*print)(const char*)) {
        print("[HACK] Scanning 192.168.1.0/24...");
        delay(2000);
        print("[HACK] 7 hosts up:");
        print("  192.168.1.1    7C:9E:BD:3F:2A:00  gateway");
        print("  192.168.1.10   D4:3C:2E:11:AB:01  gerencia");
        print("  192.168.1.11   D4:3C:2E:11:AB:02  desenvolvimento");
        print("  192.168.1.12   D4:3C:2E:11:AB:03  joe-laptop");
        print("  192.168.1.13   D4:3C:2E:11:AB:04  ralador");
        print("  192.168.1.14   D4:3C:2E:11:AB:05  seguranca");
        print("  192.168.1.100  F4:5C:89:AB:CD:EF  unknown");
        print("");
        print("[HACK] 1 unidentified host — use 'nmap <ip>'");
    }

    void nmap(const char* target, void (*print)(const char*)) {
        if (strlen(target) == 0) {
            print("Usage: nmap <ip>");
            return;
        }
        char buf[64];
        snprintf(buf, sizeof(buf), "[HACK] Scanning %s...", target);
        print(buf);
        delay(1800);

        snprintf(buf, sizeof(buf), "  PORT     STATE  SERVICE");
        print(buf);
        // Todos os hosts tem portas comuns
        print("  80/tcp   open   http");
        print("  443/tcp  open   https");
        print("  22/tcp   open   ssh");

        // Unknown (bomba) tem portas extras
        if (strcmp(target, "192.168.1.100") == 0) {
            print("  8080/tcp open   custom");
            print("  9090/tcp open   custom");
            print("  7070/tcp open   custom");
        }
    }
};
