# Manual do GameTimerPro - Bomba para Escape Room

Um guia completo para donos de sala, game masters e jogadores.

---

## 1. Visao Geral

### O que e este dispositivo?

O **GameTimerPro** e uma **bomba temporizada eletronica** projetada para jogos de Escape Room. Ele simula uma bomba real com contagem regressiva, fios para cortar, teclado numérico e efeitos sonoros e luminosos imersivos.

### Como funciona no jogo?

- Os jogadores encontram a "bomba" dentro da sala
- O display mostra o tempo restante em minutos e segundos
- Para vencer, eles precisam **desarmar a bomba antes que o tempo acabe**
- O desarme pode ser feito de varias formas: digitando um código secreto, cortando fios específicos ou seguindo uma sequência de fios em ordem correta

---

## 2. Componentes do Jogo

Estes sao os elementos que os jogadores interagem durante o jogo:

| Componente | Descricao |
|------------|-----------|
| **Display digital** | Mostra o tempo restante no formato MM:SS (minutos e segundos) |
| **Teclado numérico** | Teclas de 0 a 9 para digitar códigos de desarme |
| **Fios coloridos** | Quatro fios que podem ser cortados pelos jogadores |
| **Botao DET (Detonar)** | Botao vermelho que inicia a contagem regressiva |
| **Botões de navegação** | Botões esquerda, direita e seletor (para configuração) |
| **Luzes LED** | Faixa de luzes coloridas que indicam o estado da bomba |
| **Alto-falante** | Emite sons de tick, explosão, erro e sucesso |

---

## 3. Configuração Inicial (para o dono da sala)

### Como ajustar o tempo da contagem regressiva

1. **Ligue o dispositivo** - Ele mostrará o tempo atual no display
2. **Use os botões laterais** para ajustar:
   - Botão da **esquerda**: diminui o tempo
   - Botão da **direita**: aumenta o tempo
   - **Aperte rapidamente**: muda em 1 segundo
   - **Segure pressionado**: muda em 5 segundos continuamente
3. **Segure o botao SELECT por 1 segundo** para salvar

> **Dica**: O tempo minimo é 10 segundos e o maximo é 99 minutos e 59 segundos.

### Como entrar no menu de configuração

1. Com o dispositivo ligado, **aperte rapidamente o botão SELECT** (solte em menos de 1 segundo)
2. O display mostrará o primeiro item do menu: **BRIG**
3. Use os botões esquerda/direita para navegar entre as opções
4. Para entrar em uma opção, **aperte SELECT rapidamente**
5. Para salvar e sair, **segure SELECT por 1 segundo**

### Explicação de cada opção do menu

#### BRIG (Brilho da tela)
- Ajusta o quão clara a tela fica
- Valores de 0 (apagado) a 15 (maximo)
- Recomendado: 12-15 para salas bem iluminadas

#### VOL (Volume do som)
- Ajusta o volume dos efeitos sonoros
- Valores de 0 (mudo) a 15 (maximo)
- Recomendado: 10-12 para salas de tamanho medio

#### TICK (Som de tick)
- Liga ou desliga o som de "tick" a cada segundo
- **ON**: faz tick-tock durante a contagem
- **OFF**: silencioso, apenas sons de eventos

#### SHAK (Modo do acelerômetro)
O acelerômetro detecta quando alguém move ou sacode o dispositivo. Escolha uma opção:

| Opção | Significado | Efeito no jogo |
|-------|-------------|----------------|
| **OFF** | Desligado | O acelerômetro não faz nada |
| **STRT** | Start (Iniciar) | A contagem começa quando alguém mover a bomba |
| **FAST** | Velocidade 2x | Sacudir a bomba faz o tempo passar 2x mais rapido |
| **PAUS** | Pausa | Sacudir a bomba congela o relogio por 2 segundos (e emite um erro) |
| **BOOM** | Explosão imediata | Sacudir a bomba com força causa detonação instantanea |

> **Dica**: Modo STRT é otimo para surpreender jogadores que pegam a bomba sem querer!

#### NET (Comunicação com outros dispositivos)
- **ON**: A bomba se comunica com outros puzzles via cabo de rede
- **OFF**: Funciona sozinha, sem conexao
- Use ON se tiver outros dispositivos sincronizados na sala

