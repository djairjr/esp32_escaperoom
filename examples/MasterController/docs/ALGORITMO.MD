# MasterController — Algoritmo do Jogo

## Visão Geral

Controlador central da sala de escape. Gerencia o timer global da sala, controla relés (porta principal, luzes, fumaça, auxiliares) e coordena os puzzles via rede RS485.

## Configuração

- **masterId**: identificador na rede RS485
- **totalGameTime**: tempo total da sala em segundos
- **Relés**: MAIN_DOOR, LIGHTS, SMOKE_GEN, AUX_1..AUX_4

## Estados

WAITING → RUNNING → COMPLETE/EXPIRED

## Fluxo

1. WAITING: aguardando comando para iniciar
2. RUNNING:
   - Timer global decrementando
   - Monitora puzzles via RS485 (StatusReport)
   - GameMaster pode abrir portas via serial ('o') ou resetar timer ('r')
3. Timer zera → abre porta principal, desliga luzes
4. Todos puzzles resolvidos → COMPLETE

## Relés

- MAIN_DOOR: aberta quando jogo termina ou GameMaster comanda
- LIGHTS: controle de iluminação da sala
- SMOKE_GEN: gerador de fumaça para atmosfera
- AUX_1..4: dispositivos auxiliares configuráveis

## AP Mode + WiFi

Portal web permite controle completo da sala: iniciar, pausar, resetar, abrir portas, ajustar timer.
