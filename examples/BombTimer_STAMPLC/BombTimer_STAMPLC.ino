/**
 * BOMB TIMER — M5Stack StamPLC (ESP32-S3)
 * ==========================================
 * Port do GameTimerPro original (AVR) + BombTimer XIAO S3
 *
 * Hardware:
 *   - Display TFT ST7789v2 135x240 (M5.Lcd)
 *   - Buzzer PWM (M5StamPLC.tone)
 *   - 3 botões A/B/C (PI4IOE5V6408) + BOOT (DET)
 *   - Keypad 4x3 via PCF8574 I2C
 *   - 4 relés (AW9523B) — 2 DETONATE + 2 DEFUSE
 *   - 4 opto-inputs (AW9523B) — fios
 *   - 1 opto-input (AW9523B) — DET externo
 *   - RFID PN532 via I2C
 *   - RTC RX8130CE (RTC)
 *   - WiFi + ArduinoHA (MQTT)
 *
 * Layout TFT:
 *   ┌──────────────────────────┐
 *   │              [W][M][R][T]│ ← status bar
 *   │                          │
 *   │       ┌──────────┐       │
 *   │       │  03:00   │       │ ← timer DSEG14 gigante
 *   │       └──────────┘       │
 *   │           180s           │ ← segundos (small)
 *   │                          │
 *   │  ●Y ●R ●B ●G      IDLE  │ ← círculos fios + status
 *   └──────────────────────────┘
 */

#include <M5StamPLC.h>
#include <Wire.h>
#include <SPIFFS.h>

#include "DisplayManager.h"
#include "InputManager.h"
#include "GameLogic.h"
#include "ConfigHA.h"

// I2C bus externo (PORT.A: G2=Amarelo=SDA, G1=Branco=SCL)
TwoWire PortA(1);  // I2C1

// ============================================================================
// INSTÂNCIAS GLOBAIS
// ============================================================================
DisplayManager display;
InputManager input;
GameLogic game;
ConfigHA configHA;

// ============================================================================
// VARIÁVEIS DE ESTADO (entre ciclos de loop)
// ============================================================================
unsigned long lastTickDisplay = 0;
unsigned long lastTickSound = 0;       // tickDue() check
unsigned long lastWireScan = 0;
unsigned long lastHALoop = 0;
unsigned long lastHAPublish = 0;       // publishAll() a cada 2s
unsigned long lastRFIDScan = 0;
unsigned long lastRtcCheck = 0;
unsigned long lastKeypadPoll = 0;   // hot-plug check
int lastDisplayedRemaining = -1;
bool lastWiFiStatus = false;
bool lastMQTTStatus = false;
bool lastRFIDStatus = false;

// ─── Código de desarme (modo CODE) ────────────────────────────────────
char codeDigits[4] = {0, 0, 0, 0};
int codeDigitIndex = 0;
bool enteringCode = false;

// ─── Delay de verificação do código (mostra os 4 dígitos por 1s) ─────
bool codePendingVerification = false;
int pendingCodeValue = 0;
unsigned long verifyCodeAt = 0;

// ─── Estado do display pós-detonação/desarme ──────────────────────────
unsigned long fullScreenUntil = 0;

// ============================================================================
// FUNÇÕES AUXILIARES
// ============================================================================

void resetGame() {
    game.resetWires();
    game.setState(GameLogic::STATE_IDLE);
    game.stopTimer();
    game.clearAllRelays();
    codeDigitIndex = 0;
    enteringCode = false;
    codePendingVerification = false;
    pendingCodeValue = 0;
    verifyCodeAt = 0;
    fullScreenUntil = 0;
    lastDisplayedRemaining = -1;
    M5StamPLC.setStatusLight(0, 0, 0);  // OFF
    // Invalida status dos ícones para forçar redesenho (clearScreen apagou tudo)
    lastWiFiStatus = !configHA.isWiFiOK();
    lastMQTTStatus = !configHA.isMQTTOK();
    lastRFIDStatus = !input.hasRFID();
    display.clearScreen();
    updateDisplay();  // desenha timer, círculos, ícones e status bar
    configHA.publishAll();
}

void startGame() {
    game.resetWires();
    game.startTimer();
    enteringCode = false;
    codeDigitIndex = 0;
    fullScreenUntil = 0;
    lastDisplayedRemaining = -1;
    M5StamPLC.tone(3800, 30);
    configHA.publishAll();
}

void handleDetonation() {
    game.stopTimer();
    game.setState(GameLogic::STATE_DETONATED);
    game.triggerDetonate();
    display.drawFullScreenMsg("DETONATED!", "BOOM!", TFT_RED);
    M5StamPLC.tone(5000, 500);
    M5StamPLC.setStatusLight(255, 0, 0);
    fullScreenUntil = millis() + 3000;
    configHA.publishAll();
}

void handleDefuse() {
    game.stopTimer();
    game.setState(GameLogic::STATE_DEFUSED);
    game.triggerDefuse();
    display.drawFullScreenMsg("DEFUSED!", "SUCCESS!", TFT_GREEN);
    M5StamPLC.tone(4500, 80);
    M5StamPLC.setStatusLight(0, 255, 0);
    fullScreenUntil = millis() + 3000;
    configHA.publishAll();
}

// Função auxiliar: atualiza o display se estiver em IDLE
void refreshDisplayIfIdle() {
    if (game.getState() == GameLogic::STATE_IDLE) {
        display.clearScreen();
        updateDisplay();
    }
}

// ============================================================================
// CALLBACKS HA + HARDWARE
// ============================================================================
void haOnStart() { startGame(); }
void haOnReset() { resetGame(); }
void haHardwareChange(const char* status) {
    configHA.setI2CStatus(status);
    updateDisplay();
}

