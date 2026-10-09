/**
 * VintageShortwaveMorseRadio - Versão PropMaker RP2040
 * Autor Original: Nicolau dos Brinquedos (2018)
 * Adaptação: PropMaker EscapeRoom Library
 *
 * Hardware: Adafruit PropMaker Feather RP2040
 * - Entradas via PCF8575 (I2C): transformador e válvulas
 * - Áudio: I2S com geração de tons Morse em tempo real
 * - LEDs: NeoPixel externos (valves, dial, morse, power)
 * - SD Card: mensagem.txt em código Morse
 * - Rotary Encoder: sistema de sintonia (opcional)
 *
 * Baseado no projeto original para Arduino Nano (Escape Junior - Henrique Sitchin)
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
// CONSTANTES E CONFIGURAÇÃO
// ============================================================================

// Endereço I2C do PCF8575
#define PCF_ADDRESS         0x20

// Pinos no PCF8575 (0-15)
#define PIN_TRAFO           0   // Transformador (ativo LOW)
#define PIN_VALVE1          1   // Válvula 1
#define PIN_VALVE2          2   // Válvula 2
#define PIN_VALVE3          3   // Válvula 3 (ativa dial FM)
#define PIN_VALVE4          7   // Válvula 4 (FINAL - inicia transmissão Morse)
#define PIN_ENCODER_A       4   // Rotary Encoder A (opcional)
#define PIN_ENCODER_B       5   // Rotary Encoder B (opcional)
#define PIN_ENCODER_BTN     6   // Botão do encoder (opcional)

// Índices NeoPixel externos (0-11 = 12 LEDs para dial circular)
#define NEO_POWER           12  // LED indicador de power
#define NEO_MORSE           13  // LED indicador Morse
#define NEO_VALVE1          14  // LED válvula 1
#define NEO_VALVE2          15  // LED válvula 2
#define NEO_VALVE3          16  // LED válvula 3
#define NEO_VALVE4          18  // LED válvula 4 (FINAL)
#define NEO_TRAFO_HUM       17  // LED simulação "hum" do trafo (pulsante)
#define NEO_DIAL_START      0   // Início do dial circular (12 LEDs: 0-11)

// Parâmetros Morse
#define MORSE_DOT_MS        500     // Duração do ponto (ms)
#define MORSE_DASH_MS       1500    // Duração do traço (ms)
#define MORSE_END_LETTER_MS 2000    // Pausa entre letras
#define MORSE_SPACE_MS      3500    // Pausa entre palavras
#define MORSE_FREQ_HZ       432     // Frequência do tom
#define MORSE_VOLUME        128     // Volume (0-255)
#define MORSE_REPEAT        5       // Repetições da mensagem

// Configuração SD Card
#define MESSAGE_FILE        "/mensagem.txt"
#define MAX_MESSAGE_LEN     256

// ============================================================================
// CONFIGURAÇÃO DO DIAL FM (ARRAY DE VOLUMES LOGARÍTMICO)
// ============================================================================

// Configuração das 9 estações
#define FM_NUM_STATIONS     9

// Array de volumes - escala logarítmica de 10 a 80 e volta a 10
// Índices 0-39: sobe de 10 a 80 (log)
// Índices 40-79: desce de 80 a 10 (log)
// Total: 80 passos por ciclo completo
#define FM_VOLUME_STEPS     80
#define FM_VOLUME_LOW       25      // ~10% de 255
#define FM_VOLUME_HIGH      204     // ~80% de 255

// Array de volumes pré-calculados (escala logarítmica)
const uint8_t fmVolumeCurve[FM_VOLUME_STEPS] = {
    // Subindo: 10% -> 80% (índices 0-39) - curva logarítmica
    25, 26, 27, 28, 30, 31, 33, 34, 36, 37,      // 0-9
    39, 41, 43, 45, 47, 49, 51, 53, 56, 58,      // 10-19
    61, 64, 67, 70, 73, 76, 80, 84, 88, 92,      // 20-29
    96, 101, 106, 111, 116, 122, 128, 134, 140, 147, // 30-39 (chega em ~80%)
    // Descendo: 80% -> 10% (índices 40-79) - espelhado
    147, 140, 134, 128, 122, 116, 111, 106, 101, 96, // 40-49
    92, 88, 84, 80, 76, 73, 70, 67, 64, 61,      // 50-59
    58, 56, 53, 51, 49, 47, 45, 43, 41, 39,      // 60-69
    37, 36, 34, 33, 31, 30, 28, 27, 26, 25       // 70-79 (volta a ~10%)
};

// Pontos de troca de estação (quando índice cruza estes thresholds)
// Cada estação ocupa uma faixa do array (aproximadamente)
#define FM_STATION_THRESHOLD_1  9   // Station 1: índices 0-9 e 70-79
#define FM_STATION_THRESHOLD_2  18  // Station 2: índices 10-18 e 61-69
#define FM_STATION_THRESHOLD_3  27  // Station 3
#define FM_STATION_THRESHOLD_4  36  // Station 4
#define FM_STATION_THRESHOLD_5  45  // Station 5 (centro do array)
#define FM_STATION_THRESHOLD_6  54  // Station 6
#define FM_STATION_THRESHOLD_7  63  // Station 7
#define FM_STATION_THRESHOLD_8  72  // Station 8
// Station 9: índices restantes

// Mapeia índice do array para estação (0-8)
int indexToStation(int index) {
    if (index <= FM_STATION_THRESHOLD_1) return 0;                    // Station 1
    if (index <= FM_STATION_THRESHOLD_2) return 1;                    // Station 2
    if (index <= FM_STATION_THRESHOLD_3) return 2;                    // Station 3
    if (index <= FM_STATION_THRESHOLD_4) return 3;                    // Station 4
    if (index <= FM_STATION_THRESHOLD_5) return 4;                    // Station 5
    if (index <= FM_STATION_THRESHOLD_6) return 5;                    // Station 6
    if (index <= FM_STATION_THRESHOLD_7) return 6;                    // Station 7
    if (index <= FM_STATION_THRESHOLD_8) return 7;                    // Station 8
    return 8;                                                         // Station 9
}

// Nomes dos arquivos das estações
const char* fmStationFiles[FM_NUM_STATIONS] = {
    "/station1.mp3", "/station2.mp3", "/station3.mp3",
    "/station4.mp3", "/station5.mp3", "/station6.mp3",
    "/station7.mp3", "/station8.mp3", "/station9.mp3"
};

// Flags de disponibilidade por estação (SD tem prioridade sobre LittleFS)
bool stationAvailableSD[FM_NUM_STATIONS] = {false};
bool stationAvailableLFS[FM_NUM_STATIONS] = {false};

/**
 * Escaneia arquivos station*.mp3 no SD Card e LittleFS.
 * Marca quais estações estão disponíveis em cada sistema de arquivos.
 * SD Card tem prioridade sobre LittleFS quando arquivo existe nos dois.
 */
