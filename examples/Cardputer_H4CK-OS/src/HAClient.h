#pragma once

#include <M5Cardputer.h>
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoHA.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

#include "Config.h"
#include "ConfigManager.h"

// ============================================================================
// DEBUG SERIAL PARA MQTT/HA — descomente abaixo para ativar
// ============================================================================
// #define HA_DEBUG_SERIAL

#ifdef HA_DEBUG_SERIAL
  #define HA_ERR(fmt, ...) Serial.printf("[HA-ERR] " fmt "\n", ##__VA_ARGS__)
  #define HA_INF(fmt, ...) Serial.printf("[HA-INF] " fmt "\n", ##__VA_ARGS__)
  #define HA_DBG(fmt, ...) Serial.printf("[HA-DBG] " fmt "\n", ##__VA_ARGS__)
#else
  #define HA_ERR(...) do{}while(0)
  #define HA_INF(...) do{}while(0)
  #define HA_DBG(...) do{}while(0)
#endif

extern ConfigManager configManager;

class HAClient {
public:
    HAClient()
        : wifiClientPub_(), wifiClientHA_(),
          mqttClient_(wifiClientPub_),
          haDevice_(), haMqtt_(wifiClientHA_, haDevice_),
          haSensorEstado_("h4ckos_status"),
          haSensorLogin_("h4ckos_login_password"),
          haSensorSniffIp_("h4ckos_sniff_ip"),
          haSensorLastPayload_("h4ckos_last_payload"),
          haNumDefuseCode_("h4ckos_auth_token"),
          haNumSniffChannel_("h4ckos_sniff_channel"),
          haSelectStage_("h4ckos_stage"),
          haSwitchRF433_("h4ckos_rf433_enable")
    {
        mqttClient_.setCallback([this](char* topic, byte* payload, unsigned int len) {
            mqttCallback(topic, payload, len);
        });
        for (int i = 0; i < 3; i++) invadeSlotBuf_[i][0] = '\0';
    }

    void begin() {
        HA_DBG("begin() — SSID='%s' len=%d", configManager.ha.wifiSsid, (int)strlen(configManager.ha.wifiSsid));
        HA_DBG("begin() — Broker='%s' Port=%d", configManager.ha.broker, (int)configManager.ha.port);
        HA_DBG("begin() — User='%s' (len=%d) Pass='%s' (len=%d)",
               configManager.ha.user, (int)strlen(configManager.ha.user),
               strlen(configManager.ha.pass)>0?"***":"(none)", (int)strlen(configManager.ha.pass));

        if (strlen(configManager.ha.wifiSsid) == 0) {
            HA_ERR("WiFi SSID vazio — iniciando AP mode");
            Serial.println("[WIFI] No SSID — starting AP mode");
            startAP();
            return;
        }

        Serial.printf("[WIFI] Connecting to %s... ", configManager.ha.wifiSsid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(configManager.ha.wifiSsid, configManager.ha.wifiPass);

        int att = 0;
        while (WiFi.status() != WL_CONNECTED && att < 40) {
            delay(500);
            Serial.print(".");
            att++;
        }
        Serial.println();
        HA_DBG("WiFi loop terminou status=%d tentativas=%d", WiFi.status(), att);

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf(" IP:%s\n", WiFi.localIP().toString().c_str());
            HA_INF("WiFi OK IP=%s MAC=%s RSSI=%d",
                   WiFi.localIP().toString().c_str(),
                   WiFi.macAddress().c_str(), WiFi.RSSI());
            wifiOK_ = true;
            beginMQTT();
            beginHA();
        } else {
            HA_ERR("WiFi falhou (status=%d) — iniciando AP mode", WiFi.status());
            Serial.println(" FAIL — starting AP mode");
            startAP();
        }
    }

