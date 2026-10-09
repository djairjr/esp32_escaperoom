#pragma once

#include <M5StamPLC.h>
#include <Wire.h>
#include <RCSwitch.h>
#include <MFRC522_I2C.h>
#include <Adafruit_PN532.h>
#include "Pins.h"

// I2C bus externo (PORT.A: Grove RED — pinos do M5StamPLC pin_config.h)
extern TwoWire PortA;

// ============================================================================
// INPUT MANAGER — Botões, RF433, Keypad PCF8574 (PORT.A), RFID MFRC522/PN532
// ============================================================================
//
// Nomenclatura (baseada no GameTimerPro original):
//   BTN_PREV  (BtnA)  = LEFT    — diminuir valor / navegar
//   BTN_MENU  (BtnB)  = SELECT  — entrar no menu de configuração / confirmar
//   BTN_NEXT  (BtnC)  = RIGHT   — aumentar valor / navegar
//   BTN_START (BOOT)  = DET     — iniciar contagem regressiva / reset pós-explosão
//
// RF433 mapeia mesmas funções + extras:
//   RF_PREV, RF_NEXT, RF_MENU, RF_START
//   RF_DETONATE — detona remotamente
//   RF_DEFUSE   — desarma remotamente
//   RF_SPEED    — acelera temporizador
//   RF_PAUSE    — pausa temporizador
//
// RFID: suporta dois módulos via PORT.A:
//   - M5Stack RFID 2 Unit (WS1850S/MFRC522, I2C @ 0x28)
//   - Adafruit PN532 (I2C @ 0x24)
// ============================================================================

class InputManager {
public:
    // ─── Botões físicos ──────────────────────────────────────────────
    enum Button : uint8_t {
        BTN_PREV  = 0,   // BtnA → LEFT / diminuir
        BTN_MENU  = 1,   // BtnB → SELECT / entrar menu/config
        BTN_NEXT  = 2,   // BtnC → RIGHT / aumentar
        BTN_START = 3    // BOOT → DET / iniciar contagem
    };

    // ─── Comandos RF433 ──────────────────────────────────────────────
    enum RFCommand : int8_t {
        RF_NONE      = -1,
        RF_PREV      = 0,   // LEFT / menu left
        RF_NEXT      = 1,   // RIGHT / menu right
        RF_MENU      = 2,   // SELECT / menu config
        RF_START     = 3,   // START / iniciar contagem
        RF_DETONATE  = 4,   // DETONATE remoto
        RF_DEFUSE    = 5,   // DEFUSE remoto
        RF_SPEED     = 6,   // SPEEDUP toggle
        RF_PAUSE     = 7    // PAUSE
    };

    RFCommand lastRFCommand = RF_NONE;

    // Callback para notificar mudanças de hardware (keypad, RFID)
    void (*onHardwareChange)(const char* status) = nullptr;

    void begin() {
        pinMode(PIN_BOOT, INPUT_PULLUP);

        // RF433 Receiver (SYN531R via PORT.C G4)
        pinMode(PIN_RF433_DATA, INPUT);
        rfReceiver_.enableReceive(PIN_RF433_DATA);
        rfReceiver_.setReceiveTolerance(60);

        // Keypad, RFID e PN532 são inicializados pelo i2cScan()
    }

    // ─── I2C Scan em AMBOS os barramentos ────────────────────────────
    // Retorna string com resultado para exibir no HA
    const char* i2cScan() {
        i2cResult_[0] = '\0';
        // Varre barramento interno (Wire - G13/G15)
        scanBus(Wire, "INTERNAL", i2cResult_ + strlen(i2cResult_));
        // Varre barramento externo PORT.A
        strcat(i2cResult_, "PORT.A:");
        scanBus(PortA, "", i2cResult_ + strlen(i2cResult_));
        return i2cResult_;
    }