// ============================================================================
// ATUALIZAÇÃO DE DISPLAY
// ============================================================================

void updateDisplay() {
    // Se está entrando código, não redesenha timer — drawCodeEntry já cuida
    if (enteringCode) return;

    // Só redesenha se o valor mudou
    int rem = game.getRemainingSeconds();
    if (rem == lastDisplayedRemaining && fullScreenUntil == 0) return;
    lastDisplayedRemaining = rem;

    // Se está em tela cheia (detonação/desarme), não atualiza o resto
    if (fullScreenUntil > millis()) return;
    if (fullScreenUntil > 0 && millis() >= fullScreenUntil) {
        fullScreenUntil = 0;
        display.clearScreen();
    }

    display.drawTimer(rem / 60, rem % 60);
    display.drawSecondsSmall(rem);

    bool wires[4] = {game.isWireCut(0), game.isWireCut(1),
                     game.isWireCut(2), game.isWireCut(3)};
    display.drawAllWireCircles(wires);

    // Status bar
    bool wifiOK = configHA.isWiFiOK();
    bool mqttOK = configHA.isMQTTOK();
    bool rfidOK = input.hasRFID();
    if (wifiOK != lastWiFiStatus || mqttOK != lastMQTTStatus || rfidOK != lastRFIDStatus) {
        display.clearStatusBar();
        display.drawWiFiIcon(wifiOK);
        display.drawMQTTIcon(mqttOK);
        display.drawRFIDIcon(rfidOK);
        display.drawRTCIcon(true);
        lastWiFiStatus = wifiOK;
        lastMQTTStatus = mqttOK;
        lastRFIDStatus = rfidOK;
    }

    // Canto inferior direito
    switch (game.getState()) {
        case GameLogic::STATE_IDLE:
            display.drawBottomRight("IDLE", TFT_YELLOW);
            break;
        case GameLogic::STATE_RUNNING:
            display.drawBottomRight("RUN", TFT_GREEN);
            break;
        case GameLogic::STATE_DEFUSED:
            display.drawBottomRight("OK", TFT_GREEN);
            break;
        case GameLogic::STATE_DETONATED:
            display.drawBottomRight("BOOM", TFT_RED);
            break;
        default:
            break;
    }
}

// ============================================================================
// ATUALIZAÇÃO DOS FIOS
// ============================================================================

void scanWires() {
    auto& aw = M5StamPLC.getIOExpanderB();
    // WIRE1 = P0_5 (index 5), WIRE2 = P0_6 (6), WIRE3 = P0_7 (7), WIRE4 = P1_4 (8+4=12)
    const uint8_t wirePins[4] = {5, 6, 7, 12};
    bool anyChanged = false;

    for (int i = 0; i < 4; i++) {
        // AW9523B opto: 1 = connected (pullup HIGH), 0 = disconnected/cut (LOW)
        bool connected = (aw.digitalRead(wirePins[i]) == 1);
        bool wasCut = game.isWireCut(i);

        // Ignora desconexão inicial (fios nunca conectados não disparam)
        if (!connected && !game.wireSeenConnected_[i]) {
            continue;
        }

        // Marca que já vimos este fio conectado
        if (connected) {
            game.wireSeenConnected_[i] = true;
        }

        if (!connected && !wasCut) {
            // Wire just got cut
            game.setWireCut(i, true);
            configHA.setWireCut(i, true);
            Serial.printf("[WIRE] Fio %d CORTADO\\n", i + 1);

            if (game.getState() == GameLogic::STATE_RUNNING) {
                // Verifica DET wire
                if (game.wires.detWire <= 3 && i == game.wires.detWire) {
                    Serial.println("[WIRE] DET wire! DETONANDO...");
                    handleDetonation();
                    return;
                }
                // Verifica ORDER mode
                if (game.config.defuseMode == GameLogic::DEFUSE_ORDER) {
                    if (!game.checkOrderWire(i)) {
                        Serial.println("[WIRE] Ordem errada! DETONANDO...");
                        handleDetonation();
                        return;
                    }
                    if (game.getOrderIndex() >= 4) {
                        Serial.println("[WIRE] Ordem completa! DESARMOU!");
                        handleDefuse();
                        return;
                    }
                }
                // Verifica DEFUSE wire (modo WIRE)
                if (game.config.defuseMode == GameLogic::DEFUSE_WIRE) {
                    if (game.wires.defuseWire <= 3 && i == game.wires.defuseWire) {
                        Serial.println("[WIRE] Defuse wire! DESARMOU!");
                        handleDefuse();
                        return;
                    }
                }
                // Verifica SPEEDUP wire
                if (game.wires.speedupWire <= 3 && i == game.wires.speedupWire) {
                    game.toggleDoubleSpeed();
                    Serial.printf("[WIRE] Speedup! doubleSpeed=%d\\n", game.isDoubleSpeed());
                    M5StamPLC.tone(3000, 50);
                }
                // Verifica PAUSE wire (2s de pausa)
                if (game.wires.pauseWire <= 3 && i == game.wires.pauseWire) {
                    game.setPause(2000);
                    Serial.println("[WIRE] Pause 2s!");
                    M5StamPLC.tone(1500, 100);
                }
            }
            anyChanged = true;
        }
    }

    if (anyChanged) {
        updateDisplay();
    }
}

// ============================================================================
// ATUALIZAÇÃO DO KEYPAD (modo CODE)
// ============================================================================