    void loopHA() {
        if (apSrv_) apSrv_->handleClient();
        if (!wifiOK_) return;

        if (!mqttOK_) {
            static unsigned long lastRaw = 0;
            if (millis() - lastRaw > 5000) {
                lastRaw = millis();
                HA_DBG("loopHA: MQTT RAW não conectado — beginMQTT()");
                beginMQTT();
            }
        } else {
            mqttClient_.loop();
            static unsigned long lastRawReconnect = 0;
            if (!mqttClient_.connected() && millis() - lastRawReconnect > 10000) {
                lastRawReconnect = millis();
                HA_DBG("loopHA: MQTT RAW caiu — reconnectMQTT()");
                reconnectMQTT();
            }
        }

        if (!haOK_) {
            static unsigned long lastHaStart = 0;
            if (millis() - lastHaStart > 5000) {
                lastHaStart = millis();
                HA_DBG("loopHA: ArduinoHA não iniciado — beginHA()");
                beginHA();
            }
        } else {
            haMqtt_.loop();
            static bool lastHaConnected = false;
            bool nowConnected = haMqtt_.isConnected();
            HA_DBG("loopHA: haMqtt_.isConnected()=%s last=%s",
                   nowConnected?"true":"false", lastHaConnected?"true":"false");
            if (nowConnected && !lastHaConnected) {
                lastHaConnected = true;
                HA_INF("HA MQTT conectou — publicando estado completo");
                publishAll();
                Serial.println(F("[HA] MQTT reconnected — full state published"));
            }
            if (!nowConnected && lastHaConnected) {
                lastHaConnected = false;
                HA_ERR("HA MQTT desconectou!");
            }
        }

        static unsigned long lastDiag = 0;
        if (millis() - lastDiag > 30000) {
            lastDiag = millis();
            HA_INF("DIAG | WiFi=%s MQTT=%s HA=%s/%s heap=%d up=%lus",
                   wifiOK_?"OK":"FAIL",
                   mqttOK_?"OK":(wifiOK_?"FAIL":"N/A"),
                   haOK_?(haMqtt_.isConnected()?"CONN":"INIT"):"FAIL",
                   haMqtt_.isConnected()?"ON":"OFF",
                   ESP.getFreeHeap(), millis()/1000);
        }
    }

    bool isWiFiOK() { return wifiOK_; }
    bool isMQTTOK() { return mqttOK_; }
    bool isAPMode() { return apSrv_ != nullptr; }

    // ─── invade: envia TOGGLE no tópico MQTT do target ─────────
    bool invadeTarget(const char* targetName) {
        if (!mqttOK_) {
            HA_ERR("invadeTarget('%s'): MQTT não OK", targetName);
            return false;
        }
        for (int i = 0; i < configManager.targetCount; i++) {
            if (strcasecmp(configManager.targets[i].name, targetName) == 0) {
                const char* topic = configManager.targets[i].topic;
                bool ok = mqttClient_.publish(topic, "TOGGLE");
                HA_INF("invadeTarget('%s'): topic=%s result=%s", targetName, topic, ok?"OK":"FAIL");
                Serial.printf("[INVADE] Published TOGGLE to %s (%s)\n", targetName, topic);
                return true;
            }
        }
        HA_ERR("invadeTarget('%s'): target não encontrado", targetName);
        return false;
    }

    void publishAll() {
        if (!haOK_) { HA_ERR("publishAll(): haOK_=false"); return; }
        if (!haMqtt_.isConnected()) {
            HA_ERR("publishAll(): HA MQTT desconectado"); return;
        }
        HA_DBG("publishAll() — publicando entidades");
        haSensorEstado_.setValue(lastStatus_);
        haSensorLogin_.setValue(configManager.login.pass);
        haSensorSniffIp_.setValue(configManager.sniff.ip);
        haNumDefuseCode_.setState((int32_t)configManager.sniff.defuseCode);
        haNumSniffChannel_.setState((int8_t)(configManager.sniff.channel - 1));
        haSelectStage_.setState((int8_t)configManager.stages.current);
        haSwitchRF433_.setState(rf433Enabled_);
        HA_INF("publishAll() — estado completo publicado");
    }

    void publishStatus(const char* s) {
        strlcpy(lastStatus_, s, sizeof(lastStatus_));
        HA_DBG("publishStatus('%s')", s);
        if (haOK_ && haMqtt_.isConnected()) haSensorEstado_.setValue(s);
    }

    void lastPayload(const char* payloadName) {
        HA_DBG("lastPayload('%s')", payloadName);
        if (haOK_ && haMqtt_.isConnected()) haSensorLastPayload_.setValue(payloadName);
        Serial.printf("[HA] Payload sent: %s\n", payloadName);
    }

