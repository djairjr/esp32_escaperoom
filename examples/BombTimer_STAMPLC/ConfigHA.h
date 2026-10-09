#pragma once

#include <M5StamPLC.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoHA.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

typedef void (*VoidCallback)();

// ============================================================================
// ConfigHA — WiFi + ConfigManager (AP mode + webserver) + ArduinoHA
// ============================================================================
// Inclui sensores e controles para Home Assistant:
//   - Estado, tempo, fios, tentativas
//   - Botões: Iniciar, Resetar
//   - Números: Código desarme, Countdown, Brilho
//   - Select: Modo desarme, Fios (DET/SPEED/PAUSE)
//   - Switch: Som Tick
// ============================================================================

class ConfigHA {
public:
    void begin() {
        if (!SPIFFS.begin(true)) {
            Serial.println("[CFG] SPIFFS mount FAIL");
        } else {
            Serial.println("[CFG] SPIFFS OK");
        }
        loadConfig();
        if (strlen(wifi_.ssid) > 0) {
            Serial.printf("[WIFI] Conectando %s... ", wifi_.ssid);
            WiFi.mode(WIFI_STA);
            WiFi.begin(wifi_.ssid, wifi_.pass);
            int att = 0;
            while (WiFi.status() != WL_CONNECTED && att < 40) {
                delay(500);
                Serial.print(".");
                att++;
            }
            if (WiFi.status() == WL_CONNECTED) {
                Serial.printf(" IP:%s\n", WiFi.localIP().toString().c_str());
                wifiOK_ = true;
                beginHA();
                return;
            }
            Serial.println(" FAIL");
        }
        startAP();
    }

    void loopHA() {
        if (apSrv_) apSrv_->handleClient();
        if (WiFi.isConnected() && !haOK_) {
            beginHA();
        }
        if (haOK_) {
            haMqtt_.loop();
            // Publica I2C status pendente após MQTT estar rodando
            if (i2cPending_ && strlen(i2cStatus_) > 0) {
                haSensorI2C_.setValue(i2cStatus_);
                i2cPending_ = false;
            }
            // Detecta transição disconnected→connected e publica tudo
            static bool lastMqttConnected = false;
            bool nowConnected = haMqtt_.isConnected();
            if (nowConnected && !lastMqttConnected) {
                lastMqttConnected = true;
                extern GameLogic game;
                extern ConfigHA configHA;
                configHA.publishAll();
                Serial.println(F("[HA] MQTT connected — full state published"));
            }
            if (!nowConnected) {
                lastMqttConnected = false;
            }
        }
    }

    void publishAll() {
        if (!haOK_) return;
        extern GameLogic game;
        haSensorState_.setValue(stateName(game.getState()));
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", game.getRemainingSeconds());
        haSensorTime_.setValue(buf);
        snprintf(buf, sizeof(buf), "%d", game.getWiresCut());
        haSensorWiresCut_.setValue(buf);
        snprintf(buf, sizeof(buf), "%d", game.getCodeAttempts());
        haSensorCodeAttempts_.setValue(buf);
        haSensorMode_.setValue(modeName(game.config.defuseMode));
        haNumCountdown_.setState((int32_t)game.config.countdownSeconds);
        haNumDefuseCode_.setState((int32_t)game.config.defuseCode);
        haNumBrightness_.setState((int32_t)game.config.brightness);
        haSwitchTick_.setState(game.config.tickEnabled);
        // Atualiza selects com índice correto
        haSelectMode_.setState((int8_t)game.config.defuseMode);
        haSelectDetWire_.setState((int8_t)game.config.detWire);
        haSelectDefuseWire_.setState((int8_t)game.config.defuseWire);
        haSelectSpeedWire_.setState((int8_t)game.config.speedupWire);
        haSelectPauseWire_.setState((int8_t)game.config.pauseWire);
        // Publica I2C status (se disponível)
        if (strlen(i2cStatus_) > 0) {
            haSensorI2C_.setValue(i2cStatus_);
        }
    }

    void publishWires() {
        if (!haOK_) return;
        char buf[8];
        for (int i = 0; i < 4; i++) {
            snprintf(buf, sizeof(buf), "%d", wires_[i].state);
            wires_[i].sensor->setValue(buf);
        }
    }

