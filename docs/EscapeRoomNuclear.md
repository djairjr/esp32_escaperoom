---
name: Escape Room Nuclear - Documento de Design v2
description: Projeto conceitual revisto com 3 tecnicos presos, acesso RFID, plataformas e ReactorControl
type: project
---

# Escape Room Nuclear - Documento de Design v2

## 1. Tema e Narrativa

### Ambientacao

Uma usina nuclear de pesquisa entrou em pane geral. Os sistemas de seguranca falharam e o reator esta superaquecendo. Tres tecnicos ficaram presos em salas isoladas durante o protocolo de emergencia.

Os jogadores precisam liberar os tecnicos, cada um usando metodos de acesso diferentes. Cada tecnico libertado pode ocupar sua estacao de trabalho. O reator so pode ser estabilizado com todos os tres nos seus postos.

### Personagens

Os tres funcionarios presos na usina:

| Nome | Funcao | Cracha RFID | Especialidade |
|------|--------|-------------|---------------|
| Alexei Bogdan | Engenheiro do Reator | `CR:AC:AD:01` | Nucleo, barras de controle, temperatura |
| Boris Vadin | Tecnico de Coolant | `CR:AC:AD:02` | Bombas, pressao, refrigeracao primaria |
| Vladislav Gennady | Eletricista de Manutencao | `CR:AC:AD:03` | Turbina, sistemas eletricos, instrumentos |

Cada tecnico possui um cracha RFID funcional. O cracha do Alexei esta dentro de uma agenda de anotacoes na sala de controle — os jogadores precisam encontrar a agenda e usar o cracha combinado com uma senha para libera-lo.

### Estações de Trabalho

Cada tecnico liberado ativa uma plataforma retratil em um dos tres lados da caixa:

- **Plataforma A** (frente-esquerda) — posto do Alexei. Reator, temperatura, controle de barras.
- **Plataforma B** (frente-direita) — posto do Boris. Bombas, pressao, coolant.
- **Plataforma C** (lateral) — posto do Vladislav. Turbina, RPM, painelt eletrico.

Na base de cada plataforma ha um leitor RFID. O puzzle de cada posto so ativa se o tecnico correto estiver posicionado ali.

### Fluxo do Jogo

```
INICIO
   │
   ▼
FASE_ACESSO ─── Puzzle 1 (Keypad+RFID) ─── Libera Alexei ─── Plataforma A
   │            Puzzle 2 (Coolant ALL_CONNECTED) ─ Libera Boris ─── Plataforma B
   │            Puzzle 3 (Eletrica SEQUENCE) ──── Libera Vlad  ─── Plataforma C
   │                                                    │
   │                                                    └── Libera plataformas RFID
   │                                                        + gaveta de pecas
   ▼
FASE_POSICIONAMENTO ─── RFID em cada base ─── ReactorControl ativado
   │
   ▼
FASE_REACTOR ─── Puzzle 5 (ReactorControl: valvulas + gauges)
   │                  │
   │                  └── Ativa SimonSays na TAMPA FECHADA
   ▼
FASE_TAMPA ──── Puzzle 6 (SimonSays: pressurizacao bombas)
   │                  │
   │                  └── TAMPA DA CAIXA ABRE
   ▼
FASE_INTERIOR ── Puzzle 7 (Consertar comunicacao com pecas)
   │                  │
   │                  └── MELTDOWN COUNTDOWN INICIA
   │             Puzzle 8 (RotaryDial: chamada externa)
   │                  │
   │                  └── Dica recebida (ordem dos fios)
   ▼
FASE_MELTDOWN ── Puzzle 9 (BombTimer: desarmar com a dica)
```

---

## 2. Visão Geral dos Puzzles

| # | Puzzle | Fase | Mecanica | Hardware |
|---|--------|------|----------|----------|
| 1 | Acesso ao Reator | Acesso | Keypad 4x3 + RFID PN532 (cracha na agenda) | PCF8575 0x20, PN532, rele, NeoPixel |
| 2 | Manobras Coolant | Acesso | UniversalController ALL_CONNECTED — 3 elementos | PCF8575 0x21, 3 microswitches, rele |
| 3 | Trava Sala Eletrica | Acesso | UniversalController LINEAR_SEQUENCE + libera gaveta | PCF8575 0x22, 3 chaves, rele |

