/**
 * EXEMPLO: MULTI PUZZLE PIXELS
 * ------------------------------------------------------
 * Demonstra o uso de múltiplos puzzles na mesma placa PropMaker,
 * cada um com sua faixa de endereços NeoPixel definida.
 *
 * Configuração:
 * - Puzzle 1 (PuzzleOne): LEDs 0-9 (10 LEDs) - Efeito fade arco-íris
 * - Puzzle 2 (PuzzleTwo): LEDs 11-18 (8 LEDs) - Efeito chase sequencial
 * - Puzzle 3 (PuzzleThree): LEDs 20-25 (6 LEDs) - Efeito breathing
 *
 * Os ranges são contíguos: (0,10), (11,8), (20,6)
 * Sem sobreposição, cada puzzle controla apenas sua faixa.
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
// PUZZLE 1: Efeito Fade Arco-íris
// ============================================================================
class PuzzleOne : public PuzzleBase {
    uint8_t hue_ = 0;
    unsigned long lastUpdate_ = 0;

public:
    PuzzleOne() : PuzzleBase(1) {  // ID 1
        // Define faixa de pixels no construtor
        setPixelRange(0, 10);  // LEDs 0-9
    }

    void setupHardware() override {
        Serial.println(F("[PUZZLE 1] Setup - Rainbow Fade"));
        setState(PuzzleState::Idle);
    }

    void update() override {
        // Chama apenas se estiver Running
        if (getState() != PuzzleState::Running) return;

        // Atualiza a cada 50ms
        if (millis() - lastUpdate_ < 50) return;
        lastUpdate_ = millis();

        // Efeito arco-íris rotativo usando índices LOCAIS (0-9)
        for (uint8_t i = 0; i < pixels()->count(); i++) {
            uint8_t pixelHue = hue_ + (i * 25);  // Offset de cor por pixel
            pixels()->setPixel(i, wheel(pixelHue));
        }
        pixels()->show();

        hue_++;  // Avança matiz
    }

    // Helper para gerar cores do arco-íris
    uint32_t wheel(uint8_t wheelPos) {
        wheelPos = 255 - wheelPos;
        if (wheelPos < 85) {
            return pixels()->Color(255 - wheelPos * 3, 0, wheelPos * 3);
        } else if (wheelPos < 170) {
            wheelPos -= 85;
            return pixels()->Color(0, wheelPos * 3, 255 - wheelPos * 3);
        } else {
            wheelPos -= 170;
            return pixels()->Color(wheelPos * 3, 255 - wheelPos * 3, 0);
        }
    }
};

// ============================================================================
// PUZZLE 2: Efeito Chase Sequencial
// ============================================================================
class PuzzleTwo : public PuzzleBase {
    uint8_t position_ = 0;
    uint32_t colors_[3];
    unsigned long lastUpdate_ = 0;

public:
    PuzzleTwo() : PuzzleBase(2) {  // ID 2
        // Define faixa de pixels - contígua ao Puzzle 1
        setPixelRange(11, 8);  // LEDs 11-18
    }

    void setupHardware() override {
        Serial.println(F("[PUZZLE 2] Setup - Chase Sequence"));

        // Cores para o efeito chase
        colors_[0] = pixels()->Color(255, 0, 0);    // Vermelho
        colors_[1] = pixels()->Color(0, 255, 0);    // Verde
        colors_[2] = pixels()->Color(0, 0, 255);    // Azul

        setState(PuzzleState::Idle);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        // Atualiza a cada 100ms
        if (millis() - lastUpdate_ < 100) return;
        lastUpdate_ = millis();

        // Efeito chase: acende 3 pixels por vez, deslocando
        pixels()->clear();

        for (uint8_t i = 0; i < 3; i++) {
            uint8_t pixelIdx = (position_ + i) % pixels()->count();
            pixels()->setPixel(pixelIdx, colors_[i]);
        }
        pixels()->show();

        position_ = (position_ + 1) % pixels()->count();
    }
};

// ============================================================================
// PUZZLE 3: Efeito Breathing
// ============================================================================
class PuzzleThree : public PuzzleBase {
    uint8_t brightness_ = 0;
    int8_t direction_ = 1;
    unsigned long lastUpdate_ = 0;
    uint32_t breatheColor_;

public:
    PuzzleThree() : PuzzleBase(3) {  // ID 3
        // Define faixa de pixels - contígua aos anteriores
        setPixelRange(20, 6);  // LEDs 20-25
    }

    void setupHardware() override {
        Serial.println(F("[PUZZLE 3] Setup - Breathing Effect"));

        // Cor âmbar para o breathing
        breatheColor_ = pixels()->Color(255, 100, 0);

        setState(PuzzleState::Idle);
    }

    void update() override {
        if (getState() != PuzzleState::Running) return;

        // Atualiza a cada 30ms para suavidade
        if (millis() - lastUpdate_ < 30) return;
        lastUpdate_ = millis();

        // Calcula brilho com curva senoidal
        float phase = (float)brightness_ / 255.0f * PI;
        uint8_t intensity = (uint8_t)(sin(phase) * 255);

        // Preenche todos os pixels com intensidade calculada
        uint32_t dimmedColor = dimColor(breatheColor_, intensity);
        pixels()->setAll(dimmedColor);
        pixels()->show();

        // Atualiza brightness para próximo frame
        brightness_ += direction_ * 5;
        if (brightness_ >= 255 || brightness_ == 0) {
            direction_ *= -1;  // Inverte direção
        }
    }

    // Diminui intensidade de uma cor
    uint32_t dimColor(uint32_t color, uint8_t intensity) {
        uint8_t r = (uint8_t)(color >> 16);
        uint8_t g = (uint8_t)(color >> 8);
        uint8_t b = (uint8_t)color;

        r = (r * intensity) / 255;
        g = (g * intensity) / 255;
        b = (b * intensity) / 255;

        return pixels()->Color(r, g, b);
    }
};

// ============================================================================
// MAIN
// ============================================================================

// Instâncias dos puzzles
PuzzleOne puzzle1;
PuzzleTwo puzzle2;
PuzzleThree puzzle3;

// LogicManager coordena múltiplos puzzles (sem RS485 neste exemplo)
LogicManager master(0, false);

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    Serial.println(F("\n========================================"));
    Serial.println(F("   Multi Puzzle Pixels Demo"));
    Serial.println(F("========================================"));

    // Inicializa hardware
    hw.begin();

    // Adiciona PCF8575 (se houver)
    // hw.addPcf8575(0x20);

    // Registra puzzles no LogicManager
    master.addPuzzle(&puzzle1);
    master.addPuzzle(&puzzle2);
    master.addPuzzle(&puzzle3);

    Serial.println(F("\nConfiguracao de Pixels:"));
    Serial.printf("  Puzzle 1: offset=%d, count=%d (LEDs %d-%d)\n",
                  puzzle1.pixelOffset(), puzzle1.pixelCount(),
                  puzzle1.pixelOffset(), puzzle1.pixelOffset() + puzzle1.pixelCount() - 1);
    Serial.printf("  Puzzle 2: offset=%d, count=%d (LEDs %d-%d)\n",
                  puzzle2.pixelOffset(), puzzle2.pixelCount(),
                  puzzle2.pixelOffset(), puzzle2.pixelOffset() + puzzle2.pixelCount() - 1);
    Serial.printf("  Puzzle 3: offset=%d, count=%d (LEDs %d-%d)\n",
                  puzzle3.pixelOffset(), puzzle3.pixelCount(),
                  puzzle3.pixelOffset(), puzzle3.pixelOffset() + puzzle3.pixelCount() - 1);

    // Inicializa LogicManager (chama begin() em cada puzzle)
    master.begin();

    // Inicia todos os puzzles automaticamente
    puzzle1.run();
    puzzle2.run();
    puzzle3.run();

    Serial.println(F("\nPuzzles iniciados - Efeitos rodando!"));
    Serial.println(F("Cada puzzle controla sua faixa isolada de LEDs."));
}

void loop() {
  loopHA();
    // Atualiza todos os puzzles via LogicManager
    master.update();

    // Pequeno delay para não saturar
    delay(1);
}
