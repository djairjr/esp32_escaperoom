/**
 * ESCAPE ROOM PHONE - ESP32-S3 (Seeed XIAO)
 * Port do RP2040 PropMaker → ESP32-S3 usando ESP32EscapeRoom
 *
 * MODO DE ENTRADA (descomente UM):
 * - (nenhum): Keypad I2C com PCF8574/8575 (padrão)
 * - #define KEYPAD_GPIO: Keypad via GPIO direto
 * - #define USE_ROTARY_DIAL: Disco rotativo de telefone
 */

// ============================================================================
// SELEÇÃO DE MODO (~linha 10)
// ============================================================================
// #define KEYPAD_GPIO
// #define USE_ROTARY_DIAL

// ============================================================================
// INCLUDES (~linha 15)
// ============================================================================
#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

using namespace esp32_escaperoom;

// ============================================================================
// MAPEAMENTO D-PIN → GPIO (XIAO ESP32-S3) (~linha 25)
// ============================================================================
#define D0   1
#define D1   2
#define D2   3
#define D3   4
#define D4   5   // I2C SDA
#define D5   6   // I2C SCL
#define D6   43  // I2S BCLK
#define D7   44  // I2S WS / HOOK
#define D8   7   // SPI SCK
#define D9   8   // SPI MISO
#define D10  9   // SPI MOSI
#define D11  42
#define D12  41

// ============================================================================
// PINAGEM (~linha 38)
// ============================================================================
#define PIN_HOOK            D7
#define PIN_DOORLOCK        D0
#define PIN_RING_A          D1   // L298 IN1 ou BUZZER
#define PIN_RING_B          D12  // L298 IN2 (só Rotary)
#define PIN_ROTARY_PULSE    D4

#define HOOK_ON_HOOK        HIGH
#define HOOK_OFF_HOOK       LOW
#define LOCK_ACTIVE         LOW
#define LOCK_INACTIVE       HIGH

#define DIAL_TIMEOUT_MS     3000
#define MAX_PHONE_DIGITS    8

// ============================================================================
// ARQUIVOS DE ÁUDIO (~linha 52)
// ============================================================================
const char* FILE_DIALTONE = "/dialtone.mp3";
const char* FILE_BUSY     = "/busy.mp3";
const char* FILE_INVALID  = "/naoexiste.mp3";

const char* FILE_DTMF[] = {
    "/dtmf_0.mp3","/dtmf_1.mp3","/dtmf_2.mp3","/dtmf_3.mp3","/dtmf_4.mp3",
    "/dtmf_5.mp3","/dtmf_6.mp3","/dtmf_7.mp3","/dtmf_8.mp3","/dtmf_9.mp3"
};
const char* FILE_DTMF_STAR = "/dtmf_star.mp3";
const char* FILE_DTMF_HASH = "/dtmf_hash.mp3";

const char* FILE_ROTARY[] = {
    "/pulse_0.mp3","/pulse_1.mp3","/pulse_2.mp3","/pulse_3.mp3","/pulse_4.mp3",
    "/pulse_5.mp3","/pulse_6.mp3","/pulse_7.mp3","/pulse_8.mp3","/pulse_9.mp3"
};

#define CALL_MESSAGE_COUNT 5
const char* CALL_MESSAGE_FILES[] = {
    "/call_msg_1.mp3","/call_msg_2.mp3","/call_msg_3.mp3",
    "/call_msg_4.mp3","/call_msg_5.mp3"
};
const char* CALL_MESSAGE_NAMES[] = {
    "Mensagem 1","Mensagem 2","Mensagem 3","Mensagem 4","Mensagem 5"
};

// ============================================================================
// ESTADOS DO TELEFONE (~linha 76)
// ============================================================================
enum PhoneState : uint8_t {
    PHONE_IDLE      = 0,
    PHONE_OFFHOOK   = 1,
    PHONE_DIALING   = 2,
    PHONE_PLAYING   = 3,
    PHONE_BUSY      = 4,
    PHONE_CALLING   = 5,
    PHONE_CALL_MSG  = 6
};