    void setRF433Enabled(bool en) { rf433Enabled_ = en; }
    bool isRF433Enabled() { return rf433Enabled_; }

private:
    bool wifiOK_ = false;
    WebServer* apSrv_ = nullptr;
    bool mqttOK_ = false;
    bool haOK_ = false;
    bool rf433Enabled_ = true;
    char lastStatus_[16] = "boot";

    WiFiClient wifiClientPub_;
    WiFiClient wifiClientHA_;
    PubSubClient mqttClient_;
    HADevice haDevice_;
    HAMqtt haMqtt_;

    // ─── Sensors ─────────────────────────────────────────────────
    HASensor haSensorEstado_;
    HASensor haSensorLogin_;
    HASensor haSensorSniffIp_;
    HASensor haSensorLastPayload_;

    // ─── Numbers ─────────────────────────────────────────────────
    HANumber haNumDefuseCode_;
    HANumber haNumSniffChannel_;

    // ─── Select ──────────────────────────────────────────────────
    HASelect haSelectStage_;

    // ─── Switch ──────────────────────────────────────────────────
    HASwitch haSwitchRF433_;
    char invadeSlotBuf_[3][96];  // cache: "nome=topic" por slot

    // ================================================================
    // MQTT RAW (para targets + comandos do GM)
    // ================================================================
    void beginMQTT() {
        if (mqttOK_) { HA_DBG("beginMQTT(): já conectado"); return; }
        if (!wifiOK_) { HA_ERR("beginMQTT(): WiFi não OK"); return; }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("beginMQTT(): broker vazio"); return;
        }

