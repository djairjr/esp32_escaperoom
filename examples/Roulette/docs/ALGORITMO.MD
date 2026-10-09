# Roulette — Algoritmo do Jogo

## Visão Geral

Roleta de luzes. Uma luz percorre um anel de NeoPixels. O jogador pressiona SELECT para parar a roleta. Se parar no setor vencedor, resolve o puzzle. Similar ao Cyclone mas com desaceleração simulada.

## Configuração (JSON: roulette.json)

- **setores**: numSetores, setorVencedor
- **Hardware**: lockPin, neoCount, volume

## Estados

IDLE → SPINNING → DECELERATING → SOLVED / MISS → IDLE

## Fluxo

1. IDLE: luz parada. SELECT inicia.
2. SPINNING: luz percorre anel em velocidade máxima
3. DECELERATING: SELECT pressionado → velocidade reduz gradualmente até parar
4. Posição final == setorVencedor → SOLVED
5. Posição final != setorVencedor → MISS (tenta novamente)

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
