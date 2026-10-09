# UniversalControllerMulti — Algoritmo do Jogo

## Visão Geral

Múltiplos puzzles UniversalController rodando em uma única placa. Cada puzzle tem seu próprio JSON de configuração. O LogicManager gerencia a orquestração entre eles.

## Configuração

- **puzzles[]**: array de configurações de puzzles
- **gameMode**: PARALLEL (todos simultâneos) ou SEQUENTIAL (um após outro)

## Estados

IDLE → RUNNING → SOLVED (todos completos)

## Fluxo

1. PARALLEL: todos puzzles iniciam juntos, resolve quando todos completarem
2. SEQUENTIAL: puzzle N+1 só inicia após puzzle N resolver
3. Cada puzzle individual segue a lógica do UniversalController

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
