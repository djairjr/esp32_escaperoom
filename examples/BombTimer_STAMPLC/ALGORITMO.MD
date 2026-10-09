# BombTimer STAMPLC — Algoritmo e Estrutura Completa (M5Stack StamPLC)

## Visão Geral

Port do GameTimerPro original (RP2040/PropMaker) e BombTimer (ESP32-S3/XIAO) para **M5Stack StamPLC** (ESP32-S3).

**Hardware:**
- Display TFT ST7789v2 240×135 (rotation 1)
- Buzzer PWM (M5StamPLC.tone)
- 3 botões A/B/C (PI4IOE5V6408) + BOOT (GPIO0 = START)
- Keypad 4×3 via PCF8574 (I2C 0x20-0x27, auto-detect)
- 4 relés AW9523B (DETONATE: RLY1/RLY2, DEFUSE: RLY3/RLY4)
- 4 opto-inputs AW9523B (fios WIRE1-4)
- 1 opto-input AW9523B (DET externo)
- RFID MFRC522/WS1850S (I2C 0x28 via PORT.A, MFRC522_I2C library)
- RTC RX8130CE (I2C 0x32)
- WiFi + ArduinoHA (MQTT)
- RGB LED (PI4IOE5V6408, via M5StamPLC.setStatusLight)
- RF433 SYN531R (PORT.C white = GPIO4, rc-switch)

**I2C Buses:**
- **Internal (Wire):** G13(SDA) / G15(SCL) — PI4IOE5V6408(0x43), AW9523B(0x59), RX8130CE(0x32), LM75B(0x48), INA226(0x40)
- **PORT.A (PortA):** G2(SDA) / G1(SCL) — PCF8574(0x20-0x27), MFRC522(0x28)

---

## Arquivos do Projeto

| Arquivo | Descrição | Localização |
|---------|-----------|-------------|
| BombTimer_STAMPLC.ino | Main: setup() + loop() + máquina de estados + menu | Bibliotecas + 2 destinos |
| GameLogic.h | Timer, fios, detonação, desarme, menu, NVS persistência | Igual |
| DisplayManager.h | TFT ST7789v2, DSEG14 20pt, círculos, status bar | Igual |
| InputManager.h | Botões, RF433, keypad PCF8574, RFID MFRC522, DET opto | Igual |
| ConfigHA.h | AP mode + webserver + WiFi + ArduinoHA (MQTT) | Igual |
| Pins.h | Mapeamento de pinos I2C, AW9523B, PI4IOE5V6408, keypad | Igual |
| docs/USER_MANUAL.md | Manual do usuário do puzzle | docs/ |

**Destinos (sempre copiar para os 3):**
1. `~/Arduino/libraries/ESP32EscapeRoom/examples/BombTimer_STAMPLC/` (dev+lib)
2. Radxa: `~/shared_files/Code/ESP32_Projects/BombTimer_STAMPLC/`
3. Windows: `<Arduino>\ESP32_Projects\BombTimer_STAMPLC\`
4. Windows lib: `<Arduino>\libraries\esp32_escaperoom\examples\BombTimer_STAMPLC\`

---

## Máquina de Estados (6 estados)

```
[IDLE] --START--> [RUNNING]
  |                   |
  +--MENU--> [MENU]   +--tempo=0--> [DETONATED]
  |                   +--fio DET-->  [DETONATED]
  |                   +--START------>[DETONATED]
  |                   +--ORDER err-->[DETONATED]
  |                   +--CODE ok---->[DEFUSED]
  |                   +--ORDER ok--->[DEFUSED]
  |                   +--WIRE ok---->[DEFUSED]
  |                   +--RFID DEFU-->[DEFUSED]
  |                   +--RFID DETO-->[DETONATED]
  |                   +--CODE 4x err>[DETONATED]
  |
  [DETONATED] --START--> [IDLE]
  [DEFUSED]   --START--> [IDLE]
  [MENU]      --START--> [IDLE]  (ou timeout 15s)
  [ALARM]     --START--> [IDLE]
