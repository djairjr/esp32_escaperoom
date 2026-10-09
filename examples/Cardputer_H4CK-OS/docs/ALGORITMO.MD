# H4CK-OS — Plano de Desenvolvimento (v3)

## 1. Visão Geral

### Conceito
O M5Stack Cardputer se passa por um **computador hacker minúsculo** escondido na sala.
O jogador encontra o dispositivo, faz login, e usa ferramentas de pentest simuladas para
desarmar a bomba (BombTimer StamPLC).

### Ciclo de Funcionamento

```
[POWER ON]
    │
    ▼
[BOOT SCREEN]   ← animação estilo H4CK-OS bootando (2-3s)
    │
    ▼
[LOGIN SCREEN]  ← login: root / password: (encontrada na sala)
    │
    ▼
[SHELL]         ← bash-like com comandos
    │
    ├── scan        — descobrir dispositivos na rede
    ├── tapscan     — interceptar conversas telefônicas (MP3 do SD)
    ├── nmap        — escaneamento detalhado de um alvo
    ├── hydra       — força bruta de senhas
    ├── sniff       — capturar código da bomba via IP (pista na sala)
    ├── exploit     — gerar payload RF433
    ├── rfcomm      — transmitir comando RF433 real
    ├── ha          — controlar dispositivos da sala
    ├── status      — mostrar status da bomba
    ├── clear       — limpar terminal
    └── logout      — voltar ao login
```

---

## 2. Hardware

| Componente | Função | Interface |
|------------|--------|-----------|
| **M5Stack Cardputer** | Terminal hacker | ESP32-S3, TFT 240×135, teclado QWERTY 56 teclas, I2S speaker NS4168 |
| **SYN531R RF433 TX** no Grove | Transmissão do comando de desarme | GPIO G2 (DATA) |
| **microSD Card** | Áudio MP3 (conversas) + assets | SPI (G12 CS, G14 MOSI, G40 CLK, G39 MISO) |

### Pinagem Cardputer

```
PORT.CUSTOM (Grove HY2.0-4P):
  Amarelo = G2  → RF433 DATA (SYN531R Transmitter)
  Branco  = G1  → livre (reserva)
  Vermelho = 5V → VCC SYN531R
  Preto    = GND → GND SYN531R

SD Card: G12(CS) G14(MOSI) G40(CLK) G39(MISO)
Speaker: NS4168 I2S (G41 BCLK, G42 DATA, G44 LRCLK)
Display: ST7789V2 SPI (G38 BL, G33 RST, G34 RS, G35 DAT, G36 SCK, G37 CS)
Teclado: Matriz 4×14 via TCA8418 (I2C interno)
```

---

## 3. Arquitetura de Software

### 3.1 Estrutura de Arquivos

```
~/shared_files/Code/ESP32_Projects/Cardputer_H4CK-OS/
├── H4CK-OS.ino                  ← setup() + loop()
├── src/
│   ├── Shell.h / .cpp           ← parser de comandos + dispatch
│   ├── BootScreen.h             ← animação de boot H4CK-OS
│   ├── LoginScreen.h            ← login + senha
│   ├── Scanner.h                ← scan + nmap (simulados)
│   ├── TapScanner.h             ← tapscan: MP3 do SD (EscapeRoomPhone pattern)
│   ├── Cracker.h                ← hydra (força bruta simulada)
│   ├── Sniffer.h                ← sniff: captura código configurado pelo GM
│   ├── Exploit.h                ← gera payload RF
│   ├── Transmitter.h            ← rfcomm via RF433 real (rc-switch)
│   ├── BombStatus.h             ← status da bomba via WiFi
│   ├── HAClient.h               ← ha + entidades HA do próprio Cardputer
│   ├── SoundManager.h           ← beeps + playOnceBlocking
│   ├── ConfigManager.h          ← config JSON em SPIFFS/LittleFS
│   └── Config.h                 ← pinos, constantes
├── data/
│   ├── taps/
│   │   ├── channel1.mp3         ← conversa interceptada 1
│   │   ├── channel2.mp3         ← conversa interceptada 2
│   │   ├── channel3.mp3         ← conversa interceptada 3
│   │   └── channel4.mp3         ← conversa interceptada 4
│   ├── scan.mp3                 ← som de scan
│   ├── exploit.mp3              ← som de payload gerado
│   ├── transmit.mp3             ← som de transmissão RF
│   ├── success.mp3              ← puzzle resolvido
│   └── error.mp3                ← erro / acesso negado
├── config.json                  ← configuração padrão (SPIFFS)
├── docs/
│   ├── ALGORITMO.MD             ← este documento
│   └── USER_MANUAL.MD           ← manual do jogador (futuro)
└── ALGORITMO.MD                 ← cópia na raiz
```

