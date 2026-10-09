# BombTimer STAMPLC — Manual do Usuário

> Puzzle de escape room — timer de contagem regressiva com fios, códigos e RFID.
> Baseado no M5Stack StamPLC (ESP32-S3).

---

## 1. Visão Geral

O BombTimer é um puzzle onde os jogadores precisam **desarmar uma bomba virtual** antes que o tempo acabe. O desarme pode ser feito de 3 formas diferentes, configuráveis pelo game master:

- **CODE** — digitar um código numérico de 4 dígitos no keypad
- **WIRE** — cortar o fio correto (DEFUSE) entre 4 fios coloridos
- **ORDER** — cortar os 4 fios em uma sequência específica

O sistema possui conectividade WiFi e se integra ao **Home Assistant** via MQTT para que o game master possa monitorar e controlar o puzzle remotamente.

---

## 2. Hardware

### Frontal
```
┌──────────────────────────────────────┐
│                              [W][M][R][T]│  ← WiFi / MQTT / RFID / RTC
│                                      │
│          ┌─────────────┐             │
│          │   03:00     │             │  ← Timer (minutos:segundos)
│          └─────────────┘             │
│              180s                    │  ← Total em segundos
│                                      │
│  ●Y ●R ●B ●G            IDLE        │  ← Fios + Status
└──────────────────────────────────────┘
```

### Conexões

| Porta | Conector | Dispositivo |
|-------|----------|-------------|
| PORT.A (RED) | Grove 4 pinos | Keypad 4×3 (via PCF8574 I2C) + RFID 2 Unit (MFRC522 I2C) |
| PORT.B (BLACK) | Grove 4 pinos | (livre) |
| PORT.C (BLUE) | Grove 4 pinos | RF433 Receiver (SYN531R) — DATA no fio branco (G4) |
| USB-C | Traseiro | Alimentação 5V + Upload |

### Relés (bornes traseiros)

| Relé | Função | Bornes |
|------|--------|--------|
| RLY1 | DETONATE 1 | NA/COM/NF |
| RLY2 | DETONATE 2 | NA/COM/NF |
| RLY3 | DEFUSE 1 | NA/COM/NF |
| RLY4 | DEFUSE 2 | NA/COM/NF |

### Opto-entradas (bornes traseiros)

| Entrada | Função | Sinal |
|---------|--------|-------|
| WIRE1 (P0_5) | Fio Amarelo | HIGH=conectado, LOW=cortado |
| WIRE2 (P0_6) | Fio Vermelho | HIGH=conectado, LOW=cortado |
| WIRE3 (P0_7) | Fio Azul | HIGH=conectado, LOW=cortado |
| WIRE4 (P1_4) | Fio Verde | HIGH=conectado, LOW=cortado |
| DET (P0_4) | Detonação externa | LOW→HIGH = detona |

---

## 3. Botões e Controles

| Botão | Símbolo | Função |
|-------|---------|--------|
| BtnA (esquerdo) | ◀ | Diminuir valor / Navegar para trás no menu |
| BtnB (centro) | ● | Entrar no menu / Confirmar edição |
| BtnC (direito) | ▶ | Aumentar valor / Navegar para frente |
| BOOT (traseiro) | ▶▶ | Iniciar contagem / Resetar / Detonar |

### Keypad 4×3

```
┌───┬───┬───┐
│ 1 │ 2 │ 3 │
├───┼───┼───┤
│ 4 │ 5 │ 6 │
├───┼───┼───┤
│ 7 │ 8 │ 9 │
├───┼───┼───┤
│ * │ 0 │ # │
└───┴───┴───┘
```

- Usado para digitar código de desarme (modo CODE)
- Usado para configurar código e ordem no menu
- `*` ou `#` cancela a entrada durante a digitação

---

## 4. Estados do Puzzle

### 4.1 IDLE (Repouso)

Estado inicial. O display mostra o timer ajustável e "IDLE".

**Controles em IDLE:**
- **BtnA/BtnC (curto):** Ajusta o tempo em ±1 segundo
- **BtnA/BtnC (longo >600ms):** Ajusta o tempo em ±5 segundos (acelera a cada 150ms)
- **BtnB (centro):** Entra no menu de configuração
- **BOOT:** Inicia a contagem regressiva → RUNNING
- **RF433 START:** Inicia a contagem regressiva

### 4.2 RUNNING (Contagem Regressiva)

