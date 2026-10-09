# STATUS - Roulette

## O que foi feito
- Puzzle Roleta de Sorte com anel NeoPixel
- Modelo físico de desaceleração: velocidade * friction a cada tick
- 3 giros para vencer, setor vencedor escolhido aleatoriamente
- Efeito de trail (3 LEDs atrás do corredor)
- Setores adjacentes também vencem se winSectorCount > 1
- Botão SELECT para girar
- Áudio: spin, win, lose, solved
- Configuração via /roulette.json

## Status de compilação
Sucesso: 252760 bytes (3%), 15928 bytes RAM (6%)

## Decisões pendentes
1. **spinSpeed (8.0), friction (0.97), minSpeed (0.1):** Valores adequados? A roleta gira tempo suficiente?
2. **winSectorCount = 1:** Apenas 1 LED vencedor ou vários adjacentes?
3. **3 giros para vencer:** Número ideal? Mudar para 1 ou 5?
4. **Anel NeoPixel de 24 LEDs:** Configuração adequada ou usar menos/mais LEDs?
5. **Efeito visual idle:** Atualmente pulsar verde no setor vencedor. Revela a posição?
6. **Criar assets de áudio:** spin.mp3, win.mp3, lose.mp3, solved.mp3

## Configuração
examples/Roulette/data/roulette.json

## Assets necessários
- /sfx/spin.mp3 (toca ao girar)
- /sfx/win.mp3 (acertou o setor)
- /sfx/lose.mp3 (errou)
- /sfx/solved.mp3 (3 acertos, resolveu)