    void setWireCut(int idx, bool cut) {
        if (idx < 0 || idx > 3) return;
        wires_[idx].state = cut ? 1 : 0;
        publishWires();
    }

    void setI2CStatus(const char* status) {
        strlcpy(i2cStatus_, status, sizeof(i2cStatus_));
        i2cPending_ = true;
        if (!haOK_) return;
        haSensorI2C_.setValue(i2cStatus_);
        i2cPending_ = false;
    }
    bool isWiFiOK() { return wifiOK_; }
    bool isMQTTOK() { return haOK_ && haMqtt_.isConnected(); }
    bool isAPMode() { return apSrv_ != nullptr; }
    const char* deviceName() { return dev_.name; }

    VoidCallback onStart = nullptr;
    VoidCallback onReset = nullptr;

private:
    struct WifiCfg { char ssid[64] = ""; char pass[64] = ""; };
    struct MqttCfg { char broker[64] = ""; uint16_t port = 1883; char user[64] = ""; char pass[64] = ""; };
    struct DevCfg { char name[32] = "bombtimer-stamplc"; };
    WifiCfg wifi_;
    MqttCfg mqtt_;
    DevCfg dev_;

    bool wifiOK_ = false;
    WebServer* apSrv_ = nullptr;

    bool haOK_ = false;
    char i2cStatus_[128] = "";
    bool i2cPending_ = false;
    WiFiClient wifiClient_;
    HADevice haDevice_;
    HAMqtt haMqtt_{wifiClient_, haDevice_};

    // ─── Sensors ─────────────────────────────────────────────────────
    HASensor haSensorState_{"state"};
    HASensor haSensorTime_{"time"};
    HASensor haSensorWiresCut_{"wires_cut"};
    HASensor haSensorCodeAttempts_{"code_attempts"};
    HASensor haSensorMode_{"mode"};
    HASensor haSensorI2C_{"i2c_status"};

    struct WireSensor {
        HASensor* sensor;
        int state;  // 0=connected, 1=cut
    };
    WireSensor wires_[4] = {
        {new HASensor("wire1"), 0},
        {new HASensor("wire2"), 0},
        {new HASensor("wire3"), 0},
        {new HASensor("wire4"), 0},
    };

    // ─── Buttons ─────────────────────────────────────────────────────
    HAButton haBtnStart_{"start"};
    HAButton haBtnReset_{"reset"};

    // ─── Numbers ─────────────────────────────────────────────────────
    HANumber haNumCountdown_{"countdown"};
    HANumber haNumDefuseCode_{"defuse_code"};
    HANumber haNumBrightness_{"brightness"};

    // ─── Selects ─────────────────────────────────────────────────────
    HASelect haSelectMode_{"mode"};
    HASelect haSelectDetWire_{"det_wire"};
    HASelect haSelectDefuseWire_{"defuse_wire"};
    HASelect haSelectSpeedWire_{"speed_wire"};
    HASelect haSelectPauseWire_{"pause_wire"};

    // ─── Switch ──────────────────────────────────────────────────────
    HASwitch haSwitchTick_{"tick_sound"};

    // ─── Helpers ─────────────────────────────────────────────────────
    static const char* stateName(int s) {
        switch (s) {
            case 0: return "idle";
            case 1: return "running";
            case 2: return "defused";
            case 3: return "detonated";
            case 4: return "menu";
            case 5: return "alarm";
            default: return "unknown";
        }
    }
    static const char* modeName(int m) {
        switch (m) {
            case 0: return "CODE";
            case 1: return "WIRE";
            case 2: return "ORDER";
            default: return "?";
        }
    }

