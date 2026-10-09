# BombTimer — Algoritmo do Jogo

## Visão Geral

BombTimer é um cronômetro de contagem regressiva com múltiplos modos de desarme. O jogador deve desarmar a bomba antes que o tempo zere, usando um dos modos configurados: código numérico via teclado, corte de fio específico, ou corte de fios em ordem correta.

## Configuração Inicial

O jogo carrega um arquivo JSON (`bomb.json`) com os seguintes parâmetros:

- **countdownSeconds**: tempo inicial da contagem regressiva (5-5999 segundos)
- **defuseMode**: modo de desarme (CODE=0, WIRE=1, ORDER=2)
- **defuseCode**: código de 4 dígitos para o modo CODE
- **defuseOrder**: sequência de 4 dígitos (1-4) para o modo ORDER
- **detWire**: fio detonador (0=ALEATÓRIO, 1-4=fio específico, 5=NENHUM)
- **defuseWire**: fio desarmador (0=ALEATÓRIO, 1-4=fio específico, 5=NENHUM)
- **speedupWire**: fio acelerador (dobra velocidade da contagem)
- **pauseWire**: fio pausador (pausa a contagem por 2 segundos)
- **brightness**: brilho do display (0-15)
- **volume**: volume do áudio (0-255)
- **tickEnabled**: som de tick a cada segundo
- **accelMode**: modo do acelerômetro (OFF=0, START=1, SPEED2X=2, PAUSE=3, DETONATE=4)
- **rfidEnabled**: leitura de tags RFID para desarme/detonação
- **defuseTags**: até 4 UIDs de tags RFID que desarmam a bomba
- **detonateTags**: até 4 UIDs de tags RFID que detonam a bomba
- **maxCodeAttempts**: tentativas máximas de código (padrão 3)
- **detTriggerSec**: tempo que o relé de detonação fica ativo (1-10s)
- **defuseTriggerSec**: tempo que o relé de desarme fica ativo (1-10s)

## Estados do Jogo

```
IDLE       → Aguardando início. Jogador ajusta tempo e configurações.
RUNNING    → Contagem regressiva ativa. Jogador tenta desarmar.
DEFUSED    → Bomba desarmada com sucesso. Relé de desarme ativo por X segundos.
DETONATED  → Bomba explodiu (tempo zerou ou fio errado). Relé de detonação ativo.
MENU       → Menu de configuração local (display + botões).
ALARM      → Alarme RTC disparou em horário programado.
```

### Transições

- **IDLE → RUNNING**: botão START pressionado, ou shake no acelerômetro (modo ACCEL_START), ou comando HA/MQTT
- **RUNNING → DEFUSED**: código correto inserido, fio desarmador cortado, sequência ORDER completa, tag RFID de desarme lida
- **RUNNING → DETONATED**: tempo zerou, fio detonador cortado, código errado excedeu tentativas, tag RFID de detonação lida, shake no modo ACCEL_DETONATE
- **DEFUSED/DETONATED → IDLE**: botão START pressionado para reset
- **IDLE → MENU**: botão MENU pressionado (long press no SELECT)
- **MENU → IDLE**: timeout 15s sem input, ou botão START

## Modos de Desarme

### Modo CODE (0)
1. Durante RUNNING, o jogador digita números no teclado
2. Ao pressionar o primeiro dígito, o display mostra underscores para as 4 posições
3. Cada dígito aparece no display
4. Após 4 dígitos, o sistema compara com **defuseCode**
5. Se correto → DEFUSED
6. Se errado → incrementa tentativa. Se excedeu **maxCodeAttempts** → DETONATED
7. Teclas `*` ou `#` cancelam a entrada atual

### Modo WIRE (1)
1. Ao iniciar, os fios são sorteados (se configurados como ALEATÓRIO)
2. Fio **detWire**: se cortado → DETONATED
3. Fio **defuseWire**: se cortado → DEFUSED
4. Fio **speedupWire**: se cortado → dobra velocidade da contagem (cada segundo real vale 2)
5. Fio **pauseWire**: se cortado → pausa contagem por 2 segundos
6. Se todos os 4 fios forem cortados → DETONATED

### Modo ORDER (2)
1. O jogador deve cortar os fios na sequência exata definida por **defuseOrder**
2. A ordem é um número de 4 dígitos onde cada dígito (1-4) representa um fio
3. Exemplo: ordem "1324" significa cortar primeiro fio 1, depois fio 3, depois fio 2, depois fio 4
4. Se cortar na ordem correta até o 4º fio → DEFUSED
5. Se cortar um fio fora da ordem esperada → DETONATED

