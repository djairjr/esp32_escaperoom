# RadioClock — Algoritmo do Jogo

## Visão Geral

Sintonizador de rádio vintage. O jogador pressiona SELECT para percorrer "frequências" exibidas no display e em um anel de NeoPixels. Áudio toca estática em frequências erradas e sinal limpo na correta. Acertar 3 frequências-alvo resolve o puzzle.

## Configuração (JSON: radioClock.json)

- **frequencyCount**: quantas posições no dial
- **targetFrequencies[3]**: posições-alvo para cada round
- **roundsToWin**: rounds necessários (padrão 3)
- **Áudio**: sfxStatic, sfxSignal, sfxSolved, sfxSelect

## Estados

IDLE → RUNNING → SOLVED → IDLE (auto-reset 8s)

## Fluxo

1. IDLE: NeoPixel azul dim pulsante
2. RUNNING:
   - SELECT avança para próxima frequência
   - NeoPixel mostra posição atual (branco) com trail
   - Display HT16K33 mostra "F--X" (X = frequência atual)
3. Se frequência atual == targetFrequencies[round]:
   - Toca sfxSignal
   - NeoPixel verde pulsante
   - Display "GOOd"
   - Incrementa round
4. Se não: toca sfxStatic
5. Todos rounds completos → SOLVED

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
