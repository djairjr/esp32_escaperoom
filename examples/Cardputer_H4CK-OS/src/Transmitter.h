#pragma once

#include <M5Cardputer.h>
#include "Config.h"
#include <RCSwitch.h>

// ============================================================================
// TRANSMITTER — real RF433 via SYN531R on Grove G2
// ============================================================================
// Standardized codes with BombTimer StamPLC (InputManager.h mapRFValue):
//   0x456789 — START  (start countdown)
//   0x56789A — DETONATE  (detonate)
//   0x6789AB — DEFUSE    (defuse)
//   0x789ABC — SPEED     (speed up)
//   0x89ABCD — PAUSE     (pause)
//   0x123456 — PREV  (menu left)
//   0x234567 — NEXT  (menu right)
//   0x345678 — MENU  (menu select)
// ============================================================================

// 24-bit codes (compatible with EV1527/PT2262)
#define RF_START    0x456789UL
#define RF_DETONATE 0x56789AUL
#define RF_DEFUSE   0x6789ABUL
#define RF_SPEED    0x789ABCUL
#define RF_PAUSE    0x89ABCDUL
#define RF_PREV     0x123456UL
#define RF_NEXT     0x234567UL
#define RF_MENU     0x345678UL

class Transmitter {
public:
    void begin() {
        rf_.enableTransmit(PIN_RF433_DATA);
        rf_.setPulseLength(350);
        rf_.setProtocol(1);
        rf_.setRepeatTransmit(5);
        Serial.println("[RF] Transmitter ready on G2");
    }

    void send(unsigned long code, void (*print)(const char*)) {
        char buf[48];
        snprintf(buf, sizeof(buf), "[HACK] Transmitting 0x%06lX...", code);
        print(buf);

        rf_.send(code, 24);

        snprintf(buf, sizeof(buf), "[HACK] Code 0x%06lX sent!", code);
        print(buf);
    }

    void sendStart(void (*print)(const char*)) { send(RF_START, print); }
    void sendDetonate(void (*print)(const char*)) { send(RF_DETONATE, print); }
    void sendDefuse(void (*print)(const char*)) { send(RF_DEFUSE, print); }
    void sendSpeed(void (*print)(const char*)) { send(RF_SPEED, print); }
    void sendPause(void (*print)(const char*)) { send(RF_PAUSE, print); }

private:
    RCSwitch rf_;
};