// ============================================================================
// ISR DO GANCHO (~linha 88)
// ============================================================================
volatile bool isHookedGlobal = true;

void IRAM_ATTR handleHookISR() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > 50) {
        isHookedGlobal = (digitalRead(PIN_HOOK) == HOOK_ON_HOOK);
    }
    lastInterruptTime = interruptTime;
}

// ============================================================================
// CONFIGURAÇÃO DE ENTRADA (~linha 102)
// ============================================================================

#ifdef USE_ROTARY_DIAL
RotaryDialReader* rotaryDial = nullptr;

#elif defined(KEYPAD_GPIO)
const byte KEYPAD_ROWS = 4, KEYPAD_COLS = 3;
const char KEYPAD_MAP[4][3] = {{'1','2','3'},{'4','5','6'},{'7','8','9'},{'*','0','#'}};
const byte KEYPAD_ROW_PINS[4] = {D11,D12,D2,D3};
const byte KEYPAD_COL_PINS[3] = {D4,D5,D6};

class GpioKeypad {
public:
    void begin() {
        for(int i=0;i<4;i++){pinMode(KEYPAD_ROW_PINS[i],OUTPUT);digitalWrite(KEYPAD_ROW_PINS[i],HIGH);}
        for(int i=0;i<3;i++)pinMode(KEYPAD_COL_PINS[i],INPUT_PULLUP);
    }
    char getKey() {
        for(int r=0;r<4;r++){
            digitalWrite(KEYPAD_ROW_PINS[r],LOW);delayMicroseconds(10);
            for(int c=0;c<3;c++){
                if(digitalRead(KEYPAD_COL_PINS[c])==LOW){
                    char k=KEYPAD_MAP[r][c];
                    while(digitalRead(KEYPAD_COL_PINS[c])==LOW)delayMicroseconds(100);
                    delay(50);digitalWrite(KEYPAD_ROW_PINS[r],HIGH);return k;
                }
            }
            digitalWrite(KEYPAD_ROW_PINS[r],HIGH);
        }
        return 0;
    }
};
GpioKeypad keypadGpio;

#else
static const char keymap[] PROGMEM = "123456789*0#";
KeyboardLayout keypadLayout(4,3,keymap);
byte rowPins[4]={3,2,1,0}, colPins[3]={6,5,4};

class KeypadListener : public KeyboardListener {
public:
    void keyPressed(char key,bool held)override{if(!held){lastKey=key;lastKeyTime=millis();}}
    void keyReleased(char key)override{}
    char getLastKey(){char k=lastKey;lastKey=0;return k;}
    unsigned long getLastKeyTime(){return lastKeyTime;}
private:
    volatile char lastKey=0;
    volatile unsigned long lastKeyTime=0;
};
KeypadListener keypadListener;
MatrixKeyboardManager keypadManager;
#endif

// ============================================================================
// FORWARD DECLARATIONS (~linha 140)
// ============================================================================
class PhonePuzzle;
extern PhonePuzzle phone;

// ============================================================================
// ENTIDADES HA — GLOBAIS (auto-registradas pelo construtor) (~linha 146)
// ============================================================================
HASensor haSensorHook("hook");
HASensor haSensorDialed("dialed");
HASensor haSensorState("state");
HALock haLock("lock");
HAButton haBtnRing("ring");
HAButton haBtnStartCall("start_call");
HAButton haBtnEndCall("end_call");
HASelect haSelectMsg("call_message");

// ============================================================================
// CLASSE PHONE PUZZLE (~linha 168)
// ============================================================================
class PhonePuzzle : public PuzzleBase {
public:
    PhonePuzzle() : PuzzleBase(100) {}

