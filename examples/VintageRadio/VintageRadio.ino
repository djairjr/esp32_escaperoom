/**
 * VintageShortwaveMorseRadio - PropMaker RP2040 Version
 * Original Author: Nicolau dos Brinquedos (2018)
 * Adaptation: PropMaker EscapeRoom Library
 *
 * Hardware: Adafruit PropMaker Feather RP2040
 * - Inputs via PCF8575 (I2C): transformer and tubes
 * - Audio: I2S with real-time Morse tone generation
 * - LEDs: External NeoPixels (valves, dial, morse, power)
 * - SD Card: mensagem.txt in Morse code
 * - Rotary Encoder: tuning system (optional)
 *
 * Based on the original project for Arduino Nano (Escape Junior - Henrique Sitchin)
 */

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

#include <TaskManager.h>
#include <SwitchInput.h>

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
// CONSTANTS AND CONFIGURATION
// ============================================================================

// PCF8575 I2C address
#define PCF_ADDRESS         0x20

// Pins on the PCF8575 (0-15)
#define PIN_TRAFO           0   // Transformer (active LOW)
#define PIN_VALVE1          1   // Tube 1
#define PIN_VALVE2          2   // Tube 2
#define PIN_VALVE3          3   // Tube 3 (activates the FM dial)
#define PIN_VALVE4          7   // Tube 4 (FINAL - starts the Morse transmission)
#define PIN_ENCODER_A       4   // Rotary Encoder A (optional)
#define PIN_ENCODER_B       5   // Rotary Encoder B (optional)
#define PIN_ENCODER_BTN     6   // Encoder button (optional)

// External NeoPixel indices (0-11 = 12 LEDs for the circular dial)
#define NEO_POWER           12  // Power indicator LED
#define NEO_MORSE           13  // Morse indicator LED
#define NEO_VALVE1          14  // Valve 1 LED
#define NEO_VALVE2          15  // Valve 2 LED
#define NEO_VALVE3          16  // Valve 3 LED
#define NEO_VALVE4          18  // Valve 4 LED (FINAL)
#define NEO_TRAFO_HUM       17  // LED simulating the transformer "hum" (pulsing)
#define NEO_DIAL_START      0   // Start of the circular dial (12 LEDs: 0-11)

// Morse parameters
#define MORSE_DOT_MS        500     // Dot duration (ms)
#define MORSE_DASH_MS       1500    // Dash duration (ms)
#define MORSE_END_LETTER_MS 2000    // Pause between letters
#define MORSE_SPACE_MS      3500    // Pause between words
#define MORSE_FREQ_HZ       432     // Tone frequency
#define MORSE_VOLUME        128     // Volume (0-255)
#define MORSE_REPEAT        5       // Message repetitions

// SD Card configuration
#define MESSAGE_FILE        "/mensagem.txt"
#define MAX_MESSAGE_LEN     256

// ============================================================================
// FM DIAL CONFIGURATION (LOGARITHMIC VOLUME ARRAY)
// ============================================================================

// Configuration of the 9 stations
#define FM_NUM_STATIONS     9

// Volume array - logarithmic scale from 10 to 80 and back to 10
// Indices 0-39: rises from 10 to 80 (log)
// Indices 40-79: falls from 80 to 10 (log)
// Total: 80 steps per complete cycle
#define FM_VOLUME_STEPS     80
#define FM_VOLUME_LOW       25      // ~10% of 255
#define FM_VOLUME_HIGH      204     // ~80% of 255

