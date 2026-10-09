# KeypadCodeEntry — Algoritmo do Jogo

## Visão Geral

Puzzle de código numérico. O jogador insere um código de 4 dígitos via teclado matricial. Se correto, destrava. Configurável via JSON.

## Configuração (JSON: keypad.json)

- **code**: código de 4 dígitos
- **maxAttempts**: tentativas máximas
- **Hardware**: lockPin, neoCount, volume

## Estados

IDLE → ENTERING → CHECKING → SOLVED/ERROR → IDLE

## Fluxo

1. IDLE: aguardando primeira tecla
2. Tecla pressionada: entra em modo ENTERING
3. 4 dígitos acumulados: verifica código
4. Correto → SOLVED (destrava, áudio de sucesso)
5. Errado → incrementa tentativa. Se excedeu maxAttempts → ERROR. Senão, volta a ENTERING
6. `*` ou `#`: limpa entrada

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
