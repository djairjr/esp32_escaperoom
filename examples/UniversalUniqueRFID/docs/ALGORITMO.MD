# UniversalUniqueRFID — Algoritmo do Jogo

## Visão Geral

Puzzle RFID de aprendizagem. Qualquer tag RFID pode ser programada como "chave". O GameMaster ou jogador apresenta uma tag, o sistema a memoriza, e a partir daí apenas essa tag resolve o puzzle.

## Configuração (JSON: unique_rfid.json)

- **Modos**: LEARN (aprender tag), PLAY (jogar), RESET (limpar)
- **Hardware**: lockPin, neoCount, volume

## Estados

IDLE → LEARNING → READY → AUTHENTICATING → SOLVED → IDLE

## Fluxo

1. LEARNING: apresenta tag → sistema memoriza UID → READY
2. READY: aguardando jogador
3. AUTHENTICATING: tag apresentada → compara com UID memorizado
4. Match → SOLVED
5. No match → ignora

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
