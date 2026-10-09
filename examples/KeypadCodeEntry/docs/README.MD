# KeypadCodeEntry — Developer Documentation

## Required Hardware

| Component | Quantity | Connection |
|---|---|---|
| XIAO ESP32-S3 + Expansion Board | 1 | Base |
| PCF8574 I/O Expander (I2C 0x20) | 1 | Grove I2C |
| 4×3 matrix keypad | 1 | PCF8574 pins |
| 5V relay (electric strike) | 1 | D0 (GPIO1) |
| NeoPixel (optional) | 4 LEDs | D2 (GPIO3) |
| Passive buzzer (Expansion Board) | 1 | D3 (GPIO4) |

## Keypad Pinout on the PCF8574

```
PCF8574 P0 → Row 1    PCF8574 P4 → Column 1
PCF8574 P1 → Row 2    PCF8574 P5 → Column 2
PCF8574 P2 → Row 3    PCF8574 P6 → Column 3
PCF8574 P3 → Row 4
```

## Dependencies

| Library | Version |
|---|---|
| ArduinoHA | ^1.0.0 |
| ArduinoJson | ^7.0.0 |
| IoAbstraction | ^3.0.0 |
| TaskManagerIO | ^3.0.0 |
| Adafruit NeoPixel | ^1.0.0 |

## Code Structure

```
setup()
  ├─ loadConfig()          — JSON from /keypad.json
  ├─ PCF8574 init (0x20)
  ├─ KeyboardManager init  — 4×3 keypad
  ├─ NeoPixel init
  ├─ Relay init (HIGH = locked)
  └─ connectWiFi()         — STA or AP mode

loop()
  ├─ taskManager.runLoop() — processes keypad via IoAbstraction
  ├─ haMqtt.loop()         — MQTT for Home Assistant
  ├─ handleKey(char)       — processes the pressed key
  ├─ checks pending code (500ms delay)
  └─ auto-reset after 5s in SOLVED/ERROR
```

## State Machine

```
IDLE      → first key → ENTERING
ENTERING  → 4 digits → CHECKING (500ms delay)
CHECKING  → correct code → SOLVED
CHECKING  → wrong code, attempts < max → IDLE
CHECKING  → wrong code, attempts >= max → ERROR
SOLVED    → 5s → IDLE (auto-reset, re-locks)
ERROR     → 5s → IDLE (auto-reset)
```

## JSON Configuration (/keypad.json)

```json
{
  "code": 1234,
  "maxAttempts": 3
}
```

## Home Assistant (MQTT)

| Entity | Type | ID | Description |
|---|---|---|---|
| State | sensor | `state` | idle, entering, checking, solved, error |
| Attempts | sensor | `attempts` | error count |
| Reset | button | `reset` | returns to IDLE |
| Code | number | `code` | 0-9999 |
| Max Attempts | number | `max_attempts` | 1-9 |

## AP Mode

- SSID: `KeypadCode-ESP32` (open)
- Portal: `http://192.168.4.1` (configure WiFi + MQTT)
- File: `/wifi.json` in LittleFS

## Build

```bash
# PlatformIO
pio run -e xiao_esp32s3

# Arduino IDE
Board: Seeed XIAO ESP32-S3
Partition Scheme: Default 4MB with ffat
USB CDC On Boot: Enabled
```
