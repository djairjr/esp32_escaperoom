# SimonSays — Algoritmo do Jogo

## Visão Geral

Jogo de memória estilo Simon Says. O sistema toca uma sequência de cores/sons (4 botões coloridos). O jogador deve repetir a sequência na ordem correta. A cada round, a sequência aumenta em 1 item.

## Configuração (JSON: simon.json)

- **Botões**: 4 botões com cores (Vermelho, Azul, Verde, Amarelo)
- **maxRounds**: rounds máximos (ou infinito até erro)
- **tempoEntreNotas**: intervalo entre notas da sequência
- **Hardware**: lockPin, neoCount, volume
- **Áudio**: 4 tons por cor, sfxError, sfxRound, sfxSolved

## Estados

IDLE → SHOWING → WAITING → CORRECT / WRONG → SOLVED / GAMEOVER

## Fluxo

1. IDLE: START inicia
2. SHOWING: sistema toca sequência (luz + som por cor)
3. WAITING: jogador repete pressionando botões
4. Se acertar sequência completa → próximo round (sequência +1)
5. Se errar → GAMEOVER ou reinicia (dependendo de config)
6. Se completar maxRounds → SOLVED

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
