/*
  KeypadTester - Pinout Detector for Matrix Keypads

  This example automatically identifies the pinout of matrix keypads
  connected via PCF8574 or PCF8575.

  USAGE:
  1. Connect the keypad pins to the PCF pins (in the keypad's physical order)
  2. Define in the code the physical PCF pinout connected to the keypad
  3. Run it and press the keys when prompted
  4. The sketch returns:
     - rowPins[] and colPins[] for use with KeyboardManager (IoAbstraction)
     - Mapping of keypad physical pin -> Row/Column

  COMPATIBILITY:
  - PCF8574 (8 I/O pins): Uses 7 of the 8 available pins
  - PCF8575 (16 I/O pins): Uses P00-P06 (7 pins)

  For use with the propmaker_escaperoom library, copy the rowPins[] and
  colPins[] arrays into your sketch and use them with MatrixKeyboardManager.
*/

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

#include <KeyboardManager.h>
#include <TaskManager.h>

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
// CONFIGURATION: Define the PCF857x pins connected to the keypad
// ============================================================================

// PCF857x I2C address (0x20-0x27 for PCF8574, 0x38-0x3F for PCF8574A)
#define PCF_ADDRESS 0x20

// If true, uses the PCF8575 (16 pins). If false, uses the PCF8574 (8 pins)
#define PCF_IS_8575 true

// Physical pinout connected to the keypad (7 pins of the PCF)
// For PCF8575: use 0-15 (P00-P15)
// For PCF8574: use 0-7 (P0-P7)
// ORDER: Connect in the keypad's physical order (from left to right)
const byte pinoHardware[7] = {0, 1, 2, 3, 4, 5, 6};  // P00 to P06 on the PCF8575

// Key order for a standard 3x4 keypad
const char ordem3x4[12] = {'1','2','3','4','5','6','7','8','9','*','0','#'};

// ============================================================================

byte pinoX[12];  // Stores one of the detected pins for each key
byte pinoY[12];  // Stores the other detected pin for each key

PCF8574IoAbstraction* pcf = nullptr;

// Global listener for the validation test
class SimpleKeyListener : public KeyboardListener {
public:
    char lastKey = 0;
    void keyPressed(char key, bool held) override { if (!held) lastKey = key; }
    void keyReleased(char key) override { lastKey = 0; }
};
static SimpleKeyListener listener;