A contagem está rodando. O timer decrementa a cada segundo.

**O que acontece:**
- **Tick sonoro:** Beep a cada segundo (frequência aumenta nos últimos 10s)
- **RGB LED:** Vermelho pulsante (500ms, 250ms nos últimos 10s)
- **Fios:** Cortar um fio pode detonar, desarmar, acelerar ou pausar (depende da configuração)
- **Keypad:** Se modo CODE, pode digitar o código de desarme
- **RFID:** Se habilitado, aproximar uma tag pode desarmar ou detonar

**Controles em RUNNING:**
- **BOOT:** Detona imediatamente
- **Keypad (modo CODE):** Digitar 4 dígitos do código
- **RF433 DETONATE:** Detona remotamente
- **RF433 DEFUSE:** Desarma remotamente
- **RF433 PAUSE:** Pausa por 2s
- **RF433 SPEED:** Acelera (toggle)

### 4.3 DETONATED (Bomba explodiu!)

O tempo acabou ou um fio DET foi cortado.

**O que aparece:**
- Tela: "DETONATED! BOOM!" em vermelho
- RGB LED: Vermelho sólido
- Relés DETONATE ativados por [TDET] segundos

**Controles:**
- **BOOT:** Volta ao IDLE

### 4.4 DEFUSED (Bomba desarmada!)

O código correto foi digitado, o fio DEFUSE foi cortado, ou a sequência ORDER foi completada.

**O que aparece:**
- Tela: "DEFUSED! SUCCESS!" em verde
- RGB LED: Verde sólido
- Relés DEFUSE ativados por [TDEF] segundos

**Controles:**
- **BOOT:** Volta ao IDLE

### 4.5 ALARM (Alarme)

O alarme programado no RTC disparou.

**O que aparece:**
- Tela: "ALARM! START to stop" em magenta
- RGB LED: Laranja intermitente
- Beep 3500Hz a cada 500ms

**Controles:**
- **BOOT:** Volta ao IDLE

### 4.6 MENU (Configuração)

18 itens de configuração navegáveis.

**Navegação:**
- **BtnA/BtnC:** Navega entre os itens
- **BtnB (centro):** Entra no modo de edição do item atual
- **BtnB (centro) durante edição:** Confirma e salva
- **BOOT:** Sai do menu (volta ao IDLE)
- **Timeout 15s:** Sai automaticamente

---

## 5. Menu de Configuração (18 itens)

| # | Item | Opções | Descrição |
|---|------|--------|-----------|
| 0 | COUNTDOWN | 5-5999s | Tempo da contagem regressiva |
| 1 | MODE | CODE / WIRE / ORDER | Modo de desarme |
| 2 | CODE | 0000-9999 | Código de desarme (modo CODE) |
| 3 | ORDER | Sequência 1-4 | Ordem dos fios (modo ORDER) |
| 4 | DET WIRE | RAND / 1-4 / NONE | Fio que detona ao cortar |
| 5 | SPD WIRE | RAND / 1-4 / NONE | Fio que acelera o timer |
| 6 | PAUSE WIRE | RAND / 1-4 / NONE | Fio que pausa 2s |
| 7 | BRIGHT | 0-15 | Brilho do display |
| 8 | TICK | ON / OFF | Som do tick |
| 9 | RFID | ON / OFF | Habilita leitura RFID durante jogo |
| 10 | RF DEFU | (aprender) | Aproxime uma tag para desarme |
| 11 | RF DETO | (aprender) | Aproxime uma tag para detonação |
| 12 | TDET | 1-10s | Tempo que relés DETONATE ficam ativos |
| 13 | TDEF | 1-10s | Tempo que relés DEFUSE ficam ativos |
| 14 | ALARM H | 0-23 | Hora do alarme |
| 15 | ALARM M | 0-59 (passo 5) | Minuto do alarme |
| 16 | CLR DEFU | (confirmar) | Limpa todas as tags de desarme |
| 17 | CLR DETO | (confirmar) | Limpa todas as tags de detonação |

---

## 6. Configuração Inicial (WiFi e MQTT)

### Primeira inicialização

1. Conecte o StamPLC via USB-C
2. O sistema tenta conectar ao WiFi salvo
3. Se não houver WiFi configurado, entra em **Modo AP**
4. Conecte-se ao WiFi `BombTimer-STAMPLC` (sem senha)
5. Abra http://192.168.4.1 no navegador
6. Preencha:
   - **WiFi SSID e Senha** (da sua rede)
   - **MQTT Broker** (endereço do Home Assistant)
   - **Porta** (padrão 1883)
   - **User/Pass** (se necessário)
   - **Nome do dispositivo** (opcional)