    void setupHardware() override {
        hw.pinMode(PIN_HOOK, INPUT_PULLUP);
        hw.pinMode(PIN_DOORLOCK, OUTPUT); hw.digitalWrite(PIN_DOORLOCK, LOCK_INACTIVE);
        hw.pinMode(PIN_RING_A, OUTPUT); hw.digitalWrite(PIN_RING_A, LOW);
        #ifdef USE_ROTARY_DIAL
        hw.pinMode(PIN_RING_B, OUTPUT); hw.digitalWrite(PIN_RING_B, LOW);
        #endif

        isHookedGlobal = (hw.digitalRead(PIN_HOOK) == HOOK_ON_HOOK);
        attachInterrupt(digitalPinToInterrupt(PIN_HOOK), handleHookISR, CHANGE);
        hw.sound().setVolume(89);

        Serial.printf("[PHONE] HW Ready. Gancho: %s\n", isHookedGlobal ? "ON HOOK" : "OFF HOOK");
    }

    void update() override {
        unsigned long now = millis();
        bool currentHook = (hw.digitalRead(PIN_HOOK) == HOOK_ON_HOOK);

        if(currentHook != isHookedGlobal) {
            isHookedGlobal = currentHook;
            Serial.printf("[PHONE] Gancho: %s\n", isHookedGlobal ? "ON HOOK" : "OFF HOOK");
            if(isHookedGlobal) {
                hw.sound().stop(); phoneState = PHONE_IDLE; resetPhone();
                haSensorHook.setValue("on_hook");
                haSensorState.setValue("onhook");
            } else {
                phoneState = PHONE_OFFHOOK; callMsgPlayed = false;
                haSensorHook.setValue("off_hook");
                haSensorState.setValue("offhook");
            }
        }

        switch(phoneState) {
            case PHONE_IDLE: break;

            case PHONE_OFFHOOK:
                if(callModeActive && !callMsgPlayed) {
                    const char* f = getCallMessageFile(callMessageIndex);
                    if(f && LittleFS.exists(f)) {
                        hw.sound().stop();
                        hw.sound().playOnceBlocking(f, false);
                        waitingForStart = true;
                        phoneState = PHONE_CALL_MSG; callMsgPlayed = true;
                        haSensorState.setValue("playing_call");
                    } else { callModeActive = false; startDialtone(); }
                } else { startDialtone(); }
                processDialInput();
                break;

            case PHONE_DIALING:
                processDialInput();
                break;

            case PHONE_PLAYING:
                if(waitingForStart){if(hw.sound().isPlaying())waitingForStart=false;}
                else if(!hw.sound().isPlaying()){
                    hw.sound().playLoop(FILE_BUSY); phoneState=PHONE_BUSY;
                    haSensorState.setValue("busy");
                }
                break;

            case PHONE_BUSY:
                if(isHookedGlobal){hw.sound().stop();phoneState=PHONE_IDLE;resetPhone();haSensorState.setValue("onhook");}
                break;

            case PHONE_CALLING:
                if(!isHookedGlobal){
                    hw.sound().stop();stopRing();
                    const char* f=getCallMessageFile(callMessageIndex);
                    if(f&&LittleFS.exists(f)){
                        hw.sound().playOnceBlocking(f,false);waitingForStart=true;
                        phoneState=PHONE_CALL_MSG;callMsgPlayed=true;
                        haSensorState.setValue("playing_call");
                    }else{callModeActive=false;startDialtone();}
                }
                updateRing(now);
                break;

            case PHONE_CALL_MSG:
                if(waitingForStart){if(hw.sound().isPlaying())waitingForStart=false;}
                else if(!hw.sound().isPlaying()){
                    callModeActive=false;hw.sound().playLoop(FILE_BUSY);
                    phoneState=PHONE_BUSY;haSensorState.setValue("busy");
                }
                break;
        }
        updateRing(now);
        hw.sound().loop();
    }