void processKeypad() {
    // Se há verificação pendente, ignora novas teclas
    if (codePendingVerification) return;

    int key = input.getKey();
    if (key < 0) return;

    if (game.getState() == GameLogic::STATE_RUNNING && game.config.defuseMode == GameLogic::DEFUSE_CODE) {
        if (key >= '0' && key <= '9') {
            if (!enteringCode) {
                enteringCode = true;
                codeDigitIndex = 0;
            }
            if (codeDigitIndex < 4) {
                codeDigits[codeDigitIndex++] = key;
                display.drawCodeEntry(codeDigits, codeDigitIndex);
                M5StamPLC.tone(3500, 20);
            }
            if (codeDigitIndex >= 4) {
                // Mostra o código completo por 1s antes de verificar
                M5StamPLC.tone(4500, 30);
                enteringCode = false;
                codePendingVerification = true;
                pendingCodeValue = (codeDigits[0] - '0') * 1000 +
                                  (codeDigits[1] - '0') * 100 +
                                  (codeDigits[2] - '0') * 10 +
                                  (codeDigits[3] - '0');
                verifyCodeAt = millis() + 1000;
            }
        } else if (key == '*' || key == '#') {
            // Cancela entrada de código
            enteringCode = false;
            codeDigitIndex = 0;
            display.clearTimerArea();
            updateDisplay();
        }
    }
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    Serial.println(F("\n=== BombTimer STAMPLC ==="));

    // Inicializa hardware
    M5StamPLC.begin();
    Wire.begin(13, 15);  // SDA=G13, SCL=G15

    // I2C bus externo (PORT.A: Grove RED — pinos definidos em M5StamPLC pin_config.h)
    PortA.begin(STAMPLC_PIN_GROVE_RED_SDA, STAMPLC_PIN_GROVE_RED_SCL, 100000);

    // I2C Scan em ambos os barramentos
    const char* i2cResult = input.i2cScan();
    configHA.setI2CStatus(i2cResult);

    SPIFFS.begin(true);

    // Inicializa módulos
    display.begin();
    input.begin();
    game.begin();
    // ConfigManager: SPIFFS > AP mode + WiFi + HA
    configHA.begin();

    // HA callbacks
    configHA.onStart = haOnStart;
    configHA.onReset = haOnReset;

    // Callback de hot-plug: keypad conectado/desconectado
    input.onHardwareChange = haHardwareChange;

    // Tela inicial (usando config carregada da NVS)
    int bootCountdown = game.getCountdownSeconds();
    display.drawTimer(bootCountdown / 60, bootCountdown % 60);
    display.drawSecondsSmall(bootCountdown);
    bool wires0[4] = {false, false, false, false};
    display.drawAllWireCircles(wires0);
    display.drawBottomRight("IDLE", TFT_YELLOW);
    display.drawWiFiIcon(configHA.isWiFiOK());
    display.drawMQTTIcon(configHA.isMQTTOK());
    display.drawRFIDIcon(input.hasRFID());
    display.drawRTCIcon(true);

    configHA.publishAll();

    Serial.printf("[BOOT] Pronto! Countdown=%ds, Mode=%d, Code=%04d\n",
        bootCountdown, game.config.defuseMode, game.config.defuseCode);
}

// ============================================================================
// ATUALIZAÇÃO POR ESTADO
// ============================================================================

void updateIdle() {
    // BTN_START → inicia contagem
    if (input.wasPressed(InputManager::BTN_START)) {
        startGame();
        return;
    }

    // BTN_MENU → menu de configuração
    if (input.wasPressed(InputManager::BTN_MENU)) {
        game.currentMenuItem = GameLogic::MENU_COUNTDOWN;
        game.setState(GameLogic::STATE_MENU);
        Serial.println(F("[MENU] Entrando no menu"));
        showMenuLabel(0);
        return;
    }

    // Ajuste de tempo com long press (igual ao original GameTimerPro)
    static unsigned long leftPressStart = 0;
    static bool leftWasPressed = false;
    static unsigned long rightPressStart = 0;
    static bool rightWasPressed = false;
    static unsigned long lastAdjTime = 0;
    unsigned long now = millis();

    // PREV: detecção de curto + longo
    if (input.wasPressed(InputManager::BTN_PREV) && !leftWasPressed) {
        leftWasPressed = true;
        leftPressStart = now;
        int sec = game.getCountdownSeconds();
        if (sec > 5) { sec--; game.setCountdownSeconds(sec); updateDisplay(); }
        lastAdjTime = now;
    }
    if (leftWasPressed && input.isHeld(InputManager::BTN_PREV, 600) && now - lastAdjTime >= 150) {
        int sec = game.getCountdownSeconds();
        sec = max(sec - 5, 5);
        game.setCountdownSeconds(sec);
        updateDisplay();
        lastAdjTime = now;
    }
    if (leftWasPressed && !input.isHeld(InputManager::BTN_PREV, 50)) {
        leftWasPressed = false;
    }

    // NEXT: detecção de curto + longo
    if (input.wasPressed(InputManager::BTN_NEXT) && !rightWasPressed) {
        rightWasPressed = true;
        rightPressStart = now;
        int sec = game.getCountdownSeconds();
        if (sec < 5999) { sec++; game.setCountdownSeconds(sec); updateDisplay(); }
        lastAdjTime = now;
    }
    if (rightWasPressed && input.isHeld(InputManager::BTN_NEXT, 600) && now - lastAdjTime >= 150) {
        int sec = game.getCountdownSeconds();
        sec = min(sec + 5, 5999);
        game.setCountdownSeconds(sec);
        updateDisplay();
        lastAdjTime = now;
    }
    if (rightWasPressed && !input.isHeld(InputManager::BTN_NEXT, 50)) {
        rightWasPressed = false;
    }
}

