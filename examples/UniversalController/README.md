# UniversalController

Controladora Universal Configurável para Escape Room usando a biblioteca PropMaker EscapeRoom.

## Visão Geral

A UniversalController é um puzzle configurável via arquivo JSON que suporta múltiplos modos de operação, feedback audiovisual integrado e comunicação RS485 opcional.

## Modos de Puzzle

### 1. CUSTOM_SEQUENCE
Sequência customizada definida no arquivo JSON.
- **Exemplo**: `sequence: [1, 5, 3, 8, 2]`
- As entradas devem ser acionadas na ordem expecificada

### 2. LINEAR_SEQUENCE
Ordem linear das entradas.
- **Comportamento**: 0 → 1 → 2 → 3 → ...
- Útil para puzzles de progressão simples

### 3. ALL_CONNECTED
Todas as entradas devem ser ativadas, ordem não importa.
- **Comportamento**: AND lógico das entradas
- Progresso aumenta conforme cada entrada é ativada

### 4. RANDOM
Sequência aleatória gerada automaticamente no boot.
- Cada vez que inicia, gera uma nova ordem
- Não há repetições na sequência

## Hardware

### Requisitos
- Adafruit PropMaker Feather RP2040
- PCF8575 (expansor I2C para entradas)
- NeoPixel (LEDs endereçáveis)
- SD Card (para sons MP3)
- PCA9685 (opcional, para saída PWM)

### Conexões
```
PCF8575:
- SDA -> GPIO 24 (I2C)
- SCL -> GPIO 25 (I2C)
- VCC -> 3.3V
- GND -> GND

Entradas digitais (PCF8575):
- P0-P15: Conectar botões (INPUT_PULLUP, ativo em LOW)

NeoPixel:
- Data -> GPIO 21 (EXTERNAL_NEOPIXEL)
- VCC -> 5V (via POWER_EXT)
- GND -> GND

Saída Relê/Trava:
- Pino 10 (RELAY)
```

## Configuração

### Arquivo config.json

O arquivo `data/config.json` deve ser gravado no LittleFS da placa.

#### Estrutura

```json
{
  "controller": {
    "name": "Nome do Puzzle",
    "id": 1,
    "enabled": true
  },

  "inputs": {
    "count": 5,
    "pcfAddress": "0x20",
    "pinMapping": [0, 1, 2, 3, 4],
    "debounceMs": 50
  },

  "logic": {
    "mode": "CUSTOM_SEQUENCE",
    "sequence": [1, 3, 5, 2, 4],
    "timeoutSeconds": 300,
    "resetOnError": true,
    "allowRepeat": false
  },

  "pwm": {
    "enabled": true,
    "channel": 0,
    "minValue": 0,
    "maxValue": 100,
    "steps": 5
  },

  "audio": {
    "enabled": true,
    "inputSounds": ["/input1.mp3", "/input2.mp3"],
    "unlockSound": "/unlock.mp3",
    "errorSound": "/error.mp3",
    "resetSound": "/reset.mp3"
  },

  "neopixel": {
    "enabled": true,
    "count": 16,
    "colors": {
      "idle": [0, 0, 255],
      "active": [255, 255, 0],
      "solved": [0, 255, 0],
      "error": [255, 0, 0],
      "progress": [0, 255, 100]
    }
  },

  "rs485": {
    "enabled": false,
    "address": 1
  },

  "output": {
    "lockPin": 10,
    "activeHigh": false
  }
}
```

### Exemplos de Configuração

Vários exemplos estão disponíveis na pasta `data/`:

- **config.json**: Sequência customizada (padrão)
- **config_linear.json**: Sequência linear
- **config_all_connected.json**: Todas conectadas
- **config_random.json**: Sequência aleatória

## Áudio

### Arquivos Necessários (pasta raiz do SD Card)

| Arquivo | Descrição |
|---------|-----------|
| `input1.mp3` - `inputN.mp3` | Sons para cada input correto |
| `unlock.mp3` | Som quando puzzle resolvido |
| `error.mp3` | Som quando input errado |
| `reset.mp3` | Som quando puzzle resetado |

### Formato MP3
- Bitrate: 128 Kbps
- Sample rate: 44.1 kHz
- Stereo
- Sem tags ID3 (remover com: `ffmpeg -map_metadata -1`)

## Comandos Serial

Durante a execução, comandos podem ser enviados via Serial:

| Comando | Descrição |
|---------|-----------|
| `start` / `run` | Inicia o puzzle |
| `reset` | Reseta o puzzle para estado inicial |
| `status` | Mostra status atual |
| `solve` / `force` | Força puzzle como resolvido (debug) |
| `config` | Mostra configuração carregada |
| `help` | Mostra ajuda |

## Estados do Puzzle

```
IDLE → RUNNING → SOLVED
       ↓
      ERROR → (resetOnError ?) → RUNNING/IDLE
```

- **IDLE**: Aguardando início
- **RUNNING**: Processando inputs
- **INPUT_CORRECT**: Feedback de input certo
- **INPUT_WRONG**: Feedback de input errado
- **SOLVED**: Puzzle completo
- **ERROR**: Erro na sequência
- **TIMEOUT**: Tempo esgotado

## Feedback Visual (NeoPixel)

### Modos

1. **PROGRESS_BAR** (modo 0)
   - LEDs acendem conforme progresso
   - Cor de fundo: `idle`
   - Cor de progresso: `progress`

2. **CHASE** (modo 1)
   - Efeito de perseguição
   - Útil para modo RANDOM

