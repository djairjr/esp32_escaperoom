// ============================================================================
// HAClient.h — INCREMENTAL PATCH for MQTT/ArduinoHA debugging
// ============================================================================
// Apply these changes to the original HAClient.h.
//
// STEP 1: Add the DEBUG MACROS after #pragma once (line 2):
//
//   #define HA_DEBUG_LEVEL 3   // 0=none, 1=errors, 2=info, 3=verbose
//   
//   #if HA_DEBUG_LEVEL >= 1
//     #define HA_ERR(fmt, ...) Serial.printf("[HA-ERR] " fmt "\n", ##__VA_ARGS__)
//   #else
//     #define HA_ERR(...) do{}while(0)
//   #endif
//   
//   #if HA_DEBUG_LEVEL >= 2
//     #define HA_INF(fmt, ...) Serial.printf("[HA-INF] " fmt "\n", ##__VA_ARGS__)
//   #else
//     #define HA_INF(...) do{}while(0)
//   #endif
//   
//   #if HA_DEBUG_LEVEL >= 3
//     #define HA_DBG(fmt, ...) Serial.printf("[HA-DBG] " fmt "\n", ##__VA_ARGS__)
//   #else
//     #define HA_DBG(...) do{}while(0)
//   #endif
//
// STEP 2: Replace void begin() { ... } (lines 55-82 approx):
//
//   void begin() {
//       HA_DBG("begin() started");
//       HA_DBG("  SSID='%s' (len=%d)", configManager.ha.wifiSsid, strlen(configManager.ha.wifiSsid));
//       HA_DBG("  Broker='%s' Port=%d", configManager.ha.broker, (int)configManager.ha.port);
//       HA_DBG("  User='%s' Pass='%s'", configManager.ha.user, configManager.ha.pass);
//
//       if (strlen(configManager.ha.wifiSsid) == 0) {
//           HA_INF("Empty WiFi SSID — starting AP");
//           Serial.println("[WIFI] No SSID — starting AP mode");
//           startAP(); return;
//       }
//
//       Serial.printf("[WIFI] Connecting to %s... ", configManager.ha.wifiSsid);
//       WiFi.mode(WIFI_STA);
//       WiFi.begin(configManager.ha.wifiSsid, configManager.ha.wifiPass);
//       int att = 0;
//       while (WiFi.status() != WL_CONNECTED && att < 40) {
//           delay(500); Serial.print("."); att++;
//       }
//       Serial.println();
//       HA_DBG("WiFi loop ended status=%d attempts=%d", WiFi.status(), att);
//
//       if (WiFi.status() == WL_CONNECTED) {
//           HA_INF("WiFi OK IP=%s MAC=%s RSSI=%d",
//                  WiFi.localIP().toString().c_str(),
//                  WiFi.macAddress().c_str(), WiFi.RSSI());
//           wifiOK_ = true;
//           beginMQTT();
//           beginHA();
//       } else {
//           HA_ERR("WiFi failed status=%d", WiFi.status());
//           Serial.println(" FAIL — starting AP mode");
//           startAP();
//       }
//   }
//
// STEP 3: Replace void loopHA() (lines 84-109 approx):
//
//   void loopHA() {
//       if (apSrv_) apSrv_->handleClient();
//       if (!wifiOK_) return;
//
//       // MQTT RAW
//       if (!mqttOK_) {
//           static unsigned long lastRawAttempt = 0;
//           if (millis() - lastRawAttempt > 5000) {
//               lastRawAttempt = millis();
//               HA_DBG("MQTT RAW disconnected — trying beginMQTT()");
//               beginMQTT();
//           }
//       } else {
//           mqttClient_.loop();
//           static unsigned long lastRawReconnect = 0;
//           if (!mqttClient_.connected() && millis() - lastRawReconnect > 10000) {
//               lastRawReconnect = millis();
//               HA_DBG("MQTT RAW dropped — reconnectMQTT()");
//               reconnectMQTT();
//           }
//       }
//
//       // ArduinoHA
//       if (!haOK_) {
//           static unsigned long lastHaAttempt = 0;
//           if (millis() - lastHaAttempt > 5000) {
//               lastHaAttempt = millis();
//               HA_DBG("ArduinoHA not started — beginHA()");
//               beginHA();
//           }
//       } else {
//           haMqtt_.loop();
//           static bool lastHaConnected = false;
//           bool nowConnected = haMqtt_.isConnected();
//           HA_DBG("haMqtt_.isConnected()=%s last=%s",
//                  nowConnected ? "true" : "false",
//                  lastHaConnected ? "true" : "false");
//           if (nowConnected && !lastHaConnected) {
//               lastHaConnected = true;
//               HA_INF("HA MQTT connected — publishAll()");
//               publishAll();
//           }
//           if (!nowConnected && lastHaConnected) {
//               lastHaConnected = false;
//               HA_ERR("HA MQTT disconnected!");
//           }
//       }
//
//       // Periodic diagnostics every 30s
//       static unsigned long lastDiag = 0;
//       if (millis() - lastDiag > 30000) {
//           lastDiag = millis();
//           HA_INF("DIAG | WiFi=%s MQTT=%s HA=%s/%s heap=%d up=%lus",
//                  wifiOK_ ? "OK" : "FAIL",
//                  mqttOK_ ? "OK" : (wifiOK_ ? "FAIL" : "N/A"),
//                  haOK_ ? (haMqtt_.isConnected() ? "CONN" : "INIT") : "FAIL",
//                  haMqtt_.isConnected() ? "ON" : "OFF",
//                  ESP.getFreeHeap(), millis() / 1000);
//       }
//   }
//
// STEP 4: Replace void beginHA() (lines 225-274 approx) — THE MOST IMPORTANT:
//
//   void beginHA() {
//       if (haOK_) { HA_DBG("beginHA() already started"); return; }
//       if (!wifiOK_) { HA_ERR("beginHA(): WiFi not OK"); return; }
//       if (strlen(configManager.ha.broker) == 0) {
//           HA_ERR("beginHA(): empty broker"); return; }
//
//       HA_INF("beginHA() — configuring HADevice + entities");
//       byte mac[6]; WiFi.macAddress(mac);
//       haDevice_.setUniqueId(mac, sizeof(mac));
//       haDevice_.setName("H4CK-OS");
//       haDevice_.setSoftwareVersion(APP_VERSION);
//       haDevice_.setManufacturer("EscapeRoom");
//       haDevice_.setModel("Cardputer");
//       HA_DBG("Device ID=%02X%02X%02X%02X%02X%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
//
//       // ... entity configuration kept the same ...
//       // (copy the original lines 236-266)
//
//       HA_INF("haMqtt_.begin(broker=%s, port=%d)",
//              configManager.ha.broker, (int)configManager.ha.port);
//       HA_DBG("  user='%s' pass='%s' (lens=%d/%d)",
//              strlen(configManager.ha.user) > 0 ? "***" : "(none)",
//              strlen(configManager.ha.pass) > 0 ? "***" : "(none)",
//              strlen(configManager.ha.user), strlen(configManager.ha.pass));
//
//       // *** CRITICAL FIX #1 ***
//       // Check the signature of haMqtt_.begin() in ArduinoHA.h:
//       //   If it is: begin(const char* broker, uint16_t port, const char* user, const char* pass)
//       //   USE: haMqtt_.begin(configManager.ha.broker, configManager.ha.port, user, pass);
//       //   If it is: begin(const char* broker, const char* user, const char* pass)
//       //   USE: haMqtt_.begin(configManager.ha.broker, user, pass); // port always 1883
//       //
//       // The original code does NOT pass the port! If the broker uses a port != 1883, HA fails.
//       haMqtt_.begin(
//           configManager.ha.broker,
//           configManager.ha.port,     // <-- ADD THIS LINE if the API supports it
//           strlen(configManager.ha.user) > 0 ? configManager.ha.user : nullptr,
//           strlen(configManager.ha.pass) > 0 ? configManager.ha.pass : nullptr
//       );
//
//       haOK_ = true;
//       HA_INF("ArduinoHA initialized at %s:%d", configManager.ha.broker, (int)configManager.ha.port);
//       if (haMqtt_.isConnected()) {
//           HA_INF("MQTT already connected — publishAll()");
//           publishAll();
//       } else {
//           HA_DBG("MQTT not connected yet — waiting for loopHA()");
//       }
//   }
//
// STEP 5: Add debug to the publishAll / publishStatus / lastPayload functions:
//
//   void publishAll() {
//       if (!haOK_) { HA_ERR("publishAll(): haOK_=false"); return; }
//       if (!haMqtt_.isConnected()) {
//           HA_ERR("publishAll(): HA MQTT disconnected"); return; }
//       HA_DBG("publishAll() — publishing 9 entities");
//       haSensorEstado_.setValue(lastStatus_);
//       haSensorLogin_.setValue(configManager.login.pass);
//       haSensorSniffIp_.setValue(configManager.sniff.ip);
//       haNumDefuseCode_.setState((int32_t)configManager.sniff.defuseCode);
//       haNumSniffChannel_.setState((int8_t)(configManager.sniff.channel - 1));
//       haSelectStage_.setState((int8_t)configManager.stages.current);
//       haSwitchRF433_.setState(rf433Enabled_);
//       HA_INF("publishAll() complete");
//   }
//
//   void publishStatus(const char* s) {
//       strlcpy(lastStatus_, s, sizeof(lastStatus_));
//       HA_DBG("publishStatus('%s')", s);
//       if (haOK_ && haMqtt_.isConnected()) haSensorEstado_.setValue(s);
//   }
//
//   void lastPayload(const char* payloadName) {
//       HA_DBG("lastPayload('%s')", payloadName);
//       if (haOK_ && haMqtt_.isConnected()) haSensorLastPayload_.setValue(payloadName);
//   }
//
// ============================================================================
// POSSIBLE ROOT CAUSE OF THE HA PROBLEM
// ============================================================================
// The original code calls:
//      haMqtt_.begin(configManager.ha.broker, user, pass);
//
// That signature does NOT pass the port explicitly. ArduinoHA may use
// default port 1883. If the broker uses a different port (e.g. 1883 with TLS,
// or Mosquitto on another port), the connection will fail silently.
//
// Check the signature of begin() in ArduinoHA/src/utils/HAMqtt.h.
// If begin(broker, port, user, pass) exists, USE THAT signature.
//
// ANOTHER CAUSE: beginHA() sets haOK_=true IMMEDIATELY after begin(),
// even if MQTT hasn't connected. This causes publishAll() to be called in
// loopHA() before haMqtt_.isConnected() returns true, resulting
// in lost publishes. The publishAll() fix above already solves it.
//
// ANOTHER CAUSE: loopHA() doesn't check the heap. If LittleFS or LVGL consumes
// too much RAM, the WiFiClient may fail to connect. Add:
//      if (ESP.getFreeHeap() < 20000) { HA_ERR("low heap=%d", ESP.getFreeHeap()); }
//
// ============================================================================
// SERIAL VERIFICATION AFTER FLASHING:
// ============================================================================
// Uncomment #define HA_DEBUG_LEVEL 3 in the .ino (before #include "HAClient.h")
// or add it globally:
//      #define HA_DEBUG_LEVEL 3
//
// Open the serial monitor at 115200. You should see (in order):
//   [HA-DBG] begin() started
//   [HA-DBG]   SSID='YOUR_NETWORK' (len=N)
//   [HA-DBG]   Broker='192.168.x.x' Port=1883
//   [HA-DBG]   User='youruser' Pass='yourpass'
//   [HA-INF] Connecting to WiFi 'YOUR_NETWORK'...
//   ..........
//   [HA-INF] WiFi OK IP=192.168.x.x MAC=xx:xx:xx:xx:xx:xx RSSI=-45
//   [HA-INF] beginMQTT() — broker=192.168.x.x port=1883
//   [HA-INF] reconnectMQTT() — clientId=H4CKOS_xxxxxxxxx broker=192.168.x.x port=1883
//   [HA-INF] reconnectMQTT() — SUCCESS
//   [HA-INF] beginHA() — configuring HADevice + entities
//   [HA-DBG] Device ID=xxxxx
//   [HA-INF] haMqtt_.begin(broker=192.168.x.x, port=1883)
//   [HA-INF] ArduinoHA initialized at 192.168.x.x:1883
//   [HA-DBG] MQTT not connected yet — waiting for loopHA()
//   [HA-DBG] haMqtt_.isConnected()=true last=false
//   [HA-INF] HA MQTT connected — publishAll()
//   [HA-INF] publishAll() complete
//
// If you do NOT see "HA MQTT connected", the problem is in the MQTT connection.
// PubSubClient::state() error codes:
//   -4 = MQTT_CONNECTION_TIMEOUT
//   -3 = MQTT_CONNECTION_LOST
//   -2 = MQTT_CONNECT_FAILED (network OK but broker refused)
//   -1 = MQTT_DISCONNECTED
//    0 = MQTT_CONNECTED
//    1 = MQTT_CONNECT_BAD_PROTOCOL
//    2 = MQTT_CONNECT_BAD_CLIENT_ID
//    3 = MQTT_CONNECT_UNAVAILABLE
//    4 = MQTT_CONNECT_BAD_CREDENTIALS
//    5 = MQTT_CONNECT_UNAUTHORIZED
//
// ============================================================================
// FILES CREATED:
//   HAClient_DEBUG.h       — full drop-in replacement (already created)
//   HAClient_PATCH.md       — this file (incremental patch with instructions)
// ============================================================================
