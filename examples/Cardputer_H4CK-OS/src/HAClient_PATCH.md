// ============================================================================
// HAClient.h — INCREMENTAL PATCH para debug MQTT/ArduinoHA
// ============================================================================
// Aplique estas alterações no HAClient.h original.
//
// PASSO 1: Adicione DEBUG MACROS após #pragma once (linha 2):
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
// PASSO 2: Substitua void begin() { ... } (linhas 55-82 aprox):
//
//   void begin() {
//       HA_DBG("begin() iniciado");
//       HA_DBG("  SSID='%s' (len=%d)", configManager.ha.wifiSsid, strlen(configManager.ha.wifiSsid));
//       HA_DBG("  Broker='%s' Port=%d", configManager.ha.broker, (int)configManager.ha.port);
//       HA_DBG("  User='%s' Pass='%s'", configManager.ha.user, configManager.ha.pass);
//
//       if (strlen(configManager.ha.wifiSsid) == 0) {
//           HA_INF("WiFi SSID vazio — iniciando AP");
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
//       HA_DBG("WiFi loop terminou status=%d tentativas=%d", WiFi.status(), att);
//
//       if (WiFi.status() == WL_CONNECTED) {
//           HA_INF("WiFi OK IP=%s MAC=%s RSSI=%d",
//                  WiFi.localIP().toString().c_str(),
//                  WiFi.macAddress().c_str(), WiFi.RSSI());
//           wifiOK_ = true;
//           beginMQTT();
//           beginHA();
//       } else {
//           HA_ERR("WiFi falhou status=%d", WiFi.status());
//           Serial.println(" FAIL — starting AP mode");
//           startAP();
//       }
//   }
//
// PASSO 3: Substitua void loopHA() (linhas 84-109 aprox):
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
//               HA_DBG("MQTT RAW desconectado — tentando beginMQTT()");
//               beginMQTT();
//           }
//       } else {
//           mqttClient_.loop();
//           static unsigned long lastRawReconnect = 0;
//           if (!mqttClient_.connected() && millis() - lastRawReconnect > 10000) {
//               lastRawReconnect = millis();
//               HA_DBG("MQTT RAW caiu — reconnectMQTT()");
//               reconnectMQTT();
//           }
//       }
//
//       // ArduinoHA
//       if (!haOK_) {
//           static unsigned long lastHaAttempt = 0;
//           if (millis() - lastHaAttempt > 5000) {
//               lastHaAttempt = millis();
//               HA_DBG("ArduinoHA não iniciado — beginHA()");
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
//               HA_INF("HA MQTT conectou — publishAll()");
//               publishAll();
//           }
//           if (!nowConnected && lastHaConnected) {
//               lastHaConnected = false;
//               HA_ERR("HA MQTT desconectou!");
//           }
//       }
//
//       // Diagnóstico periódico a cada 30s
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
// PASSO 4: Substitua void beginHA() (linhas 225-274 aprox) — O MAIS IMPORTANTE:
//
//   void beginHA() {
//       if (haOK_) { HA_DBG("beginHA() já iniciado"); return; }
//       if (!wifiOK_) { HA_ERR("beginHA(): WiFi não OK"); return; }
//       if (strlen(configManager.ha.broker) == 0) {
//           HA_ERR("beginHA(): broker vazio"); return; }
//
//       HA_INF("beginHA() — configurando HADevice + entidades");
//       byte mac[6]; WiFi.macAddress(mac);
//       haDevice_.setUniqueId(mac, sizeof(mac));
//       haDevice_.setName("H4CK-OS");
//       haDevice_.setSoftwareVersion(APP_VERSION);
//       haDevice_.setManufacturer("EscapeRoom");
//       haDevice_.setModel("Cardputer");
//       HA_DBG("Device ID=%02X%02X%02X%02X%02X%02X", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
//
//       // ... configuração das entidades mantida igual ...
//       // (copie as linhas originais 236-266)
//
//       HA_INF("haMqtt_.begin(broker=%s, port=%d)",
//              configManager.ha.broker, (int)configManager.ha.port);
//       HA_DBG("  user='%s' pass='%s' (lens=%d/%d)",
//              strlen(configManager.ha.user) > 0 ? "***" : "(nenh)",
//              strlen(configManager.ha.pass) > 0 ? "***" : "(nenh)",
//              strlen(configManager.ha.user), strlen(configManager.ha.pass));
//
//       // *** CORREÇÃO CRÍTICA #1 ***
//       // Verifique a assinatura de haMqtt_.begin() na ArduinoHA.h:
//       //   Se for: begin(const char* broker, uint16_t port, const char* user, const char* pass)
//       //   USE: haMqtt_.begin(configManager.ha.broker, configManager.ha.port, user, pass);
//       //   Se for: begin(const char* broker, const char* user, const char* pass)
//       //   USE: haMqtt_.begin(configManager.ha.broker, user, pass); // porta sempre 1883
//       //
//       // O código original NÃO passa a porta! Se broker usa porta != 1883, HA falha.
//       haMqtt_.begin(
//           configManager.ha.broker,
//           configManager.ha.port,     // <-- ADICIONAR ESTA LINHA se a API suportar
//           strlen(configManager.ha.user) > 0 ? configManager.ha.user : nullptr,
//           strlen(configManager.ha.pass) > 0 ? configManager.ha.pass : nullptr
//       );
//
//       haOK_ = true;
//       HA_INF("ArduinoHA inicializado em %s:%d", configManager.ha.broker, (int)configManager.ha.port);
//       if (haMqtt_.isConnected()) {
//           HA_INF("MQTT já conectado — publishAll()");
//           publishAll();
//       } else {
//           HA_DBG("MQTT ainda não conectado — aguardando loopHA()");
//       }
//   }
//
// PASSO 5: Adicione debug às funções publishAll / publishStatus / lastPayload:
//
//   void publishAll() {
//       if (!haOK_) { HA_ERR("publishAll(): haOK_=false"); return; }
//       if (!haMqtt_.isConnected()) {
//           HA_ERR("publishAll(): HA MQTT desconectado"); return; }
//       HA_DBG("publishAll() — publicando 9 entidades");
//       haSensorEstado_.setValue(lastStatus_);
//       haSensorLogin_.setValue(configManager.login.pass);
//       haSensorSniffIp_.setValue(configManager.sniff.ip);
//       haNumDefuseCode_.setState((int32_t)configManager.sniff.defuseCode);
//       haNumSniffChannel_.setState((int8_t)(configManager.sniff.channel - 1));
//       haSelectStage_.setState((int8_t)configManager.stages.current);
//       haSwitchRF433_.setState(rf433Enabled_);
//       HA_INF("publishAll() completo");
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
// POSSÍVEL CAUSA RAIZ DO PROBLEMA HA
// ============================================================================
// O código original chama:
//      haMqtt_.begin(configManager.ha.broker, user, pass);
//
// Essa assinatura NÃO passa a porta explícita. A ArduinoHA pode usar
// porta default 1883. Se o broker usa porta diferente (ex: 1883 com TLS,
// ou Mosquitto em outra porta), a conexão falhará silenciosamente.
//
// Verifique em ArduinoHA/src/utils/HAMqtt.h a assinatura de begin().
// Se existir begin(broker, port, user, pass), USE ESSA assinatura.
//
// OUTRA CAUSA: beginHA() seta haOK_=true IMEDIATAMENTE após begin(),
// mesmo se MQTT não conectou. Isso faz publishAll() ser chamado em
// loopHA() antes de haMqtt_.isConnected() voltar true, resultando
// em publishes perdidos. A correção do publishAll() acima já resolve.
//
// OUTRA CAUSA: loopHA() não verifica heap. Se LittleFS ou LVGL consome
// muita RAM, o WiFiClient pode falhar em conectar. Adicione:
//      if (ESP.getFreeHeap() < 20000) { HA_ERR("heap baixo=%d", ESP.getFreeHeap()); }
//
// ============================================================================
// VERIFICAÇÃO NA SERIAL DEPOIS DE GRAVAR:
// ============================================================================
// Descomente #define HA_DEBUG_LEVEL 3 no .ino (antes de #include "HAClient.h")
// ou adicione globalmente:
//      #define HA_DEBUG_LEVEL 3
//
// Abra monitor serial a 115200. Deve ver (em ordem):
//   [HA-DBG] begin() iniciado
//   [HA-DBG]   SSID='SUA_REDE' (len=N)
//   [HA-DBG]   Broker='192.168.x.x' Port=1883
//   [HA-DBG]   User='seuuser' Pass='seupass'
//   [HA-INF] Connecting to WiFi 'SUA_REDE'...
//   ..........
//   [HA-INF] WiFi OK IP=192.168.x.x MAC=xx:xx:xx:xx:xx:xx RSSI=-45
//   [HA-INF] beginMQTT() — broker=192.168.x.x port=1883
//   [HA-INF] reconnectMQTT() — clientId=H4CKOS_xxxxxxxxx broker=192.168.x.x port=1883
//   [HA-INF] reconnectMQTT() — SUCESSO
//   [HA-INF] beginHA() — configurando HADevice + entidades
//   [HA-DBG] Device ID=xxxxx
//   [HA-INF] haMqtt_.begin(broker=192.168.x.x, port=1883)
//   [HA-INF] ArduinoHA inicializado em 192.168.x.x:1883
//   [HA-DBG] MQTT ainda não conectado — aguardando loopHA()
//   [HA-DBG] haMqtt_.isConnected()=true last=false
//   [HA-INF] HA MQTT conectou — publishAll()
//   [HA-INF] publishAll() completo
//
// Se você NÃO vê "HA MQTT conectou", o problema está na conexão MQTT.
// Códigos de erro PubSubClient::state():
//   -4 = MQTT_CONNECTION_TIMEOUT
//   -3 = MQTT_CONNECTION_LOST
//   -2 = MQTT_CONNECT_FAILED (rede OK mas broker recusou)
//   -1 = MQTT_DISCONNECTED
//    0 = MQTT_CONNECTED
//    1 = MQTT_CONNECT_BAD_PROTOCOL
//    2 = MQTT_CONNECT_BAD_CLIENT_ID
//    3 = MQTT_CONNECT_UNAVAILABLE
//    4 = MQTT_CONNECT_BAD_CREDENTIALS
//    5 = MQTT_CONNECT_UNAUTHORIZED
//
// ============================================================================
// ARQUIVOS CRIADOS:
//   HAClient_DEBUG.h       — drop-in replacement completo (já criado)
//   HAClient_PATCH.md       — este arquivo (patch incremental com instruções)
// ============================================================================