#### WIRE (Configuração dos fios)
Acesse um submenu com 5 opções para configurar os fios:

**ORDR** (Ordem): Define a sequencia de corte quando no modo "Ordem de Fios"
- Digite 4 digitos de 1 a 4 (exemplo: 2413 = fio 2, depois 4, depois 1, depois 3)
- Cada numero deve ser unico (nao repita)

**DET** (Fio Detonador): Qual fio explode a bomba se cortado?
- **NONE**: Nenhum fio explode
- **1/2/3/4**: Fio especifico explode
- **RAND**: Sorteia aleatoriamente a cada jogo

**DEF** (Fio Desarmador): Qual fio desarma a bomba se cortado?
- **NONE**: Nenhum fio desarma
- **1/2/3/4**: Fio especifico desarma
- **RAND**: Sorteia aleatoriamente a cada jogo

**SPD** (Fio Acelerador): Qual fio acelera a contagem se cortado?
- **NONE**: Nenhum fio acelera
- **1/2/3/4**: Fio especifico acelera para 2x
- **RAND**: Sorteia aleatoriamente

**PAUS** (Fio de Pausa): Qual fio pausa a contagem por 2 segundos?
- **NONE**: Nenhum fio pausa
- **1/2/3/4**: Fio especifico pausa
- **RAND**: Sorteia aleatoriamente

> **Importante**: Um fio nao pode ter duas funcoes ao mesmo tempo. O sistema avisa se houver conflito.

#### CODE (Código de desarme)
- Define a senha numérica de 4 digitos para desarmar a bomba
- Ao selecionar, digite os 4 numeros no teclado
- O código padrao é **1234**

#### RELE (Tempos dos gatilhos)
Configura por quanto tempo as saidas ficam ativadas:

**TDET** (Tempo de Detonação): Quanto tempo o gatilho de explosao fica ligado (1 a 10 segundos)

**TDEF** (Tempo de Desarme): Quanto tempo o gatilho de sucesso fica ligado (1 a 10 segundos)

---

## 4. Como Jogar

### Para o Game Master - Iniciando o jogo

1. **Configure o tempo** desejado para a partida
2. **Escolha o modo de desarme** (código, fio específico ou ordem de fios)
3. **Explique as regras** aos jogadores antes de eles entrarem
4. **Aperte o botão DET** (vermelho) para iniciar a contagem quando estiverem prontos

> Alternativa: No modo **SHAK = STRT**, a contagem inicia automaticamente quando alguém mover a bomba!

### Para os jogadores - O que acontece durante o jogo

**Ao iniciar:**
- O display mostra o tempo começando a diminuir
- As luzes ficam vermelhas e pulsam
- Se o TICK estiver ligado, ouve-se um "tick" a cada segundo
- O som **armed.mp3** toca indicando que a bomba esta ativa

**Durante a contagem:**
- O tempo diminui em segundos
- As luzes piscam mais rapido quando falta pouco tempo
- Se o modo SHAK estiver em FAST, sacudir a bomba acelera o tempo!

### Como desarmar a bomba

Existem **4 formas** de desarmar, dependendo da configuração:

#### 1. Modo CÓDIGO (CODE)

Ao escolher o modo de desarme por código (CODE), o jogador deve inserir um código de 4 dígitos no keypad.

**Sistema de Tentativas:**
- O jogador possui **3 tentativas** para acertar o código
- Se o código estiver correto: a bomba é desarmada
- Se o código estiver errado:
  - **1ª a 3ª tentativa:** O display mostra "_Err" e o jogador pode tentar novamente
  - **4ª tentativa:** A bomba explode imediatamente!

**Exemplo de uso:**
1. Código correto: 1234
2. Jogador digita: 1111 → Display mostra "_Err" (tentativa 1/3)
3. Jogador digita: 2222 → Display mostra "_Err" (tentativa 2/3)
4. Jogador digita: 3333 → Display mostra "_Err" (tentativa 3/3)
5. Jogador digita: 4444 → **EXPLOSÃO!**

**Dica:** Preste atenção ao código configurado no menu antes de iniciar o jogo!

#### 2. Modo FIO ESPECÍFICO (WIRE + DEF)
- Corte o fio correto (aquele configurado como DEF)
- Se cortar o fio errado... BOOM!

