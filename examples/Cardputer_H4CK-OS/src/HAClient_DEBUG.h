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
// HA CLIENT — DEBUG VERSION — logging extensivo para diagnosticar MQTT
// ============================================================================
// Use este arquivo no lugar de HAClient.h para debug.
// Substituição drop-in: mesma classe HAClient, mesma API pública.
//
// Comandos ArduinoHA usados neste arquivo:
//   HADevice::setUniqueId(mac,n)         — define ID único do dispositivo
//   HADevice::setName(s)                   — nome do dispositivo HA
//   HADevice::setSoftwareVersion(s)        — versão do firmware
//   HAMqtt::begin(broker,port,user,pass)   — inicia conexão MQTT com credenciais
//   HAMqtt::loop()                         — mantém alive + publica discovery
//   HAMqtt::isConnected()                  — retorna true se MQTT conectado
//   HASensor::setValue(s)                  — atualiza valor do sensor
//   HANumber::setState(v)                  — atualiza estado do number
//   HASelect::setState(idx)                — atualiza índice do select
//   HASwitch::setState(on)                 — atualiza estado do switch
//   Entity::setName(s)                     — nome da entidade HA
//   Entity::setIcon(s)                     — ícone Material Design
//   Entity::onCommand(cb)                  — registra callback para comandos
// ============================================================================

// ─── NÍVEL DE DEBUG ──────────────────────────────────────────────────────────
// Descomente a linha abaixo para ativar logs detalhados no Serial:
// #define HA_DEBUG_LEVEL 3   // 0=none, 1=errors, 2=info, 3=verbose

#ifndef HA_DEBUG_LEVEL
#define HA_DEBUG_LEVEL 0
#endif

#if HA_DEBUG_LEVEL >= 1
  #define HA_ERR(fmt, ...) Serial.printf("[HA-ERR] " fmt "\n", ##__VA_ARGS__)
#else
  #define HA_ERR(...) do{}while(0)
#endif

#if HA_DEBUG_LEVEL >= 2
  #define HA_INF(fmt, ...) Serial.printf("[HA-INF] " fmt "\n", ##__VA_ARGS__)
#else
  #define HA_INF(...) do{}while(0)
#endif

#if HA_DEBUG_LEVEL >= 3
  #define HA_DBG(fmt, ...) Serial.printf("[HA-DBG] " fmt "\n", ##__VA_ARGS__)