// Pre-calculated volume array (logarithmic scale)
const uint8_t fmVolumeCurve[FM_VOLUME_STEPS] = {
    // Rising: 10% -> 80% (indices 0-39) - logarithmic curve
    25, 26, 27, 28, 30, 31, 33, 34, 36, 37,      // 0-9
    39, 41, 43, 45, 47, 49, 51, 53, 56, 58,      // 10-19
    61, 64, 67, 70, 73, 76, 80, 84, 88, 92,      // 20-29
    96, 101, 106, 111, 116, 122, 128, 134, 140, 147, // 30-39 (reaches ~80%)
    // Falling: 80% -> 10% (indices 40-79) - mirrored
    147, 140, 134, 128, 122, 116, 111, 106, 101, 96, // 40-49
    92, 88, 84, 80, 76, 73, 70, 67, 64, 61,      // 50-59
    58, 56, 53, 51, 49, 47, 45, 43, 41, 39,      // 60-69
    37, 36, 34, 33, 31, 30, 28, 27, 26, 25       // 70-79 (back to ~10%)
};

// Station switching points (when the index crosses these thresholds)
// Each station occupies a range of the array (approximately)
#define FM_STATION_THRESHOLD_1  9   // Station 1: indices 0-9 and 70-79
#define FM_STATION_THRESHOLD_2  18  // Station 2: indices 10-18 and 61-69
#define FM_STATION_THRESHOLD_3  27  // Station 3
#define FM_STATION_THRESHOLD_4  36  // Station 4
#define FM_STATION_THRESHOLD_5  45  // Station 5 (center of the array)
#define FM_STATION_THRESHOLD_6  54  // Station 6
#define FM_STATION_THRESHOLD_7  63  // Station 7
#define FM_STATION_THRESHOLD_8  72  // Station 8
// Station 9: remaining indices

// Maps the array index to a station (0-8)
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

// Station file names
const char* fmStationFiles[FM_NUM_STATIONS] = {
    "/station1.mp3", "/station2.mp3", "/station3.mp3",
    "/station4.mp3", "/station5.mp3", "/station6.mp3",
    "/station7.mp3", "/station8.mp3", "/station9.mp3"
};

// Availability flags per station (SD has priority over LittleFS)
bool stationAvailableSD[FM_NUM_STATIONS] = {false};
bool stationAvailableLFS[FM_NUM_STATIONS] = {false};

/**
 * Scans station*.mp3 files on the SD Card and LittleFS.
 * Marks which stations are available in each file system.
 * SD Card has priority over LittleFS when the file exists in both.
 */
void scanStationFiles() {
    Serial.println(F("[FM] Scanning station*.mp3 files..."));

    // Checks availability for each station
    for (int i = 0; i < FM_NUM_STATIONS; i++) {
        const char* filename = fmStationFiles[i];

        // Checks the SD Card first (priority)
        if (hw.isSDMounted() && SD.exists(filename)) {
            stationAvailableSD[i] = true;
            Serial.printf("[FM] Found %s (SD Card)\n", filename);
        } else {
            stationAvailableSD[i] = false;
        }

        // Checks LittleFS
        if (hw.isLittleFSMounted() && LittleFS.exists(filename)) {
            stationAvailableLFS[i] = true;
            Serial.printf("[FM] Found %s (LittleFS)\n", filename);
        } else {
            stationAvailableLFS[i] = false;
        }

        // Warning if it was not found in any
        if (!stationAvailableSD[i] && !stationAvailableLFS[i]) {
            Serial.printf("[FM] WARNING: %s not found in any file system!\n", filename);
        }
    }

    Serial.println(F("[FM] Scan complete. Priority: SD Card > LittleFS"));
}

/**
 * Plays a station file with SD Card > LittleFS priority.
 * Returns true if it managed to play, false if the file does not exist.
 */
bool playStationFile(int stationIndex) {
    if (stationIndex < 0 || stationIndex >= FM_NUM_STATIONS) return false;

    const char* filename = fmStationFiles[stationIndex];

    // Priority 1: SD Card
    if (stationAvailableSD[stationIndex]) {
        Serial.printf("[FM] Playing %s (SD Card)\n", filename);
        return hw.sound().playLoop(filename, true);  // true = use SD
    }

    // Priority 2: LittleFS
    if (stationAvailableLFS[stationIndex]) {
        Serial.printf("[FM] Playing %s (LittleFS)\n", filename);
        return hw.sound().playLoop(filename, false); // false = use LittleFS
    }

    // Did not find it in any
    Serial.printf("[FM] ERROR: %s not available!\n", filename);
    return false;
}