    // ========================================================================
    // API PÚBLICA
    // ========================================================================

    void startCall() {
        if(!isHookedGlobal) return;
        callModeActive = true; callMsgPlayed = false;
        phoneState = PHONE_CALLING;
        startRing();
        haSensorState.setValue("calling");
        Serial.printf("[PHONE] Chamada iniciada. Msg: %d\n", callMessageIndex);
    }

    void endCall() {
        stopRing(); callModeActive = false; callMsgPlayed = false;
        if(phoneState == PHONE_CALLING || phoneState == PHONE_CALL_MSG) {
            phoneState = PHONE_IDLE; hw.sound().stop(); resetPhone();
            haSensorState.setValue("onhook");
        }
        Serial.println("[PHONE] Chamada encerrada.");
    }

    void ringBell(uint16_t durationMs = 3000) {
        if(phoneState != PHONE_IDLE) return;
        phoneState = PHONE_CALLING; ringDuration = durationMs; startRing();
    }

    void setLock(bool locked) {
        hw.digitalWrite(PIN_DOORLOCK, locked ? LOCK_ACTIVE : LOCK_INACTIVE);
        haLock.setState(locked ? HALock::StateLocked : HALock::StateUnlocked);
    }

    void selectMessage(int8_t index) {
        if(index >= 0 && index < CALL_MESSAGE_COUNT) {
            callMessageIndex = index;
            Serial.printf("[PHONE] Msg selecionada: %d\n", index);
        }
    }

    void setupHA() {
        haSensorHook.setName("Estado do Gancho");    haSensorHook.setIcon("mdi:phone");
        haSensorDialed.setName("Número Discado");    haSensorDialed.setIcon("mdi:phone-dial");
        haSensorState.setName("Estado do Telefone"); haSensorState.setIcon("mdi:phone-classic");

        haLock.setName("Fechadura");  haLock.setIcon("mdi:lock");
        haLock.onCommand(haLockCallback);

        haBtnRing.setName("Tocar Campainha");         haBtnRing.setIcon("mdi:bell-ring");
        haBtnRing.onCommand(haRingCallback);
        haBtnStartCall.setName("Iniciar Chamada");    haBtnStartCall.setIcon("mdi:phone-incoming");
        haBtnStartCall.onCommand(haStartCallCallback);
        haBtnEndCall.setName("Encerrar Chamada");     haBtnEndCall.setIcon("mdi:phone-hangup");
        haBtnEndCall.onCommand(haEndCallCallback);

        haSelectMsg.setName("Mensagem da Chamada");   haSelectMsg.setIcon("mdi:message-text");
        haSelectMsg.setOptions("Mensagem 1;Mensagem 2;Mensagem 3;Mensagem 4;Mensagem 5");
        haSelectMsg.onCommand(haSelectCallback);

        // Estado inicial
        haSensorHook.setValue(isHookedGlobal ? "on_hook" : "off_hook");
        haSensorDialed.setValue("---");
        haSensorState.setValue("onhook");
        haLock.setState(HALock::StateUnlocked);
    }

private:
    PhoneState phoneState = PHONE_IDLE;
    String dialed = "";
    bool audioPlayed = false, dialtoneSent = false, isBusyState = false;
    bool waitingForStart = false, newline = true;
    unsigned long lastDigitAt = 0;

    bool callModeActive = false, callMsgPlayed = false;
    uint8_t callMessageIndex = 0;

    bool ringActive = false;
    unsigned long ringToggleAt = 0, ringDuration = 3000, ringStartedAt = 0;
    bool ringPhase = false;

    void resetPhone() {
        dialed = ""; audioPlayed = false; dialtoneSent = false;
        isBusyState = false; waitingForStart = false; newline = true; stopRing();
        // Zera buffer do RotaryDial para evitar digito fantasma da ISR durante reproducao
        #ifdef USE_ROTARY_DIAL
        if (rotaryDial) rotaryDial->reset();
        #endif
    }