void updateRunning() {
    int rem = game.getRemainingSeconds();

    // Tick sonoro: usa tickDue() do GameLogic — independente do display
    if (game.tickDue() && game.isTickEnabled()) {
        if (rem <= 10) {
            M5StamPLC.tone(2500 + rem * 200, 30);
        } else {
            M5StamPLC.tone(2000, 15);
        }
    }

    // RGB LED: vermelho pulsante (toggle a cada 500ms, mais rápido nos últimos 10s)
    static unsigned long lastRgbToggle = 0;
    static bool rgbOn = false;
    int interval = (rem <= 10) ? 250 : 500;
    if (millis() - lastRgbToggle > interval) {
        lastRgbToggle = millis();
        rgbOn = !rgbOn;
        if (rgbOn) {
            M5StamPLC.setStatusLight(255, 0, 0);
        } else {
            M5StamPLC.setStatusLight(0, 0, 0);
        }
    }

    // Fim do timer
    if (rem <= 0) {
        handleDetonation();
        return;
    }

    // Escaneia fios
    if (millis() - lastWireScan > 100) {
        lastWireScan = millis();
        scanWires();
    }

    // Keypad (modo CODE) — só chama getKey() se PCF presente e modo CODE
    if (game.config.defuseMode == GameLogic::DEFUSE_CODE && input.isKeypadOK()) {
        processKeypad();
    }

    // Verifica código pendente (delay de 1s após digitar 4 dígitos)
    if (codePendingVerification && millis() >= verifyCodeAt) {
        codePendingVerification = false;
        enteringCode = false;
        codeDigitIndex = 0;
        if (game.checkDefuseCode(pendingCodeValue)) {
            handleDefuse();
            return;
        } else {
            int attempts = game.getCodeAttempts();
            if (attempts >= game.config.maxCodeAttempts) {
                Serial.printf("[CODE] %d tentativas excedidas! DETONANDO!\n", attempts);
                handleDetonation();
            } else {
                Serial.printf("[CODE] Erro! Tentativa %d/%d\n",
                    attempts + 1, game.config.maxCodeAttempts);
                M5StamPLC.tone(800, 150);
                display.drawFullScreenMsg("ERROR", "Try again", TFT_RED);
                fullScreenUntil = millis() + 1000;
                display.clearTimerArea();
                display.drawCodeEntry(codeDigits, 0);
            }
            return;
        }
    }

    // Botão START durante running = detona
    if (input.wasPressed(InputManager::BTN_START)) {
        handleDetonation();
        return;
    }

    // Atualiza display
    updateDisplay();

    // Publica HA a cada ~2s
    if (millis() - lastHAPublish > 2000) {
        lastHAPublish = millis();
        configHA.publishAll();
    }

    // RFID durante countdown (se habilitado e chip presente)
    if (game.config.rfidEnabled && input.hasRFID() && millis() - lastRFIDScan > 300) {
        lastRFIDScan = millis();
        char uidHex[15];
        uint8_t uidLen = 0;
        if (input.readRFID(uidHex, &uidLen) && uidLen >= 4) {
            // Converte UID hex para uint32_t (primeiros 4 bytes)
            uint32_t uid32 = 0;
            for (int i = 0; i < 4 && i * 2 < 14; i++) {
                unsigned int b;
                sscanf(uidHex + i * 2, "%02x", &b);
                uid32 = (uid32 << 8) | (b & 0xFF);
            }
            // Verifica DEFUSE tags
            bool matched = false;
            for (int i = 0; i < game.config.defuseTagCount; i++) {
                if (game.config.defuseTags[i] == uid32) {
                    Serial.printf("[RFID] DEFUSE tag! %s\n", uidHex);
                    handleDefuse();
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                // Verifica DETONATE tags
                for (int i = 0; i < game.config.detonateTagCount; i++) {
                    if (game.config.detonateTags[i] == uid32) {
                        Serial.printf("[RFID] DETONATE tag! %s\n", uidHex);
                        handleDetonation();
                        break;
                    }
                }
            }
        }
    }
}

// ============================================================================
// MENU DE CONFIGURAÇÃO
// ============================================================================
// Navegação: PREV/NEXT entre itens, MENU entra no ajuste,
// PREV/NEXT no ajuste altera valor, MENU confirma, START sai.
// Timeout 15s sem input → volta ao IDLE.
// ============================================================================

// Descrições curtas para cada item (exibidas no display)
static const char* MENU_LABELS[GameLogic::MENU_COUNT] = {
    "COUNTDOWN", "MODE", "CODE", "ORDER",
    "DET WIRE", "SPD WIRE", "PAUSE WIRE",
    "BRIGHT", "TICK", "RFID",
    "RF DEFU", "RF DETO",
    "TDET", "TDEF", "ALARM H", "ALARM M",
    "CLR DEFU", "CLR DETO"
};

void updateMenu() {
    static unsigned long lastMenuActivity = 0;
    static bool inEdit = false;

    unsigned long now = millis();

    // Inicializa lastMenuActivity na primeira chamada
    if (lastMenuActivity == 0) {
        lastMenuActivity = now;
    }

    // Timeout: 15s sem input → volta ao IDLE
    if (now - lastMenuActivity > 15000) {
        Serial.println(F("[MENU] Timeout → IDLE"));
        inEdit = false;
        game.setState(GameLogic::STATE_IDLE);
        display.clearScreen();
        updateDisplay();
        return;
    }

    // START → sai do menu
    if (input.wasPressed(InputManager::BTN_START)) {
        Serial.println(F("[MENU] START → IDLE"));
        inEdit = false;
        game.setState(GameLogic::STATE_IDLE);
        display.clearScreen();
        updateDisplay();
        return;
    }

    int item = (int)game.currentMenuItem;

    if (!inEdit) {
        // ─── Navegação entre itens ────────────────────────────────
        if (input.wasPressed(InputManager::BTN_PREV)) {
            item = (item - 1 + GameLogic::MENU_COUNT) % GameLogic::MENU_COUNT;
            game.currentMenuItem = (GameLogic::MenuItem)item;
            lastMenuActivity = now;
            showMenuLabel(item);
            return;
        }
        if (input.wasPressed(InputManager::BTN_NEXT)) {
            item = (item + 1) % GameLogic::MENU_COUNT;
            game.currentMenuItem = (GameLogic::MenuItem)item;
            lastMenuActivity = now;
            showMenuLabel(item);
            return;
        }
        // MENU → entra no modo de ajuste do item atual
        if (input.wasPressed(InputManager::BTN_MENU)) {
            inEdit = true;
            game.inSubEdit = true;
            lastMenuActivity = now;
            // CODE: input via keypad
            if (item == GameLogic::MENU_DEFUSE_CODE) {
                inputCode();
                inEdit = false;
                game.inSubEdit = false;
                showMenuLabel(item);
                return;
            }
            // ORDER: input wire order via keypad
            if (item == GameLogic::MENU_DEFUSE_ORDER) {
                inputWireOrder();
                inEdit = false;
                game.inSubEdit = false;
                showMenuLabel(item);
                return;
            }
            // RFID DEFU: learn defuse tag
            if (item == GameLogic::MENU_RFID_DEFU) {
                inEdit = false;
                game.inSubEdit = false;
                learnRfidTag(true);
                showMenuLabel(item);
                return;
            }
            // RFID DETO: learn detonate tag
            if (item == GameLogic::MENU_RFID_DETO) {
                inEdit = false;
                game.inSubEdit = false;
                learnRfidTag(false);
                showMenuLabel(item);
                return;
            }
            // Clear RFID defuse tags
            if (item == GameLogic::MENU_RFID_CLR_DEFU) {
                game.config.defuseTagCount = 0;
                memset(game.config.defuseTags, 0, sizeof(game.config.defuseTags));
                game.saveConfig();
                display.drawFullScreenMsg("CLEARED!", "DEFU tags", TFT_YELLOW);
                delay(800);
                showMenuLabel(item);
                return;
            }
            // Clear RFID detonate tags
            if (item == GameLogic::MENU_RFID_CLR_DETO) {
                game.config.detonateTagCount = 0;
                memset(game.config.detonateTags, 0, sizeof(game.config.detonateTags));
                game.saveConfig();
                display.drawFullScreenMsg("CLEARED!", "DETO tags", TFT_YELLOW);
                delay(800);
                showMenuLabel(item);
                return;
            }
            showMenuValue(item);
            return;
        }
    } else {
        // ─── Ajuste de valor ──────────────────────────────────────
        if (input.wasPressed(InputManager::BTN_PREV)) {
            lastMenuActivity = now;
            adjustMenuItem(item, -1);
            showMenuValue(item);
            return;
        }
        if (input.wasPressed(InputManager::BTN_NEXT)) {
            lastMenuActivity = now;
            adjustMenuItem(item, 1);
            showMenuValue(item);
            return;
        }
        // Long press: sustenta PREV por 600ms → acelera
        static unsigned long prevEditHeld = 0;
        if (input.isHeld(InputManager::BTN_PREV, 600) && millis() - prevEditHeld > 150) {
            prevEditHeld = millis();
            lastMenuActivity = now;
            adjustMenuItem(item, -1);
            showMenuValue(item);
        }
        // Long press: sustenta NEXT por 600ms → acelera
        static unsigned long nextEditHeld = 0;
        if (input.isHeld(InputManager::BTN_NEXT, 600) && millis() - nextEditHeld > 150) {
            nextEditHeld = millis();
            lastMenuActivity = now;
            adjustMenuItem(item, 1);
            showMenuValue(item);
        }
        // MENU → confirma e volta à navegação
        if (input.wasPressed(InputManager::BTN_MENU)) {
            inEdit = false;
            game.inSubEdit = false;
            lastMenuActivity = now;
            // Salva configuração
            game.saveConfig();
            showMenuLabel(item);
            return;
        }
    }
}

void showMenuLabel(int item) {
    M5.Lcd.fillScreen(TFT_BLACK);

    // Linha 1: nome do item (CYAN, Font2, size2, centralizado)
    M5.Lcd.setFont(&fonts::Font2);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_CYAN, TFT_BLACK);
    const char* label = MENU_LABELS[item];
    int lw = M5.Lcd.textWidth(label);
    int lx = (M5.Lcd.width() - lw) / 2;
    if (lx < 0) lx = 0;
    M5.Lcd.setCursor(lx, 20);
    M5.Lcd.print(label);

    // Linha 2: valor atual (GREEN, Font2, size2, centralizado)
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    char valBuf[24];
    getMenuValue(item, valBuf, sizeof(valBuf));
    int vw = M5.Lcd.textWidth(valBuf);
    int vx = (M5.Lcd.width() - vw) / 2;
    if (vx < 0) vx = 0;
    M5.Lcd.setCursor(vx, 50);
    M5.Lcd.print(valBuf);

    // Rodapé: instruções (Font0, size1, nas extremidades)
    M5.Lcd.setFont(&fonts::Font0);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextColor(TFT_DARKGREY, TFT_BLACK);
    M5.Lcd.setCursor(4, M5.Lcd.height() - 18);
    M5.Lcd.print("PREV");
    M5.Lcd.setCursor((M5.Lcd.width() - 12) / 2, M5.Lcd.height() - 18);
    M5.Lcd.print("SELECT");
    M5.Lcd.setCursor(M5.Lcd.width() - 36, M5.Lcd.height() - 18);
    M5.Lcd.print("NEXT");
}