void scanStationFiles() {
    Serial.println(F("[FM] Escaneando arquivos station*.mp3..."));

    // Verifica disponibilidade em cada estação
    for (int i = 0; i < FM_NUM_STATIONS; i++) {
        const char* filename = fmStationFiles[i];

        // Verifica SD Card primeiro (prioridade)
        if (hw.isSDMounted() && SD.exists(filename)) {
            stationAvailableSD[i] = true;
            Serial.printf("[FM] Found %s (SD Card)\n", filename);
        } else {
            stationAvailableSD[i] = false;
        }

        // Verifica LittleFS
        if (hw.isLittleFSMounted() && LittleFS.exists(filename)) {
            stationAvailableLFS[i] = true;
            Serial.printf("[FM] Found %s (LittleFS)\n", filename);
        } else {
            stationAvailableLFS[i] = false;
        }

        // Aviso se não encontrou em nenhum
        if (!stationAvailableSD[i] && !stationAvailableLFS[i]) {
            Serial.printf("[FM] AVISO: %s nao encontrado em nenhum sistema de arquivos!\n", filename);
        }
    }

    Serial.println(F("[FM] Scan completo. Prioridade: SD Card > LittleFS"));
}

/**
 * Toca arquivo de estação com prioridade SD Card > LittleFS.
 * Retorna true se conseguiu tocar, false se arquivo não existe.
 */
bool playStationFile(int stationIndex) {
    if (stationIndex < 0 || stationIndex >= FM_NUM_STATIONS) return false;

    const char* filename = fmStationFiles[stationIndex];

    // Prioridade 1: SD Card
    if (stationAvailableSD[stationIndex]) {
        Serial.printf("[FM] Tocando %s (SD Card)\n", filename);
        return hw.sound().playLoop(filename, true);  // true = use SD
    }

    // Prioridade 2: LittleFS
    if (stationAvailableLFS[stationIndex]) {
        Serial.printf("[FM] Tocando %s (LittleFS)\n", filename);
        return hw.sound().playLoop(filename, false); // false = use LittleFS
    }

    // Não encontrou em nenhum
    Serial.printf("[FM] ERRO: %s nao disponivel!\n", filename);
    return false;
}

