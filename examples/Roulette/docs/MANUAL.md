# Manual da Roulette - Roleta de Sorte

Guia para donos de sala, game masters e jogadores.

---

## 1. Visao Geral

### O que e este dispositivo?

A **Roulette** e uma roleta luminosa electronica. O jogador pressiona um botao para girar o anel de LEDs. A luz corre em volta do anel e desacelera gradualmente ate parar em uma posicao. Se parar no setor vencedor (marcado em verde), o jogador ganha a rodada. Tres rodadas vencidas resolvem o puzzle.

### Como funciona no jogo?

- Os jogadores veem um anel de LEDs com um setor iluminado em verde
- Ao pressionar o botao, uma luz branca comeca a correr em volta do anel
- A luz vai desacelerando ate parar
- Se parar no setor verde, o jogador vence a rodada
- Se parar fora, pode tentar de novo
- Tres acertos resolvem o puzzle

---

## 2. Componentes do Jogo

| Componente | Descricao |
|------------|-----------|
| **Anel de LEDs NeoPixel** | A roleta propriamente dita |
| **Botao SELECT** | Para girar a roleta |
| **Alto-falante** | Sons de giro, vitoria, derrota e resolucao |

---

## 3. Configuracao Inicial

### Ajuste da roleta

Edite o arquivo `/roulette.json`:

```json
{
  "spinSpeed": 8.0,
  "friction": 0.97,
  "minSpeed": 0.1,
  "winSectorCount": 1,
  "roundsToWin": 3
}
```

| Parametro | Efeito | Facil | Dificil |
|-----------|--------|-------|---------|
| `spinSpeed` | Velocidade inicial | 6.0 | 12.0 |
| `friction` | Atrito (0.99 = desacelera devagar) | 0.98 | 0.95 |
| `winSectorCount` | Quantos LEDs sao vencedores | 3 | 1 |
| `roundsToWin` | Acertos necessarios para vencer | 1 | 5 |

### Comportamento esperado

- friction = 0.97: roleta gira ~12 voltas antes de parar
- friction = 0.95: roleta gira ~6 voltas (mais rapido)
- friction = 0.99: roleta gira ~50 voltas (muito lento para parar)

---

## 4. Como Jogar

### Para o Game Master

1. Ajuste a dificuldade no JSON
2. Posicione o anel em local visivel
3. Mostre aos jogadores que o setor verde e o alvo

### Para os jogadores

1. **Veja** o setor vencedor (verde pulsante)
2. **Pressione SELECT** para girar a roleta
3. **Acompanhe** a luz desacelerando
4. **Se acertar**: verde! Mais um passo para vencer
5. **Se errar**: vermelho. Tente novamente!
6. **Apos 3 acertos**: puzzle resolvido!

### Dicas

- O setor vencedor esta sempre visivel (verde pulsante)
- A roleta sempre desacelera - nao tem truque
- Observe o padrao de desaceleracao para prever onde vai parar
- A sorte e o principal fator, mas a persistencia vence

---

## 5. O que acontece ao resolver

1. O som de vitoria toca
2. Os LEDs fazem efeito rainbow
3. A trava e liberada
4. Apos 8 segundos, um novo setor vencedor e sorteado e o puzzle volta ao inicio

---

## 6. Checklist de Audio

| Arquivo | Quando Toca | Obrigatorio? |
|---------|-------------|--------------|
| `spin.mp3` | Ao girar a roleta | Sim |
| `win.mp3` | Acertou o setor | Sim |
| `lose.mp3` | Errou o setor | Sim |
| `solved.mp3` | 3 acertos | Sim |

Formato: MP3, 128kbps, 44.1kHz, stereo, sem ID3 tags.

---

## 7. Solucao de Problemas

### A roleta nao gira

1. Verifique o botao SELECT (GPIO 4)
2. Verifique o anel NeoPixel

### A roleta para muito rapido

1. Aumente `friction` no JSON (0.98-0.99)
2. Aumente `spinSpeed` (10.0-15.0)

### A roleta demora para parar

1. Diminua `friction` (0.95-0.96)
2. Aumente `minSpeed` (0.2-0.3)

### Nunca acerta o setor

1. Aumente `winSectorCount` (3-5)
2. Verifique se `roundsToWin` nao esta muito alto

---

## Resumo Rapido

| Acao | Como Fazer |
|------|------------|
| Girar roleta | Pressione SELECT |
| Setor vencedor | LED verde pulsante |
| Acertou | LED verde fixo |
| Errou | LED vermelho fixo |
| Vencer | 3 acertos |

### Estados dos LEDs

| Cor/Padrao | Significado |
|------------|-------------|
| Verde pulsante | Setor vencedor visivel (idle) |
| Branco + trail | Roleta girando |
| Verde fixo | Acertou! |
| Vermelho fixo | Errou! |
| Rainbow | Puzzle resolvido! |

---

*Boa sorte! Que a roleta esteja a seu favor...*

*Manual Roulette - Versao 1.0 para biblioteca PropMaker EscapeRoom*