// Normalization table for accented characters (ISO-8859-1 / Windows-1252)
// Maps special characters to their simple ASCII equivalents
// Using hexadecimal values to avoid encoding problems
struct CharMapping {
    unsigned char from;
    char to;
};

const CharMapping accentMap[] = {
    // Lowercase accented letters (ISO-8859-1 values)
    {0xE0, 'A'}, {0xE1, 'A'}, {0xE2, 'A'}, {0xE3, 'A'}, {0xE4, 'A'}, {0xE5, 'A'},  // àáâãäå
    {0xE8, 'E'}, {0xE9, 'E'}, {0xEA, 'E'}, {0xEB, 'E'},                           // èéêë
    {0xEC, 'I'}, {0xED, 'I'}, {0xEE, 'I'}, {0xEF, 'I'},                           // ìíîï
    {0xF2, 'O'}, {0xF3, 'O'}, {0xF4, 'O'}, {0xF5, 'O'}, {0xF6, 'O'},              // òóôõö
    {0xF9, 'U'}, {0xFA, 'U'}, {0xFB, 'U'}, {0xFC, 'U'},                           // ùúûü
    {0xE7, 'C'}, {0xF1, 'N'},                                                      // çñ
    // Uppercase accented letters
    {0xC0, 'A'}, {0xC1, 'A'}, {0xC2, 'A'}, {0xC3, 'A'}, {0xC4, 'A'}, {0xC5, 'A'},  // ÀÁÂÃÄÅ
    {0xC8, 'E'}, {0xC9, 'E'}, {0xCA, 'E'}, {0xCB, 'E'},                           // ÈÉÊË
    {0xCC, 'I'}, {0xCD, 'I'}, {0xCE, 'I'}, {0xCF, 'I'},                           // ÌÍÎÏ
    {0xD2, 'O'}, {0xD3, 'O'}, {0xD4, 'O'}, {0xD5, 'O'}, {0xD6, 'O'},              // ÒÓÔÕÖ
    {0xD9, 'U'}, {0xDA, 'U'}, {0xDB, 'U'}, {0xDC, 'U'},                           // ÙÚÛÜ
    {0xC7, 'C'}, {0xD1, 'N'},                                                      // ÇÑ
    // Special symbols
    {0xB0, 'O'}, {0xBA, 'O'}, {0xAA, 'A'},                                         // °ºª
    {0, 0}  // Terminator
};

/**
 * Normalizes text for Morse code:
 * - Converts accents to simple ASCII characters
 * - Removes punctuation and invalid characters
 * - Converts to uppercase
 * - Keeps only: A-Z, 0-9, space
 */
void normalizeText(char* buffer) {
    if (!buffer || buffer[0] == '\0') return;

    int len = strlen(buffer);
    int writePos = 0;

    for (int readPos = 0; readPos < len && readPos < MAX_MESSAGE_LEN - 1; readPos++) {
        char c = buffer[readPos];

        // Ignores control characters (except space)
        if (c < 32 && c != ' ') continue;

        // Converts to uppercase if it is a lowercase letter without an accent
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }

        // Converts accents using the table
        for (int i = 0; accentMap[i].from != 0; i++) {
            if (c == accentMap[i].from) {
                c = accentMap[i].to;
                break;
            }
        }

        // Accepts: letters A-Z, numbers 0-9, space
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ' ') {
            buffer[writePos++] = c;
        }
        // Silently discards other characters (punctuation, symbols, etc.)
    }

    buffer[writePos] = '\0';
}

// NeoPixel Colors
#define COLOR_OFF           0
#define COLOR_POWER         Adafruit_NeoPixel::Color(255, 255, 255)  // White
#define COLOR_VALVE         Adafruit_NeoPixel::Color(0, 100, 255)    // Blue
#define COLOR_MORSE         Adafruit_NeoPixel::Color(255, 255, 255)  // White
#define COLOR_DIAL_FAR      Adafruit_NeoPixel::Color(255, 0, 0)      // Red
#define COLOR_DIAL_NEAR     Adafruit_NeoPixel::Color(255, 255, 0)    // Yellow
#define COLOR_DIAL_PEAK     Adafruit_NeoPixel::Color(0, 255, 0)      // Green