| # | Puzzle | Fase | Mecanica | Hardware |
|---|--------|------|----------|----------|
| 4 | Posicionamento | Transicao | 3 leitores RFID nas bases das plataformas | 3x PN532/RC522 |
| 5 | ReactorControl | Reator | Valvulas + gauges, manter na faixa segura | 3 pots + ADS1115, display |
| 6 | SimonSays na Tampa | Tampa | Pressionar botoes na sequencia correta (tampa fechada) | 4 botoes, NeoPixel |
| 7 | Reparo Comunicacao | Interior | Consertar radio com as pecas da gaveta (VintageRadio) | LCD, botoes, chaves, pot |
| 8 | RotaryDial | Interior | Discar numero externo para receber dica | Rotary dial pulso, NeoPixel |
| 9 | BombTimer | Meltdown | Cortar fios na ordem + SHAKE | 6 fios + LIS3DH + display |

### Fase 1: Acesso (Puzzles 1 a 3)

Os tres puzzles de acesso sao sequenciais. Cada um libera um tecnico e sua plataforma retratil. O Puzzle 3 tambem libera uma gaveta com pecas de reposicao usadas no interior da caixa.

#### Puzzle 1 — Acesso ao Reator (Keypad + RFID)

**Objetivo:** Liberar Alexei, o engenheiro do reator.

**Mecanica:** Os jogadores encontram uma agenda de anotacoes na sala de controle. Dentro dela, um cracha RFID funcional (Alexei). Para libera-lo, precisam:
1. Aproximar o cracha do leitor PN532 (RFID OK)
2. Digitar a senha de emergencia no keypad 4x3
3. O acesso so libera com ambos os fatores

Baseado no `KeypadRfidLock` modo `KEYPAD_AND_RFID`.

**Hardware:** PCF8575 0x20, teclado 4x3 membrana, PN532 (I2C), NeoPixel, rele da porta.

**Narrativa:** "Sala do Reator — Acesso Restrito. Eng. Alexei Bogdan confinado."

#### Puzzle 2 — Manobras da Sala de Coolant (UniversalController ALL_CONNECTED)

**Objetivo:** Liberar Boris, o tecnico de coolant.

**Mecanica:** Um sistema mecanico com 3 elementos que devem estar todos na posicao correta simultaneamente. O puzzle usa o `UniversalController` em modo `ALL_CONNECTED` com 3 entradas.

Cada elemento representa uma valvula ou registro do sistema de refrigeracao. O jogador manipula cada um ate atingir a posicao certa. Quando todos os 3 estao na posicao, o puzzle resolve.

**Exemplos de construcao mecanica:**
- 3 alavancas que devem estar viradas para cima
- 3 tampas que devem estar fechadas na ordem correta
- 3 plugs que devem ser inseridos nos bornes certos

**Config JSON:**
```json
{
  "inputs": { "count": 3, "pcfAddress": "0x21", "pinMapping": [0,1,2] },
  "logic": { "mode": "ALL_CONNECTED", "resetOnError": false, "timeoutSeconds": 0 }
}
```

**Hardware:** PCF8575 0x21, 3 microswitches/reed switches, rele da porta.

**Narrativa:** "Sala de Coolant — Tec. Boris Vadin aguarda resgate."

#### Puzzle 3 — Trava da Sala Eletrica (UniversalController LINEAR_SEQUENCE)

**Objetivo:** Liberar Vladislav, o eletricista.

**Mecanica:** 3 chaves (toggle ou rotativas) que devem ser acionadas em uma sequencia especifica. O puzzle usa o `UniversalController` em modo `LINEAR_SEQUENCE`.

Quando resolvido, duas coisas acontecem:
1. A plataforma retratil de Vladislav se estende
2. Uma gaveta com pecas de reposicao e destravada (as pecas serao usadas dentro da caixa para consertar o sistema de comunicacao)

**Config JSON:**
```json
{
  "inputs": { "count": 3, "pcfAddress": "0x22", "pinMapping": [0,1,2] },
  "logic": { "mode": "LINEAR_SEQUENCE", "resetOnError": true, "timeoutSeconds": 0 }
}
```

**Hardware:** PCF8575 0x22, 3 chaves SPDT, rele da porta, rele da gaveta.

**Narrativa:** "Painel Eletrico — Elet. Vladislav Gennady isolado."

### Fase 2: Posicionamento (Puzzle 4)

As 3 plataformas retrateis ja estenderam. Cada uma tem um leitor RFID na base. O jogador coloca o cracha do tecnico no leitor correto:

- Alexei → Plataforma A (Reator)
- Boris → Plataforma B (Coolant)
- Vladislav → Plataforma C (Turbina)

Quando todos os 3 estao nas posicoes certas, o `ReactorControl` ativa.

**Hardware:** 3 leitores RFID (PN532 ou RC522).

### Fase 3: Reator (Puzzle 5)

#### Puzzle 5 — ReactorControl

**Objetivo:** Ajustar 3 valvulas para manter os sistemas na faixa segura.