### 3.2 Máquina de Estados

```
[POWER ON] → [BOOT] (3s) → [LOGIN] → [SHELL]
                                        │
            ┌──────────┬──────────┬─────┼─────┬──────────┬──────────┐
            ▼          ▼          ▼     ▼     ▼          ▼          ▼
       [STAGE A]  [STAGE B]  [STAGE C] [HA] [SNIFF]  [TAPSCAN]  [STATUS]
       scan/nmap    hydra     exploit   cmd   codigo   MP3 SD    bomba
```

---

## 4. Integração Home Assistant — Entidades do Cardputer

**O jogador NÃO acessa MQTT diretamente.** O `sniff` usa um IP pre-configurado
(pista na sala) e dados simulados — não é conexão MQTT real.

**Mas o próprio Cardputer expõe entidades no Home Assistant** para o Game Master
configurar o puzzle remotamente:

### Entidades HA do Cardputer (para o GM)

| Tipo | ID | Descrição | Uso |
|------|----|-----------|-----|
| **Text** | `h4ckos_login_password` | Senha do login | GM define a senha |
| **Text** | `h4ckos_sniff_ip` | IP alvo do sniff | GM define qual IP o jogador precisa "invadir" |
| **Number** | `h4ckos_defuse_code` | Código de desarme | GM define o código que o sniff revela |
| **Select** | `h4ckos_sniff_channel` | Canal de áudio do sniff | GM troca qual MP3 toca (1-4) |
| **Select** | `h4ckos_stage` | Estágio atual | GM avança/reseta estágios manualmente |
| **Sensor** | `h4ckos_status` | Status do terminal | "login", "shell", "defused" |
| **Switch** | `h4ckos_rf433_enable` | Habilita RF433 | GM libera/bloqueia transmissão RF |

### Exemplo de configuração:

```
GM no HA:
  h4ckos_login_password = "kali"
  h4ckos_sniff_ip = "10.0.0.5"
  h4ckos_defuse_code = "7391"
  h4ckos_sniff_channel = "2"
  → jogador descobre IP "10.0.0.5" na sala
  → sniff revela código "7391"
  → pode desarmar pelo keypad da bomba
```

### Comunicação: ArduinoHA (igual ao BombTimer)

```cpp
// HAClient.h
#include <ArduinoHA.h>

HAText haLoginPassword("h4ckos_login_password");
HAText haSniffIp("h4ckos_sniff_ip");
HANumber haDefuseCode("h4ckos_defuse_code");
HASelect haStage("h4ckos_stage");
HASensor haStatus("h4ckos_status");
HASwitch haRFEnable("h4ckos_rf433_enable");

// Callbacks salvam em SPIFFS JSON
void haLoginPasswordCb(const char* val, HAText*) { /* salva em config.json */ }
void haSniffIpCb(const char* val, HAText*)       { /* salva em config.json */ }
```

---

## 5. Configuração via SPIFFS (config.json)

### 5.1 Formato