// Tabela de normalização de caracteres acentuados (ISO-8859-1 / Windows-1252)
// Mapeia caracteres especiais para seus equivalentes ASCII simples
// Usando valores hexadecimais para evitar problemas de encoding
struct CharMapping {
    unsigned char from;
    char to;
};

const CharMapping accentMap[] = {
    // Letras minúsculas acentuadas (valores ISO-8859-1)
    {0xE0, 'A'}, {0xE1, 'A'}, {0xE2, 'A'}, {0xE3, 'A'}, {0xE4, 'A'}, {0xE5, 'A'},  // àáâãäå
    {0xE8, 'E'}, {0xE9, 'E'}, {0xEA, 'E'}, {0xEB, 'E'},                           // èéêë
    {0xEC, 'I'}, {0xED, 'I'}, {0xEE, 'I'}, {0xEF, 'I'},                           // ìíîï
    {0xF2, 'O'}, {0xF3, 'O'}, {0xF4, 'O'}, {0xF5, 'O'}, {0xF6, 'O'},              // òóôõö
    {0xF9, 'U'}, {0xFA, 'U'}, {0xFB, 'U'}, {0xFC, 'U'},                           // ùúûü
    {0xE7, 'C'}, {0xF1, 'N'},                                                      // çñ
    // Letras maiúsculas acentuadas
    {0xC0, 'A'}, {0xC1, 'A'}, {0xC2, 'A'}, {0xC3, 'A'}, {0xC4, 'A'}, {0xC5, 'A'},  // ÀÁÂÃÄÅ
    {0xC8, 'E'}, {0xC9, 'E'}, {0xCA, 'E'}, {0xCB, 'E'},                           // ÈÉÊË
    {0xCC, 'I'}, {0xCD, 'I'}, {0xCE, 'I'}, {0xCF, 'I'},                           // ÌÍÎÏ
    {0xD2, 'O'}, {0xD3, 'O'}, {0xD4, 'O'}, {0xD5, 'O'}, {0xD6, 'O'},              // ÒÓÔÕÖ
    {0xD9, 'U'}, {0xDA, 'U'}, {0xDB, 'U'}, {0xDC, 'U'},                           // ÙÚÛÜ
    {0xC7, 'C'}, {0xD1, 'N'},                                                      // ÇÑ
    // Símbolos especiais
    {0xB0, 'O'}, {0xBA, 'O'}, {0xAA, 'A'},                                         // °ºª
    {0, 0}  // Terminador
};

/**
 * Normaliza texto para código Morse:
 * - Converte acentos para caracteres ASCII simples
 * - Remove pontuação e caracteres inválidos
 * - Converte para maiúsculas
 * - Mantém apenas: A-Z, 0-9, espaço
 */
void normalizeText(char* buffer) {
    if (!buffer || buffer[0] == '\0') return;

    int len = strlen(buffer);
    int writePos = 0;

    for (int readPos = 0; readPos < len && readPos < MAX_MESSAGE_LEN - 1; readPos++) {
        char c = buffer[readPos];

        // Ignora caracteres de controle (exceto espaço)
        if (c < 32 && c != ' ') continue;

        // Converte para maiúscula se for minúscula sem acento
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }

        // Converte acentos usando tabela
        for (int i = 0; accentMap[i].from != 0; i++) {
            if (c == accentMap[i].from) {
                c = accentMap[i].to;
                break;
            }
        }

        // Aceita: letras A-Z, números 0-9, espaço
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ') {
            buffer[writePos++] = c;
        }
        // Silenciosamente descarta outros caracteres (pontuação, símbolos, etc.)
    }

    buffer[writePos] = '\0';
}

// Cores NeoPixel
#define COLOR_OFF           0
#define COLOR_POWER         Adafruit_NeoPixel::Color(255, 255, 255)  // Branco
#define COLOR_VALVE         Adafruit_NeoPixel::Color(0, 100, 255)    // Azul
#define COLOR_MORSE         Adafruit_NeoPixel::Color(255, 255, 255)  // Branco
#define COLOR_DIAL_FAR      Adafruit_NeoPixel::Color(255, 0, 0)      // Vermelho
#define COLOR_DIAL_NEAR     Adafruit_NeoPixel::Color(255, 255, 0)    // Amarelo
#define COLOR_DIAL_PEAK     Adafruit_NeoPixel::Color(0, 255, 0)      // Verde

