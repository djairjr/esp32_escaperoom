# Mastermind — Algoritmo do Jogo

## Visão Geral

Jogo de adivinhação de cores. O sistema gera uma sequência secreta de N cores (1-6). O jogador faz tentativas inserindo cores via teclado. A cada tentativa, o sistema informa quantas cores estão na posição correta (P) e quantas cores existem mas em posição errada (E).

## Configuração (JSON: mastermind.json)

- **codeLength**: tamanho da sequência (2-8)
- **maxColors**: cores disponíveis (2-6)
- **maxAttempts**: tentativas máximas
- **Cores**: Vermelho(1), Azul(2), Verde(3), Amarelo(4), Magenta(5), Ciano(6)

## Estados

IDLE → RUNNING → SOLVED / GAMEOVER

## Fluxo

1. Sistema gera sequência secreta aleatória
2. Jogador insere N cores via teclado (1-6)
3. `#` envia tentativa, `*` limpa
4. Sistema calcula:
   - P (exact): cores na posição correta
   - E (color): cores presentes mas em posição errada
5. Feedback no display: "P=E=" e NeoPixel
6. P == N → SOLVED
7. Tentativas == maxAttempts → GAMEOVER

## Algoritmo de Feedback

1. Marca posições exatas (cor e posição corretas)
2. Remove essas posições da comparação
3. Para cada cor restante no palpite, verifica se existe no segredo (em posição diferente)
4. Incrementa contagem de "cor presente"

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
