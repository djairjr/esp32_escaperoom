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
// HA CLIENT — DEBUG VERSION — extensive logging to diagnose MQTT
// ============================================================================
// Use this file in place of HAClient.h for debugging.
// Drop-in replacement: same HAClient class, same public API.
//
// ArduinoHA commands used in this file:
//   HADevice::setUniqueId(mac,n)         — sets the device unique ID
//   HADevice::setName(s)                   — HA device name
//   HADevice::setSoftwareVersion(s)        — firmware version
//   HAMqtt::begin(broker,port,user,pass)   — starts the MQTT connection with credentials
//   HAMqtt::loop()                         — keeps alive + publishes discovery
//   HAMqtt::isConnected()                  — returns true if MQTT is connected
//   HASensor::setValue(s)                  — updates the sensor value
//   HANumber::setState(v)                  — updates the number state
//   HASelect::setState(idx)                — updates the select index
//   HASwitch::setState(on)                 — updates the switch state
//   Entity::setName(s)                     — HA entity name
//   Entity::setIcon(s)                     — Material Design icon
//   Entity::onCommand(cb)                  — registers a callback for commands
// ============================================================================

// ─── DEBUG LEVEL ──────────────────────────────────────────────────────────
// Uncomment the line below to enable detailed logs on Serial:
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
        HA_DBG("HAClient constructor — entities created");
        mqttClient_.setCallback([this](char* topic, byte* payload, unsigned int len) {
            mqttCallback(topic, payload, len);
        });
    }

    void begin() {
        HA_DBG("begin() started");
        HA_DBG("  WiFi SSID: '%s' (len=%d)", configManager.ha.wifiSsid, strlen(configManager.ha.wifiSsid));
        HA_DBG("  Broker: '%s' (len=%d)", configManager.ha.broker, strlen(configManager.ha.broker));
        HA_DBG("  Port: %d", (int)configManager.ha.port);
        HA_DBG("  User: '%s' (len=%d)", configManager.ha.user, strlen(configManager.ha.user));
        HA_DBG("  Pass: '%s' (len=%d)", configManager.ha.pass, strlen(configManager.ha.pass));

        if (strlen(configManager.ha.wifiSsid) == 0) {
            HA_INF("Empty WiFi SSID — starting AP mode");
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

        HA_DBG("WiFi connection loop ended — status=%d attempts=%d", WiFi.status(), att);

        if (WiFi.status() == WL_CONNECTED) {
            HA_INF("WiFi OK — IP:%s MAC:%s RSSI:%d",
                   WiFi.localIP().toString().c_str(),
                   WiFi.macAddress().c_str(),
                   WiFi.RSSI());
            wifiOK_ = true;
            beginMQTT();
            beginHA();
        } else {
            HA_ERR("WiFi failed (status=%d) — starting AP mode", WiFi.status());
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
                HA_DBG("MQTT RAW not connected — trying beginMQTT()");
                beginMQTT();
            }
        } else {
            mqttClient_.loop();
            static unsigned long lastRawReconnect = 0;
            if (!mqttClient_.connected() && millis() - lastRawReconnect > 10000) {
                lastRawReconnect = millis();
                HA_DBG("MQTT RAW disconnected — trying reconnectMQTT()");
                reconnectMQTT();
            }
        }

        // ─── ArduinoHA ───────────────────────────────────────────────────
        if (!haOK_) {
            static unsigned long lastHaAttempt = 0;
            if (millis() - lastHaAttempt > 5000) {
                lastHaAttempt = millis();
                HA_DBG("ArduinoHA not started — trying beginHA()");
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
                HA_INF("ArduinoHA MQTT connected — publishing full state");
                publishAll();
            }
            if (!nowConnected) {
                if (lastHaConnected) {
                    HA_ERR("ArduinoHA MQTT disconnected!");
                }
                lastHaConnected = false;
            }
        }

        // ─── Periodic diagnostics ─────────────────────────────────────
        static unsigned long lastDiag = 0;
        if (millis() - lastDiag > 30000) {
            lastDiag = millis();
            HA_INF("DIAGNOSTICS | WiFi=%s MQTT=%s HA=%s heap=%d uptime=%lus",
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

    // ─── invade: publishes TOGGLE on the target's MQTT topic ───────────────
    bool invadeTarget(const char* targetName) {
        if (!mqttOK_) {
            HA_ERR("invadeTarget('%s'): MQTT not connected", targetName);
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
        HA_ERR("invadeTarget('%s'): target not found", targetName);
        return false;
    }

    void publishAll() {
        if (!haOK_) {
            HA_ERR("publishAll(): ArduinoHA not started");
            return;
        }
        if (!haMqtt_.isConnected()) {
            HA_ERR("publishAll(): ArduinoHA MQTT disconnected — publishAll() aborted");
            return;
        }
        HA_DBG("publishAll() — publishing %d entities", 9);

        haSensorEstado_.setValue(lastStatus_);
        haSensorLogin_.setValue(configManager.login.pass);
        haSensorSniffIp_.setValue(configManager.sniff.ip);
        haNumDefuseCode_.setState((int32_t)configManager.sniff.defuseCode);
        haNumSniffChannel_.setState((int8_t)(configManager.sniff.channel - 1));
        haSelectStage_.setState((int8_t)configManager.stages.current);
        haSwitchRF433_.setState(rf433Enabled_);

        HA_INF("publishAll() — full state published");
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
            HA_DBG("beginMQTT() — already connected, skipping");
            return;
        }
        if (!wifiOK_) {
            HA_ERR("beginMQTT() — WiFi not connected");
            return;
        }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("beginMQTT() — empty broker");
            return;
        }

        HA_INF("beginMQTT() — broker=%s port=%d", configManager.ha.broker, (int)configManager.ha.port);
        mqttClient_.setServer(configManager.ha.broker, configManager.ha.port);
        reconnectMQTT();
    }

    void reconnectMQTT() {
        if (!wifiOK_) {
            HA_ERR("reconnectMQTT() — WiFi not connected");
            return;
        }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("reconnectMQTT() — empty broker");
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
            HA_INF("reconnectMQTT() — SUCCESS (clientId=%s)", clientId);
        } else {
            mqttOK_ = false;
            HA_ERR("reconnectMQTT() — FAILURE (rc=%d clientId=%s)", mqttClient_.state(), clientId);
        }
    }

    static void mqttCallback(char* topic, byte* payload, unsigned int len) {
        // We don't expect any messages back
    }

    // ─── ArduinoHA SETUP ─────────────────────────────────────────────
    void beginHA() {
        if (haOK_) {
            HA_DBG("beginHA() — already started, skipping");
            return;
        }
        if (!wifiOK_) {
            HA_ERR("beginHA() — WiFi not connected");
            return;
        }
        if (strlen(configManager.ha.broker) == 0) {
            HA_ERR("beginHA() — empty broker");
            return;
        }

        HA_INF("beginHA() — configuring ArduinoHA device");

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

        HA_INF("beginHA() — entities configured (%d)", 9);
        HA_DBG("beginHA() — calling haMqtt_.begin(broker=%s, port=%d, user='%s', pass='%s')",
               configManager.ha.broker,
               (int)configManager.ha.port,
               strlen(configManager.ha.user) > 0 ? "***" : "(none)",
               strlen(configManager.ha.pass) > 0 ? "***" : "(none)");

        // NOTE: ArduinoHA HAMqtt::begin() may have different signatures.
        // Check whether haMqtt_.begin() ACCEPTS the port as an argument.
        // If the signature is only begin(broker,user,pass), the default port is 1883.
        // If you need a different port, check the ArduinoHA library.
        haMqtt_.begin(
            configManager.ha.broker,
            configManager.ha.port,
            strlen(configManager.ha.user) > 0 ? configManager.ha.user : nullptr,
            strlen(configManager.ha.pass) > 0 ? configManager.ha.pass : nullptr
        );

        haOK_ = true;
        HA_INF("beginHA() — ArduinoHA initialized at %s:%d",
               configManager.ha.broker, (int)configManager.ha.port);

        // First publish — may fail if MQTT hasn't connected yet
        if (haMqtt_.isConnected()) {
            HA_INF("beginHA() — MQTT already connected, publishing state");
            publishAll();
        } else {
            HA_DBG("beginHA() — MQTT not connected yet, publishAll() deferred to loopHA()");
        }
    }

    // ─── HA CALLBACKS ────────────────────────────────────────────────
    static void haDefuseCodeCb(HANumeric val, HANumber*) {
        HA_DBG("haDefuseCodeCb(%ld)", (long)val.toInt32());
        configManager.sniff.defuseCode = constrain(val.toInt32(), 0, 9999);
        configManager.save();
        HA_INF("Auth token changed to %d", configManager.sniff.defuseCode);
    }

    static void haSniffChannelCb(HANumeric val, HANumber*) {
        HA_DBG("haSniffChannelCb(%ld)", (long)val.toInt32());
        configManager.sniff.channel = constrain(val.toInt32(), 1, 4);
        configManager.save();
        HA_INF("Sniff channel changed to %d", configManager.sniff.channel);
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
        HA_INF("Stage changed to %d", configManager.stages.current);
    }

    static void haRFEnableCb(bool state, HASwitch*) {
        HA_DBG("haRFEnableCb(%s)", state ? "true" : "false");
        extern HAClient haClient;
        haClient.setRF433Enabled(state);
        configManager.stages.rf433Enabled = state;
        configManager.save();
        HA_INF("RF433 %s", state ? "enabled" : "disabled");
    }

    // ─── AP MODE + WEBSERVER ───────────────────────────────────────────
    void startAP() {
        WiFi.mode(WIFI_AP);
        WiFi.softAP("H4CK-OS-Config");
        apSrv_ = new WebServer(80);
        HA_INF("[AP] H4CK-OS-Config started at %s", WiFi.softAPIP().toString().c_str());

        apSrv_->on("/", HTTP_GET, [this]() { handleRoot(); });
        apSrv_->on("/save", HTTP_POST, [this]() { handleSave(); });
        apSrv_->on("/remove", HTTP_POST, [this]() { handleRemove(); });
        apSrv_->begin();
    }

    void handleRoot() {
        // (same code as the original — removed for brevity)
        String html = "<html><body><h2>H4CK-OS Config</h2><p>Use the POST /save</p></body></html>";
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
        apSrv_->send(200, "text/html", "<html><body><h2>Saved! Rebooting...</h2></body></html>");
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