**Mecanica:** Tres pots. Cada um controla um subsistema:
1. **Barras de Controle** — manter temperatura abaixo de 750
2. **Vazao de Coolant** — manter pressao entre 40-70%
3. **RPM da Turbina** — manter em 1500 RPM

Cada subsistema degrada com o tempo.

**Hardware:** 3 pots, ADS1115 0x48, NeoPixel, display TM1637.

**Transicao:** Resolvido → ativa SimonSays na tampa fechada.

### Fase 4: Tampa (Puzzle 6)

#### Puzzle 6 — SimonSays na Tampa

**Objetivo:** Pressurizar as bombas repetindo a sequencia de luzes.

**Mecanica:** 4 botoes com NeoPixels montados na tampa fechada. O jogador repete a sequencia. Apos N rodadas, a tampa abre.

**Hardware:** 4 botoes, NeoPixel, buzzer, servo para abrir tampa.

### Fase 5: Interior da Caixa (Puzzles 7 a 9)

A tampa abre. Dentro ha:
1. **Comunicador de campo quebrado** — um radio transmissor antigo com disco rotativo (RotaryDial) embutido. Faltam pecas.
2. **BombTimer** — display e fios.

#### Puzzle 7 — Reparo do Comunicador (VintageRadio + RotaryDial)

**Objetivo:** Consertar o comunicador com as pecas da gaveta (liberada no P3).

**Mecanica:** O comunicador tem um RotaryDial na frente e um display LCD. Esta quebrado — faltam modulos internos. O jogador insere as pecas nos encaixes seguindo a sequencia correta (VintageRadio):

1. Inserir placa/modulo no encaixe (microswitch detecta)
2. Ajustar chave seletora para o modo correto
3. Pressionar botao de teste e verificar feedback no LCD

Apos o reparo completo (todas as etapas), o comunicador volta a funcionar. **O MELTDOWN COUNTDOWN INICIA** (5 min no TM1637).

O RotaryDial agora esta funcional. Discar o numero de emergencia de 4 digitos faz o comunicador tocar uma mensagem de audio MP3 com a **ordem correta dos fios** do BombTimer.

**Hardware:** LCD 16x2, 2 botoes, RotaryDial (pulse dial, 2 fios + GND + pull-up), 2 chaves toggle, NeoPixel, audio MP3 (I2S).

**Nota:** Puzzle customizado. O VintageRadio e referencia conceitual, mas a implementacao precisa ser validada — misturar o reparo sequencial (VintageRadio) com o RotaryDial e a reproducao de audio MP3 sob demanda pode exigir um puzzle dedicado fora dos exemplos existentes.

#### Puzzle 9 — BombTimer

**Objetivo:** Cortar os 6 fios na ordem revelada pelo Puzzle 8.

**Mecanica:** Display com contagem regressiva (5 min). Seis fios em bornes. Ordem correta veio do RotaryDial. Shake (LIS3DH) congela 3s e conta erro. Erros ou fio errado aceleram o timer.

Sem keypad. So fios e SHAKE.

**Hardware:** 6 bornes, LIS3DH, display TM1637.

```json
{
  "wireCount": 6,
  "timerMinutes": 5,
  "useKeypad": false,
  "shakePenalty": 3,
  "maxErrors": 3
}
```

---

## 3. Arquitetura Fisica

### Diagrama da Caixa (vista superior)

```
┌──────────────────────────────────────────────────┐
│                                                   │
│   ┌────────────────────────────────────────┐      │
│   │         PAINEL CENTRAL                 │      │
│   │   [Keypad] [RFID] [Chaves] [Botoes]   │      │
│   │   [NeoPixel Status Bar - 48 LEDs]     │      │
│   │   [TM1637 Timer Global]               │      │
│   └────────────────────────────────────────┘      │
│                                                   │
│   ┌──────────────┐    ┌──────────────┐            │
│   │ PLATAFORMA A │    │ PLATAFORMA B │            │
│   │ (Alexei)     │    │ (Boris)      │            │
│   │ RFID base    │    │ RFID base    │            │
│   │ ReactorCtrl  │    │ SimonSays    │            │
│   └──────────────┘    └──────────────┘            │
│                                                   │
│   ┌──────────────┐                                │
│   │ PLATAFORMA C │                                │
│   │ (Vladislav)  │                                │
│   │ RFID base    │                                │
│   │ VintageRadio │                                │
│   └──────────────┘                                │
│                                                   │
└──────────────────────────────────────────────────┘
```

### Diagrama de Blocos do Sistema