```json
{
  "login": {
    "user": "root",
    "password": "kali"
  },
  "sniff": {
    "ip": "10.0.0.5",
    "defuse_code": 7391,
    "channel": 2
  },
  "stages": {
    "current": 0,
    "scan_enabled": true,
    "hydra_enabled": false,
    "exploit_enabled": false,
    "rf433_enabled": false
  },
  "wordlist": [
    "kali",
    "toor",
    "root",
    "h4ck3r",
    "bomb",
    "defuse"
  ],
  "ha": {
    "broker": "192.168.1.100",
    "port": 1883,
    "user": "",
    "pass": ""
  }
}
```

### 5.2 Ciclo de Configuração

```
Boot → SPIFFS.begin() → load /config.json
                          │
                    ┌─────┴─────┐
                    │           │
               Existe?    Não existe?
                    │           │
                    ▼           ▼
               Carrega     Cria com defaults
               valores     Salva no SPIFFS
                    │           │
                    └─────┬─────┘
                          ▼
                  ConfigHA.begin()
                  (WiFi + MQTT + HA entities)
                          │
                          ▼
                  BOOT SCREEN → LOGIN
```

### 5.3 GM altera via HA → SPIFS

```
GM altera "h4ckos_login_password" no HA
    → callback salva em config.json
    → na próxima vez que o jogador fizer logout,
      a nova senha já está valendo
```

---

## 6. Comandos em Detalhe

### 6.1 `tapscan` — Interceptador de Chamadas (MP3 do SD)

```
root@h4ck-os:~$ tapscan
[HACK] Scanning cellular frequencies...
[HACK] 4 active channels found.

Channel 1: [BUSY]  — "o código de desarme é 7391..."
Channel 2: [IDLE]  — no traffic
Channel 3: [BUSY]  — "a senha do sistema é kali"
Channel 4: [IDLE]  — no traffic

root@h4ck-os:~$ tapscan listen 1
[HACK] Intercepting channel 1...
▶ Playing: /sd/taps/channel1.mp3
```

**Implementação:** idêntica ao EscapeRoomPhone:
- `tapscan` → lista canais com status (BUSY/IDLE) baseado em config.json
- `tapscan listen <n>` → `hw.sound().playOnceBlocking("/sd/taps/channelN.mp3")`
- GM configura qual canal está ativo via HA (`h4ckos_sniff_channel`)

### 6.2 `sniff` — Captura o Código (sem MQTT real)

```
root@h4ck-os:~$ sniff 10.0.0.5
[HACK] Connecting to 10.0.0.5:8080...
[HACK] Intercepted bomb configuration:
  → defuse_code: 7391
  → countdown: 180s
  → mode: CODE

root@h4ck-os:~$ sniff code
[HACK] Current defuse code: 7391
```

**Implementação:** o `sniff` NÃO faz conexão MQTT real.
Ele simplesmente lê o `defuse_code` do `config.json`, que foi
configurado pelo GM via HA. O IP é uma pista na sala.

**Mas parece real:** o comando mostra progresso, conexão,
"pacotes interceptados", etc.

### 6.3 `scan` / `nmap` — Stage A (simulado)

```
root@h4ck-os:~$ scan
[HACK] Scanning 192.168.1.0/24...
[HACK] 3 devices found:
  192.168.1.100  — BombTimer (RF433)
  192.168.1.101  — Unknown
  192.168.1.1    — Router

root@h4ck-os:~$ nmap 192.168.1.100
[HACK] Scanning 192.168.1.100...
  PORT     STATE  SERVICE
  8080/tcp open   RF433_TRANSMITTER
  9090/tcp open   AUTH_SERVICE
  7070/tcp open   ENCRYPTER
```

### 6.4 `hydra` — Stage B (força bruta)

```
root@h4ck-os:~$ hydra 192.168.1.100 -P passwords.txt
[HACK] Brute forcing AUTH_SERVICE on 192.168.1.100:9090...
[HACK] Trying password: kali... FAIL
[HACK] Trying password: toor... FAIL
[HACK] Trying password: root... SUCCESS!
[HACK] Key obtained: 0x7A3F
```