#### 3. Modo ORDEM DE FIOS (WIRE + ORDR)
- Corte os fios na sequencia exata configurada
- Exemplo: se a ordem for 2413, voce deve cortar o fio 2 primeiro, depois o 4, depois o 1, e por fim o 3
- Se errar a ordem: **EXPLODE!**

#### 4. Modo REDE (NET)
- A bomba se desarma automaticamente quando outros puzzles na sala forem resolvidos
- Requer conexão com outros dispositivos

### O que acontece se o tempo acabar

Quando o display chega em **00:00**:

1. O display mostra **"boom"**
2. O som de explosão toca bem alto
3. As luzes fazem um efeito de explosão (amarelo, laranja, vermelho)
4. O gatilho de saida é ativado (pode acionar fumaça, luzes, outro puzzle, etc.)
5. O jogo termina - os jogadores perderam!

### O que acontece ao desarmar

Quando a bomba é desarmada com sucesso:

1. O display mostra o código de desarme (4 numeros)
2. O som de sucesso toca
3. As luzes ficam verdes e brilhantes
4. O gatilho de saida é ativado (pode destrancar uma porta, liberar um objeto, etc.)
5. Os jogadores venceram este puzzle!

### Entendendo o sistema de fios

Os 4 fios podem ter diferentes efeitos quando cortados:

| Tipo de Fio | Efeito ao cortar |
|-------------|------------------|
| **Fio Detonador (DET)** | **EXPLODE IMEDIATAMENTE!** |
| **Fio Desarmador (DEF)** | Desarma a bomba - você vence! |
| **Fio Acelerador (SPD)** | O tempo passa 2x mais rapido! |
| **Fio de Pausa (PAUS)** | O relógio para por 2 segundos (ganha tempo) |
| **Fio Normal** | Sem efeito especial |

> **Dica para jogadores**: Cuidado! Cortar todos os fios de uma vez é uma péssima ideia... provavelmente um deles explode!

---

## 5. Dicas para o Game Master

### Como incorporar no enredo da sala

**Ideias de histórias:**
- **Sabotagem corporativa**: Os jogadores são espiões que precisam desarmar uma bomba na sede de uma empresa corrupta
- **Laboratório químico**: Um cientista louco armou um dispositivo - desarme antes que o veneno seja liberado!
- **Resgate no trem**: Uma bomba na linha do trem - desarme para salvar os passageiros
- **Casa mal-assombrada**: Um "espírito" deixou uma armadilha temporizada

**Pistas para dar aos jogadores:**
- Deixe um bilhete com parte do código (ex: "O código começa com 12...")
- Esconda um diagrama mostrando qual fio cortar
- Use outro puzzle para revelar a ordem dos fios
- Deixe um gravador de áudio com instruções embaralhadas

### Níveis de dificuldade

**Fácil (para iniciantes):**
- Tempo: 15-20 minutos
- Modo: Código simples (1234 ou código escrito em algum lugar)
- Fios: Apenas o DEF definido, outros sem função
- SHAK: OFF

**Médio (padrão):**
- Tempo: 10-15 minutos
- Modo: Fio específico (RAND - sorteado aleatoriamente)
- Fios: DET e DEF ativos
- SHAK: FAST ou PAUS

**Difícil (para experientes):**
- Tempo: 8-10 minutos
- Modo: Ordem de fios (ORDR)
- Fios: Todos com funções diferentes
- SHAK: BOOM (sacudir explode!)
- Sem som TICK (menos tensão sonora)

**Expert (desafio máximo):**
- Tempo: 5-8 minutos
- Modo: Combinação de código + ordem de fios
- Fios: Todos sorteados aleatoriamente (ninguém sabe qual é qual!)
- SHAK: FAST + NET habilitado
- Múltiplos dispositivos sincronizados

### Comportamento esperado dos jogadores

**O Cauteloso**: Tenta analisar tudo antes de agir. Dê a ele pistas no caminho.

**O Impulsivo**: Quer cortar fios imediatamente. O modo RAND é perfeito para surpreendê-lo!

**O Técnico**: Tenta entender a lógica. Modos complexos como ORDR desafiam esse tipo.