void getMenuValue(int item, char* buf, int bufsize) {
    switch (item) {
        case GameLogic::MENU_COUNTDOWN:
            snprintf(buf, bufsize, "%ds", game.config.countdownSeconds); break;
        case GameLogic::MENU_DEFUSE_MODE:
            snprintf(buf, bufsize, "%s",
                game.config.defuseMode == GameLogic::DEFUSE_CODE ? "CODE" :
                game.config.defuseMode == GameLogic::DEFUSE_WIRE ? "WIRE" : "ORDER"); break;
        case GameLogic::MENU_DEFUSE_CODE:
            snprintf(buf, bufsize, "%04d", game.config.defuseCode); break;
        case GameLogic::MENU_DEFUSE_ORDER:
            snprintf(buf, bufsize, "%d", game.config.defuseOrder); break;
        case GameLogic::MENU_DET_WIRE:
            snprintf(buf, bufsize, "%s",
                game.config.detWire == 0 ? "RAND" :
                game.config.detWire == 5 ? "NONE" : String(game.config.detWire).c_str()); break;
        case GameLogic::MENU_SPEED_WIRE:
            snprintf(buf, bufsize, "%s",
                game.config.speedupWire == 0 ? "RAND" :
                game.config.speedupWire == 5 ? "NONE" : String(game.config.speedupWire).c_str()); break;
        case GameLogic::MENU_PAUSE_WIRE:
            snprintf(buf, bufsize, "%s",
                game.config.pauseWire == 0 ? "RAND" :
                game.config.pauseWire == 5 ? "NONE" : String(game.config.pauseWire).c_str()); break;
        case GameLogic::MENU_BRIGHTNESS:
            snprintf(buf, bufsize, "%d/15", game.config.brightness); break;
        case GameLogic::MENU_TICK:
            snprintf(buf, bufsize, "%s", game.config.tickEnabled ? "ON" : "OFF"); break;
        case GameLogic::MENU_RFID_ENABLE:
            snprintf(buf, bufsize, "%s", game.config.rfidEnabled ? "ON" : "OFF"); break;
        case GameLogic::MENU_RFID_DEFU:
            snprintf(buf, bufsize, "%d/4 tags", game.config.defuseTagCount); break;
        case GameLogic::MENU_RFID_DETO:
            snprintf(buf, bufsize, "%d/4 tags", game.config.detonateTagCount); break;
        case GameLogic::MENU_RFID_CLR_DEFU:
        case GameLogic::MENU_RFID_CLR_DETO:
            snprintf(buf, bufsize, "SEL=clear"); break;
        case GameLogic::MENU_DET_TRIGGER:
            snprintf(buf, bufsize, "%ds", game.config.detTriggerSec); break;
        case GameLogic::MENU_DEFUSE_TRIGGER:
            snprintf(buf, bufsize, "%ds", game.config.defuseTriggerSec); break;
        case GameLogic::MENU_ALARM_H:
            snprintf(buf, bufsize, "%02dh", game.config.alarmHour); break;
        case GameLogic::MENU_ALARM_M:
            snprintf(buf, bufsize, "%02dmin", game.config.alarmMin); break;
        default:
            snprintf(buf, bufsize, "---"); break;
    }
}

