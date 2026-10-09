/*
  KeypadTester - Detector de Pinagem para Teclados Matriciais

  Este exemplo identifica automaticamente a pinagem de teclados matriciais
  conectados via PCF8574 ou PCF8575.

  USO:
  1. Conecte os pinos do teclado aos pinos do PCF (na ordem física do teclado)
  2. Defina no código a pinagem física do PCF conectada ao teclado
  3. Execute e pressione as teclas quando solicitado
  4. O sketch retorna:
     - rowPins[] e colPins[] para uso com KeyboardManager (IoAbstraction)
     - Mapeamento pino físico do teclado -> Linha/Coluna

  COMPATIBILIDADE:
  - PCF8574 (8 pinos I/O): Usa 7 dos 8 pinos disponíveis
  - PCF8575 (16 pinos I/O): Usa P00-P06 (7 pinos)

  Para uso com a biblioteca propmaker_escaperoom, copie os arrays rowPins[]
  e colPins[] para seu sketch e use com MatrixKeyboardManager.
*/

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

#include <KeyboardManager.h>
#include <TaskManager.h>

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
// CONFIGURAÇÃO: Defina os pinos do PCF857x que estão conectados ao teclado
// ============================================================================

// Endereço I2C do PCF857x (0x20-0x27 para PCF8574, 0x38-0x3F para PCF8574A)
#define PCF_ADDRESS 0x20

// Se true, usa PCF8575 (16 pinos). Se false, usa PCF8574 (8 pinos)
#define PCF_IS_8575 true

// Pinagem física conectada ao teclado (7 pinos do PCF)
// Para PCF8575: use 0-15 (P00-P15)
// Para PCF8574: use 0-7 (P0-P7)
// ORDEM: Conecte na ordem física do teclado (da esquerda para direita)
const byte pinoHardware[7] = {0, 1, 2, 3, 4, 5, 6};  // P00 a P06 no PCF8575

// Ordem das teclas para teclado 3x4 padrão
const char ordem3x4[12] = {'1','2','3','4','5','6','7','8','9','*','0','#'};

// ============================================================================

byte pinoX[12];  // Armazena um dos pinos detectados para cada tecla
byte pinoY[12];  // Armazena o outro pino detectado para cada tecla

PCF8574IoAbstraction* pcf = nullptr;

// Listener global para teste de validação
class SimpleKeyListener : public KeyboardListener {
public:
    char lastKey = 0;
    void keyPressed(char key, bool held) override { if (!held) lastKey = key; }
    void keyReleased(char key) override { lastKey = 0; }
};
static SimpleKeyListener listener;

void detectarParPinos(char caractere, int posicao) {
  Serial.print(F("Pressione e SEGURE a tecla '"));
  Serial.print(caractere);
  Serial.println(F("'..."));

  bool detectado = false;

  while (!detectado) {
    for (int i = 0; i < 7; i++) {
      byte outPin = pinoHardware[i];
      pcf->pinDirection(outPin, OUTPUT);
      pcf->writeValue(outPin, 0);
      pcf->runLoop();
      delay(5);

      for (int j = 0; j < 7; j++) {
        byte inPin = pinoHardware[j];
        if (outPin == inPin) continue;

        pcf->pinDirection(inPin, INPUT_PULLUP);
        pcf->runLoop();
        delayMicroseconds(100);

        if (pcf->readValue(inPin) == 0) {
          pinoX[posicao] = outPin;
          pinoY[posicao] = inPin;
          detectado = true;
          break;
        }
      }
      pcf->pinDirection(outPin, INPUT_PULLUP);
      if (detectado) break;
    }
  }

  // Calcula qual pino físico do teclado (1-7) foi detectado
  byte pinoFisico1 = 0, pinoFisico2 = 0;
  for (int i = 0; i < 7; i++) {
    if (pinoHardware[i] == pinoX[posicao]) pinoFisico1 = i + 1;
    if (pinoHardware[i] == pinoY[posicao]) pinoFisico2 = i + 1;
  }

  Serial.print(F(" -> Par: P"));
  Serial.print(pinoX[posicao]);
  Serial.print(F(" (Físico "));
  Serial.print(pinoFisico1);
  Serial.print(F(") e P"));
  Serial.print(pinoY[posicao]);
  Serial.print(F(" (Físico "));
  Serial.print(pinoFisico2);
  Serial.println(F(")"));

  Serial.println(F("Solte a tecla..."));
  while(true) {
    pcf->pinDirection(pinoX[posicao], OUTPUT);
    pcf->writeValue(pinoX[posicao], 0);
    pcf->pinDirection(pinoY[posicao], INPUT_PULLUP);
    pcf->runLoop();
    delay(5);
    if (pcf->readValue(pinoY[posicao]) == 1) break;
    delay(20);
  }
  delay(500);
}

