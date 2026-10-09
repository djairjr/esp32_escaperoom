#pragma once

#include <M5Cardputer.h>
#include "Config.h"

// ============================================================================
// TARGET STATUS — Target device status (simulated)
// ============================================================================
// Reads target state — currently simulated, later via real MQTT
// ============================================================================

class BombStatus {
public:
    void begin() {
        state_ = 0;  // IDLE
        time_ = 180;
        locks_ = 0;
        mode_ = 0;   // KEY
    }

    void fetch(void (*print)(const char*)) {
        print("[STATUS] Polling target device...");
        delay(500);
        print("  State: STANDBY");
        print("  Timer: 03:00");
        print("  Layers: 0/4 bypassed");
        print("  Auth:  KEY");
    }

    int state() { return state_; }
    int time() { return time_; }

private:
    int state_;
    int time_;
    int locks_;
    int mode_;
};