7. Clique em "Salvar e Reiniciar"

O StamPLC reinicia, conecta ao WiFi e ao MQTT, e as entidades aparecem no Home Assistant automaticamente.

### Para reconfigurar
- Se o WiFi falhar, o sistema volta ao Modo AP automaticamente
- Para forçar o Modo AP, desligue o roteador e religue o StamPLC

---

## 7. Home Assistant

### Entidades disponíveis

Após conectar ao MQTT, as seguintes entidades aparecem no Home Assistant:

**Sensores (monitoramento):**
- `sensor.bombtimer_state` — Estado atual (idle/running/defused/detonated)
- `sensor.bombtimer_time` — Tempo restante em segundos
- `sensor.bombtimer_wires_cut` — Quantos fios já foram cortados
- `sensor.bombtimer_code_attempts` — Tentativas de código (modo CODE)
- `sensor.bombtimer_mode` — Modo de desarme atual
- `sensor.bombtimer_i2c_status` — Dispositivos I2C detectados no boot
- `sensor.bombtimer_wire1` a `wire4` — Estado de cada fio (0=conectado, 1=cortado)

**Controles:**
- `button.bombtimer_start` — Iniciar a contagem
- `button.bombtimer_reset` — Resetar para IDLE
- `number.bombtimer_countdown` — Ajustar tempo (5-5999s)
- `number.bombtimer_defuse_code` — Alterar código de desarme
- `number.bombtimer_brightness` — Ajustar brilho (0-15)
- `select.bombtimer_mode` — Selecionar modo (CODE/WIRE/ORDER)
- `select.bombtimer_det_wire` — Configurar fio DET (RAND/1-4/NONE)
- `select.bombtimer_defuse_wire` — Configurar fio DEFUSE
- `select.bombtimer_speed_wire` — Configurar fio SPEED
- `select.bombtimer_pause_wire` — Configurar fio PAUSE
- `switch.bombtimer_tick_sound` — Ligar/desligar som do tick

### Automações sugeridas

**Exemplo: notificar quando detonar**
```yaml
automation:
  - alias: "Bomba detonou!"
    trigger:
      - platform: state
        entity_id: sensor.bombtimer_state
        to: "detonated"
    action:
      - service: notify.mobile_app_seu_celular
        data:
          message: "💥 A BOMBA EXPLODIU!"
```

**Exemplo: iniciar jogo automático**
```yaml
automation:
  - alias: "Iniciar bomba às 19:00"
    trigger:
      - platform: time
        at: "19:00:00"
    action:
      - service: button.press
        target:
          entity_id: button.bombtimer_start
```

---

## 8. Cenários de Jogo

### 8.1 Modo CODE (padrão)

O jogador precisa digitar o código de 4 dígitos no keypad.

**Configuração:**
- MODE = CODE
- CODE = 1234 (ou outro)
- DET WIRE = RAND (um fio aleatório detona)
- SPEED WIRE = NONE
- PAUSE WIRE = NONE

**Jogabilidade:**
- Cortar o fio DET → detona instantaneamente
- Digitar código errado → 3 tentativas, na 4ª detona
- Digitar código correto → desarma

### 8.2 Modo WIRE

O jogador precisa cortar o fio correto (e apenas ele).

**Configuração:**
- MODE = WIRE
- DEFUSE WIRE = 2 (por exemplo, fio vermelho)
- DET WIRE = RAND
- SPEED WIRE = RAND
- PAUSE WIRE = RAND

**Jogabilidade:**
- Cortar o fio DEFUSE (vermelho) → desarma
- Cortar qualquer outro fio → pode detonar (DET), acelerar ou pausar

### 8.3 Modo ORDER

O jogador precisa cortar os 4 fios em uma sequência específica.

**Configuração:**
- MODE = ORDER
- ORDER = 3142 (por exemplo)
- DET WIRE = NONE (ou RAND para dificuldade extra)

**Jogabilidade:**
- Ordem correta (amarelo→azul→verde→vermelho) → desarma ao cortar o 4º
- Qualquer erro na ordem → detona imediatamente

### 8.4 Com RFID

Aproximar uma tag RFID pode desarmar ou detonar.