### Modo NET (3 — apenas RS485)
1. O desarme depende de N outros puzzles na rede RS485 reportarem estado SOLVED
2. **netPuzzlesCount** define quantos puzzles precisam estar resolvidos
3. Quando o número necessário atinge a contagem → DEFUSED

## Funcionalidades Auxiliares

### Fios
- 4 fios físicos conectados a entradas digitais
- Estado: CONECTADO (circuito fechado, LOW) ou CORTADO (circuito aberto, HIGH)
- O sistema detecta transição CONECTADO→CORTADO como evento de corte
- Wire conflict detection: o menu impede que o mesmo fio seja atribuído a duas funções diferentes

### Acelerômetro (LIS3DH)
- **OFF**: sem efeito
- **START**: shake inicia o countdown a partir do IDLE
- **SPEED2X**: shake durante RUNNING dobra a velocidade
- **PAUSE**: shake durante RUNNING pausa por 2 segundos
- **DETONATE**: shake forte durante RUNNING causa detonação imediata

### RFID/NFC (PN532)
- Tags de desarme: até 4 UIDs que, ao serem lidas durante RUNNING, causam DEFUSED
- Tags de detonação: até 4 UIDs que, ao serem lidas durante RUNNING, causam DETONATED
- Aprendizado de tags via menu local ou HA

### Relés
- 2 relés de detonação: ativados por **detTriggerSec** segundos ao detonar
- 2 relés de desarme: ativados por **defuseTriggerSec** segundos ao desarmar
- Ambos desligam automaticamente após o tempo configurado

### Áudio
- `/armed.mp3`: tocado ao iniciar contagem
- `/tick.mp3`: tocado a cada segundo (se tickEnabled)
- `/boom.mp3`: tocado na detonação
- `/sucess.mp3`: tocado no desarme
- `/keypress.mp3`: feedback de tecla pressionada
- `/error.mp3`: feedback de erro

### NeoPixel (LED RGB)
- **NORMAL**: pulso suave branco (IDLE)
- **COUNTING**: pulso vermelho (RUNNING)
- **DOUBLE_SPEED**: pisca rápido vermelho (speedup ativo)
- **DEFUSED**: pulso verde (desarmado)
- **EXPLOSION**: animação de explosão (detonado)
- **RADIATION**: flicker aleatório verde/azul

### Display (HT16K33 14 segmentos)
- IDLE: mostra tempo configurado (MM:SS)
- RUNNING: contagem regressiva com dois pontos piscando
- Menu: nomes de 4 caracteres (BRIG, VOL, TICK, SHAK, NET, WIRE, CODE, RELE, RFID)
- Código: dígitos inseridos com underscores para posições vazias
- DEFUSED: "SAFE"
- DETONATED: "boom"

## Menu de Configuração Local

Navegação por botões físicos (LEFT/RIGHT para navegar, SELECT para entrar/confirmar):

1. **COUNTDOWN**: ajuste de tempo (5-5999s, step 1 curto / 5 longo)
2. **MODE**: seleção do modo de desarme (CODE/WIRE/ORDER)
3. **CODE**: entrada de código de 4 dígitos via teclado
4. **ORDER**: entrada da ordem dos fios (sequência de 4 dígitos 1-4)
5. **DET WIRE**: fio detonador (RAND/1/2/3/4/NONE)
6. **SPD WIRE**: fio acelerador (RAND/1/2/3/4/NONE)
7. **PAUSE WIRE**: fio pausador (RAND/1/2/3/4/NONE)
8. **BRIGHT**: brilho do display (0-15)
9. **TICK**: som de tick (ON/OFF)
10. **RFID**: habilita RFID (ON/OFF)
11. **RF DEFU**: aprender tag de desarme (até 4)
12. **RF DETO**: aprender tag de detonação (até 4)
13. **CLR DEFU**: limpar tags de desarme
14. **CLR DETO**: limpar tags de detonação
15. **TDET**: tempo do trigger de detonação (1-10s)
16. **TDEF**: tempo do trigger de desarme (1-10s)
17. **ALARM H**: hora do alarme RTC (0-23)
18. **ALARM M**: minuto do alarme RTC (0-59)

Timeout de 15 segundos sem input volta ao IDLE.

## Configuração de Rede (AP Mode + WiFi)

1. Na primeira inicialização, cria rede WiFi aberta "BombTimer-STAMPLC"
2. Acessando `http://192.168.4.1`, o GameMaster configura:
   - SSID e senha do WiFi
   - Broker MQTT (IP, porta, usuário, senha)
   - Nome do dispositivo no Home Assistant
3. Ao salvar, reinicia e conecta à rede configurada
4. Se não conseguir conectar, volta ao modo AP