```
PropMaker RP2040 (1 placa)
│
├── PCF8575 0x20 ── Puzzle 1 (Keypad+RFID) ─── Teclado 4x3 + rele
├── PCF8575 0x21 ── Puzzle 2 (Coolant) ──────── 3 microswitches + rele
├── PCF8575 0x22 ── Puzzle 3 (Eletrica) ─────── 3 chaves + rele gaveta
├── PCF8575 0x23 ── Puzzle 4 (Posicionamento) ── 3 relays plataformas
├── PCF8575 0x24 ── SimonSays ────────────────── 4 botoes
│
├── PN532 #1 (I2C 0x24) ── Puzzle 1 (leitor cracha)
├── PN532 #2 (I2C 0x25) ── Base plataforma A (ou RC522)
├── PN532 #3 (I2C 0x26) ── Base plataforma B (ou RC522)
├── PN532 #4 (I2C 0x27) ── Base plataforma C (ou RC522)
│
├── ADS1115 0x48 ── Puzzle 5 (3 pots do ReactorControl)
├── ADS1115 0x49 ── Puzzle 7 (1 pot do VintageRadio)
│
├── LCD I2C 0x30 ── Puzzle 7 (VintageRadio display)
│
├── NeoPixel externo ── 48 LEDs (ver divisao abaixo)
├── TM1637 ── Timer global
├── Audio I2S ── MP3 player compartilhado
│
└── Serial Debug ── console admin
```

### Divisao de NeoPixels

| Regiao | LEDs | Uso | Cor |
|--------|------|-----|-----|
| Status Acesso | 0-2 | Indicador tech 1/2/3 liberado | Verde |
| Puzzle 1 | 3-7 | Feedback Keypad+RFID | Azul |
| Puzzle 2 | 8-11 | Feedback microswitches | Ciano |
| Puzzle 3 | 12-14 | Feedback chaves rotativas | Branco |
| Puzzle 4 | 15-17 | Plataformas | Amarelo |
| Puzzle 5 | 18-24 | ReactorControl gauges | Vermelho/Ciano |
| Puzzle 6 | 25-29 | SimonSays | RGB |
| Puzzle 7 | 30-35 | VintageRadio | Roxo |
| Puzzle 8 | 36-41 | ConnectWires | Laranja |
| Puzzle 9 | 42-47 | BombTimer | Vermelho pulsante |

---

## 4. Hardware Consolidado

### Placa Principal

| Componente | Qtd | Observacao |
|------------|-----|------------|
| PropMaker Feather RP2040 | 1 | Placa central |

### Expansores I2C

| Componente | Endereco | Qtd | Uso |
|------------|----------|-----|-----|
| PCF8575 | 0x20-0x25 | 6 | Entradas digitais |
| ADS1115 | 0x48-0x49 | 2 | Entradas analogicas |
| PN532 | 0x24-0x27 | 4 | Leitores RFID (1 cracha + 3 bases) |

### Entradas

| Tipo | Qtd | Puzzle |
|------|-----|--------|
| Teclado membrana 4x3 | 2 | P1, P2 |
| Chave toggle SPDT | 6 | P3 |
| Botao momentaneo | 8 | P4 (3), P6 (4), P7 (1) |
| Potenciometro linear | 4 | P5 (3), P7 (1) |
| Leitor RFID PN532 | 4 | P1 (cracha), bases A/B/C |
| Chave seletora | 2 | P7 (VintageRadio) |

### Saidas

| Tipo | Qtd | Puzzle |
|------|-----|--------|
| NeoPixel strip externo | 1 (ate 48 LEDs) | Todos |
| Display TM1637 | 1 | Timer global |
| Display LCD I2C | 1 | P7 |
| Audio I2S | 1 | Compartilhado |
| Rele/SSR | 6 | P1/P2/P3 (3 portas) + P4 (3 plataformas) |

### Fonte

- PropMaker: USB-C 5V
- NeoPixel: 5V via PIN_EXTERNAL_POWER
- PCF8575, PN532, ADS1115: 3.3V (barramento I2C)
- Displays: 5V
- Reles: 5V via transistor drive

---

## 5. Implementacao do Fluxo no LogicManager

