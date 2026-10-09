# STATUS - RadioClock

## O que foi feito
- Puzzle de rádio com dial de frequências no NeoPixel
- Jogador usa SELECT para percorrer N frequências
- Áudio toca estática em frequências erradas, sinal limpo na certa
- Display HT16K33 mostra frequência atual ("F-X")
- 3 rounds para vencer, cada round tem uma frequência alvo diferente
- Efeitos: branco no centro, trail, verde pulsante ao acertar
- Configuração via /radioClock.json

## Status de compilação
Sucesso: 253216 bytes (3%), 15912 bytes RAM (6%)

## Decisões pendentes
1. **frequencyCount = 12:** Quantas frequências no dial?
2. **targetFrequencies = [3, 8, 5]:** Quais frequências-alvo para cada round?
3. **roundsToWin = 3:** Número ideal de acertos para resolver?
4. **neoCount = 24:** Usar anel completo ou menos LEDs?
5. **Áudio static vs signal:** Efeito de "estática" reproduzível com MP3? Usar tom puro?
6. **lockPin = 10:** Pino correto para a placa?
7. **Criar assets de áudio:** static.mp3, signal.mp3, solved.mp3, select.mp3

## Configuração
examples/RadioClock/data/radioClock.json

## Assets necessários
- /sfx/static.mp3 (estática em frequência errada)
- /sfx/signal.mp3 (sinal limpo ao acertar)
- /sfx/solved.mp3 (puzzle resolvido)
- /sfx/select.mp3 (ao mudar de frequência)
