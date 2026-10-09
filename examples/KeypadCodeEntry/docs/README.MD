# KeypadCodeEntry — Documentação do Desenvolvedor

## Hardware Necessário

| Componente | Quantidade | Conexão |
|---|---|---|
| XIAO ESP32-S3 + Expansion Board | 1 | Base |
| PCF8574 I/O Expander (I2C 0x20) | 1 | Grove I2C |
| Keypad matricial 4×3 | 1 | Pinos do PCF8574 |
| Relé 5V (trava elétrica) | 1 | D0 (GPIO1) |
| NeoPixel (opcional) | 4 LEDs | D2 (GPIO3) |
| Buzzer passivo (Expansion Board) | 1 | D3 (GPIO4) |

## Pinagem do Keypad no PCF8574

```
PCF8574 P0 → Linha 1    PCF8574 P4 → Coluna 1
PCF8574 P1 → Linha 2    PCF8574 P5 → Coluna 2
PCF8574 P2 → Linha 3    PCF8574 P6 → Coluna 3
PCF8574 P3 → Linha 4
```

## Dependências

| Biblioteca | Versão |
|---|---|
| ArduinoHA | ^1.0.0 |
| ArduinoJson | ^7.0.0 |
| IoAbstraction | ^3.0.0 |
| TaskManagerIO | ^3.0.0 |
| Adafruit NeoPixel | ^1.0.0 |

## Estrutura do Código

```
setup()
  ├─ loadConfig()          — JSON de /keypad.json
  ├─ PCF8574 init (0x20)
  ├─ KeyboardManager init  — keypad 4×3
  ├─ NeoPixel init
  ├─ Relé init (HIGH = travado)
  └─ connectWiFi()         — STA ou AP mode

loop()
  ├─ taskManager.runLoop() — processa keypad via IoAbstraction
  ├─ haMqtt.loop()         — MQTT para Home Assistant
  ├─ handleKey(char)       — processa tecla pressionada
  ├─ verifica código pendente (500ms delay)
  └─ auto-reset após 5s em SOLVED/ERROR
```

## Máquina de Estados

```
IDLE      → primeira tecla → ENTERING
ENTERING  → 4 dígitos → CHECKING (500ms delay)
CHECKING  → código certo → SOLVED
CHECKING  → código errado, tentativas < max → IDLE
CHECKING  → código errado, tentativas >= max → ERROR
SOLVED    → 5s → IDLE (auto-reset, re-trava)
ERROR     → 5s → IDLE (auto-reset)
```

## Configuração JSON (/keypad.json)

```json
{
  "code": 1234,
  "maxAttempts": 3
}
```

## Home Assistant (MQTT)

| Entidade | Tipo | ID | Descrição |
|---|---|---|---|
| Estado | sensor | `state` | idle, entering, checking, solved, error |
| Tentativas | sensor | `attempts` | contagem de erros |
| Resetar | button | `reset` | volta ao IDLE |
| Código | number | `code` | 0-9999 |
| Máx Tentativas | number | `max_attempts` | 1-9 |

## AP Mode

- SSID: `KeypadCode-ESP32` (aberta)
- Portal: `http://192.168.4.1` (configurar WiFi + MQTT)
- Arquivo: `/wifi.json` no LittleFS

## Compilação

```bash
# PlatformIO
pio run -e xiao_esp32s3

# Arduino IDE
Board: Seeed XIAO ESP32-S3
Partition Scheme: Default 4MB with ffat
USB CDC On Boot: Enabled
```