// Morse table (index: character - '0')
const char* morseTable[] = {
    // Numbers 0-9
    "-----", ".----", "..---", "...--", "....-",  // 0-4
    ".....", "-....", "--...", "---..", "----.",  // 5-9
    // Symbols : ; < = > ? @
    "---...", "-.-.-.", "*", "-...-", "*", "..--..", ".--.-.",
    // Letters A-Z
    ".-", "-...", "-.-.", "-..", ".", "..-.",      // A-F
    "--.", "....", "..", ".---", "-.-", ".-..",     // G-L
    "--", "-.", "---", ".--.", "--.-", ".-.",     // M-R
    "...", "-", "..-", "...-", ".--", "-..-",      // S-X
    "-.--", "--.."                                   // Y-Z
};

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Game state
bool valve1flag = false;
bool valve2flag = false;
bool valve3flag = false;
bool valve4flag = false;  // FINAL Valve - starts the Morse transmission
bool trafoflag = false;
bool gameover = false;

// Morse message
char messageText[MAX_MESSAGE_LEN] = "SOS";
bool sdInitSuccess = false;

// Tuning (optional dial system)
enum TuningState {
    TUNING_INACTIVE,
    TUNING_ACTIVE,
    TUNING_LOCKED,
    TUNING_COMPLETE
};
TuningState tuningState = TUNING_INACTIVE;
int dialPosition = 0;           // Position 0-19
int currentStation = 1;         // Station 1-9

// FM Tuning System (Volume Array)
int volumeIndex = 0;                      // Current index in the volume array (0-FM_VOLUME_STEPS-1)
int lastStationFromIndex = -1;            // Last station detected by the index
int lastEncoderState = 0;               // Previous encoder state
uint32_t lastFreqReportTime = 0;        // For debug
int currentPlayingStation = -1;         // Index of the station currently playing (-1 = none)
bool fmDialActive = false;              // Is the dial system active?

// ============================================================================
// PUZZLE CLASS
// ============================================================================

class VintageRadioPuzzle : public PuzzleBase {
public:
    VintageRadioPuzzle() : PuzzleBase(30) {}  // ID 30 = Vintage Radio

    void setupHardware() override {
        Serial.println(F("[VintageRadio] Configuring hardware..."));

        // Configures external NeoPixels
        if (hw.pixelsExternal().numPixels() > 0) {
            hw.pixelsExternal().clear();
            hw.pixelsExternal().show();
            Serial.println(F("[VintageRadio] External NeoPixel configured"));
        }

        // Loads the message from the file
        loadMessage();

        // Initial state
        resetGame();

        Serial.println(F("[VintageRadio] Setup complete"));
        setState(PuzzleState::Running);
    }

    void update() override {
        // Keeps audio active (for real-time Morse tones)
        hw.sound().loop();

        if (getState() != PuzzleState::Running) return;

        if (!gameover) {
            // Checks the transformer (hard reset if removed during the game)
            if (!checkTrafo() && trafoflag) {
                Serial.println(F("[VintageRadio] HARD RESET - Transformer removed"));
                resetGame();
                return;
            }

            // Checks the transformer connection
            if (checkTrafoConnected()) {
                if (!trafoflag) {
                    // First transformer connection
                    Serial.println(F("[VintageRadio] TRANSFORMER Connected"));
                    Serial.println(F("[VintageRadio] POWER ON"));
                    trafoflag = true;
                    setPowerLed(true);
                    startTrafoHum();
                }

                // Checks the tubes (includes activating the FM dial on tube 3)
                checkValves();

                // Processes the FM dial if active (3rd tube connected)
                if (valve3flag) {
                    processFmDial();
                }

                // Checks whether Tube 4 (FINAL) was connected - starts the Morse transmission
                // Tube 4 only works if the other 3 are connected
                if (trafoflag && valve1flag && valve2flag && valve3flag && valve4flag) {
                    // Stops the FM audio before transmitting Morse
                    stopFmAudio();

                    // Final tube connected - starts transmission
                    Serial.println(F("[VintageRadio] Tube 4 (FINAL) connected - transmitting Morse message"));
                    transmitMessage();
                    gameover = true;
                    setState(PuzzleState::Solved);
                }
            }
        } else {
            // Game finished - waits for reset via removing the transformer
            if (!checkTrafoConnected()) {
                resetGame();
            }
        }
    }

private:
    // =======================================================================
    // HARDWARE FUNCTIONS
    // =======================================================================