void detectarParPinos(char caractere, int posicao) {
  Serial.print(F("Press and HOLD the key '"));
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

  // Computes which physical keypad pin (1-7) was detected
  byte pinoFisico1 = 0, pinoFisico2 = 0;
  for (int i = 0; i < 7; i++) {
    if (pinoHardware[i] == pinoX[posicao]) pinoFisico1 = i + 1;
    if (pinoHardware[i] == pinoY[posicao]) pinoFisico2 = i + 1;
  }

  Serial.print(F(" -> Pair: P"));
  Serial.print(pinoX[posicao]);
  Serial.print(F(" (Physical "));
  Serial.print(pinoFisico1);
  Serial.print(F(") and P"));
  Serial.print(pinoY[posicao]);
  Serial.print(F(" (Physical "));
  Serial.print(pinoFisico2);
  Serial.println(F(")"));

  Serial.println(F("Release the key..."));
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

  // Initializes Hardware
  hw.begin();

  // Initializes PCF857x via IoAbstraction
  Serial.print(F("Starting PCF857"));
  Serial.print(PCF_IS_8575 ? F("5") : F("4"));
  Serial.print(F(" at 0x"));
  Serial.println(PCF_ADDRESS, HEX);

  pcf = new PCF8574IoAbstraction(PCF_ADDRESS, IO_PIN_NOT_DEFINED, &Wire, PCF_IS_8575);
  pcf->runLoop();
  Serial.println(F("PCF857x OK (via IoAbstraction)"));

  Serial.println(F("\n=== PINOUT DETECTION ==="));
  Serial.println(F("Follow the instructions for each key.\n"));

  for (int i = 0; i < 12; i++) {
    detectarParPinos(ordem3x4[i], i);
  }

  // === DEDUCTION LOGIC ===
  byte L[4], C[3];

  // 1. L1: common pin between keys '1' and '2'
  if (pinoX[0] == pinoX[1] || pinoX[0] == pinoY[1]) L[0] = pinoX[0];
  else L[0] = pinoY[0];

  // 2. L2: common pin between keys '4' and '5'
  if (pinoX[3] == pinoX[4] || pinoX[3] == pinoY[4]) L[1] = pinoX[3];
  else L[1] = pinoY[3];

  // 3. L3: common pin between keys '7' and '8'
  if (pinoX[6] == pinoX[7] || pinoX[6] == pinoY[7]) L[2] = pinoX[6];
  else L[2] = pinoY[6];

  // 4. L4: common pin between keys '*' and '0'
  if (pinoX[9] == pinoX[10] || pinoX[9] == pinoY[10]) L[3] = pinoX[9];
  else L[3] = pinoY[9];

  // 5. Columns: the pin of each key that is NOT the row
  C[0] = (pinoX[0] == L[0]) ? pinoY[0] : pinoX[0];  // Column 1 (key '1')
  C[1] = (pinoX[1] == L[0]) ? pinoY[1] : pinoX[1];  // Column 2 (key '2')
  C[2] = (pinoX[2] == L[0]) ? pinoY[2] : pinoX[2];  // Column 3 (key '3')

  Serial.println(F("\n========================================"));
  Serial.println(F("  RESULT - Copy into your sketch:"));
  Serial.println(F("========================================"));

  Serial.println(F("\n// Detected pinout:"));
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

  Serial.println(F("\n// Usage with KeyboardManager (IoAbstraction):"));
  Serial.println(F("MAKE_KEYBOARD_LAYOUT_3X4(keyboardLayout);"));
  Serial.println(F("MatrixKeyboardManager keypadManager;"));
  Serial.println(F("keypadManager.initialise(hw.portExpander(), &keyboardLayout, listener, false);"));

  // ========================================================================
  // PHYSICAL KEYPAD PIN MAPPING
  // ========================================================================
  Serial.println(F("\n========================================"));
  Serial.println(F("  MAPPING: Physical Pin -> Function"));
  Serial.println(F("========================================"));
  Serial.println(F("\nKeypad pins (left to right):\n"));

  // Determines the physical pin of each row (L1-L4) and column (C1-C3)
  byte pinoFisicoL[4];  // Physical pin (1-7) of each row
  byte pinoFisicoC[3];  // Physical pin (1-7) of each column

  // Rows: pins common between keys of the same row
  byte comum12 = (pinoX[0] == pinoX[1] || pinoX[0] == pinoY[1]) ? pinoX[0] : pinoY[0];
  byte comum45 = (pinoX[3] == pinoX[4] || pinoX[3] == pinoY[4]) ? pinoX[3] : pinoY[3];
  byte comum78 = (pinoX[6] == pinoX[7] || pinoX[6] == pinoY[7]) ? pinoX[6] : pinoY[6];
  byte comumStar0 = (pinoX[9] == pinoX[10] || pinoX[9] == pinoY[10]) ? pinoX[9] : pinoY[9];

  // Maps the physical pins of the rows
  for (int i = 0; i < 7; i++) {
    pinoFisicoL[0] = (pinoHardware[i] == comum12) ? i + 1 : pinoFisicoL[0];
    pinoFisicoL[1] = (pinoHardware[i] == comum45) ? i + 1 : pinoFisicoL[1];
    pinoFisicoL[2] = (pinoHardware[i] == comum78) ? i + 1 : pinoFisicoL[2];
    pinoFisicoL[3] = (pinoHardware[i] == comumStar0) ? i + 1 : pinoFisicoL[3];
  }

  // Columns: the pin of each key that is NOT row L1
  byte col1Pin = (pinoX[0] == comum12) ? pinoY[0] : pinoX[0];
  byte col2Pin = (pinoX[1] == comum12) ? pinoY[1] : pinoX[1];
  byte col3Pin = (pinoX[2] == comum12) ? pinoY[2] : pinoX[2];

  for (int i = 0; i < 7; i++) {
    pinoFisicoC[0] = (pinoHardware[i] == col1Pin) ? i + 1 : pinoFisicoC[0];
    pinoFisicoC[1] = (pinoHardware[i] == col2Pin) ? i + 1 : pinoFisicoC[1];
    pinoFisicoC[2] = (pinoHardware[i] == col3Pin) ? i + 1 : pinoFisicoC[2];
  }

  // Displays the mapping table
  Serial.println(F("  Pin  | PCF  | Function"));
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
      Serial.print(F("Row "));
      Serial.print(linha);
    } else if (col > 0) {
      Serial.print(F("Col "));
      Serial.print(col);
    } else {
      Serial.print(F("-"));
    }
    Serial.println();
  }

  // Connection diagram
  Serial.println(F("\n  Keypad -> PCF Connections:"));
  for (int i = 0; i < 7; i++) {
    byte pinoPcf = pinoHardware[i];
    byte linha = 0, col = 0;
    for (int l = 0; l < 4; l++) if (pinoFisicoL[l] == i + 1) linha = l + 1;
    for (int c = 0; c < 3; c++) if (pinoFisicoC[c] == i + 1) col = c + 1;

    Serial.print(F("  Pin "));
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
  Serial.println(F("  Validation test:"));
  Serial.println(F("========================================"));
  Serial.println(F("Press keys to validate (or reset to exit):\n"));
}

void loop() {
  loopHA();
  static MatrixKeyboardManager* keypadManager = nullptr;
  static KeyboardLayout* keypadLayout = nullptr;
  static bool tested = false;

  if (!tested) {
    // Rebuilds the detected pinout
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

    // Creates the keypad layout
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
    Serial.print(F("Key pressed: "));
    Serial.println(listener.lastKey);
    listener.lastKey = 0;
  }
  delay(50);
}