void showMenuValue(int item) {
    // Atualiza apenas o valor (linha 2), sem redesenharlabel/rodapé
    M5.Lcd.setFont(&fonts::Font2);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    char valBuf[24];
    getMenuValue(item, valBuf, sizeof(valBuf));
    // Limpa só a área do valor (y=42 a y=74)
    M5.Lcd.fillRect(0, 42, M5.Lcd.width(), 32, TFT_BLACK);
    int vw = M5.Lcd.textWidth(valBuf);
    int vx = (M5.Lcd.width() - vw) / 2;
    if (vx < 0) vx = 0;
    M5.Lcd.setCursor(vx, 50);
    M5.Lcd.print(valBuf);
    M5.Lcd.setFont(&fonts::Font0);
}

void adjustMenuItem(int item, int dir) {
    switch (item) {
        case GameLogic::MENU_COUNTDOWN: {
            int v = game.config.countdownSeconds + dir;
            if (v < 5) v = 5;
            if (v > 5999) v = 5999;
            game.config.countdownSeconds = v;
            break;
        }
        case GameLogic::MENU_DEFUSE_MODE: {
            int v = (int)game.config.defuseMode + dir;
            if (v < 0) v = 2;
            if (v > 2) v = 0;
            game.config.defuseMode = (GameLogic::DefuseMode)v;
            break;
        }
        case GameLogic::MENU_DEFUSE_CODE: {
            int v = game.config.defuseCode + dir;
            if (v < 0) v = 9999;
            if (v > 9999) v = 0;
            game.config.defuseCode = v;
            break;
        }
        case GameLogic::MENU_DEFUSE_ORDER: {
            int v = game.config.defuseOrder + dir * 1111;
            if (v < 1111) v = 4444;
            if (v > 4444) v = 1111;
            game.config.defuseOrder = v;
            break;
        }
        case GameLogic::MENU_DET_WIRE: {
            int v = game.config.detWire + dir;
            if (v < 0) v = 5;
            if (v > 5) v = 0;
            if (v > 0 && v < 5 && game.wireConflict(1, v)) {
                M5StamPLC.tone(800, 150);
                display.drawFullScreenMsg("CONFLICT", "Wire in use", TFT_RED);
                delay(800);
                showMenuLabel(item);
            } else {
                game.config.detWire = v;
            }
            break;
        }
        case GameLogic::MENU_SPEED_WIRE: {
            int v = game.config.speedupWire + dir;
            if (v < 0) v = 5;
            if (v > 5) v = 0;
            if (v > 0 && v < 5 && game.wireConflict(2, v)) {
                M5StamPLC.tone(800, 150);
                display.drawFullScreenMsg("CONFLICT", "Wire in use", TFT_RED);
                delay(800);
                showMenuLabel(item);
            } else {
                game.config.speedupWire = v;
            }
            break;
        }
        case GameLogic::MENU_PAUSE_WIRE: {
            int v = game.config.pauseWire + dir;
            if (v < 0) v = 5;
            if (v > 5) v = 0;
            if (v > 0 && v < 5 && game.wireConflict(3, v)) {
                M5StamPLC.tone(800, 150);
                display.drawFullScreenMsg("CONFLICT", "Wire in use", TFT_RED);
                delay(800);
                showMenuLabel(item);
            } else {
                game.config.pauseWire = v;
            }
            break;
        }
        case GameLogic::MENU_BRIGHTNESS: {
            int v = game.config.brightness + dir;
            if (v < 0) v = 15;
            if (v > 15) v = 0;
            game.config.brightness = v;
            break;
        }
        case GameLogic::MENU_TICK:
            game.config.tickEnabled = !game.config.tickEnabled;
            break;
        case GameLogic::MENU_RFID_ENABLE:
            game.config.rfidEnabled = !game.config.rfidEnabled;
            break;
        case GameLogic::MENU_ALARM_H: {
            int v = game.config.alarmHour + dir;
            if (v < 0) v = 23;
            if (v > 23) v = 0;
            game.config.alarmHour = v;
            game.config.alarmEnabled = (game.config.alarmHour > 0 || game.config.alarmMin > 0);
            break;
        }
        case GameLogic::MENU_ALARM_M: {
            int v = game.config.alarmMin + dir * 5;
            if (v < 0) v = 55;
            if (v > 59) v = 0;
            game.config.alarmMin = v;
            game.config.alarmEnabled = (game.config.alarmHour > 0 || game.config.alarmMin > 0);
            break;
        }
        case GameLogic::MENU_DET_TRIGGER: {
            int v = game.config.detTriggerSec + dir;
            if (v < 1) v = 10;
            if (v > 10) v = 1;
            game.config.detTriggerSec = v;
            break;
        }
        case GameLogic::MENU_DEFUSE_TRIGGER: {
            int v = game.config.defuseTriggerSec + dir;
            if (v < 1) v = 10;
            if (v > 10) v = 1;
            game.config.defuseTriggerSec = v;
            break;
        }
    }
    M5StamPLC.tone(3000, 20);
}

