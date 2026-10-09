# Cyclone — Algoritmo do Jogo

## Visão Geral

Jogo de timing. Uma luz percorre um anel de NeoPixels em velocidade crescente. O jogador pressiona SELECT para parar a luz no setor verde (alvo). Acertar N rounds resolve o puzzle.

## Configuração (JSON: cyclone.json)

- **Alvo**: targetSize (largura do setor verde, 1-N), roundsToWin (acertos necessários)
- **Velocidade**: startSpeed, speedIncrement (aumenta a cada acerto), maxSpeed
- **Hardware**: lockPin, neoCount, volume
- **Áudio**: sfxTick (movimento), sfxHit (acerto), sfxMiss (erro), sfxSolved

## Estados

IDLE → RUNNING → SOLVED → IDLE (auto-reset 8s)

## Fluxo do Jogo

1. IDLE: NeoPixel azul pulsante. SELECT inicia.
2. RUNNING:
   a. Posição do setor alvo sorteada aleatoriamente
   b. Luz percorre o anel a cada stepDelay = 100/speed ms
   c. Posição atual avança 1 LED por step
   d. SELECT pressionado → verifica se posição está dentro do setor alvo
3. Acerto: incrementa round, aumenta speed, sorteia novo alvo
4. Erro: mantém round atual, sorteia novo alvo
5. Todos rounds completos → SOLVED (destrava, som de vitória)

## Feedback

- NeoPixel: alvo=verde dim, luz atual=branco brilhante, acerto=verde bright, erro=vermelho
- Áudio: tick a cada movimento, som de acerto/erro
- Display: não usado (apenas NeoPixel)

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
