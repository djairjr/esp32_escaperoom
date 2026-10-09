# BombTimer — Home Assistant Entities

## Device
- **Name**: BombTimer STAMPLC (configurable)
- **Connection**: MQTT via ArduinoHA
- **Initial configuration**: AP Mode (WiFi "BombTimer-STAMPLC" + web portal at 192.168.4.1)

## Sensors (HASensor)

| Name | ID | Type | Values | Icon |
|---|---|---|---|---|
| Game State | `state` | text | idle, running, defused, detonated, menu, alarm | `mdi:bomb` |
| Remaining Time | `time` | text (s) | 0-5999 | `mdi:timer` |
| Wires Cut | `wires_cut` | text | 0-4 | `mdi:cable-data` |
| Attempts | `code_attempts` | text | 0-maxCodeAttempts | `mdi:counter` |
| I2C Devices | `i2c_status` | text | list of detected devices | `mdi:chip` |

## Binary Sensors (HASensor per wire)

| Name | ID | Values | Icon |
|---|---|---|---|
| Yellow Wire | `wire_yellow` | 0=connected, 1=cut | `mdi:cable-data` |
| Red Wire | `wire_red` | 0=connected, 1=cut | `mdi:cable-data` |
| Blue Wire | `wire_blue` | 0=connected, 1=cut | `mdi:cable-data` |
| Green Wire | `wire_green` | 0=connected, 1=cut | `mdi:cable-data` |

## Buttons (HAButton)

| Name | ID | Action | Icon |
|---|---|---|---|
| Start | `start` | Starts the countdown (IDLE→RUNNING) | `mdi:play` |
| Reset | `reset` | Resets to IDLE (any state) | `mdi:restart` |

## Numbers (HANumber)

| Name | ID | Range | Step | Unit | Icon |
|---|---|---|---|---|---|
| Time Adjustment | `countdown` | 5-5999 | 1 | s | `mdi:timer-outline` |
| Defuse Code | `defuse_code` | 0-9999 | 1 | — | `mdi:lock` |
| Brightness | `brightness` | 0-15 | 1 | — | `mdi:brightness-6` |

## Selectors (HASelect)

| Name | ID | Options | Icon |
|---|---|---|---|
| Defuse Mode | `mode` | CODE, WIRE, ORDER | `mdi:tune` |
| Detonate Wire | `det_wire` | RANDOM, Yellow, Red, Blue, Green, NONE | `mdi:alert` |
| Defuse Wire | `defuse_wire` | RANDOM, Yellow, Red, Blue, Green, NONE | `mdi:shield-check` |
| Speed 2x Wire | `speed_wire` | RANDOM, Yellow, Red, Blue, Green, NONE | `mdi:speedometer` |
| Pause Wire | `pause_wire` | RANDOM, Yellow, Red, Blue, Green, NONE | `mdi:pause` |
| Yellow Order | `order_yellow` | 1, 2, 3, 4 | `mdi:format-list-numbered` |
| Red Order | `order_red` | 1, 2, 3, 4 | `mdi:format-list-numbered` |
| Blue Order | `order_blue` | 1, 2, 3, 4 | `mdi:format-list-numbered` |
| Green Order | `order_green` | 1, 2, 3, 4 | `mdi:format-list-numbered` |

## Switch (HASwitch)

| Name | ID | Icon |
|---|---|---|
| Tick Sound | `tick_sound` | `mdi:volume-high` |

## MQTT Behavior

- **Initial publication**: on connect, publishes all states
- **Periodic publication**: every 2 seconds during RUNNING
- **On-demand publication**: on state change, wire cut, code entry
- **Commands received**: start, reset, change configuration

## Suggested Automations

1. **Detonation notification**: when `state` changes to `detonated` → notify GameMaster
2. **Defusal notification**: when `state` changes to `defused` → notify GameMaster
3. **Timer synchronization**: when `start` is pressed → start global timer in the room
4. **Cut-wire alert**: when any `wire_*` changes to 1 → log in HA
5. **Remote adjustment**: GameMaster changes `countdown`, `defuse_code` or `mode` via HA

## Network Configuration (AP Mode)

1. On first boot, creates WiFi network "BombTimer-STAMPLC" (open)
2. Web portal at `http://192.168.4.1` allows configuring:
   - WiFi (SSID + password)
   - MQTT (broker IP, port, user, password)
   - Device name in HA
3. Saves to `/wifi.json` in LittleFS and reboots
4. If WiFi fails after 20 attempts, returns to AP mode