// ============================================================================
// INPUT CODE / WIRE ORDER (via keypad, igual ao original)
// ============================================================================

void inputCode() {
    // Mostra 4 underscores
    display.drawCodeEntry("____", 0);
    int pos = 0;
    int tempCode = 0;

    while (true) {
        // Poll manual do keypad (bloqueante, igual ao original)
        int key = input.getKey();
        if (key >= 0) {
            if (key >= '0' && key <= '9' && pos < 4) {
                tempCode = tempCode * 10 + (key - '0');
                char buf[4] = {0};
                int t = tempCode;
                for (int i = pos; i >= 0; i--) { buf[i] = '0' + (t % 10); t /= 10; }
                display.drawCodeEntry(buf, pos + 1);
                pos++;
                M5StamPLC.tone(3500, 20);
            }
            if (pos >= 4) {
                game.config.defuseCode = tempCode;
                M5StamPLC.tone(4500, 30);
                game.saveConfig();
                break;
            }
        }
        // START ou * cancela
        if (input.wasPressed(InputManager::BTN_START) || key == '*' || key == '#') {
            break;
        }
        delay(10);
    }
    display.clearScreen();
    updateDisplay();
}

void inputWireOrder() {
    // Mostra 4 underscores
    display.drawCodeEntry("____", 0);
    int pos = 0;
    int used[4] = {0, 0, 0, 0};
    int tempOrder = 0;

    while (true) {
        int key = input.getKey();
        if (key >= 0) {
            if (key >= '1' && key <= '4' && pos < 4) {
                int wire = key - '0';
                if (!used[wire - 1]) {
                    used[wire - 1] = 1;
                    tempOrder = tempOrder * 10 + wire;
                    char buf[4] = {0};
                    int t = tempOrder;
                    for (int i = pos; i >= 0; i--) { buf[i] = '0' + (t % 10); t /= 10; }
                    display.drawCodeEntry(buf, pos + 1);
                    pos++;
                    M5StamPLC.tone(3500, 20);
                }
            }
            if (pos >= 4) {
                game.config.defuseOrder = tempOrder;
                M5StamPLC.tone(4500, 30);
                game.saveConfig();
                break;
            }
        }
        if (input.wasPressed(InputManager::BTN_START) || key == '*' || key == '#') {
            break;
        }
        delay(10);
    }
    display.clearScreen();
    updateDisplay();
}

// ============================================================================
// LEARN RFID TAG (DEFUSE ou DETONATE)
// ============================================================================

