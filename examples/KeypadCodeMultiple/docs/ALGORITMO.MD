# KeypadCodeMultiple — Algoritmo do Jogo

## Visão Geral

Versão multi-código do KeypadCodeEntry. Aceita múltiplos códigos configurados via JSON. Qualquer código válido resolve o puzzle. Útil para múltiplos jogadores ou múltiplas soluções.

## Configuração (JSON: keypadmulti.json)

- **codes**: array de até 8 códigos de 4 dígitos
- **codeCount**: quantos códigos estão ativos
- **Hardware**: lockPin, neoCount, volume

## Estados

IDLE → ENTERING → SOLVED/ERROR → IDLE

## Fluxo

Igual ao KeypadCodeEntry, mas verifica contra todos os códigos no array. Qualquer match resolve.

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