```cpp
class NuclearEscapeRoom : public LogicManager {
public:
    NuclearEscapeRoom() : LogicManager(0, false) {}

    void begin() override {
        // Puzzles 1-3: acesso (qualquer ordem)
        addPuzzle(&puzzleKeypadRfid_);   // P1: Alexei
        addPuzzle(&puzzleKeypadCode_);   // P2: Boris
        addPuzzle(&puzzleSequencia_);    // P3: Vlad
        
        // Puzzle 4: plataformas
        addPuzzle(&puzzlePlataformas_);
        
        // Puzzles de estabilizacao
        addPuzzle(&puzzleReactorControl_);
        addPuzzle(&puzzleSimonSays_);
        addPuzzle(&puzzleVintageRadio_);
        
        // Finale
        addPuzzle(&puzzleFinale_);
        
        LogicManager::begin();
    }

    void update() override {
        // Logica de transicao personalizada
        switch (fase_) {
        case Fase::ACESSO:
            if (puzzleKeypadRfid_.isSolved()) liberarTecnico(1);
            if (puzzleKeypadCode_.isSolved()) liberarTecnico(2);
            if (puzzleSequencia_.isSolved()) liberarTecnico(3);
            if (todosLiberados()) {
                fase_ = Fase::PLATAFORMA;
            }
            break;

        case Fase::PLATAFORMA:
            puzzlePlataformas_.run();
            if (puzzlePlataformas_.isSolved()) {
                fase_ = Fase::POSICIONAMENTO;
                ativarPlataformasTrabalho();
            }
            break;

        case Fase::POSICIONAMENTO:
            if (verificarPosicionamentoRFID()) {
                fase_ = Fase::ESTABILIZACAO;
            }
            break;

        case Fase::ESTABILIZACAO:
            // ... puzzles de estabilizacao ...
            break;
        }
        
        LogicManager::update();
    }
};
```

---

## 6. Possiveis Evolucoes

### Multi-jogador via RS485

Cada puzzle roda em uma PropMaker separada. O MasterController gerencia o timer global e o estado do jogo:

```
[Master] ──RS485── [Puzzle 1] ──RS485── [Puzzle 2] ── ... ── [Puzzle 8]
```

### Variacoes de Dificuldade

| Modo | Timer | Erros | Tolerancia |
|------|-------|-------|------------|
| Facil | 45 min | resetOnError=false | +/- 10% |
| Normal | 30 min | resetOnError=true | +/- 5% |
| Dificil | 20 min | reset + timeout 60s | +/- 3% |
| Nightmare | 15 min | sem retroalimentacao | +/- 2% |

### Sistema de Mensagens no Display

Usar mensagens contextualizadas para cada fase:

- "Sala do Reator — Acesso Restrito"
- "Eng. Alexei Bogdan — Libertado"
- "Plataforma A — Ativada"
- "Posicione o tecnico na estacao correta"
- "Reator em superaquecimento — 750 graus"
- "Bombas de coolant — sequencia de pressurizacao necessaria"
- "MELTDOWN IMINENTE — estabilize todos os sistemas"

### Cenario Fisico

Uma mesa de controle com plataformas retrateis nos tres lados:

```
┌─────────────────────────────────────────────────────────┐
│  [Keypad P1]  [RFID leitor]  [Keypad P2]  [Chaves P3] │
│                     [TM1637 Timer]                      │
│  ┌─────────────────────────────────────────────────┐    │
│  │           NeoPixel Status Bar                   │    │
│  └─────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
         │  ┌──────────────┐   ┌──────────────┐  │
         │  │ Plataforma A │   │ Plataforma B │  │
         │  │ (Alexei)     │   │ (Boris)       │  │
         │  │ Pots +       │   │ SimonSays     │  │
         │  │ Gauges       │   │               │  │
         │  └──────────────┘   └──────────────┘  │
         │  ┌──────────────┐                     │
         │  │ Plataforma C │                     │
         │  │ (Vladislav)  │                     │
         │  │ VintageRadio │                     │
         │  │ LCD + Botoes │                     │
         │  └──────────────┘                     │
```

### Easter Eggs

- Display LCD mostra mensagens em Russo quando ocioso
- Sequencia escondida de botoes no SimonSays toca musica classica
- Se o timer chegar a 60 segundos, alarme sonoro real de meltdown

---

## 7. Workflow de Desenvolvimento

### Visao Geral

8 etapas sequenciais. Cada etapa produz entregaveis testaveis antes de avancar.

```
ETAPA 1: Protoboard I2C
ETAPA 2: Puzzles de Acesso (P1-P3)
ETAPA 3: Posicionamento e Reator (P4-P5)
ETAPA 4: Tampa e Interior (P6-P9)
ETAPA 5: Integracao do Fluxo
ETAPA 6: Audio e Narrativa
ETAPA 7: Playtest e Ajustes
ETAPA 8: Montagem Final na Caixa
```

---

### Etapa 1: Protoboard I2C

**Objetivo:** Validar que todos os componentes I2C comunicam com a PropMaker.

**Passos:**

1. Montar barramento I2C em protoboard com todos os PCF8575, ADS1115, PN532
2. Subir o esboco `LibrarySmokeTest` e verificar scan I2C
3. Confirmar LittleFS montado (4MB)
4. Testar audio: copiar 1 MP3 para LittleFS, chamar `hw.sound().playOnce()`
5. Testar NeoPixel externo: `hw.setLightAll(0xFFFFFF)` pisca branco

