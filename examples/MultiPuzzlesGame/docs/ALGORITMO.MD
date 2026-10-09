# MultiPuzzlesGame — Algoritmo do Jogo

## Visão Geral

Jogo completo de escape room com múltiplos puzzles (até 9) organizados em níveis (até 5). Usa o sistema de Game Levels do LogicManager. Puzzles podem rodar em paralelo (todos simultâneos) ou sequencial (um após o outro).

## Configuração (JSON: game.json)

- **Níveis**: array de GameLevelConfig (puzzleIds, mode PARALLEL/SEQUENCE)
- **Timer global**: countdown para a sala completa

## Estados

WAITING → LEVEL_1 → LEVEL_2 → ... → COMPLETE

## Fluxo

1. Nível 1 ativo: puzzles rodam conforme modo (PARALLEL ou SEQUENCE)
2. Quando todos puzzles do nível são resolvidos → próximo nível
3. SEQUENCE: puzzles ativados um por um, próximo só após atual resolver
4. PARALLEL: todos puzzles do nível rodam simultaneamente
5. Último nível completo → COMPLETE (destrava porta final)

## Hardware típico

- 5x PCF8575 (40 entradas + 40 saídas)
- 2x ADS1115 (8 canais analógicos)
- 1x PCA9685 (16 canais PWM)
- 1x PN532 (RFID/NFC)
- 1x strip NeoPixel 48 LEDs
- Áudio I2S

## AP Mode + WiFi

Portal web mostra progresso de todos os níveis e puzzles.