    void startDialtone() {
        if(!dialtoneSent && !hw.sound().isPlaying()) {
            hw.sound().playLoop(FILE_DIALTONE); dialtoneSent = true;
            phoneState = PHONE_OFFHOOK;
        }
    }

    void processCall() {
        String mp3Path = "/" + dialed + ".mp3";
        String txtPath = "/" + dialed + ".txt";
        hw.sound().stop(); audioPlayed = true; waitingForStart = true; newline = true;

        Serial.printf("[PHONE] SD: %s\n", mp3Path.c_str());
        if(SD.exists(mp3Path.c_str())) {
            hw.sound().playOnceBlocking(mp3Path.c_str(), true);
            phoneState = PHONE_PLAYING; haSensorState.setValue("playing");
            if(SD.exists(txtPath.c_str())) {
                hw.digitalWrite(PIN_DOORLOCK, LOCK_ACTIVE);
                haLock.setState(HALock::StateLocked);
                Serial.println("[PHONE] Fechadura acionada!");
            }
        } else {
            hw.sound().playOnceBlocking(FILE_INVALID);
            phoneState = PHONE_PLAYING; haSensorState.setValue("playing");
        }
        dialed = ""; haSensorDialed.setValue("---");
    }

    void checkDialTimeout() {
        bool m = (dialed.length() == MAX_PHONE_DIGITS);
        bool t = (!newline && millis() - lastDigitAt > DIAL_TIMEOUT_MS);
        if((m||t) && dialed.length()>0){
            Serial.printf("\n[PHONE] %s\n",m?"Limite!":"Timeout.");
            if(m) delay(300); processCall();
        }
    }

    void processDialInput() {
        #if defined(USE_ROTARY_DIAL)
        if(rotaryDial && rotaryDial->available()){
            int d=rotaryDial->read();
            if(d>=0&&d<=9){
                dialtoneSent=true;hw.sound().stop();
                hw.sound().playOnceBlocking(FILE_ROTARY[d]);
                dialed+=String(d);lastDigitAt=millis();newline=false;
                phoneState=PHONE_DIALING;haSensorDialed.setValue(dialed.c_str());
            }
        }else checkDialTimeout();
        #elif defined(KEYPAD_GPIO)
        char k=keypadGpio.getKey();
        if(k){
            dialtoneSent=true;hw.sound().stop();
            const char* p=nullptr;
            if(k>='0'&&k<='9')p=FILE_DTMF[k-'0'];
            else if(k=='*')p=FILE_DTMF_STAR;
            else if(k=='#')p=FILE_DTMF_HASH;
            if(p)hw.sound().playOnceBlocking(p);
            dialed+=k;lastDigitAt=millis();newline=false;
            phoneState=PHONE_DIALING;haSensorDialed.setValue(dialed.c_str());
        }else checkDialTimeout();
        #else
        char k=keypadListener.getLastKey();
        if(k!=0){
            dialtoneSent=true;hw.sound().stop();
            const char* p=nullptr;
            if(k>='0'&&k<='9')p=FILE_DTMF[k-'0'];
            else if(k=='*')p=FILE_DTMF_STAR;
            else if(k=='#')p=FILE_DTMF_HASH;
            if(p)hw.sound().playOnceBlocking(p);
            dialed+=k;lastDigitAt=keypadListener.getLastKeyTime();newline=false;
            phoneState=PHONE_DIALING;haSensorDialed.setValue(dialed.c_str());
        }else checkDialTimeout();
        #endif
    }

