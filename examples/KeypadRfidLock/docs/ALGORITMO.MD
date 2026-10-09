# KeypadRfidLock — Algoritmo do Jogo

## Visão Geral

Fechadura com dupla autenticação: código via keypad OU tag RFID. O jogador pode destravar usando qualquer um dos métodos.

## Configuração (JSON: rfidlock.json)

- **Modos**: KEYPAD, RFID, BOTH
- **code**: código de 4 dígitos
- **rfidTags**: UIDs de tags autorizadas
- **Hardware**: lockPin, neoCount, volume

## Estados

IDLE → AUTHENTICATING → SOLVED → IDLE

## Fluxo

1. IDLE: aguardando input
2. Keypad: igual ao KeypadCodeEntry
3. RFID: leitura de tag, compara UID com lista autorizada
4. Ambos os métodos podem ser usados simultaneamente no modo BOTH
5. Sucesso → destrava por timeout configurável, depois re-trava

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