    void scanBus(TwoWire& bus, const char* name, char* out) {
        char buf[64];
        if (strlen(name) > 0) {
            snprintf(buf, sizeof(buf), "%s:", name);
            strcat(out, buf);
        }
        byte count = 0;
        for (byte addr = 1; addr < 127; addr++) {
            bus.beginTransmission(addr);
            byte error = bus.endTransmission();
            if (error == 0) {
                count++;
                const char* devName = identifyDevice(addr);
                // Se detectou dispositivo novo, inicializa
                if (addr == 0x20 && !pcfOk_) {
                    pcfOk_ = true;
                    pcfAddr_ = addr;
                    writePCF(0xFF);
                    Serial.printf("[KEYPAD] PCF8574 found at 0x%02X on PORT.A\n", addr);
                }
                if (addr == 0x28 && !rfidOk_) {
                    beginMFRC522();
                }
                if (addr == 0x24 && !pn532Ok_) {
                    beginPN532();
                }
                char item[48];
                if (strlen(devName) > 0) {
                    snprintf(item, sizeof(item), "0x%02X(%s) ", addr, devName);
                } else {
                    snprintf(item, sizeof(item), "0x%02X ", addr);
                }
                strcat(out, item);
            }
        }
        char end[16];
        snprintf(end, sizeof(end), "[%d] ", count);
        strcat(out, end);

        if (strlen(name) > 0) {
            Serial.printf("[I2C] %s:\n", name);
            for (byte addr = 1; addr < 127; addr++) {
                bus.beginTransmission(addr);
                if (bus.endTransmission() == 0) {
                    Serial.printf("  0x%02X", addr);
                    const char* d = identifyDevice(addr);
                    if (strlen(d) > 0) Serial.printf(" %s", d);
                    Serial.println();
                }
            }
            Serial.printf("[I2C] %d devices found on %s\n", count, name);
        }
    }

    const char* getI2CResult() { return i2cResult_; }

    // ─── Hot-plug do Keypad ─────────────────────────────────────────
    // Chama no loop — detecta se PCF8574 foi conectado/desconectado
    bool pollKeypad() {
        bool present = false;
        PortA.beginTransmission(0x20);
        if (PortA.endTransmission() == 0) {
            present = true;
        }
        if (present && !pcfOk_) {
            // Keypad acabou de ser conectado
            pcfOk_ = true;
            pcfAddr_ = 0x20;
            writePCF(0xFF);
            Serial.println("[KEYPAD] PCF8574 CONNECTED (hot-plug)");
            if (onHardwareChange) onHardwareChange(i2cResult_);
        } else if (!present && pcfOk_) {
            // Keypad foi desconectado
            pcfOk_ = false;
            pcfAddr_ = 0xFF;
            Serial.println("[KEYPAD] PCF8574 DISCONNECTED");
            if (onHardwareChange) onHardwareChange(i2cResult_);
        }
        return pcfOk_;
    }

    // ─── Botões A/B/C + START (BOOT) ─────────────────────────────────
    bool wasPressed(Button btn) {
        switch (btn) {
            case BTN_PREV:  return M5.BtnA.wasPressed();
            case BTN_MENU:  return M5.BtnB.wasPressed();
            case BTN_NEXT:  return M5.BtnC.wasPressed();
            case BTN_START: return (digitalRead(PIN_BOOT) == LOW);
        }
        return false;
    }

    bool isHeld(Button btn, unsigned long ms) {
        switch (btn) {
            case BTN_PREV:  return M5.BtnA.pressedFor(ms);
            case BTN_MENU:  return M5.BtnB.pressedFor(ms);
            case BTN_NEXT:  return M5.BtnC.pressedFor(ms);
            case BTN_START: return false;
        }
        return false;
    }

