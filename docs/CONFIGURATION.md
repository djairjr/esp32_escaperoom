# Configuration

Configuration is handled by `ConfigManager` (`src/esp32_escaperoom/ConfigManager.h`). It
resolves the device's **network** configuration (WiFi + MQTT + identity) and provides
read/write access to **puzzle** configuration files. When no network configuration exists,
the device opens a WiFi access point and serves a setup page.

---

## 1. Resolution order

```
begin()
  ├─ mount SD card            → load("/wifi.json")  → connect()  → done
  ├─ mount LittleFS (SPIFFS)  → load("/wifi.json")  → connect()  → done
  └─ startAP()                → captive portal on http://192.168.4.1
```

The first filesystem that mounts **and** holds a valid `/wifi.json` wins. If WiFi then
fails to connect within the timeout (40 × 500 ms = 20 s), `begin()` returns `false` and the
device keeps running offline — it does **not** fall back to the AP in the current release.

```cpp
ConfigManager config;

void setup() {
  config.begin();
  if (config.isAPMode()) {
    // portal is up; the user configures via a browser
  }
}

void loop() {
  config.handleAP();     // no-op unless in AP mode
}
```

---

## 2. The network configuration file: `/wifi.json`

```json
{
  "wifi":   { "ssid": "my-network", "pass": "…" },
  "mqtt":   { "broker": "192.168.1.10", "port": 1883, "user": "ha", "pass": "…" },
  "device": { "name": "escaperoom-puzzle", "room": "sala-01", "puzzle_id": "puzzle" }
}
```

| Path | Default | Used by |
|------|---------|---------|
| `wifi.ssid` | `""` | `wifiSSID()`, `connect()` |
| `mqtt.broker`, `mqtt.port` | `""`, `1883` | `mqttBroker()` |
| `mqtt.user`, `mqtt.pass` | `""` | `mqttUser()`, `mqttPass()` |
| `device.name` | `escaperoom-puzzle` | `devName()` — MQTT device name / HA identity |
| `device.room` | `sala-01` | `devRoom()` |
| `device.puzzle_id` | `puzzle` | `devPuzzleId()` |

Missing keys fall back to the defaults above (`"key" | default` idiom). Note that
`ConfigManager` reads the WiFi password from the file but, in the current release, calls
`WiFi.begin(ssid, "")` — the password is carried in the struct but **not yet applied**, so
only open networks or saved-credential networks connect. See *Pitfalls*.

---

## 3. Puzzle configuration files

Each example ships its own JSON (e.g. `bomb.json`, `keypad.json`, `config.json`) that is
loaded through the same SD→LittleFS resolution:

```cpp
JsonDocument doc;
if (config.loadPuzzleConfig("/bomb.json", doc)) {
  uint16_t countdown = doc["ctdn"] | 600;
  uint8_t  mode      = doc["mode"] | 0;
  // ...
}
```

`savePuzzleConfig(path, doc)` writes it back to whichever backend is mounted.

The **UniversalController** family is fully data-driven; its `config.json` describes the
whole puzzle:

```json
{
  "controller": { "name": "My Puzzle", "id": 1, "enabled": true },
  "inputs":  { "count": 5, "pcfAddress": "0x20", "pinMapping": [0,1,2,3,4], "debounceMs": 50 },
  "logic":   { "mode": "CUSTOM_SEQUENCE", "sequence": [1,3,5,2,4],
               "timeoutSeconds": 300, "resetOnError": true, "allowRepeat": false },
  "pwm":     { "enabled": true, "channel": 0, "minValue": 0, "maxValue": 100, "steps": 5 },
  "audio":   { "enabled": true, "inputSounds": ["/input1.mp3"], "unlockSound": "/unlock.mp3",
               "errorSound": "/error.mp3", "resetSound": "/reset.mp3" },
  "neopixel":{ "enabled": true, "count": 16,
               "colors": { "idle": [0,0,255], "active": [255,255,0], "solved": [0,255,0],
                           "error": [255,0,0], "progress": [0,255,100] } },
  "output":  { "lockPin": 10, "activeHigh": false }
}
```

`logic.mode` is one of `CUSTOM_SEQUENCE`, `LINEAR_SEQUENCE`, `ALL_CONNECTED`, `RANDOM`,
`MUSICAL_SEQUENCE` (interpreted by the example-local `PuzzleEngine` — see
[`ARCHITECTURE.md`](ARCHITECTURE.md)).

---

## 4. The AP-mode portal

When no configuration is found, `startAP()`:

1. puts the radio in `WIFI_AP` and raises the SSID **`ESP32-EscapeRoom`**
   (`http://192.168.4.1`);
2. serves a small HTML form (WiFi, MQTT, device identity);
3. on `POST /save`, writes `/wifi.json` to SPIFFS and reboots.

While in AP mode, call `config.handleAP()` every iteration so the web server is serviced —
it is a plain `WebServer` that only advances when `handleClient()` is pumped.

There is also a `/musical` page in the same server: a form that parses a note sequence in
the form `C4,400,100;E4,400,100;G4,800,0` (name, duration ms, gap ms) and writes a
`MUSICAL_SEQUENCE` block into `/config.json`, converting note names to MIDI with
`ConfigManager::noteNameToMidi()`.

---

## 5. SPIFFS vs LittleFS

`ConfigManager` uses the **SPIFFS** API for `/wifi.json` and `/config.json`, while the rest
of the library (audio, hardware boot) uses **LittleFS**. Both are in the build; the
migration of `ConfigManager` to LittleFS is a pending task. Consequences to know:

- format/upload `/wifi.json` to the **SPIFFS** partition, puzzle audio/JSON files to the
  filesystem the example reads (`hw.sound()` reads LittleFS by default, `useSD = true` for
  SD);
- `SPIFFS` is deprecated upstream; new boards (esp32 core ≥ 3.x) print a deprecation
  warning. It still works.

---

## 6. Pitfalls

- **The WiFi password is not applied.** `connect()` currently calls `WiFi.begin(ssid, "")`.
  The password is stored and returned by `mqttPass()`-style accessors but not passed to the
  driver. Use an open network, or pre-provision credentials, until this is fixed.
- **Puzzle JSON is not validated.** `loadPuzzleConfig` deserializes whatever is there; a
  malformed file yields default values or an empty document. Log the result.
- **`/save` reboots.** Saving from the portal calls `ESP.restart()`; anything unsaved in
  RAM is lost. That is intentional (the network config is read at boot).
- **AP SSID is generic.** Several devices in the same room all raise
  `ESP32-EscapeRoom`; configure them one at a time.
- **Storage is resolved per read.** SD, if mounted, shadows LittleFS for the same path.
  Keep filenames distinct if both are in use.