### 6.5 `exploit` — Stage C

```
root@h4ck-os:~$ exploit -k 0x7A3F -p 8080
[HACK] Generating exploit payload...
[HACK] Payload: RF433 code 0x89ABCD
[HACK] Use 'rfcomm send 0x89ABCD' to transmit
```

### 6.6 `rfcomm` — Stage D (RF433 REAL)

```
root@h4ck-os:~$ rfcomm send 0x89ABCD
[HACK] Transmitting RF433 signal...
[HACK] Code 0x89ABCD sent!
```

**RF433 é real:** SYN531R transmite via G2. BombTimer recebe.

### 6.7 `ha` — Controle da Sala

```
root@h4ck-os:~$ ha list
  light.living_room     — Living Room Light
  switch.fan            — Ceiling Fan
  lock.main_door        — Main Door Lock

root@h4ck-os:~$ ha toggle light.living_room
  → light.living_room toggled
```

**MQTT real:** conecta ao broker HA e envia comandos.

### 6.8 `status` — Status da Bomba

```
root@h4ck-os:~$ status
[STATUS] Connecting to BombTimer...
  State: RUNNING
  Time:  00:47
  Wires: 2 cut
  Mode:  CODE
```

---

## 7. Áudio

### 7.1 MP3 do SD (tapscan)

Padrão EscapeRoomPhone:
```cpp
// Play MP3 from SD, blocking until done
hw.sound().playOnceBlocking("/sd/taps/channel2.mp3");

// Sound loop no loop() principal
hw.sound().loop();
```

### 7.2 Beeps Sintetizados

```cpp
M5Cardputer.Speaker.tone(4000, 50);   // tecla
M5Cardputer.Speaker.tone(3000, 30);   // comando
M5Cardputer.Speaker.tone(800, 150);   // erro
```

---

## 8. Múltiplos Caminhos para o Jogador

| Estratégia | Ferramentas | Depende de |
|------------|-------------|------------|
| **RF433** | scan → nmap → hydra → exploit → rfcomm | Pistas na sala (senhas) |
| **Código via sniff** | sniff <IP> → digitar no keypad da bomba | Pista do IP na sala |
| **Pista por áudio** | tapscan → escutar conversas | Conteúdo dos MP3 |
| **Controle da sala** | ha → interagir com dispositivos | Nenhum (sempre disponível) |

---

## 9. Fluxo de Configuração pelo Game Master

```
1. GM acessa Home Assistant
2. Encontra entidades do Cardputer:
   - h4ckos_login_password → define senha
   - h4ckos_sniff_ip → define IP pista
   - h4ckos_defuse_code → define código revelado
   - h4ckos_sniff_channel → define qual canal de áudio ativo
3. Coloca pistas na sala:
   - Post-it com IP: "10.0.0.5"
   - Documento com senhas pro hydra
   - Pendrive com arquivos MP3 no SD
4. Liga o Cardputer
5. Ciclo: boot → login → shell → jogador explora
```

---

## 10. Cronograma

| Fase | Descrição | Horas |
|------|-----------|-------|
| 1 | Setup projeto + ConfigManager (SPIFFS JSON) | 2 |
| 2 | Boot screen + Login screen | 2 |
| 3 | Shell bash (parser + help + dispatch) | 3 |
| 4 | tapscan (MP3 do SD) | 2 |
| 5 | Stage A: scan/nmap | 2 |
| 6 | Stage B: hydra | 2 |
| 7 | sniff (código do config.json) | 1 |
| 8 | Stage C: exploit | 1 |
| 9 | Stage D: rfcomm (RF433 real) | 2 |
| 10 | HA entities do Cardputer + ConfigHA | 3 |
| 11 | BombStatus WiFi | 2 |
| 12 | HAClient (ha commands) | 2 |
| 13 | Áudio (beeps + triggers MP3) | 1 |
| 14 | Integração + testes | 3 |
| **Total** | | **~28h** |