    bool checkTrafoConnected() {
        if (!hw.hasExpander()) return false;
        // Active LOW (INPUT_PULLUP)
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

        // Tube 1
        bool valve1Connected = !pcf->digitalRead(PIN_VALVE1);
        if (valve1Connected && trafoflag && !valve1flag) {
            Serial.println(F("[VintageRadio] Valve1 Connected"));
            Serial.println(F("[VintageRadio] Morse Dot"));
            fadeLed(NEO_VALVE1, COLOR_VALVE);
            blinkMorsePattern(true);  // DOT pattern
            valve1flag = true;
        } else if (!valve1Connected) {
            valve1flag = false;
            setLight(NEO_VALVE1, COLOR_OFF);
        }

        // Tube 2
        bool valve2Connected = !pcf->digitalRead(PIN_VALVE2);
        if (valve2Connected && trafoflag && !valve2flag) {
            Serial.println(F("[VintageRadio] Valve2 Connected"));
            Serial.println(F("[VintageRadio] Morse Dash"));
            fadeLed(NEO_VALVE2, COLOR_VALVE);
            blinkMorsePattern(false);  // DASH pattern
            valve2flag = true;
        } else if (!valve2Connected) {
            valve2flag = false;
            setLight(NEO_VALVE2, COLOR_OFF);
        }

        // Tube 3 - activates the FM tuning system
        bool valve3Connected = !pcf->digitalRead(PIN_VALVE3);
        if (valve3Connected && trafoflag && !valve3flag) {
            Serial.println(F("[VintageRadio] Valve3 Connected"));
            Serial.println(F("[VintageRadio] FM dial activated - volume array"));
            fadeLed(NEO_VALVE3, COLOR_VALVE);
            valve3flag = true;

            // Scans the available station*.mp3 files
            scanStationFiles();

            // Initializes the FM system at index 0 (minimum volume)
            volumeIndex = 0;
            lastStationFromIndex = -1;
            fmDialActive = true;
            currentPlayingStation = -1;

            Serial.println(F("[FM Dial] Starting at index 0 (minimum volume)"));
            Serial.println(F("[FM Dial] Use the encoder to move through the volume array"));
            Serial.println(F("[FM Dial] Priority: SD Card > LittleFS"));
        } else if (!valve3Connected) {
            if (valve3flag) {
                // Tube 3 disconnected - stops FM audio
                Serial.println(F("[VintageRadio] Valve3 removed - FM OFF"));
                stopFmAudio();
                fmDialActive = false;
                currentPlayingStation = -1;
            }
            valve3flag = false;
            setLight(NEO_VALVE3, COLOR_OFF);
        }

        // Tube 4 - FINAL TUBE (activates the Morse transmission)
        bool valve4Connected = !pcf->digitalRead(PIN_VALVE4);
        if (valve4Connected && trafoflag && !valve4flag) {
            Serial.println(F("[VintageRadio] Valve4 Connected - FINAL VALVE"));
            Serial.println(F("[VintageRadio] Interrupting FM and starting Morse transmission"));
            fadeLed(NEO_VALVE4, COLOR_VALVE);
            valve4flag = true;
        } else if (!valve4Connected) {
            valve4flag = false;
            setLight(NEO_VALVE4, COLOR_OFF);
        }
    }

