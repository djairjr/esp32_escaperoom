/**
 * EXEMPLO: MASTER CONTROLLER (ADMIN) - COM RELÉS E CRONÔMETRO
 * ------------------------------------------------------
 * Este módulo gerencia o tempo global, a rede RS485 e 
 * controla um banco de 8 relês via expansor PCF8574.
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

using namespace esp32_escaperoom;

// ============================================================================
// HOME ASSISTANT + WIFI (adicionado pelo port ESP32)
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
  haSensorState.setName("Estado");
  haSensorState.setIcon("mdi:puzzle");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}


// ============================================================================
// CONFIGURAÇÕES DO SISTEMA
// ============================================================================
#define MASTER_ID 1
#define TOTAL_GAME_TIME 3600 // 60 minutos (Máximo 7200 para salas de 2h)

// Mapeamento dos Relês no PCF8574 (Pinos 0 a 7)
#define RELAY_MAIN_DOOR    0
#define RELAY_LIGHTS       1
#define RELAY_SMOKE_GEN    2
#define RELAY_AUX_1        3

// ============================================================================
// CLASSE MASTER (Gerência de Atuadores)
// ============================================================================
class MasterLogic : public PuzzleBase {
public:
    MasterLogic() : PuzzleBase(0) {} // ID 0 para o Controlador Central

    void setupHardware() override {
        // Configura todos os pinos do expansor como saída para os relês
        // Inicializa como HIGH (comum para módulos de relê ativos em LOW)
        for(int i=0; i<8; i++) {
            hw.inputOutput()->pinMode(i, OUTPUT);
            hw.inputOutput()->digitalWrite(i, HIGH);
        }
        
        Serial.println("[MASTER] Banco de Relês Inicializado.");
    }

    void update() override {
        // Exemplo: Se o tempo acabar, desliga as luzes e abre a porta
        // No LogicManager o tempo é gerenciado automaticamente, 
        // mas aqui podemos reagir a ele.
    }
    
    void openAllDoors() {
        hw.inputOutput()->digitalWrite(RELAY_MAIN_DOOR, LOW); // Aciona relê
        Serial.println("[MASTER] Portas Abertas!");
    }
};

// ============================================================================
// ORQUESTRAÇÃO
// ============================================================================
LogicManager master(MASTER_ID, true); // true = Define como MASTER da rede
MasterLogic logic;

void logicTask(void *parameter) {
    while (true) {
        master.update();
        
        // Verificação de fim de jogo local
        if (master.getRemainingTime() == 0) {
            logic.openAllDoors();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    // 1. Aguarda Serial para Debug
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);
    hw.begin();
    hw.addPcf8575(0x20);  // Inicializa PCF8575 para controle dos relês

    master.addPuzzle(&logic);
    master.begin();
    logic.run();

    // Inicia o cronômetro global (Sync via RS485 automático para os Slaves)
    master.startTimer(TOTAL_GAME_TIME);

    // Cria a task de lógica no Core 0
    xTaskCreate(logicTask, "MasterLogic", 8192, NULL, 1, NULL);
}

void loop() {
  loopHA();
    // Console de Administração simples via Serial
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'o') { // 'o' de Open
            logic.openAllDoors();
        }
        if (cmd == 'r') { // 'r' de Reset
            master.startTimer(TOTAL_GAME_TIME);
            Serial.println("[ADMIN] Tempo Resetado.");
        }
    }
    delay(10);
}
