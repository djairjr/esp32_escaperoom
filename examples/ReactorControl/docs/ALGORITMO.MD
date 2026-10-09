# ReactorControl — Algoritmo do Jogo

## Visão Geral

Painel de controle de reator nuclear. O jogador deve manter temperatura e pressão dentro de limites seguros usando botões de controle. Se os parâmetros saírem dos limites ou o jogador falhar na confirmação (pressionar dois botões simultaneamente), o reator "explode" (puzzle falha).

## Configuração (JSON: reactor.json)

- **Limites**: maxTemperature, maxPressure
- **Taxas**: tempRiseRate, pressureRiseRate, coolingRate
- **Hardware**: lockPin, neoCount, volume
- **Áudio**: sfxAlarm, sfxVent, sfxScram, sfxStable

## Estados

IDLE → RUNNING → STABLE / WARNING / CRITICAL → SOLVED / MELTDOWN

## Fluxo

1. IDLE: aguardando início
2. RUNNING:
   - Temperatura e pressão sobem continuamente
   - Botão ESQUERDO: reduz temperatura (ventilação)
   - Botão DIREITO: reduz pressão (válvula)
   - Ambos simultâneos: SCRAM (desligamento de emergência)
3. STABLE: parâmetros dentro dos limites
4. WARNING: um parâmetro excedeu 80%
5. CRITICAL: parâmetro excedeu limite → timer de confirmação
   - Jogador deve pressionar ambos os botões em X segundos
   - Falha → MELTDOWN
6. Tempo total decorrido sem meltdown → SOLVED

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