```

### Estados

| Estado | Função | Descrição |
|--------|--------|-----------|
| IDLE | updateIdle() | Ajusta tempo (PREV/NEXT ±1s, long press ±5s/150ms). BTN_START → RUNNING. BTN_MENU → MENU. |
| MENU | updateMenu() | 18 itens navegáveis. PREV/NEXT navega, MENU entra em edit, MENU confirma. Long press acelera ajuste. START ou 15s timeout → IDLE. |
| RUNNING | updateRunning() | Timer, tick (via tickDue()), scanWires, processKeypad, RFID check, HA publish. RGB LED vermelho pulsante (500ms, 250ms últimos 10s). |
| DETONATED | updateDetonated() | Tela "DETONATED! BOOM!", RGB vermelho sólido. Relés ativos por detTriggerSec. START → IDLE. |
| DEFUSED | updateDefused() | Tela "DEFUSED! SUCCESS!", RGB verde. Relés ativos por defuseTriggerSec. START → IDLE. |
| ALARM | updateAlarm() | Beep 3500Hz intermitente 500ms + RGB laranja. START → IDLE. |

---

## Display (DisplayManager.h)

- **Rotation:** 1 (USB à esquerda, w=240 h=135)
- **Timer DSEG14:** `TIMER_Y = 24`, y = 28 (centralizado), 5 chars + ':', ~128px
- **Segundos:** `SEC_Y = 78` — texto pequeno "180s"
- **Status bar (canto superior direito):** `[W]`(60) `[M]`(45) `[R]`(30) `[T]`(15) — posições X da direita
- **Círculos fios (canto inferior esquerdo):** YEL(8) RED(26) BLU(44) GRN(62) — X a cada 18px
- **Status texto (canto inferior direito):** IDLE/RUN/OK/BOOM
- **Menu:** NUNCA chama `display.begin()` — usa `M5.Lcd.print()` direto com `fillRect` parcial
- **Tela cheia:** `drawFullScreenMsg(line1, line2, color)` — fillScreen + texto centralizado
- **Código:** `drawCodeEntry(digits, currentDigit)` — DSEG14 laranja, 4 dígitos + cursor piscante

---

## Input (InputManager.h)

### Botões
| Botão | Método | Função |
|-------|--------|--------|
| BtnA | wasPressed(BTN_PREV) | Diminuir / navegar para trás |
| BtnB | wasPressed(BTN_MENU) | Menu (IDLE) / Confirmar (MENU edit) |
| BtnC | wasPressed(BTN_NEXT) | Aumentar / navegar para frente |
| BOOT (GPIO0) | wasPressed(BTN_START) | Iniciar (IDLE) / Reset (DETONATED/DEFUSED/ALARM) / Detona (RUNNING) |

### isHeld(btn, ms)
Usado para long press: 600ms para acionar aceleração, 50ms para detectar soltou.

### RF433 (rc-switch)
SYN531R no PORT.C (GPIO4). 8 comandos: PREV, NEXT, MENU, START, DETONATE, DEFUSE, SPEED, PAUSE.

### Keypad (PCF8574)
- Endereço detectado automaticamente (0x20-0x27) no i2cScan()
- Mapeamento de pinos configurável em Pins.h: `KEYPAD_ROW_PINS[4]` e `KEYPAD_COL_PINS[3]`
- Matriz de caracteres fixa: `123 456 789 *0#`
- Só faz polling I2C se `pcfOk_ == true` e modo atual for CODE

### RFID (MFRC522_I2C)
- I2C 0x28 via PORT.A
- Self-test no boot para confirmar presença do chip
- `readRFID(uidHex, &uidLen)` retorna UID em hex
- UID convertido para uint32_t (primeiros 4 bytes) para matching

