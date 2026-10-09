/**
 * EXAMPLE: MASTER CONTROLLER (ADMIN) - WITH RELAYS AND TIMER
 * ------------------------------------------------------
 * This module manages the global time, the RS485 network and
 * controls a bank of 8 relays via a PCF8574 expander.
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

using namespace esp32_escaperoom;

// ============================================================================
// HOME ASSISTANT + WIFI (added by the ESP32 port)
// ============================================================================
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);
HASensor haSensorState("state");

void initHA() {
  netConfig.begin();
  if (netConfig.isAPMode()) Serial.println(F("[CFG] AP: http://192.168.4.1"));
  byte mac[6]; WiFi.macAddress(mac);
  haDevice.setUniqueId(mac, sizeof(mac));
  haDevice.setName(netConfig.devName());
  haDevice.setSoftwareVersion("1.0.0");
  haSensorState.setName("State");
  haSensorState.setIcon("mdi:puzzle");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}


// ============================================================================
// SYSTEM SETTINGS
// ============================================================================
#define MASTER_ID 1
#define TOTAL_GAME_TIME 3600 // 60 minutes (Maximum 7200 for 2h rooms)

// Relay Mapping on the PCF8574 (Pins 0 to 7)
#define RELAY_MAIN_DOOR    0
#define RELAY_LIGHTS       1
#define RELAY_SMOKE_GEN    2
#define RELAY_AUX_1        3

// ============================================================================
// MASTER CLASS (Actuator Management)
// ============================================================================
class MasterLogic : public PuzzleBase {
public:
    MasterLogic() : PuzzleBase(0) {} // ID 0 for the Central Controller

    void setupHardware() override {
        // Configures all expander pins as output for the relays
        // Initializes as HIGH (common for relay modules active LOW)
        for(int i=0; i<8; i++) {
            hw.inputOutput()->pinMode(i, OUTPUT);
            hw.inputOutput()->digitalWrite(i, HIGH);
        }
        
        Serial.println("[MASTER] Relay Bank Initialized.");
    }

    void update() override {
        // Example: If the time runs out, turn off the lights and open the door
        // In the LogicManager the time is managed automatically,
        // but here we can react to it.
    }
    
    void openAllDoors() {
        hw.inputOutput()->digitalWrite(RELAY_MAIN_DOOR, LOW); // Triggers relay
        Serial.println("[MASTER] Doors Opened!");
    }
};

// ============================================================================
// ORCHESTRATION
// ============================================================================
LogicManager master(MASTER_ID, true); // true = Defines it as the network MASTER
MasterLogic logic;

void logicTask(void *parameter) {
    while (true) {
        master.update();
        
        // Local end-of-game check
        if (master.getRemainingTime() == 0) {
            logic.openAllDoors();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    // 1. Waits for Serial for Debug
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);
    hw.begin();
    hw.addPcf8575(0x20);  // Initializes the PCF8575 to control the relays

    master.addPuzzle(&logic);
    master.begin();
    logic.run();

    // Starts the global timer (Sync via RS485 automatic to the Slaves)
    master.startTimer(TOTAL_GAME_TIME);

    // Creates the logic task on Core 0
    xTaskCreate(logicTask, "MasterLogic", 8192, NULL, 1, NULL);
}

void loop() {
  loopHA();
    // Simple Administration Console via Serial
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'o') { // 'o' for Open
            logic.openAllDoors();
        }
        if (cmd == 'r') { // 'r' for Reset
            master.startTimer(TOTAL_GAME_TIME);
            Serial.println("[ADMIN] Time Reset.");
        }
    }
    delay(10);
}