**Indicador de conclusao:**
```
[I2C] Found: PCF8575(0x20) PCF8575(0x21) PCF8575(0x22) PCF8575(0x23) PCF8575(0x24)
[I2C] Found: ADS1115(0x48) ADS1115(0x49)
[I2C] Found: PN532(0x24)
[LFS] LittleFS mounted, 3MB free
[Audio] MP3 decoder OK
[Neo] External NeoPixel OK
```

**Teste:** `hw.test()` executa sem erros.

---

### Etapa 2: Puzzles de Acesso (P1-P3)

**Objetivo:** Implementar e testar cada puzzle de acesso individualmente.

#### P1 — KeypadRfidLock

**Passos:**

1. Copiar `examples/KeypadRfidLock` como base
2. Conectar teclado 4x3 ao PCF8575 0x20
3. Conectar PN532 ao barramento I2C
4. Configurar JSON com modo `KEYPAD_AND_RFID`
5. Testar: aproximar cracha -> digitar senha -> rele abre
6. Testar: senha errada -> negado
7. Testar: cracha errado -> negado

**Indicador:** Rele do P1 abre quando cracha + senha corretos.

#### P2 — UniversalController ALL_CONNECTED

**Passos:**

1. Copiar `examples/UniversalController` como base
2. Conectar 3 microswitches ao PCF8575 0x21 (pinos 0,1,2)
3. Configurar JSON modo `ALL_CONNECTED`
4. Testar cada switch individualmente (LED NeoPixel acende quando fecha)
5. Testar: todos fechados -> rele abre

**Indicador:** Rele do P2 abre quando os 3 microswitches estao fechados.

#### P3 — UniversalController LINEAR_SEQUENCE

**Passos:**

1. Copiar `examples/UniversalController` como base (outra instancia)
2. Conectar 3 chaves SPDT ao PCF8575 0x22 (pinos 0,1,2)
3. Configurar JSON modo `LINEAR_SEQUENCE`, sequencia `[2,0,1]`
4. Testar: acionar na ordem 2-0-1 -> rele da porta abre + rele da gaveta abre
5. Testar: errar ordem -> reseta

**Indicador:** Rele da porta P3 + rele da gaveta abrem quando sequencia correta.

---

### Etapa 3: Posicionamento e Reator (P4-P5)

**Objetivo:** Implementar deteccao RFID nas plataformas e ReactorControl.

#### P4 — RFID nas Plataformas

**Passos:**

1. Conectar 3 leitores PN532 (ou RC522) nas bases das plataformas
2. Configurar cada leitor com o UID do tecnico correspondente
3. Testar: aproximar cracha do Alexei na base A -> LED verde acende
4. Testar: aproximar cracha do Alexei na base B -> LED vermelho
5. Testar: todos os 3 nas posicoes corretas -> sinal para ReactorControl

**Indicador:** Quando P4 conclui, o ReactorControl entra em estado Running.

#### P5 — ReactorControl

**Passos:**

1. Conectar 3 pots ao ADS1115 0x48 (canais A0, A1, A2)
2. Implementar logica: cada pot controla um subsistema
3. Testar: ajustar pot 1 para temperatura < 750 -> LED verde
4. Testar: temperatura sobe > 750 -> LED vermelho + alarme
5. Testar: todos os 3 na faixa por 5s -> puzzle resolvido

**Indicador:** Puzzle resolvido -> SimonSays ativa na tampa.

---

### Etapa 4: Tampa e Interior (P6-P9)

**Objetivo:** Implementar SimonSays na tampa, reparo do comunicador e BombTimer.

#### P6 — SimonSays na Tampa

**Passos:**

1. Montar 4 botoes com NeoPixels na face externa da tampa
2. Copiar `examples/SimonSays` como base
3. Configurar sequencia de N rodadas (ex: 5)
4. Testar: repetir sequencia correta -> todas as rodadas -> servo abre tampa
5. Testar: errar sequencia -> reseta rodada

**Indicador:** Servo abre a tampa ao final da ultima rodada.

#### P7 — Comunicador (Custom)

**Passos:**

1. Montar LCD 16x2, 2 botoes, RotaryDial, 2 chaves toggle, NeoPixel
2. Implementar logica de reparo (VintageRadio-like):
   - Etapa 1: inserir placa A (microswitch detecta), LCD confirma
   - Etapa 2: girar chave 1 para posicao correta
   - Etapa 3: inserir placa B, girar chave 2, pressionar teste
3. Apos todas as etapas: acender LED verde, iniciar timer meltdown
4. Testar leitura do RotaryDial: cada pulso conta como digito
5. Implementar logica: digito correto de 4 pulsos = OK
6. Disparar audio MP3 com a dica quando numero correto discado