// Tabela Morse (índice: caractere - '0')
const char* morseTable[] = {
    // Números 0-9
    "-----", ".----", "..---", "...--", "....-",  // 0-4
    ".....", "-....", "--...", "---..", "----.",  // 5-9
    // Símbolos : ; < = > ? @
    "---...", "-.-.-.", "*", "-...-", "*", "..--..", ".--.-.",
    // Letras A-Z
    ".-", "-...", "-.-.", "-..", ".", "..-.",      // A-F
    "--.", "....", "..", ".---", "-.-", ".-..",     // G-L
    "--", "-.", "---", ".--.", "--.-", ".-.",     // M-R
    "...", "-", "..-", "...-", ".--", "-..-",      // S-X
    "-.--", "--.."                                   // Y-Z
};

// ============================================================================
// VARIÁVEIS GLOBAIS
// ============================================================================

// Estado do jogo
bool valve1flag = false;
bool valve2flag = false;
bool valve3flag = false;
bool valve4flag = false;  // Válvula FINAL - inicia transmissão Morse
bool trafoflag = false;
bool gameover = false;

// Mensagem Morse
char messageText[MAX_MESSAGE_LEN] = "SOS";
bool sdInitSuccess = false;

// Sintonia (sistema opcional de dial)
enum TuningState {
    TUNING_INACTIVE,
    TUNING_ACTIVE,
    TUNING_LOCKED,
    TUNING_COMPLETE
};
TuningState tuningState = TUNING_INACTIVE;
int dialPosition = 0;           // Posição 0-19
int currentStation = 1;         // Estação 1-9

// Sistema de Sintonia FM (Array de Volumes)
int volumeIndex = 0;                      // Índice atual no array de volumes (0-FM_VOLUME_STEPS-1)
int lastStationFromIndex = -1;            // Última estação detectada pelo índice
int lastEncoderState = 0;               // Estado anterior do encoder
uint32_t lastFreqReportTime = 0;        // Para debug
int currentPlayingStation = -1;         // Índice da estação atualmente tocando (-1 = nenhuma)
bool fmDialActive = false;              // Sistema de dial está ativo?

// ============================================================================
// CLASSE DO PUZZLE
// ============================================================================

class VintageRadioPuzzle : public PuzzleBase {
public:
    VintageRadioPuzzle() : PuzzleBase(30) {}  // ID 30 = Vintage Radio

    void setupHardware() override {
        Serial.println(F("[VintageRadio] Configurando hardware..."));

        // Configura NeoPixel externos
        if (hw.pixelsExternal().numPixels() > 0) {
            hw.pixelsExternal().clear();
            hw.pixelsExternal().show();
            Serial.println(F("[VintageRadio] NeoPixel externos configurados"));
        }

        // Carrega mensagem do arquivo
        loadMessage();

        // Estado inicial
        resetGame();

        Serial.println(F("[VintageRadio] Setup completo"));
        setState(PuzzleState::Running);
    }

    void update() override {
        // Mantém áudio ativo (para tons Morse em tempo real)
        hw.sound().loop();

        if (getState() != PuzzleState::Running) return;

        if (!gameover) {
            // Verifica transformador (hard reset se removido durante jogo)
            if (!checkTrafo() && trafoflag) {
                Serial.println(F("[VintageRadio] HARD RESET - Trafo removido"));
                resetGame();
                return;
            }

            // Verifica conexão do transformador
            if (checkTrafoConnected()) {
                if (!trafoflag) {
                    // Primeira conexão do trafo
                    Serial.println(F("[VintageRadio] TRAFO Conectado"));
                    Serial.println(F("[VintageRadio] POWER ON"));
                    trafoflag = true;
                    setPowerLed(true);
                    startTrafoHum();
                }

                // Verifica válvulas (inclui ativação do dial FM na válvula 3)
                checkValves();

                // Processa dial FM se ativo (3ª válvula conectada)
                if (valve3flag) {
                    processFmDial();
                }

                // Verifica se Válvula 4 (FINAL) foi conectada - inicia transmissão Morse
                // A válvula 4 só funciona se as outras 3 estiverem conectadas
                if (trafoflag && valve1flag && valve2flag && valve3flag && valve4flag) {
                    // Para o áudio FM antes de transmitir Morse
                    stopFmAudio();

                    // Válvula final conectada - inicia transmissão
                    Serial.println(F("[VintageRadio] Valvula 4 (FINAL) conectada - transmitindo mensagem Morse"));
                    transmitMessage();
                    gameover = true;
                    setState(PuzzleState::Solved);
                }
            }
        } else {
            // Jogo finalizado - aguarda reset via remoção do trafo
            if (!checkTrafoConnected()) {
                resetGame();
            }
        }
    }

private:
    // =======================================================================
    // FUNÇÕES DE HARDWARE
    // =======================================================================

