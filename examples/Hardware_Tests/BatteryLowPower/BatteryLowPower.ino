/*
  BatteryLowPower

  Demonstrates the Battery Monitoring and LowPower Mode of the ESP32EscapeRoom library.

  Required hardware:
  - Seeed XIAO ESP32-S3
  - External NeoPixel strip (optional)
  - HT16K33 display (optional)
  - 2:1 voltage divider (2x 100Kohm) between BAT and GND,
    with the center connected to pin A0 (GPIO1 / D0)

  Behavior:
  - Shows the battery voltage on the display and Serial
  - Animates NeoPixels in rainbow
  - With LowPower active, brightness and volume follow
    the battery drop (proportional dimming)
  - Below 3.5V, EXTERNAL_POWER is cut

  Assemble the divider on A0 (GPIO1 / D0):
    BAT ---[100K]---[100K]--- GND
                      |
                      A0

  Use a variable supply to simulate battery discharge.
*/

#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>

using namespace esp32_escaperoom;

// ============================================================================
// HOME ASSISTANT + WIFI (added by the ESP32 port)
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
  haSensorState.setName("State");
  haSensorState.setIcon("mdi:puzzle");
  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();
  netConfig.handleAP();
}


// ============================================
// Battery management + power save
// ============================================
class BatteryDemo {
public:
  void begin() {
    hw.begin();

    // Expanders (adjust according to your hardware)
    if (hw.hasExpander() == false) {
      hw.addPcf8575(0x20);
    }

    // Starts monitoring on A0 (GPIO1 = D0, use the ADC pin)
    hw.beginBatteryMonitor();

    // Configure thresholds (more aggressive for the demo)
    hw.setPowerSaveThresholds(3.8, 3.5);

    // Enable power save
    hw.setLowPowerMode(true);

    Serial.println(F("[DEMO] Battery LowPower demo started"));
    Serial.println(F("[DEMO] Connect a variable supply to A0 (GPIO1)"));
    Serial.println(F("[DEMO] Adjust between 0-4.2V to simulate the battery\n"));
    report();
  }

  void update() {
    unsigned long now = millis();

    // Power save (proportional dimming)
    hw.updatePowerSave();

    // NeoPixel animation
    if (now - lastAnim_ > animInterval_) {
      lastAnim_ = now;
      hue_ = (hue_ + 5) % 360;
      uint32_t c = strip().ColorHSV(hue_ * 182, 255, currentBrightness());
      hw.setLightAll(c);
      hw.setLightInternal(0, hw.pixelsInternal().ColorHSV((hue_ + 180) * 182, 255, currentBrightness()));
    }

    // Update the display every 2s
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
      bat ? "BATTERY" : "USB    ",
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