**Indicador:** Radio consertado -> timer 5 min comeca. Disco correto -> audio toca.

#### P9 — BombTimer

**Passos:**

1. Copiar `examples/BombTimer` como base
2. Configurar 6 fios, sem keypad, timer 5 min
3. A ordem correta dos fios e a mesma da dica do P7
4. Testar: cortar fios na ordem certa -> puzzle resolvido
5. Testar: fio errado -> timer acelera
6. Testar: sacudir a caixa -> timer congela 3s

**Indicador:** Puzzle resolvido = jogo vencido. Timer zero = meldown.

---

### Etapa 5: Integracao do Fluxo

**Objetivo:** Unir todos os puzzles no LogicManager com transicoes corretas.

**Passos:**

1. Criar classe `NuclearEscapeRoom` herdando `LogicManager`
2. Instanciar todos os 9 puzzles
3. Implementar maquina de estados:
   - `stateAcesso`: P1 -> P2 -> P3
   - `statePosicionamento`: P4 (RFID)
   - `stateReator`: P5
   - `stateTampa`: P6
   - `stateInterior`: P7 -> P9
4. Configurar transicoes com `onStateChange()` ou polling no `update()`
5. Testar fluxo completo do inicio ao fim

**Testes:**

| Teste | Entrada | Resultado Esperado |
|-------|---------|-------------------|
| Fluxo feliz | Resolver tudo certo | Jogo vence em < 30 min |
| Erro P3 | Errar sequencia 3x | Puzzle reseta, timer continua |
| Shake durante BombTimer | Sacudir caixa | Timer congela 3s |
| Fio errado | Cortar fio errado | Timer acelera 2x |

**Indicador:** Jogo completo jogavel do inicio ao fim.

---

### Etapa 6: Audio e Narrativa

**Objetivo:** Produzir e integrar todos os efeitos sonoros e mensagens.

**Passos:**

1. Listar todos os eventos que disparam audio:

| Evento | Arquivo | Conteudo |
|--------|---------|----------|
| Puzzle 1 resolvido | `/sfx/alexei_free.mp3` | "Engenheiro Alexei libertado" |
| Puzzle 2 resolvido | `/sfx/boris_free.mp3` | "Tecnico Boris libertado" |
| Puzzle 3 resolvido | `/sfx/vlad_free.mp3` | "Eletricista Vladislav libertado" |
| Puzzle 5 resolvido | `/sfx/reactor_stable.mp3` | "Reator estabilizado" |
| Tampa abre | `/sfx/lid_open.mp3` | Selo pneumatico + click |
| Meltdown inicia | `/sfx/meltdown_start.mp3` | Alarme + "Meltdown em 5 minutos" |
| Dica RotaryDial | `/sfx/hint_wires.mp3` | "Corte azul, vermelho, preto..." |
| Puzzle 9 resolvido | `/sfx/victory.mp3` | "Nucleo desarmado" |
| Game over | `/sfx/meltdown_explosion.mp3` | Explosao |

2. Codificar MP3: 128Kbps, 44KHz, stereo, sem ID3 tags
3. Copiar para `data/sfx/` no LittleFS
4. Configurar cada puzzle para chamar `hw.sound().playOnce()` no evento

**Indicador:** Cada transicao importante tem audio correspondente.

---

### Etapa 7: Playtest e Ajustes

**Objetivo:** Validar tempo, dificuldade e robustez.

**Cenarios de teste:**

| Cenario | O que medir | Alvo |
|---------|-------------|------|
| Jogador experiente | Tempo total | 10-15 min |
| Jogador medio (1a vez) | Tempo total | 20-30 min |
| Jogador iniciante | Tempo total | 30-45 min |
| Erro deliberado | Recuperacao | Puzzle reseta sem crash |
| Queda de energia | Reinicio | Estado persiste? (se LittleFS) |
| Shake intenso | Falso positivo | Acelerometro nao dispara sem motivo |

**Ajustes comuns:**

- **Tolerancia do ReactorControl:** Se muito facil, reduzir faixa de 5% para 3%
- **Rodadas do SimonSays:** Aumentar ou diminuir N
- **Timer do BombTimer:** 5 min pode ser muito ou pouco
- **Ordem dos fios:** Mudar entre partidas para evitar decoracao

**Indicador:** 3 jogadores conseguem completar sem assistencia em < 45 min.

---

### Etapa 8: Montagem Final na Caixa

**Objetivo:** Transferir da protoboard para a caixa definitiva.

#### Diagrama de Montagem (vista explodida)