    bool checkTrafoConnected() {
        if (!hw.hasExpander()) return false;
        // Ativo LOW (INPUT_PULLUP)
        return !hw.portExpander()->digitalRead(PIN_TRAFO);
    }

    bool checkTrafo() {
        if (trafoflag) {
            return checkTrafoConnected();
        }
        return false;
    }

    void checkValves() {
        if (!hw.hasExpander()) return;

        auto* pcf = hw.portExpander();

        // Válvula 1
        bool valve1Connected = !pcf->digitalRead(PIN_VALVE1);
        if (valve1Connected && trafoflag && !valve1flag) {
            Serial.println(F("[VintageRadio] Valvula1 Conectada"));
            Serial.println(F("[VintageRadio] Morse Dot"));
            fadeLed(NEO_VALVE1, COLOR_VALVE);
            blinkMorsePattern(true);  // DOT pattern
            valve1flag = true;
        } else if (!valve1Connected) {
            valve1flag = false;
            setLight(NEO_VALVE1, COLOR_OFF);
        }

        // Válvula 2
        bool valve2Connected = !pcf->digitalRead(PIN_VALVE2);
        if (valve2Connected && trafoflag && !valve2flag) {
            Serial.println(F("[VintageRadio] Valvula2 Conectada"));
            Serial.println(F("[VintageRadio] Morse Dash"));
            fadeLed(NEO_VALVE2, COLOR_VALVE);
            blinkMorsePattern(false);  // DASH pattern
            valve2flag = true;
        } else if (!valve2Connected) {
            valve2flag = false;
            setLight(NEO_VALVE2, COLOR_OFF);
        }

        // Válvula 3 - ativa sistema de sintonia FM
        bool valve3Connected = !pcf->digitalRead(PIN_VALVE3);
        if (valve3Connected && trafoflag && !valve3flag) {
            Serial.println(F("[VintageRadio] Valvula3 Conectada"));
            Serial.println(F("[VintageRadio] Dial FM ativado - array de volumes"));
            fadeLed(NEO_VALVE3, COLOR_VALVE);
            valve3flag = true;

            // Escanear arquivos station*.mp3 disponíveis
            scanStationFiles();

            // Inicializa sistema FM no índice 0 (volume mínimo)
            volumeIndex = 0;
            lastStationFromIndex = -1;
            fmDialActive = true;
            currentPlayingStation = -1;

            Serial.println(F("[FM Dial] Iniciando no índice 0 (volume mínimo)"));
            Serial.println(F("[FM Dial] Use encoder para percorrer o array de volumes"));
            Serial.println(F("[FM Dial] Prioridade: SD Card > LittleFS"));
        } else if (!valve3Connected) {
            if (valve3flag) {
                // Válvula 3 desconectada - para áudio FM
                Serial.println(F("[VintageRadio] Valvula3 removida - FM OFF"));
                stopFmAudio();
                fmDialActive = false;
                currentPlayingStation = -1;
            }
            valve3flag = false;
            setLight(NEO_VALVE3, COLOR_OFF);
        }

        // Válvula 4 - VÁLVULA FINAL (ativa transmissão Morse)
        bool valve4Connected = !pcf->digitalRead(PIN_VALVE4);
        if (valve4Connected && trafoflag && !valve4flag) {
            Serial.println(F("[VintageRadio] Valvula4 Conectada - VALVULA FINAL"));
            Serial.println(F("[VintageRadio] Interrompendo FM e iniciando transmissao Morse"));
            fadeLed(NEO_VALVE4, COLOR_VALVE);
            valve4flag = true;
        } else if (!valve4Connected) {
            valve4flag = false;
            setLight(NEO_VALVE4, COLOR_OFF);
        }
    }

    // =======================================================================
    // CONTROLE DE LEDs
    // =======================================================================

    void setLight(uint8_t index, uint32_t color) {
        if (hw.pixelsExternal().numPixels() > 0) {
            hw.pixelsExternal().setPixelColor(index, color);
            hw.pixelsExternal().show();
        }
    }

    void setPowerLed(bool on) {
        setLight(NEO_POWER, on ? COLOR_POWER : COLOR_OFF);
    }