**O Líder**: Organiza a equipe. Configure puzzles em rede para forçar cooperação.

---

## 6. Solução de Problemas Simples

### O display não acende

1. Verifique se o cabo de energia esta conectado
2. Verifique se o display esta bem encaixado nos conectores
3. Tente ajustar o brilho no menu (opção BRIG)
4. Desligue e ligue novamente

### Não há som

1. Verifique se o volume não esta em ZERO (menu VOL)
2. Verifique se os arquivos de som estão carregados no dispositivo
3. Verifique se o alto-falante esta conectado corretamente
4. Tente reiniciar o dispositivo

### Como resetar o dispositivo

Para voltar todas as configurações ao padrão de fábrica:

1. Desligue o dispositivo
2. Ligue segurando o botão SELECT pressionado
3. Aguarde o som de inicialização
4. Todas as configurações voltaram ao padrao:
   - Tempo: 10 minutos (600 segundos)
   - Código: 1234
   - Volume: 50%
   - Brilho: Maximo
   - Todos os modos especiais desligados

### A bomba não inicia a contagem

1. Verifique se você apertou o botão DET (vermelho)
2. Se estiver no modo SHAK=STRT, tente mover/sacudir levemente a bomba
3. Verifique se já não esta em contagem (olhe se os dois pontos do display estão piscando)
4. Reinicie o dispositivo e tente novamente

### Os fios não funcionam

1. Verifique se os fios estão bem conectados
2. Verifique se você configurou algum fio com função (DET, DEF, etc.)
3. No menu WIRE, verifique se os valores estão corretos
4. Modo RAND só sorteia quando a contagem inicia - o fio muda a cada jogo!

---

## 7. Checklist de Áudio

Certifique-se de que estes arquivos estão no dispositivo antes de usar:

| Arquivo de Som | Quando Toca | Obrigatório? |
|----------------|-------------|--------------|
| `armed.mp3` | Quando a contagem regressiva inicia | **SIM** |
| `tick.mp3` | A cada segundo (se TICK ligado) | Nao |
| `boom.mp3` | Quando a bomba explode | **SIM** |
| `sucess.mp3` | Quando a bomba é desarmada | **SIM** |
| `error.mp3` | Quando digitado código errado ou shake no modo PAUS | Recomendado |
| `keypress.mp3` | Ao pressionar botoes no menu | Recomendado |

### Formato correto dos arquivos de som

Para garantir que os sons funcionem perfeitamente:

- **Formato**: MP3
- **Taxa de bits**: 128 kbps
- **Frequência**: 44.1 kHz (44100 Hz)
- **Canais**: Estéreo (2 canais)
- **IMPORTANTE**: Sem tags ID3 (metadata)

**Como converter seus arquivos de audio:**

Se tiver um arquivo MP3 e precisar ajustar:

```
Use o programa ffmpeg com estes comandos:
ffmpeg -i seu_arquivo.mp3 -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 arquivo_pronto.mp3
```

---

## Resumo Rapido

### Controles Basicos

| Ação | Como Fazer |
|------|------------|
| Ligar/Desligar | Conecte/desconecte a energia |
| Iniciar jogo | Aperte o botão DET (vermelho) ou mova a bomba (modo STRT) |
| Ajustar tempo | Botões esquerda/direita no menu principal |
| Entrar no menu | Aperte SELECT rapidamente |
| Salvar configuração | Segure SELECT por 1 segundo |
| Desarmar (modo CODE) | Digite 4 numeros (3 tentativas) |
| Desarmar (modo WIRE) | Corte o fio correto |
| Reiniciar após jogo | Aperte DET novamente |

### Estados das Luzes LED

| Cor/Efeito | Significado |
|------------|-------------|
| **Branco suave** | Aguardando (idle) |
| **Vermelho pulsante** | Contagem em andamento |
| **Vermelho piscando rapido** | Velocidade 2x ativada |
| **Verde brilhante** | Bomba desarmada! |
| **Amarelo/Laranja/Vermelho** | EXPLODIU! |
| **Apagado** | Pausa de 2 segundos |

---

**Bom jogo! Que os jogadores tenham nervos de aço...**

*Manual GameTimerPro - Versão 1.0 para biblioteca PropMaker EscapeRoom*
