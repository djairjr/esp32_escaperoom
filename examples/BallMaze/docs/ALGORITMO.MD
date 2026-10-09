# BallMaze — Algoritmo do Jogo

## Visão Geral

Labirinto físico onde o jogador inclina uma caixa com acelerômetro LIS3DH para mover uma bola virtual por um plano 2D. A bola deve visitar N zonas-alvo distribuídas pelo labirinto. Resolve quando todos os alvos forem atingidos.

## Configuração (JSON: ballmaze.json)

- **Física**: accelScale (sensibilidade), damping (amortecimento), bounce (restituição), maxSpeed (velocidade terminal), deadZone (zona morta)
- **Labirinto**: mazeW/H (dimensões), startX/Y (posição inicial)
- **Alvos**: targetCount (2-8), targetRadius, requireOrder (sequencial ou livre)
- **Hardware**: lockPin, neoCount, volume
- **Áudio**: sfxTargetHit, sfxSolved, sfxStart

## Estados

IDLE → RUNNING → SOLVED → IDLE (auto-reset 10s)

## Física (Euler integration)

1. Lê tilt do acelerômetro (X, Y) calibrado
2. Aplica zona morta (ignora inclinações mínimas)
3. Converte tilt em aceleração (ax, ay)
4. Integra velocidade: v += a * dt
5. Aplica damping: v *= (1 - damping * dt)
6. Limita velocidade terminal
7. Integra posição: pos += v * dt
8. Colisão com paredes: bounce (inverte velocidade com restituição)
9. Detecta se bola está sobre alvo (distância < radius)

## Detecção de Alvos

- Se requireOrder=true: alvos devem ser visitados na ordem do array
- Se requireOrder=false: qualquer ordem
- Ao atingir todos → SOLVED (destrava lockPin)

## Feedback

- NeoPixel: mapeamento 2D (bola=branco, alvos=amarelo, atingidos=verde)
- Áudio: som ao atingir alvo, som de vitória
- Display HT16K33: número de alvos restantes

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1 (SSID, senha, MQTT broker, nome do dispositivo).
