# STATUS - Cyclone

## O que foi feito
- Puzzle Cyclone (luz que percorre anel NeoPixel, jogador para no alvo verde)
- 3 rounds para vencer, velocidade aumenta a cada round
- Botão SELECT para parar a luz, DET como segundo botão (opcional)
- Efeitos visuais: verde na zona-alvo, vermelho fora, trail de esteira
- Áudio: tick a cada passo, hit/miss por round, solved ao final
- Configuração via /cyclone.json

## Status de compilação
Sucesso

## Decisões pendentes
1. **Velocidade inicial (0.05), incremento (0.02), max (0.4):** Valores adequados? Testar com hardware real.
2. **Tamanho do alvo (3 LEDs):** Facilidade/dificuldade adequada?
3. **3 rounds para vencer:** Número ideal?
4. **Usar DET como segundo botão** para 2 jogadores? Atualmente DET também para a luz.
5. **Efeito de "miss"** - atualmente só vermelho. Deve piscar ou ter padrão específico?
6. **Criar assets de áudio:** tick.mp3, hit.mp3, miss.mp3, solved.mp3

## Configuração
examples/Cyclone/data/cyclone.json

## Assets necessários
- /sfx/tick.mp3 (passo da luz)
- /sfx/hit.mp3 (acertou o alvo)
- /sfx/miss.mp3 (errou o alvo)
- /sfx/solved.mp3 (puzzle resolvido)