    // ─── Load/Save ───────────────────────────────────────────────────
    void loadConfig() {
        File f = SPIFFS.open("/wifi.json", "r");
        if (!f) { Serial.println("[CFG] No wifi.json, usando defaults"); return; }
        JsonDocument doc;
        if (deserializeJson(doc, f)) { f.close(); return; }
        f.close();
        if (doc["wifi"]["ssid"].is<const char*>())
            strlcpy(wifi_.ssid, doc["wifi"]["ssid"], sizeof(wifi_.ssid));
        if (doc["wifi"]["pass"].is<const char*>())
            strlcpy(wifi_.pass, doc["wifi"]["pass"], sizeof(wifi_.pass));
        if (doc["mqtt"]["broker"].is<const char*>()) {
            strlcpy(mqtt_.broker, doc["mqtt"]["broker"], sizeof(mqtt_.broker));
            mqtt_.port = doc["mqtt"]["port"] | 1883;
            strlcpy(mqtt_.user, doc["mqtt"]["user"] | "", sizeof(mqtt_.user));
            strlcpy(mqtt_.pass, doc["mqtt"]["pass"] | "", sizeof(mqtt_.pass));
        }
        strlcpy(dev_.name, doc["device"]["name"] | "bombtimer-stamplc", sizeof(dev_.name));
    }

    void saveConfig() {
        JsonDocument doc;
        doc["wifi"]["ssid"] = wifi_.ssid;
        doc["wifi"]["pass"] = wifi_.pass;
        doc["mqtt"]["broker"] = mqtt_.broker;
        doc["mqtt"]["port"] = mqtt_.port;
        doc["mqtt"]["user"] = mqtt_.user;
        doc["mqtt"]["pass"] = mqtt_.pass;
        doc["device"]["name"] = dev_.name;
        File f = SPIFFS.open("/wifi.json", FILE_WRITE);
        if (f) { serializeJson(doc, f); f.close(); Serial.println("[CFG] WiFi config saved"); }
    }

