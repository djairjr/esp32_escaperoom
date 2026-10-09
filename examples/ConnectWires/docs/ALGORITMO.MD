# ConnectWires — Algoritmo do Jogo

## Visão Geral

Puzzle de conexão de fios. O jogador deve conectar pares de soquetes com fios banana. Um voltímetro (ADS1115) lê a tensão em cada par para verificar conexão correta. Resolve quando todos os pares estiverem conectados corretamente.

## Configuração (JSON: connectwires.json)

- **Soquetes**: numSockets (quantidade total), connectionPairs (pares de soquetes que formam conexões corretas)
- **Hardware**: lockPin, neoCount, volume
- **Áudio**: sfxConnect, sfxDisconnect, sfxSolved, sfxError

## Estados

IDLE → RUNNING → SOLVED → IDLE (reset por botão)

## Lógica de Conexão

1. Cada soquete é ligado a um pino digital no PCF8575
2. Para testar um par (A, B):
   - Coloca pino A como OUTPUT LOW
   - Lê pino B como INPUT_PULLUP
   - Se B lê LOW: A e B estão conectados (circuito fechado)
3. Verifica todos os pares configurados
4. Se todos os pares corretos estão conectados E nenhum par incorreto: SOLVED

## Modos de Validação

- **ALL_CONNECTED**: todos os pares corretos devem estar conectados
- **ONLY_CORRECT**: apenas pares corretos podem estar conectados (conexão extra = erro)
- **EXACT_MATCH**: conexões ativas devem corresponder exatamente aos pares configurados

## Feedback

- NeoPixel: verde para conexões corretas, vermelho para incorretas
- Áudio: beep ao conectar, som de erro ao conectar errado
- Display HT16K33: número de conexões restantes

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
