# Manual do Cyclone - Jogo de Reflexo

Guia para donos de sala, game masters e jogadores.

---

## 1. Visao Geral

### O que e este dispositivo?

O **Cyclone** e um jogo de reflexo onde uma luz corre em circulo em um anel de LEDs e o jogador deve pressionar um botao no momento certo para fazer a luz parar dentro de uma zona-alvo verde. A cada acerto, a velocidade aumenta. Tres acertos resolvem o puzzle.

### Como funciona no jogo?

- Os jogadores veem um anel de luzes com uma luz branca correndo em circulo
- Uma zona verde indica onde a luz deve parar
- O jogador pressiona o botao para tentar parar a luz na zona verde
- Se acertar, a luz fica verde e a velocidade aumenta
- Se errar, a luz fica vermelha e o jogador tenta de novo
- Apos 3 acertos, o puzzle e resolvido

---

## 2. Componentes do Jogo

| Componente | Descricao |
|------------|-----------|
| **Anel de LEDs NeoPixel** | Luzes que mostram a luz correndo e a zona-alvo |
| **Botao SELECT** | Botao principal para parar a luz |
| **Botao DET** | Botao alternativo (funciona igual ao SELECT) |
| **Alto-falante** | Sons de tick, acerto, erro e vitoria |

---

## 3. Configuracao Inicial

### Ajuste de dificuldade

Edite o arquivo `/cyclone.json`:

```json
{
  "targetSize": 3,
  "startSpeed": 0.05,
  "speedIncrement": 0.02,
  "maxSpeed": 0.4,
  "roundsToWin": 3
}
```

| Parametro | Efeito | Facil | Dificil |
|-----------|--------|-------|---------|
| `targetSize` | Tamanho da zona-alvo | 5 (grande) | 1 (ponto) |
| `startSpeed` | Velocidade inicial | 0.03 | 0.08 |
| `speedIncrement` | Aumento por round | 0.01 | 0.04 |
| `maxSpeed` | Velocidade maxima | 0.2 | 0.6 |
| `roundsToWin` | Acertos necessarios | 2 | 5 |

---

## 4. Como Jogar

### Para o Game Master

1. Ajuste a dificuldade no JSON antes de comecar
2. Posicione o anel de LEDs em local visivel
3. Pressione SELECT para iniciar o jogo

### Para os jogadores

1. **Observe** a luz correndo no anel
2. **Aguarde** ate que a luz se aproxime da zona verde
3. **Pressione SELECT** para parar a luz
4. **Se acertar**: a luz fica verde, a velocidade aumenta, va para o proximo round
5. **Se errar**: a luz fica vermelha, tente de novo
6. **Apos 3 acertos**: puzzle resolvido!

### Dicas

- Antecipe o movimento: a luz sempre se move na mesma direcao
- Pressione um pouco antes de a luz chegar ao alvo (tempo de reacao)
- A velocidade aumenta a cada acerto - fique mais rapido!
- Use ambos os botoes (SELECT ou DET) se disponiveis

---

## 5. O que acontece ao resolver

1. O som de vitoria toca
2. Os LEDs fazem efeito rainbow
3. A trava e liberada
4. Apos 8 segundos, o puzzle volta ao estado inicial

---

## 6. Checklist de Audio

| Arquivo | Quando Toca | Obrigatorio? |
|---------|-------------|--------------|
| `tick.mp3` | Cada passo da luz (opcional) | Nao |
| `hit.mp3` | Acertou o alvo | Sim |
| `miss.mp3` | Errou o alvo | Sim |
| `solved.mp3` | Puzzle resolvido | Sim |

Formato: MP3, 128kbps, 44.1kHz, stereo, sem ID3 tags.

---

## 7. Solucao de Problemas

### A luz nao se move

1. Verifique se o anel NeoPixel esta conectado
2. Teste com `hw.test()` via Serial
3. Verifique neoCount no JSON

### O botao nao funciona

1. Verifique o pino SELECT (GPIO 4)
2. Teste com `switches` via debug Serial

### A luz esta muito rapida

1. Reduza `startSpeed` e `speedIncrement` no JSON
2. Aumente `targetSize` para facilitar

---

## Resumo Rapido

| Acao | Como Fazer |
|------|------------|
| Iniciar jogo | Pressione SELECT |
| Parar a luz | Pressione SELECT |
| Acertar | Luz para na zona verde |
| Errar | Luz para fora da zona |
| Vencer | 3 acertos |

### Estados dos LEDs

| Cor/Padrao | Significado |
|------------|-------------|
| Azul pulsante | Aguardando inicio |
| Branco + trail + verde | Jogo em andamento |
| Verde fixo | Acertou! |
| Vermelho fixo | Errou! |
| Rainbow | Puzzle resolvido! |

---

*Manual Cyclone - Versao 1.0 para biblioteca PropMaker EscapeRoom*