    // ─── RF433 Polling ───────────────────────────────────────────────
    RFCommand pollRF() {
        if (rfReceiver_.available()) {
            unsigned long value = rfReceiver_.getReceivedValue();
            rfReceiver_.resetAvailable();
            if (value != 0) {
                lastRFCommand = mapRFValue(value);
                if (lastRFCommand != RF_NONE) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "[RF] Cmd:%d Val:%lu", lastRFCommand, value);
                    Serial.println(buf);
                }
                return lastRFCommand;
            }
        }
        return RF_NONE;
    }

    void clearRF() { lastRFCommand = RF_NONE; }

    // ─── DET Opto Input (AW9523B no barramento INTERNO) ─────────────
    bool isDetOptoActive() {
        auto& aw = M5StamPLC.getIOExpanderB();
        return (aw.digitalRead(4) == 0);  // P0_4 = index 4
    }

    // ─── Keypad Matrix via PCF8574 I2C @ 0x20 (PORT.A) ──────────────
    void beginKeypad() {
        if (pcfAddr_ == 0xFF) return;
        PortA.beginTransmission(pcfAddr_);
        if (PortA.endTransmission() == 0) {
            pcfOk_ = true;
            writePCF(0xFF);
            Serial.printf("[KEYPAD] PCF8574 @ 0x%02X on PORT.A\n", pcfAddr_);
        } else {
            Serial.printf("[KEYPAD] PCF8574 @ 0x%02X NOT FOUND on PORT.A\n", pcfAddr_);
        }
    }

    bool isKeypadOK() { return pcfOk_; }

    int getKey() {
        if (!pcfOk_) return -1;
        for (int r = 0; r < 4; r++) {
            uint8_t out = 0xFF;
            out &= ~(1 << KEYPAD_ROW_PINS[r]);
            PortA.beginTransmission(pcfAddr_);
            PortA.write(out);
            if (PortA.endTransmission() != 0) return -1;
            delayMicroseconds(200);
            PortA.requestFrom(pcfAddr_, (uint8_t)1);
            if (PortA.available()) {
                uint8_t in = PortA.read();
                for (int c = 0; c < 3; c++) {
                    if ((in & (1 << KEYPAD_COL_PINS[c])) == 0) {
                        delay(20);
                        PortA.requestFrom(pcfAddr_, (uint8_t)1);
                        if (!PortA.available()) continue;
                        in = PortA.read();
                        if ((in & (1 << KEYPAD_COL_PINS[c])) != 0) continue;
                        while (true) {
                            PortA.requestFrom(pcfAddr_, (uint8_t)1);
                            if (!PortA.available()) break;
                            in = PortA.read();
                            if ((in & (1 << KEYPAD_COL_PINS[c])) != 0) break;
                            delay(5);
                        }
                        writePCF(0xFF);
                        const char keyMap[4][3] = {
                            {'1','2','3'},
                            {'4','5','6'},
                            {'7','8','9'},
                            {'*','0','#'}
                        };
                        return keyMap[r][c];
                    }
                }
            }
        }
        writePCF(0xFF);
        return -1;
    }

    // ─── RFID interface genérica ─────────────────────────────────────
    // Suporta MFRC522 (0x28) ou PN532 (0x24)
    // O tipo é auto-detectado no i2cScan()

    // ─── MFRC522_I2C (WS1850S) via PORT.A I2C @ 0x28 ────────────────
    void beginMFRC522() {
        if (rfidOk_ || pn532Ok_) return;  // já tem outro RFID
        mfrc522_ = new MFRC522_I2C(0x28, -1, &PortA);
        mfrc522_->PCD_Init();
        if (!mfrc522_->PCD_PerformSelfTest()) {
            Serial.println("[RFID] MFRC522 self-test FAILED");
            delete mfrc522_;
            mfrc522_ = nullptr;
            return;
        }
        rfidType_ = RFID_MFRC522;
        rfidOk_ = true;
        Serial.println("[RFID] MFRC522_I2C (WS1850S) detected @ 0x28");
    }

    // ─── Adafruit PN532 via PORT.A I2C @ 0x24 ───────────────────────
    void beginPN532() {
        if (rfidOk_ || pn532Ok_) return;  // já tem outro RFID
        // Construtor PN532: (IRQ, Reset, TwoWire*)
        // PN532 não tem IRQ no módulo básico, -1 = não usado
        // Reset: alguns módulos usam GPIO, -1 = não conectado
        pn532_ = new Adafruit_PN532(-1, -1, &PortA);
        pn532_->begin();
        uint32_t ver = pn532_->getFirmwareVersion();
        if (ver == 0) {
            Serial.println("[RFID] PN532 not responding");
            delete pn532_;
            pn532_ = nullptr;
            return;
        }
        pn532_->SAMConfig();
        pn532_->setPassiveActivationRetries(0xFF);
        rfidType_ = RFID_PN532;
        rfidOk_ = true;
        pn532Ok_ = true;
        Serial.printf("[RFID] PN532 detected @ 0x24 (FW: %lu)\n", ver);
    }

    // ─── Leitura RFID unificada ──────────────────────────────────────
    // Retorna UID em hex, independente do chip
    bool readRFID(char* uidHex, uint8_t* uidLen) {
        if (!rfidOk_) return false;
        switch (rfidType_) {
            case RFID_MFRC522:
                return readMFRC522(uidHex, uidLen);
            case RFID_PN532:
                return readPN532(uidHex, uidLen);
            default:
                return false;
        }
    }

    bool hasRFID() { return rfidOk_; }

    // Retorna nome legível do chip RFID detectado
    const char* getRFIDType() {
        switch (rfidType_) {
            case RFID_MFRC522: return "MFRC522";
            case RFID_PN532:   return "PN532";
            default:           return "NONE";
        }
    }