```
┌──────────────────────────────────────────────────────────────┐
│                        TAMPA (P6)                            │
│   ┌──┐  ┌──┐  ┌──┐  ┌──┐                                   │
│   │B1│  │B2│  │B3│  │B4│  4 botoes SimonSays na tampa       │
│   └──┘  └──┘  └──┘  └──┘                                   │
│                    ┌─────┐                                   │
│                    │servo│  abertura automatica              │
│                    └─────┘                                   │
├──────────────────────────────────────────────────────────────┤
│                     PAINEL SUPERIOR                           │
│                                                              │
│  ┌─────────┐  ┌──────────┐  ┌──────────┐                    │
│  │Keypad 1 │  │  RFID    │  │ Chaves 3 │   P1, P3           │
│  │(P1)     │  │  leitor  │  │ PCF 0x22 │                    │
│  └─────────┘  └──────────┘  └──────────┘                    │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │              NeoPixel Status Bar (48 LEDs)           │    │
│  └──────────────────────────────────────────────────────┘    │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐    │
│  │              TM1637 Timer Global                     │    │
│  └──────────────────────────────────────────────────────┘    │
├──────────────────────────────────────────────────────────────┤
│                    PLATAFORMAS LATERAIS                        │
│                                                              │
│     PLATAFORMA A (esq)     PLATAFORMA B (dir)                │
│  ┌──────────────────┐  ┌──────────────────┐                  │
│  │ 3 pots (P5)      │  │ Microswitches    │  P2              │
│  │ RFID base        │  │ RFID base        │                  │
│  └──────────────────┘  └──────────────────┘                  │
│                                                              │
│     PLATAFORMA C (fundo)                                     │
│  ┌──────────────────┐                                        │
│  │ 3 chaves (P3)    │                                        │
│  │ RFID base        │                                        │
│  └──────────────────┘                                        │
├──────────────────────────────────────────────────────────────┤
│                  INTERIOR DA CAIXA                             │
│                                                              │
│  ┌────────────────────┐  ┌────────────────────┐              │
│  │  COMUNICADOR (P7)  │  │  BOMBTIMER (P9)    │              │
│  │  LCD 16x2          │  │  6 bornes fios     │              │
│  │  RotaryDial        │  │  Display TM1637    │              │
│  │  2 botoes          │  │                    │              │
│  │  2 chaves toggle   │  │                    │              │
│  └────────────────────┘  └────────────────────┘              │
│                                                              │
├──────────────────────────────────────────────────────────────┤
│                  COMPARTIMENTO TECNICO                         │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐  │
│  │  PropMaker RP2040 (fixa com suporte M3)               │  │
│  │  Barramento I2C: PCFs + ADS + PN532s                  │  │
│  │  Fonte 5V 3A (USB-C ou terminal)                      │  │
│  │  Reles: 3 portas + 1 gaveta (modulo relé 4 canais)   │  │
│  │  Servo: saida PWM padrao (pino servo da PropMaker)   │  │
│  └────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

#### Roteamento de Cabos

1. **I2C:** Daisy chain. PropMaker SDA/SCL -> PCF 0x20 -> 0x21 -> ... -> PN532
   - Cabo max 50cm entre dispositivos
   - Pull-ups 4.7K no primeiro e ultimo dispositivo
2. **Microswitches:** Par trançado ate o PCF correspondente. Identificar por cor.
3. **Pots:** 3 vias blindado ate o ADS1115 (sinal + GND + VCC)
4. **NeoPixel:** 3 vias (DATA + 5V + GND). DATA no pino externo NeoPixel da PropMaker
5. **Audio:** Saida I2S diretamente para o alto-falante incluido na PropMaker
6. **Alimentacao:** 5V externo para NeoPixel e reles. 3.3V do barramento I2C

#### Checklist de Montagem

- [ ] Todos os PCFs fixos e conectados ao I2C
- [ ] Scan I2C reconhece todos os dispositivos
- [ ] Reles acionam portas e gaveta corretamente
- [ ] Servo abre a tampa sem obstrucao
- [ ] NeoPixel visivel de todos os angulos
- [ ] RotaryDial com furo no painel do comunicador
- [ ] Bornes do BombTimer acessiveis com a tampa aberta
- [ ] Fonte 5V 3A testada com todos os componentes ligados
- [ ] Cabos organizados com presilhas, sem risco de pinçamento
- [ ] Tampa fecha sem pressionar componentes internos
- [ ] Jogo completo jogavel sem acesso ao compartimento tecnico

---

## 8. Referencias

- **Projeto original:** Nicolau dos Brinquedos (2019) — `Escape_Room_Nuclear_Reactor/`
- **Biblioteca:** PropMaker Escape Room (`propmaker_escaperoom/`)
- **Placa:** Adafruit PropMaker Feather RP2040
- **Exemplos base:** KeypadRfidLock, KeypadCodeEntry, UniversalController, SimonSays, VintageRadio, ReactorControl