    // ─── AP Mode ─────────────────────────────────────────────────────
    void startAP() {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("BombTimer-STAMPLC");
        Serial.printf("[AP] BombTimer-STAMPLC http://%s\n", WiFi.softAPIP().toString().c_str());
        apSrv_ = new WebServer(80);
        apSrv_->on("/", HTTP_GET, [this]() {
            String html = R"rawliteral(
<html><head><meta charset=UTF-8><meta name=viewport content='width=device-width,initial-scale=1'>
<style>body{font-family:system-ui;max-width:500px;margin:auto;padding:20px}
label{display:block;margin-top:10px;font-weight:bold}
input{width:100%;padding:8px;margin-top:4px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}
button{margin-top:20px;padding:10px 20px;background:#06c;color:white;border:none;border-radius:4px;cursor:pointer;font-size:16px}
.s{border:1px solid #ddd;border-radius:8px;padding:15px;margin-top:15px}
</style><body><h2>BombTimer STAMPLC</h2>
<form action=/save method=POST>
<div class=s><h3>WiFi</h3><label>SSID: <input name=ws required value=')rawliteral";
            html += wifi_.ssid;
            html += R"rawliteral('></label><label>Senha: <input name=wp type=password value=')rawliteral";
            html += wifi_.pass;
            html += R"rawliteral('></label></div>
<div class=s><h3>MQTT</h3><label>Broker: <input name=mb value=')rawliteral";
            html += mqtt_.broker;
            html += R"rawliteral('></label><label>Porta: <input name=mp value=)rawliteral";
            html += String(mqtt_.port);
            html += R"rawliteral( type=number></label><label>User: <input name=mu value=')rawliteral";
            html += mqtt_.user;
            html += R"rawliteral('></label><label>Pass: <input name=mpw type=password></label></div>
<div class=s><h3>Dispositivo</h3><label>Nome: <input name=dn value=')rawliteral";
            html += dev_.name;
            html += R"rawliteral('></label></div>
<button type=submit>Salvar e Reiniciar</button></form></body></html>)rawliteral";
            apSrv_->send(200, "text/html", html);
        });
        apSrv_->on("/save", HTTP_POST, [this]() {
            if (apSrv_->hasArg("ws")) strlcpy(wifi_.ssid, apSrv_->arg("ws").c_str(), sizeof(wifi_.ssid));
            if (apSrv_->hasArg("wp")) strlcpy(wifi_.pass, apSrv_->arg("wp").c_str(), sizeof(wifi_.pass));
            if (apSrv_->hasArg("mb")) strlcpy(mqtt_.broker, apSrv_->arg("mb").c_str(), sizeof(mqtt_.broker));
            mqtt_.port = apSrv_->arg("mp").toInt();
            if (apSrv_->hasArg("mu")) strlcpy(mqtt_.user, apSrv_->arg("mu").c_str(), sizeof(mqtt_.user));
            if (apSrv_->hasArg("mpw")) strlcpy(mqtt_.pass, apSrv_->arg("mpw").c_str(), sizeof(mqtt_.pass));
            if (apSrv_->hasArg("dn")) strlcpy(dev_.name, apSrv_->arg("dn").c_str(), sizeof(dev_.name));
            saveConfig();
            apSrv_->send(200, "text/html", "<html><body><h2>Salvo! Reiniciando...</h2></body></html>");
            delay(1000);
            ESP.restart();
        });
        apSrv_->begin();
    }

    // ─── HA ──────────────────────────────────────────────────────────
    void beginHA() {
        if (haOK_) return;
        if (!WiFi.isConnected()) return;

        byte mac[6];
        WiFi.macAddress(mac);
        haDevice_.setUniqueId(mac, sizeof(mac));
        haDevice_.setName(dev_.name);

        // Sensors
        haSensorState_.setName("Estado");
        haSensorState_.setIcon("mdi:bomb");
        haSensorTime_.setName("Tempo Restante");
        haSensorTime_.setUnitOfMeasurement("s");
        haSensorTime_.setIcon("mdi:timer");
        haSensorWiresCut_.setName("Fios Cortados");
        haSensorWiresCut_.setIcon("mdi:cable-data");
        haSensorCodeAttempts_.setName("Tentativas");
        haSensorCodeAttempts_.setIcon("mdi:counter");
        haSensorMode_.setName("Modo Desarme");
        haSensorMode_.setIcon("mdi:tune");
        haSensorI2C_.setName("I2C Devices");
        haSensorI2C_.setIcon("mdi:bus-electrical");
        for (int i = 0; i < 4; i++) {
            char name[16];
            snprintf(name, sizeof(name), "Fio %d", i + 1);
            wires_[i].sensor->setName(name);
            wires_[i].sensor->setIcon("mdi:cable-data");
        }

        // Buttons
        haBtnStart_.setName("Iniciar");
        haBtnStart_.setIcon("mdi:play");
        haBtnStart_.onCommand(haStartCb);
        haBtnReset_.setName("Resetar");
        haBtnReset_.setIcon("mdi:restart");
        haBtnReset_.onCommand(haResetCb);

        // Numbers
        haNumCountdown_.setName("Tempo (segundos)");
        haNumCountdown_.setIcon("mdi:timer-outline");
        haNumCountdown_.setMin(5);
        haNumCountdown_.setMax(5999);
        haNumCountdown_.setStep(1);
        haNumCountdown_.setUnitOfMeasurement("s");
        haNumCountdown_.onCommand(haCountdownCb);
        haNumDefuseCode_.setName("Codigo Desarme");
        haNumDefuseCode_.setIcon("mdi:lock");
        haNumDefuseCode_.setMin(0);
        haNumDefuseCode_.setMax(9999);
        haNumDefuseCode_.setStep(1);
        haNumDefuseCode_.onCommand(haDefuseCodeCb);
        haNumBrightness_.setName("Brilho");
        haNumBrightness_.setIcon("mdi:brightness-6");
        haNumBrightness_.setMin(0);
        haNumBrightness_.setMax(15);
        haNumBrightness_.setStep(1);
        haNumBrightness_.onCommand(haBrightnessCb);

        // Selects
        haSelectMode_.setName("Modo Desarme");
        haSelectMode_.setIcon("mdi:tune");
        haSelectMode_.setOptions("CODE;WIRE;ORDER");
        haSelectMode_.onCommand(haModeCb);
        haSelectDetWire_.setName("Fio DET");
        haSelectDetWire_.setIcon("mdi:alert");
        haSelectDetWire_.setOptions("RAND;1;2;3;4;NONE");
        haSelectDetWire_.onCommand(haDetWireCb);
        haSelectDefuseWire_.setName("Fio DEFUSE");
        haSelectDefuseWire_.setIcon("mdi:shield-check");
        haSelectDefuseWire_.setOptions("RAND;1;2;3;4;NONE");
        haSelectDefuseWire_.onCommand(haDefuseWireCb);
        haSelectSpeedWire_.setName("Fio SPEED");
        haSelectSpeedWire_.setIcon("mdi:speedometer");
        haSelectSpeedWire_.setOptions("RAND;1;2;3;4;NONE");
        haSelectSpeedWire_.onCommand(haSpeedWireCb);
        haSelectPauseWire_.setName("Fio PAUSE");
        haSelectPauseWire_.setIcon("mdi:pause");
        haSelectPauseWire_.setOptions("RAND;1;2;3;4;NONE");
        haSelectPauseWire_.onCommand(haPauseWireCb);

        // Switch
        haSwitchTick_.setName("Som Tick");
        haSwitchTick_.setIcon("mdi:volume-high");
        haSwitchTick_.onCommand(haTickCb);

        haMqtt_.begin(mqtt_.broker, mqtt_.user, mqtt_.pass);
        haOK_ = true;
        Serial.printf("[HA] MQTT %s:%d\n", mqtt_.broker, mqtt_.port);
    }

    // ─── Static Callbacks ────────────────────────────────────────────
    static void haStartCb(HAButton*) {
        extern ConfigHA configHA;
        if (configHA.onStart) configHA.onStart();
    }
    static void haResetCb(HAButton*) {
        extern ConfigHA configHA;
        if (configHA.onReset) configHA.onReset();
    }
    static void haCountdownCb(HANumeric val, HANumber*) {
        extern GameLogic game;
        void refreshDisplayIfIdle();
        int v = constrain(val.toInt32(), 5, 5999);
        game.config.countdownSeconds = v;
        game.saveConfig();
        refreshDisplayIfIdle();
        Serial.printf("[HA] countdown set to %d via MQTT\n", v);
    }
    static void haDefuseCodeCb(HANumeric val, HANumber*) {
        extern GameLogic game;
        int v = constrain(val.toInt32(), 0, 9999);
        game.config.defuseCode = v;
        game.saveConfig();
        Serial.printf("[HA] defuseCode set to %04d via MQTT\n", v);
    }
    static void haBrightnessCb(HANumeric val, HANumber*) {
        extern GameLogic game;
        int v = constrain(val.toInt32(), 0, 15);
        game.config.brightness = v;
        game.saveConfig();
        Serial.printf("[HA] brightness set to %d via MQTT\n", v);
    }
    static void haModeCb(int8_t idx, HASelect*) {
        extern GameLogic game;
        void refreshDisplayIfIdle();
        game.config.defuseMode = (GameLogic::DefuseMode)constrain(idx, 0, 2);
        game.saveConfig();
        refreshDisplayIfIdle();
        Serial.printf("[HA] defuseMode set to %d via MQTT\n", idx);
    }
    static void haDetWireCb(int8_t idx, HASelect*) {
        extern GameLogic game;
        int newVal = idx; // 0=RAND, 1-4, 5=NONE
        if (newVal > 0 && newVal < 5 && game.wireConflict(1, newVal)) return;
        game.config.detWire = newVal;
        game.saveConfig();
        Serial.printf("[HA] detWire set to %d via MQTT\n", newVal);
    }
    static void haDefuseWireCb(int8_t idx, HASelect*) {
        extern GameLogic game;
        int newVal = idx;
        if (newVal > 0 && newVal < 5 && game.wireConflict(4, newVal)) return;
        game.config.defuseWire = newVal;
        game.saveConfig();
        Serial.printf("[HA] defuseWire set to %d via MQTT\n", newVal);
    }
    static void haSpeedWireCb(int8_t idx, HASelect*) {
        extern GameLogic game;
        int newVal = idx;
        if (newVal > 0 && newVal < 5 && game.wireConflict(2, newVal)) return;
        game.config.speedupWire = newVal;
        game.saveConfig();
        Serial.printf("[HA] speedWire set to %d via MQTT\n", newVal);
    }
    static void haPauseWireCb(int8_t idx, HASelect*) {
        extern GameLogic game;
        int newVal = idx;
        if (newVal > 0 && newVal < 5 && game.wireConflict(3, newVal)) return;
        game.config.pauseWire = newVal;
        game.saveConfig();
        Serial.printf("[HA] pauseWire set to %d via MQTT\n", newVal);
    }
    static void haTickCb(bool state, HASwitch*) {
        extern GameLogic game;
        game.config.tickEnabled = state;
        game.saveConfig();
        Serial.printf("[HA] tick set to %d via MQTT\n", state);
    }
};
