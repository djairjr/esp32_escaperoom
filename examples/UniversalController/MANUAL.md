# Manual do Usuário - Universal Controller

## Controladora Universal para Escape Room

**Versão:** 1.0  
**Data:** Abril 2026  
**Compatibilidade:** Adafruit PropMaker Feather RP2040

---

## Índice

1. [Visão Geral](#visão-geral)
2. [Conteúdo da Embalagem](#conteúdo-da-embalagem)
3. [Instalação](#instalação)
4. [Configuração](#configuração)
5. [Modos de Operação](#modos-de-operação)
6. [Operação Diária](#operação-diária)
7. [Solução de Problemas](#solução-de-problemas)
8. [Manutenção](#manutenção)
9. [Especificações Técnicas](#especificações-técnicas)

---

## Visão Geral

A **Universal Controller** é uma controladora programável para puzzles de escape room. Ela permite criar puzzles sequenciais configuráveis via arquivo JSON, sem necessidade de programação.

### Recursos Principais

- Até **16 entradas digitais** (botões, sensores, etc.)
- **4 modos de puzzle** configuráveis
- **Áudio sincronizado** com eventos do puzzle
- **Efeitos visuais** com LEDs endereçáveis
- **Saída PWM gradativa** (voltímetro, servo, etc.)
- **Comunicação RS485** opcional para redes

### Aplicações Típicas

- Caixas com botões em sequência
- Painéis de controle
- Sistemas de alavancas
- Puzzles de conexão
- Sistemas multi-etapa

---

## Conteúdo da Embalagem

Verifique se todos os componentes estão presentes:

| Item | Quantidade | Descrição |
|------|------------|-----------|
| PropMaker Feather RP2040 | 1 | Placa controladora principal |
| PCF8575 | 1 | Expansor de I/O (16 entradas) |
| Módulo SD Card | 1 | Para armazenamento de áudio |
| Resistores 10kΩ | 16 | Pull-up para entradas (se necessário) |
| Cabo USB-C | 1 | Para programação e alimentação |
| Manual do Usuário | 1 | Este documento |

**Acessórios recomendados (não incluídos):**

- Fita LED NeoPixel (WS2812B)
- Botões ou sensores para entradas
- Módulo RS485 (para comunicação em rede)
- Fonte de alimentação 5V 3A

---

## Instalação

### Diagrama de Conexões

```
┌─────────────────────────────────────────────────────────────┐
│              Adafruit PropMaker Feather RP2040              │
│                                                             │
│   ┌──────────┐     I2C       ┌──────────┐                   │
│   │          │◄──────────────►│ PCF8575  │                   │
│   │          │  SDA: GPIO 24 │          │──► Entradas P0-P15│
│   │          │  SCL: GPIO 25 └──────────┘                   │
│   │          │                                               │
│   │          │◄──────────────┐                              │
│   │          │  SPI         │                              │
│   │          │  MISO: GPIO 8 │                              │
│   │          │  MOSI: GPIO 15│     ┌──────────┐            │
│   │          │  SCK: GPIO 14 │     │ SD Card  │            │
│   │          │  CS: GPIO 9   │────►│          │            │
│   │          │              │     └──────────┘            │
│   │          │               │                              │
│   │          │◄──────────────┐                              │
│   │          │  NeoPixel    │                              │
│   │          │  Data: GPIO 21│     ┌──────────┐            │
│   │          │──────────────►────►│ LEDs     │            │
│   │          │               │     └──────────┘            │
│   │          │               │                              │
│   │          │◄──────────────┐                              │
│   │          │  Relê/Trava   │     ┌──────────┐            │
│   │          │  Pino 10      │────►│ Trava    │            │
│   │          │               │     └──────────┘            │
│   └──────────┘              │                              │
│                              │                              │
└──────────────────────────────┴──────────────────────────────┘
```

### Passo a Passo da Instalação

#### 1. Conexão do PCF8575

| PCF8575 | PropMaker |
|---------|-----------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO 24 (SDA) |
| SCL | GPIO 25 (SCL) |
| A0, A1, A2 | Configurar endereço (veja tabela abaixo) |

**Configuração de Endereço I2C:**

| A2 | A1 | A0 | Endereço |
|----|----|----|----------|
| GND | GND | GND | 0x20 |
| GND | GND | VCC | 0x21 |
| GND | VCC | GND | 0x22 |
| GND | VCC | VCC | 0x23 |
| VCC | GND | GND | 0x24 |
| VCC | GND | VCC | 0x25 |
| VCC | VCC | GND | 0x26 |
| VCC | VCC | VCC | 0x27 |

#### 2. Conexão das Entradas (P0-P15)

Cada entrada aceita:
- Botão (contato normalmente aberto)
- Sensor de proximidade
- Chave magnética
- Qualquer contato seco

**Esquema de Ligação:**

```
    +3.3V
      │
     [R] 10kΩ (pull-up interno do PCF8575)
      │
      ├──► P0 (entrada do PCF8575)
      │
    [BOTÃO]
      │
     GND
```

> **Nota:** O PCF8575 possui pull-up interno. Resistores externos são opcionais para maior estabilidade.

#### 3. Conexão do NeoPixel (Opcional)

| NeoPixel | PropMaker |
|----------|-----------|
| VCC | 5V (via pino de energia externa) |
| GND | GND |
| Data | GPIO 21 |

> **Atenção:** Ligar pino 23 (POWER_EXT) em HIGH para energizar LEDs externos.

#### 4. Conexão da Saída de Trava (Opcional)

| Dispositivo | PropMaker |
|-------------|-----------|
| Relê/Trava | Pino 10 |
| GND | GND |

---

## Configuração

### Preparação do Cartão SD

1. Formate o cartão SD em FAT32
2. Crie a pasta `/sounds/` na raiz
3. Copie os arquivos MP3 necessários (veja tabela abaixo)

**Arquivos de Áudio Obrigatórios:**

| Arquivo | Descrição | Formato |
|---------|-----------|---------|
| `input1.mp3` | Som para acerto do 1º input | MP3, 128kbps, 44.1kHz |
| `input2.mp3` | Som para acerto do 2º input | MP3, 128kbps, 44.1kHz |
| ... | ... | ... |
| `unlock.mp3` | Som de puzzle resolvido | MP3, 128kbps, 44.1kHz |
| `error.mp3` | Som de erro | MP3, 128kbps, 44.1kHz |
| `reset.mp3` | Som de reset | MP3, 128kbps, 44.1kHz |

> **Importante:** Remova tags ID3 dos arquivos MP3 usando o comando:  
> `ffmpeg -i input.mp3 -map_metadata -1 output.mp3`

### Configuração do Arquivo JSON

O arquivo `config.json` deve ser gravado no **LittleFS** da placa.

#### Exemplo de Configuração Básica

```json
{
  "controller": {
    "name": "Puzzle da Caixa",
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
    "resetOnError": true
  },

  "audio": {
    "enabled": true,
    "inputSounds": [
      "/input1.mp3",
      "/input2.mp3",
      "/input3.mp3",
      "/input4.mp3",
      "/input5.mp3"
    ],
    "unlockSound": "/unlock.mp3",
    "errorSound": "/error.mp3",
    "resetSound": "/reset.mp3"
  },

  "neopixel": {
    "enabled": true,
    "count": 8,
    "colors": {
      "idle": [0, 0, 255],
      "solved": [0, 255, 0],
      "error": [255, 0, 0],
      "progress": [0, 255, 100]
    }
  },

  "output": {
    "lockPin": 10,
    "activeHigh": false
  }
}
```

---

## Modos de Operação

### 1. CUSTOM_SEQUENCE (Sequência Customizada)

As entradas devem ser acionadas na ordem exata especificada no JSON.

**Exemplo de Configuração:**
```json
"logic": {
  "mode": "CUSTOM_SEQUENCE",
  "sequence": [1, 3, 5, 2, 4],
  "timeoutSeconds": 300,
  "resetOnError": true
}
```

**Comportamento:**
- Input correto: avança para próximo
- Input errado: executa som de erro e reseta (se `resetOnError: true`)

**Aplicação:** Puzzles onde a ordem importa, como códigos secretos.

---

### 2. LINEAR_SEQUENCE (Sequência Linear)

Entradas devem ser acionadas em ordem numérica crescente.

**Exemplo de Configuração:**
```json
"logic": {
  "mode": "LINEAR_SEQUENCE",
  "timeoutSeconds": 600,
  "resetOnError": true
}
```

**Comportamento:**
- Ordem obrigatória: 1 → 2 → 3 → 4 → 5...
- Não requer array `sequence`

**Aplicação:** Puzzles de progressão simples.

---

### 3. ALL_CONNECTED (Todas Conectadas)

Todas as entradas devem estar ativas simultaneamente.

**Exemplo de Configuração:**
```json
"logic": {
  "mode": "ALL_CONNECTED",
  "timeoutSeconds": 0,
  "resetOnError": false
}
```

**Comportamento:**
- Ordem de ativação não importa
- Barra de progresso aumenta conforme entradas são ativadas
- Puzzle resolvido quando todas ativas

**Aplicação:** Puzzles de conexão onde múltiplos itens devem ser ativados.

---

### 4. RANDOM (Sequência Aleatória)

Sequência gerada aleatoriamente no boot da placa.

**Exemplo de Configuração:**
```json
"logic": {
  "mode": "RANDOM",
  "timeoutSeconds": 300,
  "resetOnError": true
}
```

**Comportamento:**
- Gera nova ordem a cada boot
- Sem repetições na sequência

**Aplicação:** Puzzles onde a ordem deve ser descoberta.

---

## Operação Diária

### Inicialização

1. Ligue a alimentação da placa (USB-C ou fonte externa)
2. Aguarde o boot (LED interno pisca azul)
3. Puzzle entra em estado **IDLE** (LEDs azuis)

### Iniciar o Jogo

**Método 1 - Botão físico:**
- Pressione o botão conectado ao pino 19 da placa
- LED muda para cor de progresso

**Método 2 - Comando Serial:**
- Conecte via USB ao computador
- Abra terminal serial (115200 baud)
- Digite: `start`

### Durante o Jogo

| Indicador | Significado |
|-----------|-------------|
| LEDs azuis | Aguardando início |
| LEDs amarelos | Input correto detectado |
| Barra crescendo | Progresso do puzzle |
| Flash verde | Input correto |
| Flash vermelho | Input errado |
| LEDs verdes piscando | Puzzle resolvido |
| Som "inputX.mp3" | Feedback de input correto |
| Som "error.mp3" | Feedback de input errado |
| Som "unlock.mp3" | Puzzle completado |

### Reiniciar o Puzzle

**Via Serial:**
```
reset
```

**Via Hardware:**
- Pressione o botão de reset da placa

### Verificar Status

**Via Serial:**
```
status
```

Saída esperada:
```
----- STATUS -----
Estado: Running
Progresso: 60% (3/5)
Tempo restante: 245s
------------------
```

---

## Solução de Problemas

### Problemas Comuns

#### Não inicializa

| Sintoma | Causa Provável | Solução |
|---------|----------------|---------|
| LED não acende | Sem alimentação | Verifique cabo USB/fonte |
| LED vermelho contínuo | Erro de hardware | Verifique conexões I2C |
| Boot loop | Arquivo config.json corrompido | Regrave configuração |

#### Sem áudio

| Sintoma | Causa Provável | Solução |
|---------|----------------|---------|
| Sem som algum | SD card não detectado | Verifique inserção do cartão |
| Ruído em vez de música | MP3 com tags ID3 | Remova tags com ffmpeg |
| Som distorcido | Bitrate incorreto | Use 128kbps, 44.1kHz |

#### Entradas não respondem

| Sintoma | Causa Provável | Solução |
|---------|----------------|---------|
| Nenhuma entrada funciona | Endereço I2C errado | Verifique jumpers A0-A2 |
| Entrada específica não funciona | Fio solto | Verifique conexão |
| Entrada aciona sozinha | Interferência elétrica | Adicione capacitor 100nF |

#### LEDs não acendem

| Sintoma | Causa Provável | Solução |
|---------|----------------|---------|
| LEDs escuros | Sem energia externa | Ative pino 23 (POWER_EXT) |
| Cor errada | Ordem RGB diferente | Ajuste configuração |
| Apenas alguns acendem | Quebra na fita | Verifique soldagens |

### Diagnóstico via Serial

Conecte via USB e abra o monitor serial (115200 baud).

**Durante o boot, você verá:**
```
========================================
   UNIVERSAL CONTROLLER v1.0
   Escape Room Configurable Puzzle
========================================

[Setup] Nome: Puzzle da Caixa | ID: 1
[Setup] Entradas: 5 | PCF Address: 0x20
[Setup] Modo: CUSTOM_SEQUENCE | Timeout: 300s
[Setup] PCF8575 adicionado no endereço 0x20
[Setup] Hardware configurado. Aguardando início...
```

**Se houver erro:**
```
[Setup] ERRO: Falha ao inicializar LittleFS
[Setup] Usando configuração padrão
```

### Reset de Fábrica

Para restaurar configurações padrão:

1. Desligue a alimentação
2. Conecte o pino 19 ao GND
3. Ligue a alimentação
4. Mantenha por 5 segundos
5. Desconecte o pino 19
6. Configuração padrão será carregada

---

## Manutenção

### Limpeza

- Use pano seco ou levemente úmido
- Não use produtos químicos agressivos
- Desligue antes de limpar

### Atualização de Firmware

1. Baixe o arquivo .uf2 mais recente
2. Conecte a placa via USB
3. Pressione BOOTSEL + RESET
4. Copie o arquivo .uf2 para a unidade RPI-RP2

### Backup da Configuração

1. Conecte via USB
2. Use ferramenta LittleFS para extrair `/config.json`
3. Guarde cópia em local seguro

---

## Especificações Técnicas

### PropMaker Feather RP2040

| Parâmetro | Especificação |
|-----------|---------------|
| Microcontrolador | Raspberry Pi RP2040 |
| Clock | 133 MHz dual-core |
| Memória RAM | 264 KB |
| Memória Flash | 8 MB (QSPI) |
| Tensão de operação | 3.3V |
| Alimentação | USB-C ou 5V externo |
| Consumo | ~100mA (idle), ~500mA (com LEDs) |

### PCF8575

| Parâmetro | Especificação |
|-----------|---------------|
| Canais I/O | 16 |
| Tensão | 2.5V - 5.5V |
| Corrente por pino | 25mA (sink) |
| Endereço I2C | 0x20 - 0x27 |
| Pull-up interno | 100μA |

### NeoPixel

| Parâmetro | Especificação |
|-----------|---------------|
| Tipo | WS2812B |
| Tensão | 5V |
| Corrente máxima | 60mA por LED (branco total) |
| Máximo recomendado | 16 LEDs sem amplificador |

### Entradas Digitais

| Parâmetro | Especificação |
|-----------|---------------|
| Tensão máxima | 5.5V |
| Impedância | ~100kΩ (pull-up) |
| Debounce | 50ms (configurável) |
| Resposta | <10ms |

---

## Garantia

A controladora possui garantia de **12 meses** contra defeitos de fabricação.

**Não coberto:**
- Danos por mau uso
- Sobretensão ou curto-circuito
- Modificações não autorizadas
- Exposição a umidade ou temperatura extrema

---

## Suporte Técnico

Para suporte técnico, entre em contato com o fabricante.

**Antes de contatar:**
1. Verifique as conexões
2. Consulte a seção "Solução de Problemas"
3. Anote mensagens de erro do Serial

---

## Histórico de Versões

| Versão | Data | Alterações |
|--------|------|------------|
| 1.0 | Abril 2026 | Versão inicial |

---

**Documentação complementar:**
- README.md (desenvolvedor)
- ALGORITMO.MD (especificação técnica)
- Exemplos de configuração na pasta `/data/`

---

*Documento gerado em Abril de 2026.*  
*Universal Controller - Sistema de Puzzle para Escape Room*