void learnRfidTag(bool isDefuse) {
    display.drawFullScreenMsg(isDefuse ? "RFID DEFU" : "RFID DETO",
        "Tap tag to learn", TFT_CYAN);
    delay(1500);

    unsigned long started = millis();
    while (millis() - started < 10000) {  // timeout 10s
        char uidHex[15];
        uint8_t uidLen = 0;
        if (input.readRFID(uidHex, &uidLen) && uidLen >= 4) {
            // Converte UID hex para uint32_t
            uint32_t uid32 = 0;
            for (int i = 0; i < 4 && i * 2 < 14; i++) {
                unsigned int b;
                sscanf(uidHex + i * 2, "%02x", &b);
                uid32 = (uid32 << 8) | (b & 0xFF);
            }

            uint32_t* tags = isDefuse ? game.config.defuseTags : game.config.detonateTags;
            uint8_t& count = isDefuse ? game.config.defuseTagCount : game.config.detonateTagCount;

            // Verifica duplicata
            bool dup = false;
            for (int i = 0; i < count; i++) {
                if (tags[i] == uid32) { dup = true; break; }
            }
            if (dup) {
                display.drawFullScreenMsg("DUP!", uidHex, TFT_YELLOW);
                delay(1000);
                break;
            }
            if (count >= 4) {
                display.drawFullScreenMsg("FULL!", uidHex, TFT_RED);
                delay(1000);
                break;
            }
            // Adiciona tag
            tags[count++] = uid32;
            game.saveConfig();
            display.drawFullScreenMsg("LEARNED!", uidHex, TFT_GREEN);
            M5StamPLC.tone(4500, 80);
            delay(1000);
            break;
        }
        if (input.wasPressed(InputManager::BTN_START)) break;
        delay(50);
    }
    display.clearScreen();
    updateDisplay();
}

// ============================================================================
// ESTADOS PÓS-JOGO (DETONATED / DEFUSED)
// ============================================================================

void updateDetonated() {
    // Mantém relés ativos até o timer de release
    game.updateRelays();

    // START → reseta e volta ao IDLE
    if (input.wasPressed(InputManager::BTN_START)) {
        M5StamPLC.noTone();
        game.clearAllRelays();
        resetGame();
        display.clearScreen();
        updateDisplay();
    }
}

void updateDefused() {
    // Mantém relés ativos até o timer de release
    game.updateRelays();

    // START → reseta e volta ao IDLE
    if (input.wasPressed(InputManager::BTN_START)) {
        M5StamPLC.noTone();
        game.clearAllRelays();
        resetGame();
        display.clearScreen();
        updateDisplay();
    }
}

// ============================================================================
// ESTADO ALARME
// ============================================================================

void updateAlarm() {
    static unsigned long alarmBuzzStart = 0;
    static bool alarmToggle = false;

    // Efeito de alarme: beep intermitente + RGB laranja
    if (millis() - alarmBuzzStart > 500) {
        alarmBuzzStart = millis();
        alarmToggle = !alarmToggle;
        if (alarmToggle) {
            M5StamPLC.tone(3500, 200);
            M5StamPLC.setStatusLight(255, 128, 0);  // ORANGE
        } else {
            M5StamPLC.noTone();
            M5StamPLC.setStatusLight(0, 0, 0);
        }
    }

    // START → desliga alarme e volta ao IDLE
    if (input.wasPressed(InputManager::BTN_START)) {
        M5StamPLC.noTone();
        game.setState(GameLogic::STATE_IDLE);
        display.clearScreen();
        updateDisplay();
    }
}

void loop() {
    M5StamPLC.update();

    // ─── HA loop ────────────────────────────────────────────────────
    if (millis() - lastHALoop > 10) {
        lastHALoop = millis();
        configHA.loopHA();
    }

    // ─── Dispatcher de estados ──────────────────────────────────────
    switch (game.getState()) {
        case GameLogic::STATE_IDLE:
            updateIdle();
            break;
        case GameLogic::STATE_MENU:
            updateMenu();
            break;
        case GameLogic::STATE_RUNNING:
            updateRunning();
            break;
        case GameLogic::STATE_DETONATED:
            updateDetonated();
            break;
        case GameLogic::STATE_DEFUSED:
            updateDefused();
            break;
        case GameLogic::STATE_ALARM:
            updateAlarm();
            break;
        default:
            break;
    }

    // ─── Relay release ──────────────────────────────────────────────
    game.updateRelays();

    // ─── RTC alarm check ────────────────────────────────────────────
    if (game.config.alarmEnabled && game.getState() == GameLogic::STATE_IDLE
        && millis() - lastRtcCheck > 1000) {
        lastRtcCheck = millis();
        struct tm t;
        M5StamPLC.getRtcTime(&t);
        if (game.checkAlarm(t.tm_hour, t.tm_min)) {
            Serial.printf("[RTC] ALARM! %02d:%02d\n", t.tm_hour, t.tm_min);
            game.setState(GameLogic::STATE_ALARM);
            display.drawFullScreenMsg("ALARM!", "START to stop", TFT_MAGENTA);
        }
    }

    // ─── RF433 Polling ──────────────────────────────────────────────
    {
        InputManager::RFCommand rfCmd = input.pollRF();
        if (rfCmd != InputManager::RF_NONE) {
            switch (game.getState()) {
                case GameLogic::STATE_IDLE:
                    if (rfCmd == InputManager::RF_START) {
                        startGame();
                    }
                    break;
                case GameLogic::STATE_RUNNING:
                    switch (rfCmd) {
                        case InputManager::RF_DETONATE:
                            handleDetonation();
                            break;
                        case InputManager::RF_DEFUSE:
                            handleDefuse();
                            break;
                        case InputManager::RF_SPEED:
                            game.toggleDoubleSpeed();
                            M5StamPLC.tone(3000, 50);
                            break;
                        case InputManager::RF_PAUSE:
                            game.setPause(2000);
                            M5StamPLC.tone(1500, 100);
                            break;
                        default: break;
                    }
                    break;
                default: break;
            }
        }
    }

    // ─── Keypad hot-plug: verifica PCF8574 a cada 2s ───────────────
    if (millis() - lastKeypadPoll > 2000) {
        lastKeypadPoll = millis();
        input.pollKeypad();
    }

    delay(1);
}
