/**
 * EXEMPLO: REACTOR CONTROL - PAINEL DE CONTROLE NUCLEAR
 * ----------------------------------------------------------
 * Puzzle de ajuste analógico com potenciômetros, gauges PWM e botões.
 *
 * NARRATIVA:
 * Reator nuclear em estado crítico. Três técnicos operam um painel de controle
 * com 3 faces. Em cada face há uma válvula (potenciômetro) e um gauge (voltímetro).
 * A válvula de uma face NUNCA controla o gauge da mesma face — o mapeamento é
 * aleatório (derangement). Além disso, cada gauge pode responder no sentido
 * normal ou reverso.
 *
 * Se os jogadores falharem na confirmação (botões simultâneos), o painel
 * reconfigura o mapeamento e as orientações, forçando-os a redescobrir tudo.
 *
 * HARDWARE: Adafruit PropMaker Feather RP2040
 * - ADS1115 (I2C 0x48) — 3 canais para potenciômetros
 * - PCA9685 (I2C 0x40) — 3 canais PWM para voltímetros
 * - PCF8575 (I2C 0x20) — 3 botões de confirmação
 * - NeoPixel externo — 3 LEDs para feedback dos botões + barra de status
 * - Trava magnética — pino digital via hw.digitalWrite()
 *
 * ÁUDIOS NECESSÁRIOS (LittleFS ou SD):
 * - ready.mp3       : Puzzle iniciado
 * - alarm.mp3       : Gauge fora da zona verde
 * - stabilizing.mp3 : Gauge entrou na zona verde
 * - warning.mp3     : Todos os gauges na zona (tensão)
 * - button_press.mp3: Botão pressionado
 * - fail.mp3        : Confirmação falhou (tempo esgotado)
 * - reshuffle.mp3   : Reconfiguração após falha
 * - victory.mp3     : Puzzle resolvido
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

#include <TaskManager.h>
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
// CONSTANTES
// ============================================================================
#define PCF_ADDRESS         0x20

// Botões de confirmação (P0-P2 no PCF8575)
#define BUTTON_1_PIN        0
#define BUTTON_2_PIN        1
#define BUTTON_3_PIN        2

// NeoPixel para cada botão (índices no strip externo)
#define BUTTON_1_PIXEL      0
#define BUTTON_2_PIXEL      1
#define BUTTON_3_PIXEL      2

// Pino da trava magnética
#define LOCK_PIN            20

// Configuração do jogo
static constexpr uint8_t NUM_VALVES = 3;
static constexpr uint8_t ADS_CHANNELS[NUM_VALVES] = {0, 1, 2};
static constexpr uint8_t PCA_CHANNELS[NUM_VALVES] = {0, 1, 2};
static constexpr uint8_t BUTTON_PINS[NUM_VALVES] = {BUTTON_1_PIN, BUTTON_2_PIN, BUTTON_3_PIN};
static constexpr uint8_t BUTTON_PIXELS[NUM_VALVES] = {BUTTON_1_PIXEL, BUTTON_2_PIXEL, BUTTON_3_PIXEL};

// Zona verde: targets em escala 0-10, com tolerância
static constexpr float TARGETS[NUM_VALVES] = {7.0f, 5.0f, 3.0f};
static constexpr float TOLERANCE = 0.5f;

// Tempos (ms)
static constexpr unsigned long CONFIRM_WINDOW_MS = 2000UL;
static constexpr unsigned long RESHUFFLE_DURATION_MS = 1500UL;
static constexpr unsigned long EFFECT_INTERVAL_MS = 200UL;

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================
extern volatile bool btnPressed[NUM_VALVES];

// Declarações forward
void onButtonPressed(pinid_t pin, bool held);

// ============================================================================
// CLASSE PUZZLE
// ============================================================================
class ReactorControlPuzzle : public PuzzleBase {
public:
    ReactorControlPuzzle() : PuzzleBase(31) {}  // ID 31 na rede RS485

    void setupHardware() override {
        Serial.println(F("[REACTOR] Setup iniciado"));

        // Inicializa botões via SwitchInput no PCF8575
        if (hw.hasExpander()) {
            Serial.println(F("[REACTOR] Inicializando SwitchInput no PCF8575..."));
            switches.init(hw.io(), SWITCHES_POLL_KEYS_ONLY, true);

            for (uint8_t i = 0; i < NUM_VALVES; i++) {
                switches.addSwitch(BUTTON_PINS[i], onButtonPressed, NO_REPEAT, false);
            }
            Serial.println(F("[REACTOR] SwitchInput OK: 3 botoes"));
        } else {
            Serial.println(F("[REACTOR] ERRO: PCF8575 nao detectado!"));
        }

        // Inicializa trava
        hw.pinMode(LOCK_PIN, OUTPUT);
        hw.digitalWrite(LOCK_PIN, HIGH);  // Trava ativa em HIGH

        // Zera gauges PCA9685
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            hw.setAnalogOutput(PCA_CHANNELS[i], 0.0f);
        }

        // Gera mapeamento inicial
        randomizeMapping();

        // Estado inicial
        setState(PuzzleState::Idle);

        Serial.println(F("[REACTOR] Painel de Controle Nuclear Pronto."));
    }

    void update() override {
        // Processa switch input (botoes)
        switches.runLoop();
        taskManager.runLoop();

        // Atualiza efeitos visuais/sonoros (nao-bloqueante)
        updateEffects();

        // Máquina de estados
        switch (getState()) {
            case PuzzleState::Idle:
                if (buttonPressed(BUTTON_1_PIN)) {
                    startPuzzle();
                }
                break;

            case PuzzleState::Running:
                switch (subState_) {
                    case SubState::Adjusting:
                        readValvesAndWriteGauges();
                        checkZones();
                        processButtonsAdjusting();
                        updateGaugeColors();
                        break;

                    case SubState::Confirming:
                        readValvesAndWriteGauges();
                        checkZones();
                        processButtonsConfirming();
                        updateConfirmationEffect();
                        break;

                    case SubState::Reshuffling:
                        updateReshuffleEffect();
                        break;
                }
                break;

            case PuzzleState::Solved:
                // Aguarda reset manual
                if (buttonPressed(BUTTON_1_PIN)) {
                    resetPuzzle();
                }
                break;

            default:
                break;
        }
    }

    void onNetworkPacket(const Rs485Payload& p) override {
        switch (p.pCommand) {
            case NetCommand::ForceSolve:
                if (getState() == PuzzleState::Running) {
                    Serial.println(F("[REACTOR] Comando remoto: ForceSolve"));
                    handleSolve();
                }
                break;

            case NetCommand::GlobalEvent:
                Serial.println(F("[REACTOR] Comando remoto: GlobalEvent (reset)"));
                resetPuzzle();
                break;

            case NetCommand::StatusReport:
                Serial.printf("[REACTOR] Status: %s, SubState: %d, Zonas: %d/%d\n",
                              stateName(getState()), (int)subState_,
                              zonesActive_, NUM_VALVES);
                break;

            default:
                break;
        }
    }

private:
    enum class SubState { Adjusting, Confirming, Reshuffling };
    SubState subState_ = SubState::Adjusting;

    // Mapeamento aleatório
    uint8_t valveToGauge_[NUM_VALVES];
    bool gaugeReversed_[NUM_VALVES];

    // Estado dos gauges
    bool inZone_[NUM_VALVES] = {false};
    uint8_t zonesActive_ = 0;

    // Confirmação
    unsigned long confirmWindowStart_ = 0;
    bool buttonPressedInWindow_[NUM_VALVES] = {false};
    uint8_t buttonsConfirmed_ = 0;

    // Efeitos
    enum class EffectPhase { None, Alarm, Stabilizing, AllGreen, Victory, Reshuffle };
    EffectPhase effectPhase_ = EffectPhase::None;
    unsigned long effectTimer_ = 0;
    uint8_t effectStep_ = 0;

    // =======================================================================
    // MAPEAMENTO
    // =======================================================================
    void randomizeMapping() {
        // Derangement de 3 elementos: apenas 2 possibilidades
        // [1,2,0] ou [2,0,1]
        uint8_t derangements[2][NUM_VALVES] = {
            {1, 2, 0},
            {2, 0, 1}
        };
        uint8_t choice = random(0, 2);
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            valveToGauge_[i] = derangements[choice][i];
        }

        // Orientação normal/reversa para cada gauge
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            gaugeReversed_[i] = random(0, 2) == 1;
        }

        Serial.println(F("[REACTOR] Mapeamento reconfigurado:"));
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            Serial.printf("  Valvula %d -> Gauge %d (%s)\n",
                          i, valveToGauge_[i],
                          gaugeReversed_[valveToGauge_[i]] ? "REVERSO" : "NORMAL");
        }
    }

    // =======================================================================
    // LEITURA E ESCRITA ANALÓGICA
    // =======================================================================
    void readValvesAndWriteGauges() {
        for (uint8_t valveIdx = 0; valveIdx < NUM_VALVES; valveIdx++) {
            uint8_t gaugeIdx = valveToGauge_[valveIdx];

            // Lê potenciômetro via ADS1115 (0-65535 raw)
            uint16_t raw = hw.readAnalogRaw(ADS_CHANNELS[valveIdx]);

            // Converte para percentual 0.0 - 100.0
            float percent = map((long)raw, 0L, 65535L, 0L, 1000L) / 10.0f;
            percent = constrain(percent, 0.0f, 100.0f);

            // Aplica inversão se necessário
            if (gaugeReversed_[gaugeIdx]) {
                percent = 100.0f - percent;
            }

            // Escreve no gauge via PCA9685
            hw.setAnalogOutput(PCA_CHANNELS[gaugeIdx], percent);
        }
    }

    // =======================================================================
    // VERIFICAÇÃO DE ZONA
    // =======================================================================
    void checkZones() {
        zonesActive_ = 0;
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            float percent = getCurrentGaugePercent(i);
            float value = percent / 10.0f;  // Escala 0-10
            bool wasInZone = inZone_[i];
            inZone_[i] = (fabs(value - TARGETS[i]) <= TOLERANCE);

            if (inZone_[i]) {
                zonesActive_++;
            }

            // Som de transição (somente quando entra na zona)
            if (!wasInZone && inZone_[i] && effectPhase_ != EffectPhase::Victory) {
                hw.sound().playOnce("/stabilizing.mp3");
            }
        }
    }

    float getCurrentGaugePercent(uint8_t gaugeIdx) {
        // Reconstrói o percentual do gauge a partir do potenciômetro que o controla
        uint8_t valveIdx = 255;
        for (uint8_t v = 0; v < NUM_VALVES; v++) {
            if (valveToGauge_[v] == gaugeIdx) {
                valveIdx = v;
                break;
            }
        }
        if (valveIdx == 255) return 0.0f;

        uint16_t raw = hw.readAnalogRaw(ADS_CHANNELS[valveIdx]);
        float percent = map((long)raw, 0L, 65535L, 0L, 1000L) / 10.0f;
        percent = constrain(percent, 0.0f, 100.0f);
        if (gaugeReversed_[gaugeIdx]) {
            percent = 100.0f - percent;
        }
        return percent;
    }

    bool isInZone(uint8_t gaugeIdx) {
        float percent = getCurrentGaugePercent(gaugeIdx);
        float value = percent / 10.0f;
        return fabs(value - TARGETS[gaugeIdx]) <= TOLERANCE;
    }

    // =======================================================================
    // BOTÕES
    // =======================================================================
    void processButtonsAdjusting() {
        // Quando todos os gauges estão na zona, inicia janela de confirmação
        if (zonesActive_ == NUM_VALVES && subState_ == SubState::Adjusting) {
            startConfirmationWindow();
        }
    }

    void processButtonsConfirming() {
        // Verifica se janela de confirmação expirou
        unsigned long elapsed = millis() - confirmWindowStart_;
        if (elapsed >= CONFIRM_WINDOW_MS) {
            // Tempo esgotado
            if (buttonsConfirmed_ < NUM_VALVES) {
                handleConfirmationFail();
            }
            return;
        }

        // Processa botões pressionados na janela
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            if (!buttonPressedInWindow_[i] && buttonPressed(BUTTON_PINS[i])) {
                buttonPressedInWindow_[i] = true;
                buttonsConfirmed_++;
                hw.sound().playOnce("/button_press.mp3");

                // Acende NeoPixel do botão
                hw.setLight(BUTTON_PIXELS[i], Adafruit_NeoPixel::Color(0, 255, 0));

                Serial.printf("[REACTOR] Botao %d pressionado (%d/%d)\n",
                              i + 1, buttonsConfirmed_, NUM_VALVES);
            }
        }

        // Verifica vitória
        if (buttonsConfirmed_ >= NUM_VALVES) {
            handleSolve();
        }
    }

    boolean buttonPressed(int button) {
        int idx = button - BUTTON_1_PIN;
        if (idx < 0 || idx >= (int)NUM_VALVES) return false;

        if (btnPressed[idx]) {
            btnPressed[idx] = false;  // Limpa flag
            return true;
        }
        return false;
    }

    // =======================================================================
    // EFEITOS VISUAIS
    // =======================================================================
    void updateGaugeColors() {
        // Cores por gauge: vermelho=fora, amarelo=na zona, verde=todos na zona
        if (subState_ == SubState::Confirming || subState_ == SubState::Reshuffling) {
            return;
        }

        uint32_t colors[NUM_VALVES];
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            if (inZone_[i]) {
                colors[i] = Adafruit_NeoPixel::Color(0, 255, 0);  // Verde
            } else {
                colors[i] = Adafruit_NeoPixel::Color(255, 0, 0);  // Vermelho
            }
        }

        // Se todos na zona, pisca verde
        if (zonesActive_ == NUM_VALVES) {
            unsigned long t = millis() % 1000;
            if (t < 500) {
                for (uint8_t i = 0; i < NUM_VALVES; i++) {
                    hw.setLight(BUTTON_PIXELS[i], colors[i]);
                }
            } else {
                hw.setLightAll(0);
            }
            return;
        }

        // Alarme se nenhum na zona
        if (zonesActive_ == 0) {
            unsigned long t = millis() % 600;
            if (t < 300) {
                for (uint8_t i = 0; i < NUM_VALVES; i++) {
                    hw.setLight(BUTTON_PIXELS[i], colors[i]);
                }
            } else {
                hw.setLightAll(0);
            }
            return;
        }

        // Estado parcial: acende cada botão com a cor do seu gauge
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            hw.setLight(BUTTON_PIXELS[i], colors[i]);
        }
    }

    void updateConfirmationEffect() {
        // Pisca branco nos botões não pressionados durante confirmação
        unsigned long t = millis() % 400;
        uint32_t color = (t < 200) ? Adafruit_NeoPixel::Color(255, 255, 255) : 0;

        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            if (!buttonPressedInWindow_[i]) {
                hw.setLight(BUTTON_PIXELS[i], color);
            }
        }
    }

    void updateReshuffleEffect() {
        unsigned long elapsed = millis() - effectTimer_;
        if (elapsed >= RESHUFFLE_DURATION_MS) {
            // Fim do re-embaralhamento
            subState_ = SubState::Adjusting;
            effectPhase_ = EffectPhase::None;
            hw.setLightAll(0);
            Serial.println(F("[REACTOR] Reconfiguracao completa. Continue ajustando."));
            return;
        }

        // Pisca branco em todos os LEDs durante reconfiguração
        unsigned long t = millis() % 150;
        if (t < 75) {
            hw.setLightAll(Adafruit_NeoPixel::Color(255, 255, 255));
        } else {
            hw.setLightAll(0);
        }
    }

    void updateEffects() {
        switch (effectPhase_) {
            case EffectPhase::Alarm:
                if (millis() - effectTimer_ > EFFECT_INTERVAL_MS) {
                    effectPhase_ = EffectPhase::None;
                }
                break;

            case EffectPhase::Victory:
                updateVictoryEffect();
                break;

            case EffectPhase::Reshuffle:
                // Controlado por updateReshuffleEffect()
                break;

            default:
                break;
        }
    }

    void updateVictoryEffect() {
        unsigned long elapsed = millis() - effectTimer_;

        switch (effectStep_) {
            case 0:
                hw.setLightAll(Adafruit_NeoPixel::Color(0, 255, 0));
                if (elapsed >= 200) {
                    effectStep_ = 1;
                }
                break;
            case 1:
                hw.setLightAll(0);
                if (elapsed >= 400) {
                    hw.setLightAll(Adafruit_NeoPixel::Color(0, 255, 0));
                    effectStep_ = 2;
                }
                break;
            case 2:
                if (elapsed >= 800) {
                    hw.setLightAll(Adafruit_NeoPixel::Color(0, 255, 0));
                    effectPhase_ = EffectPhase::None;
                }
                break;
        }
    }

    // =======================================================================
    // ESTADOS DO PUZZLE
    // =======================================================================
    void startPuzzle() {
        Serial.println(F("[REACTOR] Iniciando sequencia..."));
        hw.sound().playOnce("/ready.mp3");
        subState_ = SubState::Adjusting;
        setState(PuzzleState::Running);
    }

    void startConfirmationWindow() {
        subState_ = SubState::Confirming;
        confirmWindowStart_ = millis();
        buttonsConfirmed_ = 0;
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            buttonPressedInWindow_[i] = false;
        }

        hw.sound().playOnce("/warning.mp3");
        Serial.println(F("[REACTOR] Todos os gauges na zona! Janela de confirmacao aberta (2s)"));
    }

    void handleSolve() {
        Serial.println(F("[REACTOR] Puzzle resolvido! Reator estabilizado."));

        hw.sound().playOnce("/victory.mp3");

        // Abre trava
        hw.digitalWrite(LOCK_PIN, LOW);

        // Efeito visual de vitória
        effectPhase_ = EffectPhase::Victory;
        effectTimer_ = millis();
        effectStep_ = 0;

        setState(PuzzleState::Solved);
    }

    void handleConfirmationFail() {
        Serial.println(F("[REACTOR] Confirmacao falhou! Reconfigurando painel..."));

        hw.sound().playOnce("/fail.mp3");

        // Re-embaralha mapeamento e orientações
        randomizeMapping();

        // Reseta estado de confirmação
        subState_ = SubState::Reshuffling;
        effectPhase_ = EffectPhase::Reshuffle;
        effectTimer_ = millis();
        buttonsConfirmed_ = 0;
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            buttonPressedInWindow_[i] = false;
        }

        // Som de reconfiguração após um breve delay não-bloqueante
        // (será tocado na próxima iteração de updateEffects)
        hw.sound().playOnce("/reshuffle.mp3");
    }

    void resetPuzzle() {
        Serial.println(F("[REACTOR] Resetando puzzle..."));

        // Travar
        hw.digitalWrite(LOCK_PIN, HIGH);

        // Zerar gauges
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            hw.setAnalogOutput(PCA_CHANNELS[i], 0.0f);
        }

        // Limpar LEDs
        hw.setLightAll(0);

        // Resetar estado
        subState_ = SubState::Adjusting;
        zonesActive_ = 0;
        buttonsConfirmed_ = 0;
        effectPhase_ = EffectPhase::None;
        for (uint8_t i = 0; i < NUM_VALVES; i++) {
            inZone_[i] = false;
            buttonPressedInWindow_[i] = false;
        }

        // Re-embaralhar para nova rodada
        randomizeMapping();

        setState(PuzzleState::Idle);
    }
};

// ============================================================================
// CALLBACKS SwitchInput (fora da classe)
// ============================================================================
volatile bool btnPressed[NUM_VALVES] = {false, false, false};

void onButtonPressed(pinid_t pin, bool held) {
    if (held) return;

    int idx = pin - BUTTON_1_PIN;
    if (idx >= 0 && idx < NUM_VALVES) {
        btnPressed[idx] = true;
        Serial.printf("[BTN] Botao %d (P%d) PRESSIONADO\n", idx + 1, pin);
    }
}

// ============================================================================
// MAIN
// ============================================================================
LogicManager master(31, true);  // ID 31, com rede RS485 habilitada
ReactorControlPuzzle puzzle;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    Serial.println(F("============================================"));
    Serial.println(F("  ReactorControl - PropMaker EscapeRoom"));
    Serial.println(F("  Painel de Controle Nuclear"));
    Serial.println(F("  Abril 2026 - Nicolau dos Brinquedos"));
    Serial.println(F("============================================"));

    hw.begin();
    hw.addPcf8575(PCF_ADDRESS);  // Adiciona expander PCF

    master.addPuzzle(&puzzle);
    master.begin();
    master.setNetworkEnabled(true);  // Habilita comunicação RS485

    Serial.println(F("[Main] Sistema pronto"));
    Serial.println(F("[Main] Aguardando comando de inicio (rede ou botao 1)"));
}

void loop() {
  loopHA();
    master.update();
    delay(1);
}