**Configuração:**
- RFID = ON
- RF DEFU: aproxime uma tag para ensinar (até 4 tags)
- RF DETO: aproxime uma tag para ensinar

**Jogabilidade:**
- Aproximar tag de desarme → desarma
- Aproximar tag de detonação → detona

### 8.5 Com RF433

Controle remoto via RF 433MHz.

**Comandos disponíveis:**
- PREV / NEXT / MENU / START — mesmos botões físicos
- DETONATE — detona remotamente
- DEFUSE — desarma remotamente
- SPEED — acelera/desacelera (toggle)
- PAUSE — pausa por 2s

---

## 9. Manutenção

### Atualização de firmware
1. Conecte via USB-C ao computador
2. Abra o Arduino IDE ou arduino-cli
3. Selecione placa: M5Stack StamPLC (ESP32-S3)
4. Faça upload do novo firmware

### Verificação de dispositivos I2C
No boot, o sistema varre ambos os barramentos I2C e publica o resultado no sensor `i2c_status` do Home Assistant. Dispositivos esperados:

**Barramento interno:** PI4IOE5V6408(0x43), AW9523B(0x59), RX8130CE(0x32), LM75B(0x48), INA226(0x40)

**PORT.A:** PCF8574(0x20-0x27), MFRC522(0x28)

### Solução de problemas

| Problema | Causa provável | Solução |
|----------|---------------|---------|
| Não conecta ao WiFi | Senha incorreta | Entre no Modo AP e reconfigue |
| Keypad não funciona | PCF8574 não detectado | Verifique cabos do Grove PORT.A |
| RFID não funciona | Chip não presente ou I2C 0x28 ocupado | Verifique endereço I2C |
| Tick não toca | TICK desligado no menu | Ative no menu (item 8) |
| Relés não acionam | TDET/TDEF = 0 ou falha AW9523B | Verifique configuração e I2C |
| Display apagado | Brilho = 0 | Ajuste BRIGHT no menu (item 7) |
| Botões não respondem | PI4IOE5V6408 não detectado | Verifique barramento I2C interno |

---

## 10. Especificações Técnicas

| Item | Especificação |
|------|---------------|
| Microcontrolador | ESP32-S3 (M5Stack StamPLC) |
| Display | TFT ST7789v2 240×135, rotação 1 |
| Timer | Contagem regressiva 5-5999s |
| Precisão | ±1s (baseado em millis()) |
| Fios | 4 opto-isolados (YEL/RED/BLU/GRN) |
| Relés | 4 canais (2 DETONATE + 2 DEFUSE) |
| Keypad | 4×3 matriz via PCF8574 I2C |
| RFID | MFRC522/WS1850S I2C (RFID 2 Unit) |
| RF433 | SYN531R (rc-switch) |
| RTC | RX8130CE (alarme programável) |
| Conectividade | WiFi 2.4GHz + MQTT (ArduinoHA) |
| Alimentação | USB-C 5V |
| Consumo | ~200mA (com WiFi ativo) |
| Persistência | NVS (Preferences) + SPIFFS (WiFi config) |

---

## 11. Licença e Créditos

Projeto desenvolvido para uso em escape rooms. Baseado no GameTimerPro original (AVR/RP2040) e BombTimer XIAO S3.

**Bibliotecas utilizadas:**
- M5StamPLC / M5Unified / M5GFX — display e hardware
- ArduinoHA — integração Home Assistant
- MFRC522_I2C — leitura RFID
- rc-switch — RF433
- ArduinoJson — configuração WiFi
- PubSubClient — MQTT

---

## 12. Anexo: Mapeamento PCF8574 → Keypad

O keypad 4×3 usa 7 pinos do PCF8574: 4 para linhas (output) e 3 para colunas (input). A pinagem varia conforme o modelo do keypad.

**Para ajustar a pinagem:** edite `Pins.h` e altere os arrays:

```cpp
// PCF8574 pin → row do keypad (pino que recebe LOW para ativar a linha)
static const uint8_t KEYPAD_ROW_PINS[4] PROGMEM = {0, 4, 5, 6};

// PCF8574 pin → col do keypad (pino lido para detectar tecla)
static const uint8_t KEYPAD_COL_PINS[3] PROGMEM = {3, 1, 2};
```

Use um multímetro ou o programa auxiliar de keypad para descobrir a correspondência correta para o seu modelo.
