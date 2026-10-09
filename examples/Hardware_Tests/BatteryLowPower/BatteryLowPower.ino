/*
  BatteryLowPower

  Demonstra o Battery Monitoring e LowPower Mode da biblioteca ESP32EscapeRoom.

  Hardware necessario:
  - Seeed XIAO ESP32-S3
  - Fita NeoPixel externa (opcional)
  - Display HT16K33 (opcional)
  - Divisor de tensao 2:1 (2x 100Kohm) entre BAT e GND,
    com o centro ligado ao pino A0 (GPIO1 / D0)

  Comportamento:
  - Exibe tensao da bateria no display e Serial
  - Anima NeoPixels em arco-iris
  - Com LowPower ativo, brilho e volume acompanham
    a queda da bateria (dimming proporcional)
  - Abaixo de 3.5V, EXTERNAL_POWER e cortado

  Monte o divisor no A0 (GPIO1 / D0):
    BAT ---[100K]---[100K]--- GND
                      |
                      A0

  Use fonte variavel para simular descarga da bateria.
*/

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

using namespace esp32_escaperoom;

// ============================================================================
// HOME ASSISTANT + WIFI (adicionado pelo port ESP32)
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
  haSensorState.setName("Estado");
  haSensorState.setIcon("mdi:puzzle");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}


// ============================================
// Gerenciamento de bateria + power save
// ============================================
class BatteryDemo {
public:
  void begin() {
    hw.begin();

    // Expansores (ajuste conforme seu hardware)
    if (hw.hasExpander() == false) {
      hw.addPcf8575(0x20);
    }

    // Inicia monitoramento no A0 (GPIO1 = D0, usar pino ADC)
    hw.beginBatteryMonitor();

    // Configura thresholds (mais agressivos para demo)
    hw.setPowerSaveThresholds(3.8, 3.5);

    // Ativa power save
    hw.setLowPowerMode(true);

    Serial.println(F("[DEMO] Battery LowPower demo started"));
    Serial.println(F("[DEMO] Conecte fonte variavel ao A0 (GPIO1)"));
    Serial.println(F("[DEMO] Ajuste entre 0-4.2V para simular bateria\n"));
    report();
  }

  void update() {
    unsigned long now = millis();

    // Power save (dimming proporcional)
    hw.updatePowerSave();

    // Animacao NeoPixel
    if (now - lastAnim_ > animInterval_) {
      lastAnim_ = now;
      hue_ = (hue_ + 5) % 360;
      uint32_t c = strip().ColorHSV(hue_ * 182, 255, currentBrightness());
      hw.setLightAll(c);
      hw.setLightInternal(0, hw.pixelsInternal().ColorHSV((hue_ + 180) * 182, 255, currentBrightness()));
    }

    // Atualiza display cada 2s
    if (now - lastDisplay_ > 2000) {
      lastDisplay_ = now;
      if (hw.hasDisplay()) {
        float v = hw.readBatteryVoltage();
        if (hw.isBatteryConnected()) {
          int dispVal = (int)(v * 100);
          hw.display().printNum(dispVal, true);
        } else {
          hw.display().printString("USB ");
        }
      }
      report();
    }

    hw.sound().loop();
  }

private:
  unsigned long lastAnim_ = 0;
  unsigned long lastDisplay_ = 0;
  uint16_t animInterval_ = 30;
  uint16_t hue_ = 0;

  Adafruit_NeoPixel &strip() { return hw.pixelsExternal(); }

  uint8_t currentBrightness() {
    return strip().getBrightness();
  }

  void report() {
    float v = hw.readBatteryVoltage();
    int pct = (int)hw.readBatteryPercent();
    bool bat = hw.isBatteryConnected();
    bool lp = hw.getLowPowerMode();

    Serial.printf("[DEMO] BAT=%.2fV (%d%%) %s | LowPower=%s | NeoBri=%d\n",
      v, pct,
      bat ? "BATERIA" : "USB    ",
      lp ? "ON " : "OFF",
      currentBrightness());
  }
};

BatteryDemo demo;

void setup() {
  uint32_t start = millis();
  while (!Serial && (millis() - start < 3000)) delay(10);

  demo.begin();
}

void loop() {
  loopHA();
  demo.update();
}