private:
    enum RFIDType : uint8_t {
        RFID_NONE    = 0,
        RFID_MFRC522 = 1,
        RFID_PN532   = 2
    };

    char i2cResult_[128] = "";
    RCSwitch rfReceiver_;
    uint8_t pcfAddr_ = PCF_KEYPAD_ADDR;
    bool pcfOk_ = false;
    bool rfidOk_ = false;
    bool pn532Ok_ = false;
    RFIDType rfidType_ = RFID_NONE;
    MFRC522_I2C* mfrc522_ = nullptr;
    Adafruit_PN532* pn532_ = nullptr;

    // ─── Identifica dispositivo I2C pelo endereço ────────────────────
    const char* identifyDevice(byte addr) {
        switch (addr) {
            case 0x43: return "PI4IOE5V6408";
            case 0x59: return "AW9523B";
            case 0x32: return "RX8130CE";
            case 0x48: return "LM75B";
            case 0x40: return "INA226";
            case 0x28: return "MFRC522";
            case 0x24: return "PN532";
            case 0x20: return "PCF8574";
            default:   return "";
        }
    }

    void writePCF(uint8_t val) {
        PortA.beginTransmission(pcfAddr_);
        PortA.write(val);
        PortA.endTransmission();
    }

    // ─── Leitura via MFRC522 ─────────────────────────────────────────
    bool readMFRC522(char* uidHex, uint8_t* uidLen) {
        if (!mfrc522_) return false;
        if (!mfrc522_->PICC_IsNewCardPresent()) return false;
        if (!mfrc522_->PICC_ReadCardSerial()) return false;

        *uidLen = mfrc522_->uid.size;
        if (*uidLen > 7) *uidLen = 7;
        for (uint8_t i = 0; i < *uidLen; i++) {
            sprintf(uidHex + (i * 2), "%02X", mfrc522_->uid.uidByte[i]);
        }
        uidHex[*uidLen * 2] = '\0';

        mfrc522_->PICC_HaltA();
        return true;
    }

    // ─── Leitura via PN532 ───────────────────────────────────────────
    bool readPN532(char* uidHex, uint8_t* uidLen) {
        if (!pn532_) return false;
        uint8_t uid[7];
        uint8_t len = 0;
        if (!pn532_->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len, 50)) {
            return false;
        }
        *uidLen = len;
        if (*uidLen > 7) *uidLen = 7;
        for (uint8_t i = 0; i < *uidLen; i++) {
            sprintf(uidHex + (i * 2), "%02X", uid[i]);
        }
        uidHex[*uidLen * 2] = '\0';
        return true;
    }

    // ─── Mapeamento RF ───────────────────────────────────────────────
    RFCommand mapRFValue(unsigned long value) {
        switch (value) {
            case 0x123456: return RF_PREV;
            case 0x234567: return RF_NEXT;
            case 0x345678: return RF_MENU;
            case 0x456789: return RF_START;
            case 0x56789A: return RF_DETONATE;
            case 0x6789AB: return RF_DEFUSE;
            case 0x789ABC: return RF_SPEED;
            case 0x89ABCD: return RF_PAUSE;
            default:
                uint8_t d = value % 10;
                if (d <= 7) return (RFCommand)d;
                return RF_NONE;
        }
    }
};