    // =======================================================================
    // LED CONTROL
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
        // Extracts color components
        uint8_t r = (targetColor >> 16) & 0xFF;
        uint8_t g = (targetColor >> 8) & 0xFF;
        uint8_t b = targetColor & 0xFF;

        // Gradual fade-in
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
        // Blinks the Morse LED 3x
        int duration = isDot ? MORSE_DOT_MS : MORSE_DASH_MS;
        for (int i = 0; i < 3; i++) {
            setLight(NEO_MORSE, COLOR_MORSE);
            hw.sound().tone(MORSE_FREQ_HZ, MORSE_VOLUME);
            delay(duration);
            setLight(NEO_MORSE, COLOR_OFF);
            hw.sound().noTone();
            delay(duration);

            // Checks for hard reset during the blink
            if (!checkTrafo()) {
                hw.sound().noTone();
                return;
            }
        }
    }

    void startTrafoHum() {
        // Visual simulation of the transformer "hum"
        // Uses a pulsing LED at the NEO_TRAFO_HUM index
        // In practice, the hum sound would be an MP3 or a low-frequency tone
    }

    // =======================================================================
    // MORSE TRANSMISSION
    // =======================================================================

    void transmitMessage() {
        // Fade out of the transformer hum
        for (int i = 25; i >= 0; i--) {
            setLight(NEO_TRAFO_HUM, Adafruit_NeoPixel::Color(i, i, i));
            delay(5);
        }
        setLight(NEO_TRAFO_HUM, COLOR_OFF);

        // Transmits the message MORSE_REPEAT times
        for (int r = 0; r < MORSE_REPEAT; r++) {
            if (!checkTrafo()) {
                Serial.println(F("[VintageRadio] Transmission interrupted"));
                return;
            }
            Serial.printf("[VintageRadio] Transmitting (repetition %d/%d): %s\n",
                         r + 1, MORSE_REPEAT, messageText);
            morseCode(messageText);
            delay(MORSE_SPACE_MS);
        }
        Serial.println(F("[VintageRadio] Transmission complete"));
    }

    void morseCode(const char* asciimsg) {
        // Copies the message (already normalized in loadMessage)
        char msg[MAX_MESSAGE_LEN];
        strncpy(msg, asciimsg, MAX_MESSAGE_LEN - 1);
        msg[MAX_MESSAGE_LEN - 1] = '\0';

        int tamanho = strlen(msg);
        int index = 0;

        while (index < tamanho) {
            // Checks for hard reset
            if (!checkTrafo()) {
                gameover = true;
                hw.sound().noTone();
                return;
            }

            char letra = msg[index];
            Serial.print(letra);
            Serial.print(" ");

            // Space between words
            if (letra == ' ') {
                delay(MORSE_SPACE_MS);
                Serial.print(" ");
                index++;
                continue;
            }

            // Valid character (0-9, A-Z)
            if (letra >= '0' && letra <= 'Z') {
                const char* morseLetter = morseTable[letra - '0'];
                int tamMorse = strlen(morseLetter);

                for (int indMorse = 0; indMorse < tamMorse; indMorse++) {
                    // Checks for hard reset during the symbol
                    if (!checkTrafo()) {
                        gameover = true;
                        hw.sound().noTone();
                        return;
                    }

                    // Turns on the LED and starts the tone
                    setLight(NEO_MORSE, COLOR_MORSE);
                    hw.sound().tone(MORSE_FREQ_HZ, MORSE_VOLUME);

                    char dotDash = morseLetter[indMorse];
                    Serial.print(dotDash);

                    if (dotDash == '.') {
                        delay(MORSE_DOT_MS);
                    } else if (dotDash == '-') {
                        delay(MORSE_DASH_MS);
                    }

                    // Turns off the LED and stops the tone
                    setLight(NEO_MORSE, COLOR_OFF);
                    hw.sound().noTone();
                    delay(MORSE_DOT_MS);  // Pause between symbols
                }

                delay(MORSE_END_LETTER_MS);  // Pause between letters
                Serial.print(" ");
            }

            index++;
        }
        Serial.println();
    }

    // =======================================================================
    // FM TUNING SYSTEM (VOLUME ARRAY)
    // =======================================================================

    /**
     * Processes the rotary encoder and moves the index in the volume array.
     * Encoder right: increments the index (rises in the array)
     * Encoder left: decrements the index (falls in the array)
     * The volume is read from the array at the current position.
     * Switches MP3 when the index crosses thresholds.
     * Called every loop when the 3rd tube is connected.
     */
    void processFmDial() {
        if (!hw.hasExpander()) return;

        auto* pcf = hw.portExpander();

        // Reads the current encoder state
        bool pinA = pcf->digitalRead(PIN_ENCODER_A);
        bool pinB = pcf->digitalRead(PIN_ENCODER_B);
        int currentState = (pinA << 1) | pinB;

        // Detects encoder movement
        if (currentState != lastEncoderState) {
            int delta = 0;
            int transition = (lastEncoderState << 2) | currentState;

            // Clockwise: increments the index (rises)
            if (transition == 0b0001 || transition == 0b0111 ||
                transition == 0b1110 || transition == 0b1000) {
                delta = 1;
                Serial.println(F("[FM Dial] Encoder -> right"));
            }
            // Counterclockwise: decrements the index (falls)
            else if (transition == 0b0010 || transition == 0b1011 ||
                     transition == 0b1101 || transition == 0b0100) {
                delta = -1;
                Serial.println(F("[FM Dial] Encoder -> left"));
            }

            if (delta != 0) {
                // Moves the index in the array
                volumeIndex += delta;

                // Wrap around (circular)
                if (volumeIndex >= FM_VOLUME_STEPS) volumeIndex = 0;
                if (volumeIndex < 0) volumeIndex = FM_VOLUME_STEPS - 1;

                // Updates the audio immediately
                updateFmAudio();
            }

            lastEncoderState = currentState;
        }

        // Updates the audio continuously (to keep the loop)
        updateFmAudio();
    }

    /**
     * Updates the FM audio based on the volume array index.
     * Switches station when the index crosses thresholds.
     * Priority: SD Card > LittleFS
     */
    void updateFmAudio() {
        // Gets the volume from the array
        uint8_t volume = fmVolumeCurve[volumeIndex];

        // Determines the station based on the current index
        int stationFromIndex = indexToStation(volumeIndex);

        // If the station changed
        if (stationFromIndex != lastStationFromIndex) {
            Serial.printf("[FM] Switching station: %d -> %d (index=%d, vol=%d)\n",
                          lastStationFromIndex + 1, stationFromIndex + 1,
                          volumeIndex, volume);

            // Plays the new file (with SD > LittleFS priority)
            playStationFile(stationFromIndex);

            lastStationFromIndex = stationFromIndex;
            currentPlayingStation = stationFromIndex;
        }

        // If it is starting (first time)
        if (currentPlayingStation == -1) {
            Serial.printf("[FM] Starting station %d (index=%d, vol=%d)\n",
                          stationFromIndex + 1, volumeIndex, volume);
            playStationFile(stationFromIndex);
            currentPlayingStation = stationFromIndex;
            lastStationFromIndex = stationFromIndex;
        }

        // Adjusts volume
        hw.sound().setVolume(volume);

        // Periodic debug (every 1 second)
        if (millis() - lastFreqReportTime > 1000) {
            Serial.printf("[FM] Index=%d, Station=%d, Volume=%d\n",
                          volumeIndex, stationFromIndex + 1, volume);
            lastFreqReportTime = millis();
        }
    }

    /**
     * Stops the FM audio (used when the puzzle is solved or reset).
     */
    void stopFmAudio() {
        if (currentPlayingStation != -1) {
            hw.sound().stop();
            currentPlayingStation = -1;
            fmDialActive = false;
            Serial.println(F("[FM] Audio stopped"));
        }
    }

    // =======================================================================
    // SD CARD AND MESSAGE
    // =======================================================================

    void loadMessage() {
        Serial.println(F("[VintageRadio] Loading message..."));

        if (!hw.isSDMounted()) {
            Serial.println(F("[VintageRadio] SD Card not mounted, using fallback"));
            strcpy(messageText, "SOS");
            return;
        }

        File f = SD.open(MESSAGE_FILE);
        if (!f) {
            Serial.println(F("[VintageRadio] File mensagem.txt not found"));
            strcpy(messageText, "SOS");
            return;
        }

        // Reads the full file content (one or more lines)
        int i = 0;
        while (f.available() && i < MAX_MESSAGE_LEN - 1) {
            char c = f.read();
            // Accepts a newline as a space (to support multi-line)
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

        // Removes double spaces at the end
        while (i > 0 && messageText[i-1] == ' ') {
            messageText[--i] = '\0';
        }

        Serial.printf("[VintageRadio] Original message: %s\n", messageText);

        // Normalizes: removes accents, punctuation, converts to valid Morse
        normalizeText(messageText);

        Serial.printf("[VintageRadio] Normalized message: %s\n", messageText);

        // Fallback if the message became empty after normalization
        if (messageText[0] == '\0') {
            Serial.println(F("[VintageRadio] Empty message after normalization, using fallback"));
            strcpy(messageText, "SOS");
        }
    }

    // =======================================================================
    // RESET AND UTILITIES
    // =======================================================================

    void resetGame() {
        Serial.println(F("[VintageRadio] INITIAL CONDITION"));

        // Turns off the LEDs
        setPowerLed(false);
        setLight(NEO_MORSE, COLOR_OFF);
        setLight(NEO_VALVE1, COLOR_OFF);
        setLight(NEO_VALVE2, COLOR_OFF);
        setLight(NEO_VALVE3, COLOR_OFF);
        setLight(NEO_VALVE4, COLOR_OFF);
        setLight(NEO_TRAFO_HUM, COLOR_OFF);

        // Resets flags
        valve1flag = false;
        valve2flag = false;
        valve3flag = false;
        valve4flag = false;
        trafoflag = false;
        gameover = false;
        tuningState = TUNING_INACTIVE;
        dialPosition = 0;

        // Resets FM variables
        fmDialActive = false;
        volumeIndex = 0;
        lastStationFromIndex = -1;
        currentPlayingStation = -1;
        lastEncoderState = 0;

        // Resets file availability
        for (int i = 0; i < FM_NUM_STATIONS; i++) {
            stationAvailableSD[i] = false;
            stationAvailableLFS[i] = false;
        }

        // Stops sound
        hw.sound().noTone();
        hw.sound().stop();
    }
};

