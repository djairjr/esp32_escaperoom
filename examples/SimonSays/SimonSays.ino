/**
 * EXEMPLO: SIMON SAYS - JOGO GENIUS
 * ------------------------------------------------------
 * Implementação do jogo clássico Simon Says (Genius) para Escape Room.
 * 4 botões coloridos com anéis NeoPixel, cada um com som distinto.
 *
 * CARACTERÍSTICA PRINCIPAL: Sem punição por erro!
 * Se o jogador errar, a mesma sequência é repetida até acertar.
 *
 * HARDWARE: Adafruit PropMaker Feather RP2040 + PCF8574
 * - PCF8574 endereço I2C: 0x20
 * - Botões: P0-P3 (ativos em LOW, INPUT_PULLUP)
 * - NeoPixel: 48 LEDs (4 anéis de 12 LEDs cada)
 * - Fechadura: PIN_EXTERNAL_POWER
 *
 * ÁUDIOS NECESSÁRIOS (LittleFS ou SD):
 * - ready.mp3    : Sistema pronto
 * - begin.mp3    : Início do jogo
 * - red.mp3      : Som do botão vermelho
 * - blue.mp3     : Som do botão azul
 * - yellow.mp3   : Som do botão amarelo
 * - green.mp3    : Som do botão verde
 * - correct.mp3  : Sequência correta
 * - error.mp3    : Erro na sequência
 * - victory.mp3  : Vitória (puzzle resolvido)
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

#include <SwitchInput.h>

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
// CONFIGURAÇÃO
// ============================================================================
#define PCF_ADDRESS         0x20

// Botões (P0-P3 no PCF8574)
#define BUTTON_RED_PIN      0
#define BUTTON_BLUE_PIN     1
#define BUTTON_YELLOW_PIN   2
#define BUTTON_GREEN_PIN    3

// Constantes do jogo
#define BUTTONS             4
#define NUM_LEDS_PER_RING   12
#define TOTAL_LEDS          (NUM_LEDS_PER_RING * BUTTONS)
#define MIN_LEVELS          2
#define MAX_LEVELS          7
#define TIMEOUT             5000UL
#define MAX_INTERVAL        800
#define MIN_INTERVAL        300
#define RESET_HOLD_TIME     2000UL
#define RAINBOW_SPEED       15

// Cores RGB
const uint32_t COLORS[BUTTONS] = {
    Adafruit_NeoPixel::Color(255, 0, 0),     // Vermelho
    Adafruit_NeoPixel::Color(0, 0, 255),     // Azul
    Adafruit_NeoPixel::Color(255, 215, 0),   // Amarelo/Dourado
    Adafruit_NeoPixel::Color(0, 128, 0)      // Verde
};

// Nomes das cores
const char* COLOR_NAMES[BUTTONS] = {"Red", "Blue", "Yellow", "Green"};

// Arquivos de som
const char* COLOR_SOUNDS[BUTTONS] = {"red.mp3", "blue.mp3", "yellow.mp3", "green.mp3"};

// Estados do jogo
enum GameStatus {
    Initialising,
    WaitCommand,
    Overture,
    Running,
    Solved,
    WaitForReset
};

// Enum de cores
enum Color {
    COLOR_RED = 0,
    COLOR_BLUE = 1,
    COLOR_YELLOW = 2,
    COLOR_GREEN = 3
};

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================
GameStatus gameStatus = Initialising;
int gameSequence[MAX_LEVELS];
int currentLevel = MIN_LEVELS;
int gameSpeed = MAX_INTERVAL;
int correctMoves = 0;
bool rainbowActive = false;
uint8_t gHue = 0;
unsigned long lastRainbowUpdate = 0;

// Callbacks para botões
void onButtonPressed(pinid_t pin, bool held);

// ============================================================================
// CLASSE SIMON SAYS PUZZLE
// ============================================================================
class SimonSaysPuzzle : public PuzzleBase {
public:
    SimonSaysPuzzle() : PuzzleBase(20) {}  // ID 20 na rede RS485

    void setupHardware() override {
        Serial.println(F("\n========================================"));
        Serial.println(F("  Simon Says - Jogo Genius"));
        Serial.println(F("========================================"));

        // Inicializa SwitchInput
        if (hw.hasPortExpander()) {
            Serial.println(F("[SIMON] Inicializando SwitchInput..."));
            switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);

            // Adiciona botões com callback
            switches.addSwitch(BUTTON_RED_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_BLUE_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_YELLOW_PIN, onButtonPressed, NO_REPEAT, false);
            switches.addSwitch(BUTTON_GREEN_PIN, onButtonPressed, NO_REPEAT, false);

            Serial.println(F("[SIMON] 4 botoes configurados"));
        } else {
            Serial.println(F("[SIMON] ERRO: PCF8574 nao detectado!"));
        }

        // Inicializa NeoPixel externos
        hw.pixelsExternal().updateLength(TOTAL_LEDS);
        hw.pixelsExternal().begin();
        hw.pixelsExternal().setBrightness(70);
        hw.pixelsExternal().clear();
        hw.pixelsExternal().show();

        // Configura fechadura
        hw.pinMode(PIN_EXTERNAL_POWER, OUTPUT);
        hw.digitalWrite(PIN_EXTERNAL_POWER, LOW);  // Trancado

        // Gera sequência do jogo
        generateGameSequence();

        // Toca som de pronto
        hw.sound().playOnce("ready.mp3");

        gameStatus = WaitCommand;
        Serial.println(F("[SIMON] Pronto - pressione qualquer botao para iniciar"));
    }

    void update() override {
        // Processa audio
        hw.sound().loop();

        // Processa inputs
        switches.runLoop();

        // Máquina de estados
        switch (gameStatus) {
            case WaitCommand:
                handleWaitCommand();
                break;
            case Overture:
                handleOverture();
                break;
            case Running:
                handleRunning();
                break;
            case Solved:
                handleSolved();
                break;
            case WaitForReset:
                handleWaitForReset();
                break;
            default:
                break;
        }

        // Atualiza animação rainbow se ativa
        if (rainbowActive) {
            updateRainbow();
        }
    }

private:
    // ------------------------------------------------------------------------
    // MANIPULADORES DE ESTADO
    // ------------------------------------------------------------------------
    void handleWaitCommand() {
        hw.digitalWrite(PIN_EXTERNAL_POWER, LOW);  // Garante trancado

        // Aguarda qualquer botão
        if (switches.isSwitchPressed(BUTTON_RED_PIN) ||
            switches.isSwitchPressed(BUTTON_BLUE_PIN) ||
            switches.isSwitchPressed(BUTTON_YELLOW_PIN) ||
            switches.isSwitchPressed(BUTTON_GREEN_PIN)) {
            delay(300);
            gameStatus = Overture;
        }
    }

    void handleOverture() {
        Serial.println(F("[SIMON] Iniciando jogo..."));
        hw.sound().playOnce("begin.mp3");

        // Acende todos os anéis
        for (int i = 0; i < TOTAL_LEDS; i++) {
            int ring = i / NUM_LEDS_PER_RING;
            hw.pixelsExternal().setPixelColor(i, COLORS[ring]);
        }
        hw.pixelsExternal().show();
        delay(1000);

        // Apaga todos
        hw.pixelsExternal().clear();
        hw.pixelsExternal().show();
        delay(1000);

        gameStatus = Running;
    }

    void handleRunning() {
        hw.digitalWrite(PIN_EXTERNAL_POWER, LOW);  // Trancado
        playGame();
    }

    void handleSolved() {
        Serial.println(F("[SIMON] PUZZLE RESOLVIDO! Caixa aberta."));
        hw.sound().playOnce("victory.mp3");
        startRainbow();
        hw.digitalWrite(PIN_EXTERNAL_POWER, HIGH);  // Abre fechadura

        delay(5000);  // Aguarda estabilização
        gameStatus = WaitForReset;
    }

    void handleWaitForReset() {
        static unsigned long pressStart = 0;
        static bool wasPressed = false;

        bool anyPressed = switches.isSwitchPressed(BUTTON_RED_PIN) ||
                         switches.isSwitchPressed(BUTTON_BLUE_PIN) ||
                         switches.isSwitchPressed(BUTTON_YELLOW_PIN) ||
                         switches.isSwitchPressed(BUTTON_GREEN_PIN);

        if (anyPressed) {
            if (!wasPressed) {
                pressStart = millis();
                wasPressed = true;
            }
            if (millis() - pressStart >= RESET_HOLD_TIME) {
                stopRainbow();
                resetAnimation();
                resetGame();
                wasPressed = false;
            }
        } else {
            wasPressed = false;
            pressStart = 0;
        }
    }

    // ------------------------------------------------------------------------
    // LÓGICA DO JOGO
    // ------------------------------------------------------------------------
    void playGame() {
        while (gameStatus == Running) {
            hw.sound().loop();

            // Mostra sequência
            showSequence();
            delay(500);

            // Aguarda jogador repetir
            int result = checkPlayerInput();

            // Se errou, repete mesma sequência (sem punição!)
            while (result == 0 && gameStatus == Running) {
                hw.sound().playOnce("error.mp3");
                delay(1000);
                showSequence();
                delay(500);
                result = checkPlayerInput();
            }

            // Se acertou completamente
            if (result == 1) {
                hw.sound().playOnce("correct.mp3");
                delay(500);

                if (currentLevel < MAX_LEVELS) {
                    currentLevel++;
                    gameSpeed = calculateSpeed(currentLevel);
                }

                if (currentLevel == MAX_LEVELS) {
                    gameStatus = Solved;
                }
            }
        }
    }

    void showSequence() {
        Serial.print(F("[SIMON] Sequencia: "));
        for (int i = 0; i < currentLevel; i++) {
            Serial.print(COLOR_NAMES[gameSequence[i]]);
            Serial.print(F(" "));
            flashRing(gameSequence[i], gameSpeed);
        }
        Serial.println();
        delay(gameSpeed);
    }

    int checkPlayerInput() {
        for (int i = 0; i < currentLevel; i++) {
            int button = -1;
            unsigned long startTime = millis();

            // Aguarda botão ou timeout
            while (button == -1) {
                hw.sound().loop();
                switches.runLoop();

                if (switches.isSwitchPressed(BUTTON_RED_PIN)) {
                    button = COLOR_RED;
                } else if (switches.isSwitchPressed(BUTTON_BLUE_PIN)) {
                    button = COLOR_BLUE;
                } else if (switches.isSwitchPressed(BUTTON_YELLOW_PIN)) {
                    button = COLOR_YELLOW;
                } else if (switches.isSwitchPressed(BUTTON_GREEN_PIN)) {
                    button = COLOR_GREEN;
                }

                // Timeout
                if (millis() - startTime >= TIMEOUT) {
                    return 0;
                }
            }

            // Feedback visual/sonoro
            flashRing(button, 500);

            // Verifica se acertou
            if (button != gameSequence[i]) {
                return 0;  // Errou
            }

            correctMoves++;
        }

        correctMoves = 0;
        return 1;  // Acertou tudo
    }

    // ------------------------------------------------------------------------
    // EFEITOS VISUAIS
    // ------------------------------------------------------------------------
    void flashRing(int colorIdx, int duration) {
        // Acende anel
        for (int i = colorIdx * NUM_LEDS_PER_RING;
             i < (colorIdx + 1) * NUM_LEDS_PER_RING;
             i++) {
            hw.pixelsExternal().setPixelColor(i, COLORS[colorIdx]);
        }
        hw.pixelsExternal().show();

        // Toca som
        hw.sound().playOnce(COLOR_SOUNDS[colorIdx]);
        delay(duration);

        // Apaga anel
        for (int i = colorIdx * NUM_LEDS_PER_RING;
             i < (colorIdx + 1) * NUM_LEDS_PER_RING;
             i++) {
            hw.pixelsExternal().setPixelColor(i, 0);
        }
        hw.pixelsExternal().show();
        delay(duration);
    }

    void startRainbow() {
        rainbowActive = true;
        gHue = 0;
        lastRainbowUpdate = millis();
        Serial.println(F("[SIMON] Rainbow iniciado"));
    }

    void stopRainbow() {
        rainbowActive = false;
        hw.pixelsExternal().clear();
        hw.pixelsExternal().show();
    }

    void updateRainbow() {
        if (!rainbowActive) return;

        unsigned long now = millis();
        if (now - lastRainbowUpdate < RAINBOW_SPEED) return;

        lastRainbowUpdate = now;

        // Apaga todos
        hw.pixelsExternal().clear();

        // Para cada anel, acende 1 LED com cor rainbow
        for (int ring = 0; ring < BUTTONS; ring++) {
            int ledPos = (ring * NUM_LEDS_PER_RING) + (gHue % NUM_LEDS_PER_RING);

            // Converte HSV para RGB
            uint8_t r, g, b;
            hsvToRgb(gHue + (ring * 64), 255, 255, r, g, b);
            hw.pixelsExternal().setPixelColor(ledPos, r, g, b);
        }

        hw.pixelsExternal().show();
        gHue++;
    }

    void resetAnimation() {
        // Ciclo individual
        for (int cycle = 0; cycle < BUTTONS; cycle++) {
            for (int i = 0; i < NUM_LEDS_PER_RING; i++) {
                int ledIdx = (cycle * NUM_LEDS_PER_RING) + i;
                hw.pixelsExternal().setPixelColor(ledIdx, COLORS[cycle]);
                hw.pixelsExternal().show();
                delay(50);
                hw.pixelsExternal().setPixelColor(ledIdx, 0);
                hw.pixelsExternal().show();
                delay(50);
            }
        }

        // Duas piscadas
        for (int p = 0; p < 2; p++) {
            for (int i = 0; i < TOTAL_LEDS; i++) {
                int ring = i / NUM_LEDS_PER_RING;
                hw.pixelsExternal().setPixelColor(i, COLORS[ring]);
            }
            hw.pixelsExternal().show();
            delay(150);
            hw.pixelsExternal().clear();
            hw.pixelsExternal().show();
            delay(150);
        }
    }

    // ------------------------------------------------------------------------
    // GERENCIAMENTO DO JOGO
    // ------------------------------------------------------------------------
    void resetGame() {
        currentLevel = MIN_LEVELS;
        gameSpeed = MAX_INTERVAL;
        generateGameSequence();
        delay(1000);
        hw.sound().playOnce("ready.mp3");
        delay(500);
        gameStatus = WaitCommand;
        Serial.println(F("[SIMON] Jogo resetado"));
    }

    void generateGameSequence() {
        // Sequência base
        int baseSequence[BUTTONS] = {COLOR_RED, COLOR_BLUE, COLOR_YELLOW, COLOR_GREEN};

        // Preenche repetição
        for (int i = 0; i < MAX_LEVELS; i++) {
            gameSequence[i] = baseSequence[i % BUTTONS];
        }

        // Evita repetições consecutivas
        for (int i = 1; i < MAX_LEVELS; i++) {
            if (gameSequence[i] == gameSequence[i - 1]) {
                for (int j = i + 1; j < MAX_LEVELS; j++) {
                    if (gameSequence[j] != gameSequence[i - 1]) {
                        int temp = gameSequence[i];
                        gameSequence[i] = gameSequence[j];
                        gameSequence[j] = temp;
                        break;
                    }
                }
            }
        }

        // Embaralha adicional
        bubbleUnsort(gameSequence, MAX_LEVELS);

        // Verificação final
        for (int i = 1; i < MAX_LEVELS; i++) {
            if (gameSequence[i] == gameSequence[i - 1]) {
                for (int j = i + 1; j < MAX_LEVELS; j++) {
                    if (gameSequence[j] != gameSequence[i - 1]) {
                        int temp = gameSequence[i];
                        gameSequence[i] = gameSequence[j];
                        gameSequence[j] = temp;
                        break;
                    }
                }
            }
        }
    }

    int calculateSpeed(int level) {
        int levelsProgress = MAX_LEVELS - MIN_LEVELS;
        int totalReduction = MAX_INTERVAL - MIN_INTERVAL;
        float reductionPerLevel = (float)totalReduction / levelsProgress;
        int speed = MAX_INTERVAL - (int)((level - MIN_LEVELS) * reductionPerLevel);
        return constrain(speed, MIN_INTERVAL, MAX_INTERVAL);
    }

    // ------------------------------------------------------------------------
    // UTILITÁRIOS
    // ------------------------------------------------------------------------
    void bubbleUnsort(int* list, int elem) {
        for (int a = elem - 1; a > 0; a--) {
            int r = random(a + 1);
            if (r != a) {
                int temp = list[a];
                list[a] = list[r];
                list[r] = temp;
            }
        }
    }

    void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
        uint8_t region = h / 43;
        uint8_t remainder = (h - (region * 43)) * 6;
        uint8_t p = (v * (255 - s)) >> 8;
        uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
        uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

        switch (region) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }
};

// ============================================================================
// CALLBACKS GLOBAIS
// ============================================================================
void onButtonPressed(pinid_t pin, bool held) {
    // Callback necessário para SwitchInput
    // Lógica principal está no checkPlayerInput
}

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(20);
SimonSaysPuzzle puzzle;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    hw.begin();
    hw.addPcf8574(PCF_ADDRESS);  // Inicializa PCF8574 (8 pinos, usando P0-P3 para 4 botoes)

    master.addPuzzle(&puzzle);
    master.begin();
      initHA();
  puzzle.run();
}

void loop() {
  loopHA();
    master.update();
}