3. **INDIVIDUAL** (modo 2)
   - Cada LED representa uma entrada

### Cores

- **idle**: Cor quando não há atividade
- **active**: Cor quando input ativado
- **solved**: Cor quando puzzle resolvido
- **error**: Cor quando erro na sequência
- **progress**: Cor da barra de progresso

## Saída PWM

A saída PWM pode ser usada para:
- Controlar voltagem de um voltímetro
- Mover um servo conforme progresso
- Controlar intensidade de LEDs

O valor do PWM é calculado automaticamente:
```
PWM = minValue + ((maxValue - minValue) * progresso) / 100
```

## RFID Alternativo com CH552G

Alternativa de baixo custo para leitores RFID. Cada CH552G executa o driver MFRC522 de forma independente e se comporta como um sensor digital convencional (LOW = tag valida, HIGH = negado).

### Arquitetura

Diferente do MultiRFID classico (que usa um unico controlador + expansor I2C + multiplos MFRC522 no mesmo SPI), esta abordagem usa **um CH552G por leitor**:

```
PropMaker RP2040 (Universal Controller)
  │
  ├── GPIO_N  ←── CH552G #1 (P1.1)  ── MFRC522 #1
  ├── GPIO_N  ←── CH552G #2 (P1.1)  ── MFRC522 #2
  ├── GPIO_N  ←── CH552G #3 (P1.1)  ── MFRC522 #3
  └── ...     ←── CH552G #N (P1.1)  ── MFRC522 #N
```

Cada CH552G:
- Armazena ate 7 tags na EEPROM interna (DataFlash)
- Tem seu proprio botao SW1 para cadastrar/limpar tags
- Envia LOW no P1.1 quando uma tag cadastrada e detectada
- Envia HIGH caso contrario
- Custa uma fracao de um leitor RFID dedicado

### Conexao

```
CH552G (P1.1)  ──  PropMaker GPIO (qualquer pino livre)
CH552G (GND)   ──  PropMaker GND
```

Nao precisa de I2C, SPI, ou nivelador de tensao. O CH552G opera em 3.3V.

### Leitura no Sketch

```cpp
// Cada CH552G entra como um pino digital comum
#define RFID_TAG1_PIN  12   // Pino conectado ao CH552G #1
#define RFID_TAG2_PIN  13  // Pino conectado ao CH552G #2

void setup() {
  pinMode(RFID_TAG1_PIN, INPUT_PULLUP);
  pinMode(RFID_TAG2_PIN, INPUT_PULLUP);
}

void loop() {
  if (digitalRead(RFID_TAG1_PIN) == LOW) {
    // Puzzle 1 resolvido - tag conhecida detectada
  }
  if (digitalRead(RFID_TAG2_PIN) == LOW) {
    // Puzzle 2 resolvido
  }
}
```

### Configuracao no config.json

Cada CH552G ocupa 1 entrada digital do PCF8575 (assim como um botao ou chave):

```json
{
  "inputs": {
    "count": 4,
    "pcfAddress": "0x20",
    "pinMapping": [0, 1, 2, 3],
    "debounceMs": 50,
    "labels": ["Botao", "Chave", "RFID_Porta1", "RFID_Porta2"]
  },

  "logic": {
    "mode": "ALL_CONNECTED",
    "sequence": [0, 1, 2, 3]
  }
}
```

O pino do CH552G se conecta ao PCF8575 exatamente como um botao:
```
CH552G (P1.1)  ──  PCF8575 P0  ──  GND via resistor 10k (pull-up externo)
```

O PCF8575 ja tem pull-up interno, entao:
- LOW = tag valida (CH552G puxou para GND)
- HIGH = sem tag ou desconhecida (pull-up mantem HIGH)

### Vantagens

| Caracteristica | MultiRFID classico | CH552G alternativo |
|---------------|-------------------|-------------------|
| Custo por leitor | Alto (expansor I2C + leitor) | Baixo (CH552G + MFRC522) |
| Complexidade | Protocolo I2C + biblioteca | Sensor digital simples |
| Leitores por placa | Limitado a 8 (PCF) | Ilimitado (1 pino cada) |
| Autonomia | Depende da central | Independente (EEPROM propria) |
| Cadastro de tags | Via config.json | Via botao SW1 local |

## RS485 (Opcional)

Para integração em rede de puzzles:

1. Habilitar em `config.json`:
   ```json
   "rs485": {
     "enabled": true,
     "address": 1,
     "baudRate": 115200
   }
   ```

2. Conectar módulo RS485:
   - TX -> GPIO 0
   - RX -> GPIO 1

## Compilação

Usando Arduino CLI:

```bash
arduino-cli compile --fqbn rp2040:rp2040:adafruit_feather_prop_maker UniversalController.ino
```

## Upload do Sketch

```bash
arduino-cli upload --fqbn rp2040:rp2040:adafruit_feather_prop_maker --port COM3 UniversalController.ino
```

## Upload da Configuração (LittleFS)

Usando plugin LittleFS para Arduino:

```bash
# Empacota pasta data/
# Faz upload para a placa
```

Ou via código no setup (primeira vez):

```cpp
// Copiar config.json do SD para LittleFS
File src = SD.open("/config.json");
File dst = LittleFS.open("/config.json", "w");
// ... copiar conteúdo
```

## Dependências

- PropMakerEscapeRoom
- ArduinoJson
- LittleFS
- Adafruit NeoPixel
- IOAbstraction
- TaskManager
- SwitchInput

## Licença

MIT License - Mesma da biblioteca PropMakerEscapeRoom.