---

## Game Logic (GameLogic.h)

### Config (NVS namespace "bomb")
17 parâmetros salvos via Preferences:
- `countdownSeconds` (5-5999)
- `defuseMode`: CODE(0), WIRE(1), ORDER(2)
- `defuseCode` (0000-9999)
- `defuseOrder` (1111-4444)
- `detWire`, `speedupWire`, `pauseWire` (0=RAND, 1-4=fio, 5=NONE)
- `brightness` (0-15)
- `tickEnabled`, `rfidEnabled`
- `alarmHour`, `alarmMin`, `alarmEnabled`
- `maxCodeAttempts` (default 3)
- `detTriggerSec`, `defuseTriggerSec` (1-10s)
- `defuseTags[4]`, `detonateTags[4]` (RFID UIDs)

### Fios (Wires)
| Fio | Config | Efeito |
|-----|--------|--------|
| DET | detWire (RAND/1-4/NONE) | Cortar → detona imediatamente |
| DEFUSE | defuseMode=WIRE, defuseWire | Cortar → desarma |
| ORDER | defuseMode=ORDER, defuseOrder | Sequência correta → desarma, errada → detona |
| SPEEDUP | speedupWire | Cortar → toggle doubleSpeed (beep agudo) |
| PAUSE | pauseWire | Cortar → pausa 2s (beep grave) |

### Código (CODE mode)
- 3 tentativas (`maxCodeAttempts`, configurável)
- A cada erro: tela "ERROR / Try again" por 1s (fullScreenUntil, NÃO delay)
- 4ª tentativa: detona
- `*`/`#` ou START: cancela entrada

### Menu (18 itens)
COUNTDOWN → MODE → CODE → ORDER → DET WIRE → SPD WIRE → PAUSE WIRE → BRIGHT → TICK → RFID ENABLE → RFID DEFU (learn) → RFID DETO (learn) → TDET → TDEF → ALARM H → ALARM M → CLR DEFU → CLR DETO

### Timed Relay Release
- `triggerDetonate()`: RLY1+2 ON, auto-release após detTriggerSec (updateRelays() no loop)
- `triggerDefuse()`: RLY3+4 ON, auto-release após defuseTriggerSec
- `clearAllRelays()`: desliga todos imediatamente

---

## ConfigHA (AP Mode + WiFi + HA MQTT)

### Boot sequence
1. SPIFFS mount → load `/wifi.json`
2. Se SSID salvo: `WiFi.begin(ssid, pass)`, 20s timeout
3. Se conectou: `beginHA()` (ArduinoHA + MQTT)
4. Se falhou: AP mode `BombTimer-STAMPLC` + webserver na porta 80
5. Publicação automática: `loopHA()` detecta transição disconnected→connected no MQTT e chama `publishAll()`

### AP mode
- SSID: `BombTimer-STAMPLC`
- IP: `192.168.4.1`
- Formulário: WiFi (SSID+Senha), MQTT (Broker, Porta, User, Pass), Device Name
- Salva → SPIFFS `/wifi.json` → `ESP.restart()`

### Home Assistant Entities
| Tipo | ID | Nome | Descrição |
|------|----|------|-----------|
| Sensor | state | Estado | idle/running/defused/detonated/menu/alarm |
| Sensor | time | Tempo Restante | segundos restantes |
| Sensor | wires_cut | Fios Cortados | quantidade |
| Sensor | code_attempts | Tentativas | tentativas de código |
| Sensor | mode | Modo Desarme | CODE/WIRE/ORDER |
| Sensor | i2c_status | I2C Devices | dispositivos detectados |
| Sensor | wire1-4 | Fio 1-4 | 0=conectado, 1=cortado |
| Button | start | Iniciar | inicia contagem |
| Button | reset | Resetar | reseta para IDLE |
| Number | countdown | Tempo (segundos) | 5-5999 |
| Number | defuse_code | Codigo Desarme | 0-9999 |
| Number | brightness | Brilho | 0-15 |
| Select | mode | Modo Desarme | CODE/WIRE/ORDER |
| Select | det_wire | Fio DET | RAND/1/2/3/4/NONE |
| Select | defuse_wire | Fio DEFUSE | RAND/1/2/3/4/NONE |
| Select | speed_wire | Fio SPEED | RAND/1/2/3/4/NONE |
| Select | pause_wire | Fio PAUSE | RAND/1/2/3/4/NONE |
| Switch | tick_sound | Som Tick | ON/OFF |

