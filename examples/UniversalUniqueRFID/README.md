# UniversalUniqueRFID

Leitor RFID Universal Configuravel para Escape Room usando a biblioteca PropMaker EscapeRoom.

## Visao Geral

O UniversalUniqueRFID e um puzzle RFID configuravel via arquivo JSON que suporta dois modos de operacao com feedback audiovisual e comunicacao RS485 opcional.

## Modos de Puzzle

### 1. UNIQUE (Unico)

Qualquer tag RFID programada no JSON resolve o puzzle.

**Comportamento:**
- Aproxime qualquer tag cadastrada
- Puzzle resolvido imediatamente
- Ideal para puzzles de "chave unica"

### 2. SEQUENCE (Sequencia)

Tags devem ser lidas em ordem especifica.

**Exemplo:** `sequence: [0, 2, 1]` significa Tag 0 -> Tag 2 -> Tag 1

**Comportamento:**
- Ordem obrigatoria conforme sequencia
- Tag errada: toca som de erro (reseta se configurado)
- Ideal para puzzles de "codigo RFID"

## Hardware

### Requisitos
- Adafruit PropMaker Feather RP2040
- PN532 NFC/RFID module (I2C)
- NeoPixel (LEDs enderecaveis)
- SD Card (para sons MP3)
- Tags RFID (MIFARE Classic, NTAG, etc.)

### Conexoes
```
PN532 (I2C):
- SDA -> GPIO 24 (I2C SDA)
- SCL -> GPIO 25 (I2C SCL)
- VCC -> 3.3V
- GND -> GND
- IRQ  -> (opcional, nao conectado por padrao)

NeoPixel:
- Data -> GPIO 21 (EXTERNAL_NEOPIXEL)
- VCC -> 5V (via POWER_EXT)
- GND -> GND

Saida Rele/Trava:
- Pino 10 (RELAY)
```

## Configuracao

### Arquivo config.json

O arquivo `data/config.json` deve ser gravado no LittleFS da placa.

#### Estrutura

```json
{
  "controller": {
    "name": "Nome do Puzzle",
    "id": 2,
    "enabled": true
  },

  "rfid": {
    "mode": "UNIQUE",
    "tags": [
      { "uid": "A1B2C3D4", "name": "Tag Vermelha" },
      { "uid": "E5F6A7B8", "name": "Tag Azul" },
      { "uid": "9A0B1C2D", "name": "Tag Verde" }
    ],
    "validTags": [
      { "uid": "A1B2C3D4", "name": "Tag Vermelha" },
      { "uid": "E5F6A7B8", "name": "Tag Azul" },
      { "uid": "9A0B1C2D", "name": "Tag Verde" },
      { "uid": "11223344", "name": "Tag Amarela" }
    ],
    "sequence": [0, 1, 2],
    "timeoutSeconds": 300,
    "resetOnError": true
  },

  "audio": {
    "enabled": true,
    "tagSound": "/tag.mp3",
    "correctSound": "/correct.mp3",
    "wrongSound": "/wrong.mp3",
    "unlockSound": "/unlock.mp3",
    "errorSound": "/error.mp3",
    "resetSound": "/reset.mp3"
  },

  "neopixel": {
    "enabled": true,
    "count": 8,
    "colors": {
      "idle": [0, 0, 255],
      "tagRead": [255, 255, 0],
      "correct": [0, 255, 0],
      "wrong": [255, 0, 0],
      "solved": [0, 255, 0]
    }
  },

  "output": {
    "lockPin": 10,
    "activeHigh": false
  }
}
```

### Obtendo UIDs das Tags

Para descobrir o UID de uma tag:
1. Compile e faca upload do sketch
2. Abra o monitor serial (115200 baud)
3. Aproxime a tag do leitor PN532
4. O UID sera exibido no formato hexadecimal

## Audio

### Arquivos Necessarios (raiz do LittleFS ou SD Card)

| Arquivo | Descricao |
|---------|-----------|
| `tag.mp3` | Som ao detectar qualquer tag |
| `correct.mp3` | Som quando tag correta |
| `wrong.mp3` | Som quando tag errada |
| `unlock.mp3` | Som quando puzzle resolvido |
| `error.mp3` | Som de erro |
| `reset.mp3` | Som de reset |

### Formato MP3
- Bitrate: 128 Kbps
- Sample rate: 44.1 kHz
- Stereo
- Sem tags ID3 (remover com: `ffmpeg -map_metadata -1`)

## Comandos Serial

| Comando | Descricao |
|---------|-----------|
| `start` / `run` | Inicia o puzzle |
| `reset` | Reseta o puzzle para estado inicial |
| `status` | Mostra status atual |
| `solve` / `force` | Forca puzzle como resolvido (debug) |
| `config` | Mostra configuracao carregada |
| `tags` | Lista tags programadas |
| `help` | Mostra ajuda |

## Estados do Puzzle

```
IDLE -> RUNNING -> SOLVED
              -> TAG_WRONG -> (resetOnError? ?) -> RUNNING/IDLE
```

- **IDLE**: Aguardando inicio
- **RUNNING**: Processando leitura de tags
- **TAG_CORRECT**: Feedback de tag correta
- **TAG_WRONG**: Feedback de tag errada
- **SOLVED**: Puzzle completo
- **TIMEOUT**: Tempo esgotado

## Compilacao

```bash
arduino-cli compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker UniversalUniqueRFID.ino
```

## Upload do Sketch

```bash
arduino-cli upload --fqbn rp2040:rp2040:adafruit_feather_prop_maker --port COM3 UniversalUniqueRFID.ino
```

## Dependencias

- PropMakerEscapeRoom
- ArduinoJson
- LittleFS
- Adafruit NeoPixel
- Adafruit PN532
- IOAbstraction
- TaskManager
- SwitchInput

## Licenca

MIT License - Mesma da biblioteca PropMakerEscapeRoom.