void setup() {
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);

  Serial.println(F("\n========================================"));
  Serial.println(F("  KEYPAD TESTER - PropMaker EscapeRoom"));
  Serial.println(F("========================================"));

  // Inicializa Hardware
  hw.begin();

  // Inicializa PCF857x via IoAbstraction
  Serial.print(F("Iniciando PCF857"));
  Serial.print(PCF_IS_8575 ? F("5") : F("4"));
  Serial.print(F(" em 0x"));
  Serial.println(PCF_ADDRESS, HEX);

  pcf = new PCF8574IoAbstraction(PCF_ADDRESS, IO_PIN_NOT_DEFINED, &Wire, PCF_IS_8575);
  pcf->runLoop();
  Serial.println(F("PCF857x OK (via IoAbstraction)"));

  Serial.println(F("\n=== DETECÇÃO DE PINAGEM ==="));
  Serial.println(F("Siga as instruções para cada tecla.\n"));

  for (int i = 0; i < 12; i++) {
    detectarParPinos(ordem3x4[i], i);
  }

  // === LÓGICA DE DEDUÇÃO ===
  byte L[4], C[3];

  // 1. L1: pino comum entre teclas '1' e '2'
  if (pinoX[0] == pinoX[1] || pinoX[0] == pinoY[1]) L[0] = pinoX[0];
  else L[0] = pinoY[0];

  // 2. L2: pino comum entre teclas '4' e '5'
  if (pinoX[3] == pinoX[4] || pinoX[3] == pinoY[4]) L[1] = pinoX[3];
  else L[1] = pinoY[3];

  // 3. L3: pino comum entre teclas '7' e '8'
  if (pinoX[6] == pinoX[7] || pinoX[6] == pinoY[7]) L[2] = pinoX[6];
  else L[2] = pinoY[6];

  // 4. L4: pino comum entre teclas '*' e '0'
  if (pinoX[9] == pinoX[10] || pinoX[9] == pinoY[10]) L[3] = pinoX[9];
  else L[3] = pinoY[9];

  // 5. Colunas: pino de cada tecla que NÃO é a linha
  C[0] = (pinoX[0] == L[0]) ? pinoY[0] : pinoX[0];  // Coluna 1 (tecla '1')
  C[1] = (pinoX[1] == L[0]) ? pinoY[1] : pinoX[1];  // Coluna 2 (tecla '2')
  C[2] = (pinoX[2] == L[0]) ? pinoY[2] : pinoX[2];  // Coluna 3 (tecla '3')

  Serial.println(F("\n========================================"));
  Serial.println(F("  RESULTADO - Copie para seu sketch:"));
  Serial.println(F("========================================"));

  Serial.println(F("\n// Pinagem detectada:"));
  Serial.print(F("byte rowPins[4] = {"));
  Serial.print(L[0]); Serial.print(F(", "));
  Serial.print(L[1]); Serial.print(F(", "));
  Serial.print(L[2]); Serial.print(F(", "));
  Serial.print(L[3]);
  Serial.println(F("};"));

  Serial.print(F("byte colPins[3] = {"));
  Serial.print(C[0]); Serial.print(F(", "));
  Serial.print(C[1]); Serial.print(F(", "));
  Serial.print(C[2]);
  Serial.println(F("};"));

  Serial.println(F("\n// Uso com KeyboardManager (IoAbstraction):"));
  Serial.println(F("MAKE_KEYBOARD_LAYOUT_3X4(keyboardLayout);"));
  Serial.println(F("MatrixKeyboardManager keypadManager;"));
  Serial.println(F("keypadManager.initialise(hw.portExpander(), &keyboardLayout, listener, false);"));

  // ========================================================================
  // MAPEAMENTO DE PINOS FÍSICOS DO TECLADO
  // ========================================================================
  Serial.println(F("\n========================================"));
  Serial.println(F("  MAPEAMENTO: Pino Físico -> Função"));
  Serial.println(F("========================================"));
  Serial.println(F("\nPinos do teclado (esquerda para direita):\n"));

  // Determina pino físico de cada linha (L1-L4) e coluna (C1-C3)
  byte pinoFisicoL[4];  // Pino físico (1-7) de cada linha
  byte pinoFisicoC[3];  // Pino físico (1-7) de cada coluna

  // Linhas: pinos comuns entre teclas da mesma linha
  byte comum12 = (pinoX[0] == pinoX[1] || pinoX[0] == pinoY[1]) ? pinoX[0] : pinoY[0];
  byte comum45 = (pinoX[3] == pinoX[4] || pinoX[3] == pinoY[4]) ? pinoX[3] : pinoY[3];
  byte comum78 = (pinoX[6] == pinoX[7] || pinoX[6] == pinoY[7]) ? pinoX[6] : pinoY[6];
  byte comumStar0 = (pinoX[9] == pinoX[10] || pinoX[9] == pinoY[10]) ? pinoX[9] : pinoY[9];

  // Mapeia pinos físicos das linhas
  for (int i = 0; i < 7; i++) {
    pinoFisicoL[0] = (pinoHardware[i] == comum12) ? i + 1 : pinoFisicoL[0];
    pinoFisicoL[1] = (pinoHardware[i] == comum45) ? i + 1 : pinoFisicoL[1];
    pinoFisicoL[2] = (pinoHardware[i] == comum78) ? i + 1 : pinoFisicoL[2];
    pinoFisicoL[3] = (pinoHardware[i] == comumStar0) ? i + 1 : pinoFisicoL[3];
  }

  // Colunas: pino de cada tecla que NÃO é a linha L1
  byte col1Pin = (pinoX[0] == comum12) ? pinoY[0] : pinoX[0];
  byte col2Pin = (pinoX[1] == comum12) ? pinoY[1] : pinoX[1];
  byte col3Pin = (pinoX[2] == comum12) ? pinoY[2] : pinoX[2];

  for (int i = 0; i < 7; i++) {
    pinoFisicoC[0] = (pinoHardware[i] == col1Pin) ? i + 1 : pinoFisicoC[0];
    pinoFisicoC[1] = (pinoHardware[i] == col2Pin) ? i + 1 : pinoFisicoC[1];
    pinoFisicoC[2] = (pinoHardware[i] == col3Pin) ? i + 1 : pinoFisicoC[2];
  }

  // Exibe tabela de mapeamento
  Serial.println(F("  Pino | PCF  | Função"));
  Serial.println(F("  -----+------+--------"));
  for (int i = 0; i < 7; i++) {
    byte pinoPcf = pinoHardware[i];
    byte linha = 0, col = 0;
    for (int l = 0; l < 4; l++) if (pinoFisicoL[l] == i + 1) linha = l + 1;
    for (int c = 0; c < 3; c++) if (pinoFisicoC[c] == i + 1) col = c + 1;

    Serial.print(F("    "));
    Serial.print(i + 1);
    Serial.print(F("  | P"));
    Serial.print(pinoPcf);
    Serial.print(F("  | "));
    if (linha > 0) {
      Serial.print(F("Linha "));
      Serial.print(linha);
    } else if (col > 0) {
      Serial.print(F("Col "));
      Serial.print(col);
    } else {
      Serial.print(F("-"));
    }
    Serial.println();
  }

  // Diagrama de conexões
  Serial.println(F("\n  Conexões Teclado -> PCF:"));
  for (int i = 0; i < 7; i++) {
    byte pinoPcf = pinoHardware[i];
    byte linha = 0, col = 0;
    for (int l = 0; l < 4; l++) if (pinoFisicoL[l] == i + 1) linha = l + 1;
    for (int c = 0; c < 3; c++) if (pinoFisicoC[c] == i + 1) col = c + 1;

    Serial.print(F("  Pino "));
    Serial.print(i + 1);
    Serial.print(F(" -> P"));
    Serial.print(pinoPcf);
    Serial.print(F(" ("));
    if (linha > 0) {
      Serial.print(F("L"));
      Serial.print(linha);
    } else if (col > 0) {
      Serial.print(F("C"));
      Serial.print(col);
    }
    Serial.println(F(")"));
  }

  Serial.println(F("\n========================================"));
  Serial.println(F("  Teste de validação:"));
  Serial.println(F("========================================"));
  Serial.println(F("Pressione teclas para validar (ou reset para sair):\n"));
}