    void fadeLed(uint8_t pixelIndex, uint32_t targetColor) {
        // Extrai componentes de cor
        uint8_t r = (targetColor >> 16) & 0xFF;
        uint8_t g = (targetColor >> 8) & 0xFF;
        uint8_t b = targetColor & 0xFF;

        // Fade-in gradual
        for (int i = 0; i <= 255; i += 5) {
            uint8_t cr = (r * i) / 255;
            uint8_t cg = (g * i) / 255;
            uint8_t cb = (b * i) / 255;
            setLight(pixelIndex, Adafruit_NeoPixel::Color(cr, cg, cb));
            delay(5);
        }
        setLight(pixelIndex, targetColor);
    }

    void blinkMorsePattern(bool isDot) {
        // Pisca LED Morse 3x
        int duration = isDot ? MORSE_DOT_MS : MORSE_DASH_MS;
        for (int i = 0; i < 3; i++) {
            setLight(NEO_MORSE, COLOR_MORSE);
            hw.sound().tone(MORSE_FREQ_HZ, MORSE_VOLUME);
            delay(duration);
            setLight(NEO_MORSE, COLOR_OFF);
            hw.sound().noTone();
            delay(duration);

            // Verifica hard reset durante piscada
            if (!checkTrafo()) {
                hw.sound().noTone();
                return;
            }
        }
    }

    void startTrafoHum() {
        // Simulação visual do "hum" do transformador
        // Usa um LED pulsante no índice NEO_TRAFO_HUM
        // Na prática, o som de hum seria um MP3 ou tom de baixa frequência
    }

    // =======================================================================
    // TRANSMISSÃO MORSE
    // =======================================================================

    void transmitMessage() {
        // Fade out do trafo hum
        for (int i = 25; i >= 0; i--) {
            setLight(NEO_TRAFO_HUM, Adafruit_NeoPixel::Color(i, i, i));
            delay(5);
        }
        setLight(NEO_TRAFO_HUM, COLOR_OFF);

        // Transmite mensagem MORSE_REPEAT vezes
        for (int r = 0; r < MORSE_REPEAT; r++) {
            if (!checkTrafo()) {
                Serial.println(F("[VintageRadio] Transmissão interrompida"));
                return;
            }
            Serial.printf("[VintageRadio] Transmitindo (repeticao %d/%d): %s\n",
                         r + 1, MORSE_REPEAT, messageText);
            morseCode(messageText);
            delay(MORSE_SPACE_MS);
        }
        Serial.println(F("[VintageRadio] Transmissao completa"));
    }

    void morseCode(const char* asciimsg) {
        // Copia mensagem (já normalizada em loadMessage)
        char msg[MAX_MESSAGE_LEN];
        strncpy(msg, asciimsg, MAX_MESSAGE_LEN - 1);
        msg[MAX_MESSAGE_LEN - 1] = '\0';

        int tamanho = strlen(msg);
        int index = 0;

        while (index < tamanho) {
            // Verifica hard reset
            if (!checkTrafo()) {
                gameover = true;
                hw.sound().noTone();
                return;
            }

            char letra = msg[index];
            Serial.print(letra);
            Serial.print(" ");

            // Espaço entre palavras
            if (letra == ' ') {
                delay(MORSE_SPACE_MS);
                Serial.print(" ");
                index++;
                continue;
            }

            // Caractere válido (0-9, A-Z)
            if (letra >= '0' && letra <= 'Z') {
                const char* morseLetter = morseTable[letra - '0'];
                int tamMorse = strlen(morseLetter);

                for (int indMorse = 0; indMorse < tamMorse; indMorse++) {
                    // Verifica hard reset durante símbolo
                    if (!checkTrafo()) {
                        gameover = true;
                        hw.sound().noTone();
                        return;
                    }

                    // Acende LED e inicia tom
                    setLight(NEO_MORSE, COLOR_MORSE);
                    hw.sound().tone(MORSE_FREQ_HZ, MORSE_VOLUME);

                    char dotDash = morseLetter[indMorse];
                    Serial.print(dotDash);

                    if (dotDash == '.') {
                        delay(MORSE_DOT_MS);
                    } else if (dotDash == '-') {
                        delay(MORSE_DASH_MS);
                    }

                    // Apaga LED e para tom
                    setLight(NEO_MORSE, COLOR_OFF);
                    hw.sound().noTone();
                    delay(MORSE_DOT_MS);  // Pausa entre símbolos
                }

                delay(MORSE_END_LETTER_MS);  // Pausa entre letras
                Serial.print(" ");
            }

            index++;
        }
        Serial.println();
    }