---

## RTC e Alarme

```cpp
struct tm t;
M5StamPLC.getRtcTime(&t);
game.checkAlarm(t.tm_hour, t.tm_min);  // match → STATE_ALARM
```

Alarme habilitado automaticamente se alarmHour > 0 ou alarmMin > 0.

---

## RGB LED

| Estado | Efeito | Cores |
|--------|--------|-------|
| IDLE | Off | (0,0,0) |
| RUNNING | Vermelho pulsante 500ms (250ms <10s) | (255,0,0) toggle |
| DETONATED | Vermelho sólido | (255,0,0) |
| DEFUSED | Verde sólido | (0,255,0) |
| ALARM | Laranja intermitente 500ms | (255,128,0) toggle |
| MENU | Off | (0,0,0) |

---

## Tick Sonoro (importante: intermitência)

### Causa raiz identificada e corrigida
O tick usava `lastDisplayedRemaining` como referência — variável compartilhada com `updateDisplay()`. Se alguma função demorasse (I2C polling do keypad, RFID, MQTT), o display não atualizava e o tick não disparava.

### Correções aplicadas
1. **tickDue()** — função independente em GameLogic.h com `lastTickSecond_` próprio
2. **processKeypad()** — só chama `getKey()` se `mode == DEFUSE_CODE` **E** `pcfOk_ == true`
3. **RFID** — só escaneia se `hasRFID()` true (chip detectado no boot)
4. **publishAll()** — a cada 2s com `lastHAPublish`, não `millis() % 1000`
5. **delay(5)** → **delay(1)** no loop principal
6. **HA publish automático** na conexão MQTT (transição disconnected→connected)

---

## Compilação

```
FQBN: esp32:esp32:esp32s3:PartitionScheme=default_8MB,FlashMode=qio,PSRAM=enabled,UploadSpeed=921600
Partição: default_8MB (1.25MB APP)
Uso: ~38% flash (1.303.055 / 3.342.336 bytes)
RAM: ~16% (54.280 / 327.680 bytes)
```

---

## RFID Tags

- Tags armazenadas como uint32_t (primeiros 4 bytes do UID)
- Até 4 tags de desarme + 4 de detonação
- Menu: RF DEFU (aprender tag de desarme), RF DETO (aprender tag de detonação)
- Menu: CLR DEFU, CLR DETO (limpar tags)
- Durante RUNNING com rfidEnabled: poll a cada 300ms, verifica defuseTags e detonateTags

---

## Key Differences from Original GameTimerPro

| Feature | Original RP2040 | StamPLC Port |
|---------|----------------|--------------|
| Display | HT16K33 4×14seg | TFT 240×135 ST7789v2 DSEG14 |
| Áudio | I2S MP3 | M5StamPLC.tone (PWM) |
| Botões | PCF8575 + IoAbstraction | M5.BtnA/B/C + BOOT |
| Neopixel | 48 LEDs WS2812 | RGB LED PI4IOE5V6408 |
| Config | LittleFS bomb.json | NVS (Preferences) |
| Rede | RS485 | WiFi + ArduinoHA (MQTT) |
| Acelerômetro | LIS3DH | Removido |
| Servo | PCA9685 | Removido |
| Partição | 8MB (4MB APP) | 8MB (1.25MB APP default) |
