# BombTimer — Table of Contents

## Proposito
Bomba de contagem regressiva para escape room. O jogador precisa desarmar a bomba cortando os fios corretos, inserindo codigo ou aproximando tag RFID — tudo contra o tempo. Display 14-segmentos, efeitos NeoPixel, audio MP3, controle via Home Assistant (MQTT).

## Hardware
| Dispositivo | Barramento | Funcao |
|------------|-----------|--------|
| Seeed XIAO ESP32-S3 | — | Microcontrolador |
| PCF8575 | I2C 0x20 | 4 botoes + 4 wires |
| HT16K33 | I2C 0x70 | Display 14-seg 4 digitos |
| PN532 | I2C 0x24 | NFC/RFID reader (opcional) |
| MAX98357A | I2S | Audio amplifier |
| NeoPixel Strip | GPIO (D2) | 48 LEDs |
| Expansion Board | I2C / SPI | RTC PCF8563 + SD Card |
| HLK-10M05 | Power | Fonte 5V |
| HLK-10M12 | Power | Fonte 12V (MagLock) |

## Arquivos Descritores

| Arquivo | Formato | Descricao |
|---------|---------|-----------|
| BombTimer_algoritmo.md | Markdown | Maquina de estados: Idle → Running → Solved/Exploded |
| BombTimer_bom.h | C Header (JSON) | Dispositivos detectados |
| BombTimer_conexoes.md | Markdown | Conexoes por barramento |
| BombTimer_orcamento.md | Markdown | Precificacao dos materiais |
| data/bomb.json | JSON | Configuracao runtime (countdown, modo, codigo) |

## Pastas
- `sch/` — Descritores de esquema eletrico
- `budget/` — Pesquisa de precos dos materiais

## Supabase
- `thoughts.metadata.type = "project_descriptor"`
- `thoughts.metadata.project_name = "BombTimer"`
- `thoughts.metadata.library = "esp32_escaperoom"`
- Tags: `escape_room`, `timer`, `i2c`, `i2s`, `mqtt`, `rfid`, `neopixel`