    // =======================================================================
    // SISTEMA DE SINTONIA FM (ARRAY DE VOLUMES)
    // =======================================================================

    /**
     * Processa o rotary encoder e move o índice no array de volumes.
     * Encoder direita: incrementa índice (sobe no array)
     * Encoder esquerda: decrementa índice (desce no array)
     * O volume é lido do array na posição atual.
     * Troca de MP3 quando o índice cruza thresholds.
     * Chamado a cada loop quando a 3ª válvula está conectada.
     */
    void processFmDial() {
        if (!hw.hasExpander()) return;

        auto* pcf = hw.portExpander();

        // Lê estado atual do encoder
        bool pinA = pcf->digitalRead(PIN_ENCODER_A);
        bool pinB = pcf->digitalRead(PIN_ENCODER_B);
        int currentState = (pinA << 1) | pinB;

        // Detecta movimento do encoder
        if (currentState != lastEncoderState) {
            int delta = 0;
            int transition = (lastEncoderState << 2) | currentState;

            // Sentido horário: incrementa índice (sobe)
            if (transition == 0b0001 || transition == 0b0111 ||
                transition == 0b1110 || transition == 0b1000) {
                delta = 1;
                Serial.println(F("[FM Dial] Encoder -> direita"));
            }
            // Sentido anti-horário: decrementa índice (desce)
            else if (transition == 0b0010 || transition == 0b1011 ||
                     transition == 0b1101 || transition == 0b0100) {
                delta = -1;
                Serial.println(F("[FM Dial] Encoder -> esquerda"));
            }

            if (delta != 0) {
                // Move índice no array
                volumeIndex += delta;

                // Wrap around (circular)
                if (volumeIndex >= FM_VOLUME_STEPS) volumeIndex = 0;
                if (volumeIndex < 0) volumeIndex = FM_VOLUME_STEPS - 1;

                // Atualiza áudio imediatamente
                updateFmAudio();
            }

            lastEncoderState = currentState;
        }

        // Atualiza áudio continuamente (para manter loop)
        updateFmAudio();
    }

    /**
     * Atualiza o áudio FM baseado no índice do array de volumes.
     * Troca estação quando índice cruza thresholds.
     * Prioridade: SD Card > LittleFS
     */
    void updateFmAudio() {
        // Obtém volume do array
        uint8_t volume = fmVolumeCurve[volumeIndex];

        // Determina estação baseada no índice atual
        int stationFromIndex = indexToStation(volumeIndex);

        // Se mudou de estação
        if (stationFromIndex != lastStationFromIndex) {
            Serial.printf("[FM] Trocando estação: %d -> %d (índice=%d, vol=%d)\n",
                          lastStationFromIndex + 1, stationFromIndex + 1,
                          volumeIndex, volume);

            // Toca novo arquivo (com prioridade SD > LittleFS)
            playStationFile(stationFromIndex);

            lastStationFromIndex = stationFromIndex;
            currentPlayingStation = stationFromIndex;
        }

        // Se está iniciando (primeira vez)
        if (currentPlayingStation == -1) {
            Serial.printf("[FM] Iniciando estação %d (índice=%d, vol=%d)\n",
                          stationFromIndex + 1, volumeIndex, volume);
            playStationFile(stationFromIndex);
            currentPlayingStation = stationFromIndex;
            lastStationFromIndex = stationFromIndex;
        }

        // Ajusta volume
        hw.sound().setVolume(volume);

        // Debug periódico (a cada 1 segundo)
        if (millis() - lastFreqReportTime > 1000) {
            Serial.printf("[FM] Índice=%d, Estação=%d, Volume=%d\n",
                          volumeIndex, stationFromIndex + 1, volume);
            lastFreqReportTime = millis();
        }
    }

    /**
     * Para o áudio FM (usado quando o puzzle é resolvido ou resetado).
     */
    void stopFmAudio() {
        if (currentPlayingStation != -1) {
            hw.sound().stop();
            currentPlayingStation = -1;
            fmDialActive = false;
            Serial.println(F("[FM] Audio parado"));
        }
    }

    // =======================================================================
    // SD CARD E MENSAGEM
    // =======================================================================

