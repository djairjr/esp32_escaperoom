# STATUS - BallMaze

## O que foi feito
- Port completo do puzzle BallMaze usando acelerômetro LIS3DH embutido
- Física de bola virtual com integração de Euler (aceleração -> velocidade -> posição)
- Detecção de zonas-alvo por distância circular (até 8 zonas)
- 3 estados: Idle (shake para iniciar) -> Running (física + alvos) -> Solved (rainbow + auto-reset)
- Mapeamento 2D -> linear para NeoPixel (2 linhas no strip)
- Efeitos visuais: bola branca, alvos amarelos, glow de proximidade, alvo verde ao acertar
- Áudio: loop de rolamento, hit de alvo, fanfarra de solved
- Configuração via /ballmaze.json

## Status de compilação
Sucesso: 253024 bytes (3%), 15932 bytes RAM (6%)

## Decisões pendentes
1. **Zonas-alvo fixas ou aleatórias?** Atualmente definidas no JSON. Permitir geração aleatória?
2. **Ordem dos alvos:** Atualmente ordem livre. Implementar modo sequencial?
3. **Sensibilidade:** Aceleração (800), damping (4), bounce (0.3) precisam de calibração prática
4. **Shake threshold:** 15.0 m/s^2 pode precisar de ajuste
5. **NeoPixel count:** 30 está bom ou precisa de mais/menos?
6. **Número de alvos:** 3 alvos no JSON, definir quantos por padrão?
7. **Criar assets de áudio:** ball_roll.mp3, target_hit.mp3, solved.mp3

## Configuração
examples/BallMaze/data/ballmaze.json

## Assets necessários
- /sfx/ball_roll.mp3 (loop enquanto bola se move)
- /sfx/target_hit.mp3 (ao acertar um alvo)
- /sfx/solved.mp3 (puzzle resolvido)
- /sfx/idle_chime.mp3 (a cada 15s no idle, opcional)
