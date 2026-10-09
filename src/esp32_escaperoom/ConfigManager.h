/* ConfigManager.h - SD > SPIFFS > AP mode w/ webserver config */
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <SD.h>
#include "PlatformPins.h"

namespace esp32_escaperoom {

struct WifiCfg { char ssid[64]="",pass[64]=""; bool ok=false; };
struct MqttCfg { char broker[64]=""; uint16_t port=1883; char user[64]="",pass[64]=""; bool ok=false; };
struct DevCfg { char name[32]="escaperoom-puzzle",room[32]="sala-01",puzzleId[32]="puzzle"; };

class ConfigManager {
public:
  ConfigManager() {}
  bool begin() {
    if(mountSD()&&load("/wifi.json"))return connect();
    if(mountSPIFFS()&&load("/wifi.json"))return connect();
    Serial.println("[CFG] No config. AP mode..."); startAP(); return false;
  }
  bool loadPuzzleConfig(const char* p,JsonDocument& d){
    if(sdOk){File f=SD.open(p,"r");if(f){deserializeJson(d,f);f.close();return true;}}
    if(spiffsOk){File f=SPIFFS.open(p,"r");if(f){deserializeJson(d,f);f.close();return true;}}
    return false;
  }
  bool savePuzzleConfig(const char* p,JsonDocument& d){
    if(sdOk){File f=SD.open(p,FILE_WRITE);if(f){serializeJson(d,f);f.close();return true;}}
    if(spiffsOk){File f=SPIFFS.open(p,FILE_WRITE);if(f){serializeJson(d,f);f.close();return true;}}
    return false;
  }
  const char* wifiSSID()const{return w.ssid;}
  const char* mqttBroker()const{return m.broker;}
  const char* mqttUser()const{return m.user;}
  const char* mqttPass()const{return m.pass;}
  const char* devName()const{return d.name;}
  const char* devRoom()const{return d.room;}
  const char* devPuzzleId()const{return d.puzzleId;}
  bool isAPMode()const{return apSrv!=nullptr;}
  void handleAP(){if(apSrv)apSrv->handleClient();}
  // Converte nome de nota (ex: "C4") para MIDI. Retorna -1 se inválido.
  static int noteNameToMidi(const String& name){
    if(name.length()<2) return -1;
    char letter=name.charAt(0);
    int octave=name.substring(1).toInt();
    int semitone;
    switch(letter){
      case 'C': semitone=0; break;
      case 'D': semitone=2; break;
      case 'E': semitone=4; break;
      case 'F': semitone=5; break;
      case 'G': semitone=7; break;
      case 'A': semitone=9; break;
      case 'B': semitone=11; break;
      default: return -1;
    }
    // MIDI = (octave+1)*12 + semitone  (C4 = 60)
    return (octave+1)*12 + semitone;
  }
private:
  WifiCfg w; MqttCfg m; DevCfg d;
  bool sdOk=false,spiffsOk=false;
  WebServer* apSrv=nullptr;
  bool mountSD(){if(!SD.begin(PIN_SD_CS)||SD.cardType()==CARD_NONE)return false;sdOk=true;return true;}
  bool mountSPIFFS(){if(!SPIFFS.begin(true))return false;spiffsOk=true;return true;}
  bool load(const char* path){
    File f=sdOk?SD.open(path,"r"):SPIFFS.open(path,"r");
    if(!f)return false;
    JsonDocument doc; if(deserializeJson(doc,f)){f.close();return false;} f.close();
    if(doc["wifi"]["ssid"].is<const char*>()){
      strlcpy(w.ssid,doc["wifi"]["ssid"],sizeof(w.ssid));
      w.ok=strlen(w.ssid)>0;
    }
    if(doc["mqtt"]["broker"].is<const char*>()){
      strlcpy(m.broker,doc["mqtt"]["broker"],sizeof(m.broker));
      m.port=doc["mqtt"]["port"]|1883;
      strlcpy(m.user,doc["mqtt"]["user"]|"",sizeof(m.user));
      strlcpy(m.pass,doc["mqtt"]["pass"]|"",sizeof(m.pass));
    }
    strlcpy(d.name,doc["device"]["name"]|"escaperoom-puzzle",sizeof(d.name));
    strlcpy(d.room,doc["device"]["room"]|"sala-01",sizeof(d.room));
    strlcpy(d.puzzleId,doc["device"]["puzzle_id"]|"puzzle",sizeof(d.puzzleId));
    return true;
  }
  bool connect(){
    WiFi.mode(WIFI_STA);WiFi.begin(w.ssid,"");
    Serial.printf("[WIFI] %s... ",w.ssid);
    for(int i=0;i<40&&WiFi.status()!=WL_CONNECTED;i++){delay(500);Serial.print(".");}
    if(WiFi.status()==WL_CONNECTED){Serial.printf(" IP:%s\n",WiFi.localIP().toString().c_str());return true;}
    Serial.println(" FAIL"); return false;
  }
  void startAP(){
    WiFi.mode(WIFI_AP);WiFi.softAP("ESP32-EscapeRoom");
    apSrv=new WebServer(80);
    apSrv->on("/",HTTP_GET,[this](){apSrv->send(200,"text/html",
      "<html><head><meta charset=UTF-8><meta name=viewport content='width=device-width,initial-scale=1'>"
      "<style>body{font-family:system-ui;max-width:500px;margin:auto;padding:20px}"
      "label{display:block;margin-top:10px;font-weight:bold}"
      "input{width:100%;padding:8px;margin-top:4px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box}"
      "button{margin-top:20px;padding:10px 20px;background:#06c;color:white;border:none;border-radius:4px;cursor:pointer;font-size:16px}"
      ".s{border:1px solid #ddd;border-radius:8px;padding:15px;margin-top:15px}"
      "</style><body><h2>EscapeRoom Config</h2>"
      "<form action=/save method=POST>"
      "<div class=s><h3>WiFi</h3><label>SSID: <input name=ws required></label></div>"
      "<div class=s><h3>MQTT</h3><label>Broker: <input name=mb placeholder='192.168.1.10'></label>"
      "<label>Porta: <input name=mp value=1883 type=number></label>"
      "<label>User: <input name=mu></label><label>Pass: <input name=mpw type=password></label></div>"
      "<div class=s><h3>Dispositivo</h3>"
      "<label>Nome: <input name=dn value='escaperoom-puzzle'></label>"
      "<label>Sala: <input name=dr value=sala-01></label>"
      "<label>Puzzle ID: <input name=dp value=puzzle></label></div>"
      "<button type=submit>Salvar</button></form></body></html>");
    });
    apSrv->on("/save",HTTP_POST,[this](){
      JsonDocument doc;
      doc["wifi"]["ssid"]=apSrv->arg("ws");
      doc["mqtt"]["broker"]=apSrv->arg("mb");
      doc["mqtt"]["port"]=apSrv->arg("mp").toInt();
      doc["mqtt"]["user"]=apSrv->arg("mu");
      doc["mqtt"]["pass"]=apSrv->arg("mpw");
      doc["device"]["name"]=apSrv->arg("dn");
      doc["device"]["room"]=apSrv->arg("dr");
      doc["device"]["puzzle_id"]=apSrv->arg("dp");
      if(!spiffsOk)SPIFFS.begin(true);
      File f=SPIFFS.open("/wifi.json",FILE_WRITE);
      if(f){serializeJson(doc,f);f.close();apSrv->send(200,"text/html","<html><body><h2>Salvo!</h2></body></html>");delay(3000);ESP.restart();}
      else apSrv->send(500,"text/plain","Erro");
    });
    // Editor de sequência musical (MUSICAL_SEQUENCE)
    apSrv->on("/musical",HTTP_GET,[this](){
      apSrv->send(200,"text/html",
        "<html><head><meta charset=UTF-8></head><body>"
        "<h2>Sequencia Musical</h2>"
        "<form action=/musical/save method=POST>"
        "<label>Notas (C4,400,100;E4,400,100;G4,800,0): "
        "<input name=seq></label><br>"
        "<label>Tolerancia (cents): <input name=tol value=25 type=number></label><br>"
        "<label>Max gap (ms): <input name=gap value=1500 type=number></label><br>"
        "<label>Max nota (ms): <input name=ndur value=3000 type=number></label><br>"
        "<label>Timeout total (ms): <input name=tout value=15000 type=number></label><br>"
        "<label>Min amplitude: <input name=amp value=200 type=number></label><br>"
        "<button type=submit>Salvar</button></form></body></html>");
    });
    apSrv->on("/musical/save",HTTP_POST,[this](){
      // Parse da sequência: "C4,400,100;E4,400,100;G4,800,0"
      String seq = apSrv->arg("seq");
      JsonDocument doc;
      doc["musical"]["enabled"] = true;
      doc["musical"]["tolerance_cents"] = apSrv->arg("tol").toInt();
      doc["musical"]["max_gap_ms"] = apSrv->arg("gap").toInt();
      doc["musical"]["max_note_duration_ms"] = apSrv->arg("ndur").toInt();
      doc["musical"]["total_timeout_ms"] = apSrv->arg("tout").toInt();
      doc["musical"]["min_amplitude"] = apSrv->arg("amp").toInt();
      JsonArray notes = doc["musical"].createNestedArray("notes");
      int start = 0;
      while (start < (int)seq.length()) {
        int semi = seq.indexOf(';', start);
        if (semi < 0) semi = seq.length();
        String noteStr = seq.substring(start, semi);
        noteStr.trim();
        if (noteStr.length() > 0) {
          int c1 = noteStr.indexOf(',');
          int c2 = noteStr.indexOf(',', c1 + 1);
          String name = noteStr.substring(0, c1);
          int dur = noteStr.substring(c1 + 1, c2).toInt();
          int gap = noteStr.substring(c2 + 1).toInt();
          // Converte nome de nota (C4) para MIDI
          int midi = noteNameToMidi(name);
          if (midi >= 0) {
            JsonObject n = notes.createNestedObject();
            n["midi"] = midi;
            n["duration_ms"] = dur;
            n["gap_ms"] = gap;
          }
        }
        start = semi + 1;
      }
      if(!spiffsOk)SPIFFS.begin(true);
      File f=SPIFFS.open("/config.json",FILE_WRITE);
      if(f){serializeJson(doc,f);f.close();apSrv->send(200,"text/html","<html><body><h2>Sequencia salva!</h2></body></html>");}
      else apSrv->send(500,"text/plain","Erro");
    });
    apSrv->begin();
    Serial.printf("[AP] ESP32-EscapeRoom http://%s\n",WiFi.softAPIP().toString().c_str());
  }
};
}
