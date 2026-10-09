# UniversalController — Algoritmo do Jogo

## Visão Geral

Puzzle universal configurável via JSON. Suporta múltiplos modos de operação: sensor único, múltiplos sensores, sequência, RFID, keypad, etc. O comportamento é inteiramente definido pelo arquivo de configuração.

## Configuração (JSON: universal.json)

- **mode**: tipo de puzzle (SINGLE_SENSOR, MULTI_SENSOR, SEQUENCE, RFID, KEYPAD, etc.)
- **inputs**: pinos de entrada e seus papéis
- **outputs**: pinos de saída (relés, NeoPixel)
- **timing**: timeouts, delays
- **Áudio**: sfx para cada evento

## Estados

IDLE → RUNNING → SOLVED / ERROR → IDLE

## Modos Suportados

1. **SINGLE_SENSOR**: um sensor deve ser ativado
2. **MULTI_SENSOR**: múltiplos sensores em qualquer ordem
3. **SEQUENCE**: sensores em ordem específica
4. **RFID**: tag RFID específica
5. **KEYPAD**: código numérico
6. **TOGGLE**: alternar estado de sensores
7. **TIMER**: aguardar tempo específico

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
