# KeypadRfidLock - Fechadura com Senha + RFID

Combina teclado 4x3 (I2C/PCF8575) + leitor PN532 (I2C) para controle de acesso.

## Modos de Autenticacao

| Modo | Comportamento |
|------|---------------|
| `KEYPAD_ONLY` | Apenas senha numerica |
| `RFID_ONLY` | Apenas tag RFID |
| `KEYPAD_OR_RFID` | Senha OU tag (padrao) |
| `KEYPAD_AND_RFID` | Senha E tag (2 fatores) |

## Hardware

- Teclado 4x3 via PCF8575 em 0x20
- PN532 NFC Reader via I2C
- NeoPixel externo para feedback
- Rele no pino configurado (lockPin, padrao = 10)

## Configuracao

Edite `data/lock.json` e grave no LittleFS:

```json
{
  "mode": "KEYPAD_OR_RFID",
  "lockPin": 10,
  "unlockDurationMs": 3000,
  "codes": ["1234", "5678"],
  "rfidTags": ["04:2A:6B:01", "DE:AD:BE:EF"],
  "audio": {
    "keypress": "/sfx/beep.mp3",
    "unlock": "/sfx/magic.mp3",
    "denied": "/sfx/error.mp3",
    "reset": "/sfx/cancel.mp3"
  }
}
```

Se o arquivo nao existir, usa padrao (code=1234, mode=KEYPAD_OR_RFID).

## Audio (LittleFS ou SD)

Os arquivos de audio devem estar em /sfx/:
- /sfx/beep.mp3 - Tecla pressionada
- /sfx/magic.mp3 - Acesso liberado
- /sfx/error.mp3 - Acesso negado
- /sfx/cancel.mp3 - Reset/Limpar

## Compilacao

```bash
arduino-cli compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker KeypadRfidLock.ino
```

## Dependencias

- PropMakerEscapeRoom (inclui PN532, NeoPixel, Audio)
- IoAbstraction + TaskManager (keypad)
