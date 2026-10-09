# VintageRadio — Algoritmo do Jogo

## Visão Geral

Rádio vintage com múltiplos modos de interação: sintonia via rotary encoder, leitura RFID para autenticação de "estações", e áudio MP3 simulando transmissões de rádio antigas.

## Configuração (JSON: vintage.json)

- **Entrada**: Rotary Encoder (sintonia), RFID (autenticação)
- **Modos**: TUNE (sintonizar frequência), AUTH (autenticar com tag)
- **Hardware**: lockPin, neoCount, volume
- **Áudio**: sfxStatic, sfxTune, sfxStation, sfxSolved

## Estados

IDLE → TUNING → AUTHENTICATED → SOLVED

## Fluxo

1. IDLE: rádio desligado
2. TUNING: rotary encoder ajusta "frequência"
   - NeoPixel mostra posição no dial
   - Áudio toca estática entre frequências
3. AUTHENTICATED: tag RFID correta apresentada na frequência certa
4. SOLVED: destrava

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