    void startRing() {
        ringActive=true;ringStartedAt=millis();ringToggleAt=millis();ringPhase=false;
        phoneState=PHONE_CALLING;
    }
    void stopRing(){
        ringActive=false;
        #ifdef USE_ROTARY_DIAL
        hw.digitalWrite(PIN_RING_A,LOW);hw.digitalWrite(PIN_RING_B,LOW);
        #else
        hw.digitalWrite(PIN_RING_A,LOW);
        #endif
    }
    void updateRing(unsigned long now){
        if(!ringActive)return;
        if(now-ringStartedAt>ringDuration){
            stopRing();
            if(isHookedGlobal){callModeActive=false;phoneState=PHONE_IDLE;resetPhone();haSensorState.setValue("onhook");}
            return;
        }
        if(now-ringToggleAt>=25){
            ringToggleAt=now;ringPhase=!ringPhase;
            #ifdef USE_ROTARY_DIAL
            hw.digitalWrite(PIN_RING_A,ringPhase?HIGH:LOW);
            hw.digitalWrite(PIN_RING_B,ringPhase?LOW:HIGH);
            #else
            hw.digitalWrite(PIN_RING_A,ringPhase?HIGH:LOW);
            #endif
        }
    }
    const char* getCallMessageFile(uint8_t i){return(i<CALL_MESSAGE_COUNT)?CALL_MESSAGE_FILES[i]:nullptr;}
};

// ============================================================================
// CALLBACKS HA (funções globais, chamam a instância phone)
// ============================================================================
void haLockCallback(HALock::LockCommand cmd, HALock*) {
    phone.setLock(cmd == HALock::CommandLock);
}
void haRingCallback(HAButton*) { phone.ringBell(3000); }
void haStartCallCallback(HAButton*) { phone.startCall(); }
void haEndCallCallback(HAButton*) { phone.endCall(); }
void haSelectCallback(int8_t idx, HASelect*) { phone.selectMessage(idx); }

// ============================================================================
// INSTÂNCIAS GLOBAIS (~linha 350)
// ============================================================================
PhonePuzzle phone;
LogicManager master(1);
ConfigManager netConfig;
WiFiClient wifiClient;
HADevice haDevice;
HAMqtt haMqtt(wifiClient, haDevice);

// ============================================================================
// SETUP (~linha 360)
// ============================================================================
void setup() {
    Serial.begin(115200);
    uint32_t start = millis();
    while(!Serial && (millis()-start < 3000)) delay(10);

    Serial.println(F("========================================"));
    Serial.println(F("  ESCAPE ROOM PHONE - ESP32-S3"));
    Serial.println(F("========================================"));

    hw.begin();
    hw.addPcf8575(0x20);

    netConfig.begin();
    if(netConfig.isAPMode()) Serial.println(F("[CFG] AP: http://192.168.4.1"));

    byte mac[6]; WiFi.macAddress(mac);
    haDevice.setUniqueId(mac, sizeof(mac));
    haDevice.setName(netConfig.devName());
    haDevice.setSoftwareVersion("1.0.0");

    phone.setupHA();
    haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());

    #if defined(USE_ROTARY_DIAL)
    rotaryDial = hw.createRotaryDial(PIN_ROTARY_PULSE);
    if(rotaryDial) Serial.println(F("[SETUP] Rotary OK"));
    else Serial.println(F("[SETUP] ERRO Rotary"));
    #elif defined(KEYPAD_GPIO)
    keypadGpio.begin(); Serial.println(F("[SETUP] Keypad GPIO OK"));
    #else
    for(int i=0;i<4;i++)keypadLayout.setRowPin(i,rowPins[i]);
    for(int i=0;i<3;i++)keypadLayout.setColPin(i,colPins[i]);
    keypadManager.initialise(hw.portExpander(),&keypadLayout,&keypadListener,false);
    Serial.println(F("[SETUP] Keypad I2C OK"));
    #endif

    master.addPuzzle(&phone);
    master.begin();
    phone.run();
    Serial.println(F("[PHONE] Pronto!"));
}

// ============================================================================
// LOOP (~linha 398)
// ============================================================================
void loop() {
    haMqtt.loop();
    netConfig.handleAP();
    master.update();
    delay(1);
}
