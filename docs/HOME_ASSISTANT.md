# Home Assistant integration

On the RP2040 original, puzzles talked to each other over an **RS485** bus. On ESP32 the
library drops RS485: the puzzle is a network device instead. The examples use **ArduinoHA**,
which wraps MQTT and speaks Home Assistant's **MQTT discovery** protocol, so a puzzle appears
in HA automatically — no manual YAML per entity.

The library itself does not hard-depend on ArduinoHA; it is the examples that add it. For the
room-level view — what the integration is for, the Home Assistant Green, and how a room is
organized in HA (floors and areas) — see
[`HOME_ASSISTANT_IN_ESCAPE_ROOMS.md`](HOME_ASSISTANT_IN_ESCAPE_ROOMS.md).

What the *library* provides for this integration is:

- `ConfigManager` — broker, port, credentials and device identity (`device.name`,
  `device.room`, `device.puzzle_id`), read from `/wifi.json`;
- `PuzzleBase::setStateChangeCallback()` — the hook that mirrors FSM transitions onto MQTT
  without polling;
- The `hw` API is used to expose the settings that control various entities, such as relay, volume, brightness,
  countdown).

---

## 1. Shape of an integration

```cpp
#include <ESP32EscapeRoom.h>
#include <esp32_escaperoom/ConfigManager.h>
#include <WiFi.h>
#include <ArduinoHA.h>
using namespace esp32_escaperoom;

ConfigManager netConfig;
WiFiClient    wifiClient;
HADevice      haDevice;
HAMqtt        haMqtt(wifiClient, haDevice);

HASensor haSensorState("state");        // entities: one object each

void initHA() {
  netConfig.begin();
  byte mac[6]; WiFi.macAddress(mac);
  haDevice.setUniqueId(mac, sizeof(mac));   // MAC-based unique id → stable in HA
  haDevice.setName(netConfig.devName());
  haDevice.setSoftwareVersion("1.0.0");

  haSensorState.setName("State");
  haSensorState.setIcon("mdi:puzzle");

  haMqtt.begin(netConfig.mqttBroker(), netConfig.mqttUser(), netConfig.mqttPass());
}

void loopHA() {
  haMqtt.loop();          // pump MQTT every iteration
  netConfig.handleAP();
}
```

Every `loop()` must pump `haMqtt.loop()` (and `config.handleAP()` while in AP mode), the
same way it must pump `hw.sound().loop()`.

---

## 2. Mirroring the state machine

Register the FSM callback so a transition publishes itself:

```cpp
void onState(PuzzleBase* p, PuzzleState, PuzzleState now, void*) {
  haSensorState.setValue(stateName(now));     // "IDLE", "RUNNING", "SOLVED", …
}

puzzle.setStateChangeCallback(onState);
```

Because `setState()` only fires on a real change, the entity is published exactly once per
transition — no polling, no chatter.

---

## 3. Entity patterns used by the examples

| Entity | ArduinoHA type | Typical use |
|--------|----------------|-------------|
| State | `HASensor` | Text state of the FSM (`stateName()`). |
| Progress / time left | `HASensor` | `getRemainingTime()`, percentage solved. |
| Counters | `HASensor` | attempts, wires cut, mode. |
| Lock / relay | `HALock` | The room's door or the puzzle's lock. |
| Start / reset / solve / detonate | `HAButton` | Drive the puzzle from HA. |
| Mode | `HASelect` | Choose a puzzle mode. |
| Countdown / code | `HANumber` | Set a timer or a code. |
| Sound toggle | `HASwitch` | Enable/disable cues. |

`BombTimer` is the complete worked example: eight sensors, one lock, four buttons, a
select, two numbers and a switch, all declared as globals and updated from the puzzle.

---

## 4. Pitfalls

- **`HASensor::setValue()` takes a string, not a state object.** For a `HASensor`, use
  `setValue(stateName(...))`. For a `HANumber`, read the incoming value with
  `val.toInt32()` (not `toDouble()`). Using the wrong setter compiles but publishes nothing.
- **No lambdas with captures.** ArduinoHA callbacks are C function pointers; use a global
  function plus a static/global context.
- **Callbacks run inside the MQTT pump.** Do not block or call heavy puzzle code there.
- **Unique id must be stable.** Derive it from the MAC (as above). A name-based id changes
  when you rename the device and orphan the entities in HA.
- **One broker per room.** All puzzles in a room share the MQTT broker from `/wifi.json`;
  keep the `device.name` distinct.