void loop() {
  loopHA();
  static MatrixKeyboardManager* keypadManager = nullptr;
  static KeyboardLayout* keypadLayout = nullptr;
  static bool tested = false;

  if (!tested) {
    // Reconstroi pinagem detectada
    byte L[4], C[3];
    if (pinoX[0] == pinoX[1] || pinoX[0] == pinoY[1]) L[0] = pinoX[0];
    else L[0] = pinoY[0];
    if (pinoX[3] == pinoX[4] || pinoX[3] == pinoY[4]) L[1] = pinoX[3];
    else L[1] = pinoY[3];
    if (pinoX[6] == pinoX[7] || pinoX[6] == pinoY[7]) L[2] = pinoX[6];
    else L[2] = pinoY[6];
    if (pinoX[9] == pinoX[10] || pinoX[9] == pinoY[10]) L[3] = pinoX[9];
    else L[3] = pinoY[9];
    C[0] = (pinoX[0] == L[0]) ? pinoY[0] : pinoX[0];
    C[1] = (pinoX[1] == L[0]) ? pinoY[1] : pinoX[1];
    C[2] = (pinoX[2] == L[0]) ? pinoY[2] : pinoX[2];

    // Cria layout do teclado
    static const char keymap[] PROGMEM = "123456789*0#";
    keypadLayout = new KeyboardLayout(4, 3, keymap);
    for (int i = 0; i < 4; i++) keypadLayout->setRowPin(i, L[i]);
    for (int i = 0; i < 3; i++) keypadLayout->setColPin(i, C[i]);

    keypadManager = new MatrixKeyboardManager();
    keypadManager->initialise(pcf, keypadLayout, &listener, false);
    tested = true;
  }

  taskManager.runLoop();

  if (listener.lastKey != 0) {
    Serial.print(F("Tecla pressionada: "));
    Serial.println(listener.lastKey);
    listener.lastKey = 0;
  }
  delay(50);
}