        HA_INF("beginMQTT() — broker=%s port=%d", configManager.ha.broker, (int)configManager.ha.port);
        mqttClient_.setServer(configManager.ha.broker, configManager.ha.port);
        reconnectMQTT();
    }

    void reconnectMQTT() {
        if (!wifiOK_) { HA_ERR("reconnectMQTT(): WiFi não OK"); return; }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("reconnectMQTT(): broker vazio"); return;
        }

        char clientId[32];
        snprintf(clientId, sizeof(clientId), "H4CKOS_%08X", (uint32_t)ESP.getEfuseMac());

        HA_INF("reconnectMQTT() — broker=%s:%d clientId=%s", configManager.ha.broker, (int)configManager.ha.port, clientId);
        HA_DBG("  user='%s' pass='%s' lens=%d/%d",
               configManager.ha.user, configManager.ha.pass,
               strlen(configManager.ha.user), strlen(configManager.ha.pass));

        bool ok = mqttClient_.connect(clientId,
            strlen(configManager.ha.user) > 0 ? configManager.ha.user : nullptr,
            strlen(configManager.ha.pass) > 0 ? configManager.ha.pass : nullptr);

        if (ok) {
            mqttOK_ = true;
            // Subscreve tópicos de comando do GM para invade targets
            mqttClient_.subscribe("h4ckos/cmd/invade1");
            mqttClient_.subscribe("h4ckos/cmd/invade2");
            mqttClient_.subscribe("h4ckos/cmd/invade3");
            HA_INF("reconnectMQTT() — SUCESSO, subscribed to h4ckos/cmd/invade#");
            Serial.println("[MQTT] Connected! Subscribed to h4ckos/cmd/invade#");
        } else {
            mqttOK_ = false;
            int rc = mqttClient_.state();
            HA_ERR("reconnectMQTT() — FALHA rc=%d (%s)", rc,
                   rc==-4?"TIMEOUT":rc==-3?"LOST":rc==-2?"CONN_FAIL":rc==-1?"DISC":rc==1?"BAD_PROTO":rc==2?"BAD_CID":rc==3?"UNAVAIL":rc==4?"BAD_CREDS":rc==5?"UNAUTH":"???");
            Serial.printf("[MQTT] Failed (rc=%d)\n", rc);
        }
    }

    void mqttCallback(char* topic, byte* payload, unsigned int len) {
        // Processa comandos do GM via MQTT
        char val[96];
        unsigned int copyLen = (len < sizeof(val)-1) ? len : sizeof(val)-1;
        memcpy(val, payload, copyLen);
        val[copyLen] = '\0';

        HA_INF("mqttCallback: topic=%s payload='%s'", topic, val);

        // Determina slot pelo tópico
        int slot = -1;
        if (strstr(topic, "invade1")) slot = 0;
        else if (strstr(topic, "invade2")) slot = 1;
        else if (strstr(topic, "invade3")) slot = 2;

        if (slot < 0) {
            HA_ERR("mqttCallback: tópico desconhecido '%s'", topic);
            return;
        }

        // Processa comando: "nome=topic" ou vazio para remover
        if (strlen(val) == 0) {
            // Remove target deste slot
            int removeIdx = -1;
            // Mapeia slot para índice real no array de targets
            // Usamos slot como índice fixo: slot 0 = targets[0], slot 1 = targets[1], etc.
            if (slot < configManager.targetCount) {
                removeIdx = slot;
                for (int i = removeIdx; i < configManager.targetCount - 1; i++)
                    configManager.targets[i] = configManager.targets[i + 1];
                configManager.targetCount--;
                Serial.printf("[HA] invade slot %d cleared\n", slot);
            }
        } else {
            // Formato: nome=topic (ex: "luz_sala=cmnd/luz_sala/POWER")
            const char* eq = strchr(val, '=');
            if (eq != nullptr) {
                size_t nameLen = eq - val;
                if (nameLen > 0 && nameLen < 24) {
                    // Expande array se necessário para caber este slot
                    // Slot é posição fixa: queremos que slot 0 sempre seja targets[0], etc.
                    // Se o slot atual está vazio (targetCount <= slot), precisamos
                    // garantir que o array tenha tamanho suficiente
                    while (configManager.targetCount <= slot && configManager.targetCount < MAX_TARGETS) {
                        configManager.targets[configManager.targetCount].name[0] = '\0';
                        configManager.targets[configManager.targetCount].topic[0] = '\0';
                        configManager.targetCount++;
                    }

                    if (slot < configManager.targetCount) {
                        strlcpy(configManager.targets[slot].name, String(val).substring(0, nameLen).c_str(),
                            sizeof(configManager.targets[slot].name));
                        strlcpy(configManager.targets[slot].topic, eq + 1,
                            sizeof(configManager.targets[slot].topic));
                        Serial.printf("[HA] invade slot %d set to '%s' topic '%s'\n",
                            slot, configManager.targets[slot].name, configManager.targets[slot].topic);
                    }
                }
            }
        }
        configManager.save();
        Serial.printf("[HA] invade slot %d processed\n", slot);
    }

    // ================================================================
    // HA SETUP (ArduinoHA)
    // ================================================================
    void beginHA() {
        if (haOK_) { HA_DBG("beginHA(): já iniciado"); return; }
        if (!wifiOK_) { HA_ERR("beginHA(): WiFi não conectado"); return; }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("beginHA(): broker vazio"); return;
        }

        HA_INF("beginHA() — configurando dispositivo");
        byte mac[6];
        WiFi.macAddress(mac);
        HA_DBG("  MAC=%02X:%02X:%02X:%02X:%02X:%02X",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

        haDevice_.setUniqueId(mac, sizeof(mac));
        haDevice_.setName("H4CK-OS");
        haDevice_.setSoftwareVersion(APP_VERSION);
        haDevice_.setManufacturer("EscapeRoom");
        haDevice_.setModel("Cardputer");

        haSensorEstado_.setName("Status Atual");
        haSensorEstado_.setIcon("mdi:console");
        haSensorLogin_.setName("Login Password");
        haSensorLogin_.setIcon("mdi:form-textbox-password");
        haSensorSniffIp_.setName("IP da Bomba");
        haSensorSniffIp_.setIcon("mdi:ip-network");
        haSensorLastPayload_.setName("Hack Sala");
        haSensorLastPayload_.setIcon("mdi:rocket-launch");

        haNumDefuseCode_.setName("Codigo Desarme");
        haNumDefuseCode_.setIcon("mdi:lock");
        haNumDefuseCode_.setMin(0);
        haNumDefuseCode_.setMax(9999);
        haNumDefuseCode_.setStep(1);
        haNumDefuseCode_.onCommand(haDefuseCodeCb);

        haNumSniffChannel_.setName("Sniff Channel");
        haNumSniffChannel_.setIcon("mdi:radio");
        haNumSniffChannel_.setMin(1);
        haNumSniffChannel_.setMax(4);
        haNumSniffChannel_.setStep(1);
        haNumSniffChannel_.onCommand(haSniffChannelCb);

        haSelectStage_.setName("Stage");
        haSelectStage_.setIcon("mdi:layers-triple");
        haSelectStage_.setOptions("SCAN;HYDRA;EXPLOIT;RF433");
        haSelectStage_.onCommand(haStageCb);

        haSwitchRF433_.setName("Controle Remoto RF");
        haSwitchRF433_.setIcon("mdi:antenna");
        haSwitchRF433_.onCommand(haRFEnableCb);

        HA_INF("haMqtt_.begin(broker=%s, port=%d, user=%s, pass=%s)",
               configManager.ha.broker, (int)configManager.ha.port,
               strlen(configManager.ha.user)>0?"***":"(none)",
               strlen(configManager.ha.pass)>0?"***":"(none)");

        haMqtt_.begin(
            configManager.ha.broker,
            configManager.ha.port,
            strlen(configManager.ha.user) > 0 ? configManager.ha.user : nullptr,
            strlen(configManager.ha.pass) > 0 ? configManager.ha.pass : nullptr
        );

        haOK_ = true;
        HA_INF("ArduinoHA inicializado em %s:%d", configManager.ha.broker, (int)configManager.ha.port);
        if (haMqtt_.isConnected()) {
            HA_INF("HA MQTT já conectado — publishAll()");
            publishAll();
        } else {
            HA_DBG("HA MQTT ainda não conectado — aguardando loopHA()");
        }
    }

    // ================================================================
    // CALLBACKS HA
    // ================================================================
    static void haDefuseCodeCb(HANumeric val, HANumber*) {
        HA_DBG("haDefuseCodeCb(%ld)", (long)val.toInt32());
        configManager.sniff.defuseCode = constrain(val.toInt32(), 0, 9999);
        configManager.save();
        Serial.printf("[HA] authToken=%d\n", configManager.sniff.defuseCode);
    }

    static void haSniffChannelCb(HANumeric val, HANumber*) {
        HA_DBG("haSniffChannelCb(%ld)", (long)val.toInt32());
        configManager.sniff.channel = constrain(val.toInt32(), 1, 4);
        configManager.save();
        Serial.printf("[HA] sniffChannel=%d\n", configManager.sniff.channel);
    }

    static void haStageCb(int8_t idx, HASelect*) {
        HA_DBG("haStageCb(%d)", (int)idx);
        configManager.stages.current = constrain(idx, 0, 3);
        configManager.save();
        configManager.stages.scanEnabled = (idx == 0);
        configManager.stages.hydraEnabled = (idx >= 1);
        configManager.stages.exploitEnabled = (idx >= 2);
        configManager.stages.rf433Enabled = (idx >= 3);
        configManager.save();
        Serial.printf("[HA] stage=%d\n", configManager.stages.current);
    }

    static void haRFEnableCb(bool state, HASwitch*) {
        HA_DBG("haRFEnableCb(%s)", state?"true":"false");
        extern HAClient haClient;
        haClient.setRF433Enabled(state);
        configManager.stages.rf433Enabled = state;
        configManager.save();
        Serial.printf("[HA] rf433=%d\n", state);
    }

    // ─── Helpers invade ──────────────────────────────────────────
    const char* getInvadeSlot(int slot) {
        if (slot < 0 || slot >= 3) return "";
        return invadeSlotBuf_[slot];
    }

    void syncInvadeFromHA() {
        for (int s = 0; s < 3; s++) invadeSlotBuf_[s][0] = '\0';
        for (int i = 0; i < configManager.targetCount && i < 3; i++) {
            snprintf(invadeSlotBuf_[i], sizeof(invadeSlotBuf_[i]),
                "%s=%s", configManager.targets[i].name, configManager.targets[i].topic);
        }
    }

    // ================================================================
    // AP MODE + WEBSERVER
    // ================================================================
    void startAP() {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("H4CK-OS-Config");
        apSrv_ = new WebServer(80);
        HA_INF("AP 'H4CK-OS-Config' iniciado em %s", WiFi.softAPIP().toString().c_str());
        Serial.printf("[AP] H4CK-OS-Config http://%s\n", WiFi.softAPIP().toString().c_str());

        apSrv_->on("/", HTTP_GET, [this]() { handleRoot(); });
        apSrv_->on("/save", HTTP_POST, [this]() { handleSave(); });
        apSrv_->begin();
    }

    void handleRoot() {
        String html = R"rawliteral(
<!DOCTYPE html><html><meta charset=UTF-8><meta name=viewport content='width=device-width,initial-scale=1'>
<style>
body{font-family:system-ui;max-width:500px;margin:auto;padding:20px;background:#111;color:#eee}
h2{color:#0f0;border-bottom:1px solid #333;padding-bottom:10px}
h3{color:#0f0;margin-top:20px}
label{display:block;margin-top:12px;font-weight:bold;color:#0f0}
input{width:100%;padding:8px;margin-top:4px;background:#222;color:#eee;border:1px solid #444;border-radius:4px;box-sizing:border-box}
button{margin-top:20px;padding:10px 20px;background:#0f0;color:#000;border:none;border-radius:4px;cursor:pointer;font-size:16px;font-weight:bold}
.s{border:1px solid #333;border-radius:8px;padding:15px;margin-top:15px;background:#1a1a1a}
.sm{font-size:0.8em;color:#888;margin-top:4px}
</style><body>
<h2>&#x1F527; H4CK-OS Config</h2>
)rawliteral";

        html += "<form action=/save method=POST>";
        // WiFi
        html += "<div class=s><h3>WiFi</h3>";
        html += "<label>SSID: <input name=ws required value='" + htmlEscape(configManager.ha.wifiSsid) + "'></label>";
        html += "<label>Password: <input name=wp type=password></label></div>";

        // MQTT / HA
        html += "<div class=s><h3>Home Assistant</h3>";
        html += "<label>Broker: <input name=mb value='" + htmlEscape(configManager.ha.broker) + "'></label>";
        html += "<label>Port: <input name=mp value=" + htmlEscape(String(configManager.ha.port)) + "></label>";
        html += "<label>User: <input name=mu value='" + htmlEscape(configManager.ha.user) + "'></label>";
        html += "<label>Pass: <input name=mpw type=password></label>";
        html += "<div class=sm>Necessário para entidades HA e controle remoto.</div></div>";

        html += "<button type=submit>Salvar e Reiniciar</button></form></body></html>";
        apSrv_->send(200, "text/html", html);
    }

    void handleSave() {
        if (apSrv_->hasArg("ws"))
            strlcpy(configManager.ha.wifiSsid, apSrv_->arg("ws").c_str(), sizeof(configManager.ha.wifiSsid));
        if (apSrv_->hasArg("wp"))
            strlcpy(configManager.ha.wifiPass, apSrv_->arg("wp").c_str(), sizeof(configManager.ha.wifiPass));

        if (apSrv_->hasArg("mb"))
            strlcpy(configManager.ha.broker, apSrv_->arg("mb").c_str(), sizeof(configManager.ha.broker));
        configManager.ha.port = constrain(apSrv_->arg("mp").toInt(), 1, 65535);
        if (apSrv_->hasArg("mu"))
            strlcpy(configManager.ha.user, apSrv_->arg("mu").c_str(), sizeof(configManager.ha.user));
        if (apSrv_->hasArg("mpw"))
            strlcpy(configManager.ha.pass, apSrv_->arg("mpw").c_str(), sizeof(configManager.ha.pass));

        configManager.save();
        apSrv_->send(200, "text/html",
            "<html><body><h2>Salvo! Reiniciando...</h2></body></html>");
        delay(1000);
        ESP.restart();
    }

    String htmlEscape(const String& s) {
        String out;
        for (char c : s) {
            switch (c) {
                case '&': out += "&amp;"; break;
                case '<': out += "&lt;"; break;
                case '>': out += "&gt;"; break;
                case '"': out += "&quot;"; break;
                default:  out += c;
            }
        }
        return out;
    }
};