    void loadMessage() {
        Serial.println(F("[VintageRadio] Carregando mensagem..."));

        if (!hw.isSDMounted()) {
            Serial.println(F("[VintageRadio] SD Card nao montado, usando fallback"));
            strcpy(messageText, "SOS");
            return;
        }

        File f = SD.open(MESSAGE_FILE);
        if (!f) {
            Serial.println(F("[VintageRadio] Arquivo mensagem.txt nao encontrado"));
            strcpy(messageText, "SOS");
            return;
        }

        // Lê conteúdo completo do arquivo (uma ou mais linhas)
        int i = 0;
        while (f.available() && i < MAX_MESSAGE_LEN - 1) {
            char c = f.read();
            // Aceita nova linha como espaço (para suportar multi-linha)
            if (c == '\n' || c == '\r') {
                if (i > 0 && messageText[i-1] != ' ') {
                    messageText[i++] = ' ';
                }
            } else {
                messageText[i++] = c;
            }
        }
        messageText[i] = '\0';
        f.close();

        // Remove espaços duplos no final
        while (i > 0 && messageText[i-1] == ' ') {
            messageText[--i] = '\0';
        }

        Serial.printf("[VintageRadio] Mensagem original: %s\n", messageText);

        // Normaliza: remove acentos, pontuação, converte para válido Morse
        normalizeText(messageText);

        Serial.printf("[VintageRadio] Mensagem normalizada: %s\n", messageText);

        // Fallback se mensagem ficou vazia após normalização
        if (messageText[0] == '\0') {
            Serial.println(F("[VintageRadio] Mensagem vazia após normalizacao, usando fallback"));
            strcpy(messageText, "SOS");
        }
    }

    // =======================================================================
    // RESET E UTILITÁRIOS
    // =======================================================================

    void resetGame() {
        Serial.println(F("[VintageRadio] CONDICAO INICIAL"));

        // Apaga LEDs
        setPowerLed(false);
        setLight(NEO_MORSE, COLOR_OFF);
        setLight(NEO_VALVE1, COLOR_OFF);
        setLight(NEO_VALVE2, COLOR_OFF);
        setLight(NEO_VALVE3, COLOR_OFF);
        setLight(NEO_VALVE4, COLOR_OFF);
        setLight(NEO_TRAFO_HUM, COLOR_OFF);

        // Reseta flags
        valve1flag = false;
        valve2flag = false;
        valve3flag = false;
        valve4flag = false;
        trafoflag = false;
        gameover = false;
        tuningState = TUNING_INACTIVE;
        dialPosition = 0;

        // Reseta variáveis FM
        fmDialActive = false;
        volumeIndex = 0;
        lastStationFromIndex = -1;
        currentPlayingStation = -1;
        lastEncoderState = 0;

        // Reseta disponibilidade de arquivos
        for (int i = 0; i < FM_NUM_STATIONS; i++) {
            stationAvailableSD[i] = false;
            stationAvailableLFS[i] = false;
        }

        // Para som
        hw.sound().noTone();
        hw.sound().stop();
    }
};

// ============================================================================
// MAIN
// ============================================================================

propmaker_escaperoom::LogicManager master(30, false);  // ID 30, sem RS485
VintageRadioPuzzle puzzle;

void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while (!Serial && (millis() - start < 3000)) delay(10);

    Serial.println(F("============================================"));
    Serial.println(F("  VintageShortwaveMorseRadio 2.0"));
    Serial.println(F("  PropMaker EscapeRoom"));
    Serial.println(F("  Original: Nicolau dos Brinquedos (2018)"));
    Serial.println(F("============================================"));

    // Inicializa seed para sorteio aleatório das estações FM
    randomSeed(analogRead(A0) + millis());

    // Inicializa hardware
    hw.begin();

    // Adiciona PCF8575 para entradas
    hw.addPcf8575(PCF_ADDRESS);
    Serial.println(F("[Main] PCF8575 adicionado em 0x20"));
    auto* pcf = hw.portExpander();
    if (pcf) {
        // Configura pinos como entradas com pull-up
        // Pinos 0-6 (Trafo, Valve1-3, Encoder A/B/Btn) e pino 7 (Valve4)
        for (int i = 0; i <= PIN_VALVE4; i++) {
            pcf->pinMode(i, INPUT_PULLUP);
        }
        Serial.println(F("[Main] Pinos configurados como INPUT_PULLUP"));
    } else {
        Serial.println(F("[Main] ERRO: PCF8575 nao detectado!"));
    }

    // Registra puzzle
    master.addPuzzle(&puzzle);
    master.begin();

    // Coloca puzzle em estado Running
      initHA();
  puzzle.run();

    Serial.println(F("[Main] Sistema pronto"));
    Serial.println(F("[Main] Aguardando conexao do transformador..."));
}

void loop() {
  loopHA();
    master.update();
    delay(1);
}