// ============================================================================
// MAIN
// ============================================================================

propmaker_escaperoom::LogicManager master(30, false);  // ID 30, without RS485
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

    // Initializes the seed for the random draw of the FM stations
    randomSeed(analogRead(A0) + millis());

    // Initializes hardware
    hw.begin();

    // Adds the PCF8575 for inputs
    hw.addPcf8575(PCF_ADDRESS);
    Serial.println(F("[Main] PCF8575 added at 0x20"));
    auto* pcf = hw.portExpander();
    if (pcf) {
        // Configures the pins as inputs with pull-up
        // Pins 0-6 (Transformer, Valve1-3, Encoder A/B/Btn) and pin 7 (Valve4)
        for (int i = 0; i <= PIN_VALVE4; i++) {
            pcf->pinMode(i, INPUT_PULLUP);
        }
        Serial.println(F("[Main] Pins configured as INPUT_PULLUP"));
    } else {
        Serial.println(F("[Main] ERROR: PCF8575 not detected!"));
    }

    // Registers puzzle
    master.addPuzzle(&puzzle);
    master.begin();

    // Puts the puzzle in the Running state
      initHA();
  puzzle.run();

    Serial.println(F("[Main] System ready"));
    Serial.println(F("[Main] Waiting for the transformer connection..."));
}

void loop() {
  loopHA();
    master.update();
    delay(1);
}