#else
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
        HA_DBG("HAClient construtor — entidades criadas");
        mqttClient_.setCallback([this](char* topic, byte* payload, unsigned int len) {
            mqttCallback(topic, payload, len);
        });
    }

    void begin() {
        HA_DBG("begin() iniciado");
        HA_DBG("  WiFi SSID: '%s' (len=%d)", configManager.ha.wifiSsid, strlen(configManager.ha.wifiSsid));
        HA_DBG("  Broker: '%s' (len=%d)", configManager.ha.broker, strlen(configManager.ha.broker));
        HA_DBG("  Port: %d", (int)configManager.ha.port);
        HA_DBG("  User: '%s' (len=%d)", configManager.ha.user, strlen(configManager.ha.user));
        HA_DBG("  Pass: '%s' (len=%d)", configManager.ha.pass, strlen(configManager.ha.pass));

        if (strlen(configManager.ha.wifiSsid) == 0) {
            HA_INF("WiFi SSID vazio — iniciando AP mode");
            startAP();
            return;
        }

        HA_INF("Connecting to WiFi '%s'...", configManager.ha.wifiSsid);
        WiFi.mode(WIFI_STA);
        WiFi.begin(configManager.ha.wifiSsid, configManager.ha.wifiPass);

        int att = 0;
        while (WiFi.status() != WL_CONNECTED && att < 40) {
            delay(500);
            Serial.print(".");
            att++;
        }
        Serial.println(); // flush dots

        HA_DBG("WiFi conexão loop terminou — status=%d tentativas=%d", WiFi.status(), att);

        if (WiFi.status() == WL_CONNECTED) {
            HA_INF("WiFi OK — IP:%s MAC:%s RSSI:%d",
                   WiFi.localIP().toString().c_str(),
                   WiFi.macAddress().c_str(),
                   WiFi.RSSI());
            wifiOK_ = true;
            beginMQTT();
            beginHA();
        } else {
            HA_ERR("WiFi falhou (status=%d) — iniciando AP mode", WiFi.status());
            startAP();
        }
    }

    void loopHA() {
        // AP
        if (apSrv_) {
            apSrv_->handleClient();
        }

        if (!wifiOK_) {
            return;
        }

        // ─── MQTT RAW ────────────────────────────────────────────────────
        if (!mqttOK_) {
            static unsigned long lastRawAttempt = 0;
            if (millis() - lastRawAttempt > 5000) {
                lastRawAttempt = millis();
                HA_DBG("MQTT RAW não conectado — tentando beginMQTT()");
                beginMQTT();
            }
        } else {
            mqttClient_.loop();
            static unsigned long lastRawReconnect = 0;
            if (!mqttClient_.connected() && millis() - lastRawReconnect > 10000) {
                lastRawReconnect = millis();
                HA_DBG("MQTT RAW desconectado — tentando reconnectMQTT()");
                reconnectMQTT();
            }
        }

        // ─── ArduinoHA ───────────────────────────────────────────────────
        if (!haOK_) {
            static unsigned long lastHaAttempt = 0;
            if (millis() - lastHaAttempt > 5000) {
                lastHaAttempt = millis();
                HA_DBG("ArduinoHA não iniciado — tentando beginHA()");
                beginHA();
            }
        } else {
            haMqtt_.loop();

            static bool lastHaConnected = false;
            bool nowConnected = haMqtt_.isConnected();

            HA_DBG("haMqtt_.isConnected()=%s lastHaConnected=%s",
                   nowConnected ? "true" : "false",
                   lastHaConnected ? "true" : "false");

            if (nowConnected && !lastHaConnected) {
                lastHaConnected = true;
                HA_INF("ArduinoHA MQTT conectado — publicando estado completo");
                publishAll();
            }
            if (!nowConnected) {
                if (lastHaConnected) {
                    HA_ERR("ArduinoHA MQTT desconectou!");
                }
                lastHaConnected = false;
            }
        }

        // ─── Diagnóstico periódico ─────────────────────────────────────
        static unsigned long lastDiag = 0;
        if (millis() - lastDiag > 30000) {
            lastDiag = millis();
            HA_INF("DIAGNOSTICO | WiFi=%s MQTT=%s HA=%s heap=%d uptime=%lus",
                   wifiOK_ ? "OK" : "FAIL",
                   mqttOK_ ? "OK" : (wifiOK_ ? "FAIL" : "N/A"),
                   haOK_ ? (haMqtt_.isConnected() ? "CONN" : "INIT") : "FAIL",
                   ESP.getFreeHeap(),
                   millis() / 1000);
        }
    }

    bool isWiFiOK() { return wifiOK_; }
    bool isMQTTOK() { return mqttOK_; }
    bool isAPMode() { return apSrv_ != nullptr; }

    // ─── invade: envia TOGGLE no tópico MQTT do target ───────────────
    bool invadeTarget(const char* targetName) {
        if (!mqttOK_) {
            HA_ERR("invadeTarget('%s'): MQTT não conectado", targetName);
            return false;
        }
        for (int i = 0; i < configManager.targetCount; i++) {
            if (strcasecmp(configManager.targets[i].name, targetName) == 0) {
                const char* topic = configManager.targets[i].topic;
                bool ok = mqttClient_.publish(topic, "TOGGLE");
                HA_INF("invadeTarget('%s'): topic=%s result=%s", targetName, topic, ok ? "OK" : "FAIL");
                return true;
            }
        }
        HA_ERR("invadeTarget('%s'): target não encontrado", targetName);
        return false;
    }

    void publishAll() {
        if (!haOK_) {
            HA_ERR("publishAll(): ArduinoHA não iniciado");
            return;
        }
        if (!haMqtt_.isConnected()) {
            HA_ERR("publishAll(): ArduinoHA MQTT desconectado — publishAll() abortado");
            return;
        }
        HA_DBG("publishAll() — publicando %d entidades", 9);

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
        if (haOK_ && haMqtt_.isConnected()) {
            haSensorEstado_.setValue(s);
        }
    }

    void lastPayload(const char* payloadName) {
        HA_DBG("lastPayload('%s')", payloadName);
        if (haOK_ && haMqtt_.isConnected()) {
            haSensorLastPayload_.setValue(payloadName);
        }
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

    HASensor haSensorEstado_;
    HASensor haSensorLogin_;
    HASensor haSensorSniffIp_;
    HASensor haSensorLastPayload_;
    HANumber haNumDefuseCode_;
    HANumber haNumSniffChannel_;
    HASelect haSelectStage_;
    HASwitch haSwitchRF433_;

    // ─── MQTT RAW ────────────────────────────────────────────────────
    void beginMQTT() {
        if (mqttOK_) {
            HA_DBG("beginMQTT() — já conectado, pulando");
            return;
        }
        if (!wifiOK_) {
            HA_ERR("beginMQTT() — WiFi não conectado");
            return;
        }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("beginMQTT() — broker vazio");
            return;
        }

        HA_INF("beginMQTT() — broker=%s port=%d", configManager.ha.broker, (int)configManager.ha.port);
        mqttClient_.setServer(configManager.ha.broker, configManager.ha.port);
        reconnectMQTT();
    }

    void reconnectMQTT() {
        if (!wifiOK_) {
            HA_ERR("reconnectMQTT() — WiFi não conectado");
            return;
        }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("reconnectMQTT() — broker vazio");
            return;
        }

        char clientId[32];
        snprintf(clientId, sizeof(clientId), "H4CKOS_%08X", (uint32_t)ESP.getEfuseMac());

        HA_INF("reconnectMQTT() — clientId=%s broker=%s port=%d",
               clientId, configManager.ha.broker, (int)configManager.ha.port);
        HA_DBG("  user='%s' pass='%s' (lens=%d/%d)",
               configManager.ha.user, configManager.ha.pass,
               strlen(configManager.ha.user), strlen(configManager.ha.pass));

        bool ok = mqttClient_.connect(clientId,
            strlen(configManager.ha.user) > 0 ? configManager.ha.user : nullptr,
            strlen(configManager.ha.pass) > 0 ? configManager.ha.pass : nullptr);

        if (ok) {
            mqttOK_ = true;
            HA_INF("reconnectMQTT() — SUCESSO (clientId=%s)", clientId);
        } else {
            mqttOK_ = false;
            HA_ERR("reconnectMQTT() — FALHA (rc=%d clientId=%s)", mqttClient_.state(), clientId);
        }
    }

    static void mqttCallback(char* topic, byte* payload, unsigned int len) {
        // Não esperamos mensagens de volta
    }

    // ─── ArduinoHA SETUP ─────────────────────────────────────────────
    void beginHA() {
        if (haOK_) {
            HA_DBG("beginHA() — já iniciado, pulando");
            return;
        }
        if (!wifiOK_) {
            HA_ERR("beginHA() — WiFi não conectado");
            return;
        }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("beginHA() — broker vazio");
            return;
        }

        HA_INF("beginHA() — configurando dispositivo ArduinoHA");

        byte mac[6];
        WiFi.macAddress(mac);
        HA_DBG("  MAC: %02X:%02X:%02X:%02X:%02X:%02X",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        haDevice_.setUniqueId(mac, sizeof(mac));
        haDevice_.setName("H4CK-OS");
        haDevice_.setSoftwareVersion(APP_VERSION);
        haDevice_.setManufacturer("EscapeRoom");
        haDevice_.setModel("Cardputer");
        HA_DBG("  Device: H4CK-OS / ID=%02X%02X%02X%02X%02X%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);

        haSensorEstado_.setName("Status");
        haSensorEstado_.setIcon("mdi:console");
        haSensorLogin_.setName("Login Password");
        haSensorLogin_.setIcon("mdi:form-textbox-password");
        haSensorSniffIp_.setName("Sniff Target IP");
        haSensorSniffIp_.setIcon("mdi:ip-network");
        haSensorLastPayload_.setName("Last Payload");
        haSensorLastPayload_.setIcon("mdi:rocket-launch");

        haNumDefuseCode_.setName("Auth Token");
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

        haSwitchRF433_.setName("RF433 Enable");
        haSwitchRF433_.setIcon("mdi:antenna");
        haSwitchRF433_.onCommand(haRFEnableCb);

        HA_INF("beginHA() — entidades configuradas (%d)", 9);
        HA_DBG("beginHA() — chamando haMqtt_.begin(broker=%s, port=%d, user='%s', pass='%s')",
               configManager.ha.broker,
               (int)configManager.ha.port,
               strlen(configManager.ha.user) > 0 ? "***" : "(nenhum)",
               strlen(configManager.ha.pass) > 0 ? "***" : "(nenhum)");

        // NOTA: ArduinoHA HAMqtt::begin() pode ter assinaturas diferentes.
        // Verifique se haMqtt_.begin() ACEITA a porta como argumento.
        // Se a assinatura for apenas begin(broker,user,pass), a porta default é 1883.
        // Se precisar de porta diferente, verifique a biblioteca ArduinoHA.
        haMqtt_.begin(
            configManager.ha.broker,
            configManager.ha.port,
            strlen(configManager.ha.user) > 0 ? configManager.ha.user : nullptr,
            strlen(configManager.ha.pass) > 0 ? configManager.ha.pass : nullptr
        );

        haOK_ = true;
        HA_INF("beginHA() — ArduinoHA inicializado em %s:%d",
               configManager.ha.broker, (int)configManager.ha.port);

        // Primeira publicação — pode falhar se MQTT ainda não conectou
        if (haMqtt_.isConnected()) {
            HA_INF("beginHA() — MQTT já conectado, publicando estado");
            publishAll();
        } else {
            HA_DBG("beginHA() — MQTT ainda não conectado, publishAll() adiado para loopHA()");
        }
    }

    // ─── CALLBACKS HA ────────────────────────────────────────────────
    static void haDefuseCodeCb(HANumeric val, HANumber*) {
        HA_DBG("haDefuseCodeCb(%ld)", (long)val.toInt32());
        configManager.sniff.defuseCode = constrain(val.toInt32(), 0, 9999);
        configManager.save();
        HA_INF("Auth token alterado para %d", configManager.sniff.defuseCode);
    }

    static void haSniffChannelCb(HANumeric val, HANumber*) {
        HA_DBG("haSniffChannelCb(%ld)", (long)val.toInt32());
        configManager.sniff.channel = constrain(val.toInt32(), 1, 4);
        configManager.save();
        HA_INF("Sniff channel alterado para %d", configManager.sniff.channel);
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
        HA_INF("Stage alterado para %d", configManager.stages.current);
    }

    static void haRFEnableCb(bool state, HASwitch*) {
        HA_DBG("haRFEnableCb(%s)", state ? "true" : "false");
        extern HAClient haClient;
        haClient.setRF433Enabled(state);
        configManager.stages.rf433Enabled = state;
        configManager.save();
        HA_INF("RF433 %s", state ? "ativado" : "desativado");
    }

    // ─── AP MODE + WEBSERVER ───────────────────────────────────────────
    void startAP() {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("H4CK-OS-Config");
        apSrv_ = new WebServer(80);
        HA_INF("[AP] H4CK-OS-Config iniciado em %s", WiFi.softAPIP().toString().c_str());

        apSrv_->on("/", HTTP_GET, [this]() { handleRoot(); });
        apSrv_->on("/save", HTTP_POST, [this]() { handleSave(); });
        apSrv_->on("/remove", HTTP_POST, [this]() { handleRemove(); });
        apSrv_->begin();
    }

    void handleRoot() {
        // (mesmo código do original — removido por brevidade)
        String html = "<html><body><h2>H4CK-OS Config</h2><p>Use o POST /save</p></body></html>";
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
        apSrv_->send(200, "text/html", "<html><body><h2>Salvo! Reiniciando...</h2></body></html>");
        delay(1000);
        ESP.restart();
    }

    void handleRemove() {
        int idx = apSrv_->arg("idx").toInt();
        if (idx >= 0 && idx < configManager.targetCount) {
            for (int i = idx; i < configManager.targetCount - 1; i++) {
                configManager.targets[i] = configManager.targets[i + 1];
            }
            configManager.targetCount--;
            configManager.save();
        }
        apSrv_->sendHeader("Location", "/");
        apSrv_->send(303);
    }
};
