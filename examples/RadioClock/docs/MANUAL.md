# Manual do RadioClock - Sintonizador de Radio

Guia para donos de sala, game masters e jogadores.

---

## 1. Visao Geral

### O que e este dispositivo?

O **RadioClock** e um puzzle de sintonizador de radio. O jogador percorre frequencias em um dial luminoso usando um botao. Em frequencias erradas, o radio toca estatica. Na frequencia correta, um sinal limpo e tocado. O jogador precisa encontrar 3 frequencias diferentes para resolver o puzzle.

### Como funciona no jogo?

- Os jogadores veem um anel de LEDs que representa o dial de um radio
- Cada posicao no anel e uma frequencia diferente
- Ao pressionar o botao, o dial avanca para a proxima frequencia
- O display mostra o numero da frequencia atual
- Em frequencias erradas: estatica no alto-falante
- Na frequencia certa: som limpo e o jogador avanca de round
- Tres frequencias encontradas resolvem o puzzle

---

## 2. Componentes do Jogo

| Componente | Descricao |
|------------|-----------|
| **Anel de LEDs NeoPixel** | Dial do radio - mostra a frequencia atual |
| **Botao SELECT** | Avanca para a proxima frequencia |
| **Display 4 digitos** | Mostra "F-3" (frequencia 3) ou feedback |
| **Alto-falante** | Estatica nas frequencias erradas, sinal nas certas |

---

## 3. Configuracao Inicial

### Definindo as frequencias alvo

Edite o arquivo `/radioClock.json`:

```json
{
  "frequencyCount": 12,
  "targetFrequencies": [3, 8, 5],
  "roundsToWin": 3
}
```

| Parametro | O que faz | Sugestao |
|-----------|-----------|----------|
| `frequencyCount` | Quantas frequencias no dial | 8-20 |
| `targetFrequencies` | Array com as 3 frequencias-alvo | Diferentes entre si |
| `roundsToWin` | Quantas acertar para vencer | 1-5 |

### Exemplos de configuracao

**Facil:** frequencyCount=8, targetFrequencies=[2, 5], roundsToWin=2
**Medio:** frequencyCount=12, targetFrequencies=[3, 8, 5], roundsToWin=3
**Dificil:** frequencyCount=20, targetFrequencies=[7, 15, 3], roundsToWin=3

---

## 4. Como Jogar

### Para o Game Master

1. Defina as frequencias alvo no JSON
2. Posicione o anel em local visivel
3. O display mostra "rAdi" quando pronto
4. Pressione SELECT para comecar

### Para os jogadores

1. **Pressione SELECT** para avancar para a proxima frequencia
2. **Escute**: estatica = frequencia errada, sinal limpo = frequencia certa
3. **Veja** o display: mostra "F-3" (frequencia 3)
4. **Veja** o dial: a luz branca mostra onde esta no dial
5. **Ao acertar**: o display mostra "GOOd" e os LEDs ficam verdes
6. **Continue**: encontre as 3 frequencias alvo para vencer

### Estrategia

- Percorra todas as frequencias em ordem
- Preste atencao ao som - e a unica dica
- A frequencia alvo muda a cada round
- Nao ha penalidade por errar - continue tentando

---

## 5. O que acontece ao resolver

1. O som de vitoria toca
2. Os LEDs fazem efeito rainbow
3. Display mostra "donE"
4. A trava e liberada
5. Apos 8 segundos, o puzzle volta ao estado inicial

---

## 6. Checklist de Audio

| Arquivo | Quando Toca | Obrigatorio? |
|---------|-------------|--------------|
| `static.mp3` | Frequencia errada (estatica) | Sim |
| `signal.mp3` | Frequencia certa (sinal limpo) | Sim |
| `solved.mp3` | Puzzle resolvido | Sim |
| `select.mp3` | Mudar de frequencia | Recomendado |

Formato: MP3, 128kbps, 44.1kHz, stereo, sem ID3 tags.

### Dica para os arquivos de audio

- **static.mp3**: Pode ser um arquivo de ruido branco ou estatica de radio real
- **signal.mp3**: Um tom limpo, musica classica ou voz gravada "Voce encontrou a frequencia!"
- **select.mp3**: Um "click" curto de radio

---

## 7. Solucao de Problemas

### O som de estatica nao toca

1. Verifique se /sfx/static.mp3 existe no LittleFS
2. Teste com `hw.test()` via Serial

### O display mostra caracteres estranhos

1. Verifique o HT16K33 no endereco 0x70
2. Ajuste o brilho no JSON se necessario

### Nao encontra a frequencia alvo

1. Verifique os valores em targetFrequencies no JSON
2. Os valores devem ser menores que frequencyCount
3. Frequencias comecam em 0 (F-0, F-1, etc.)

### O puzzle nao comeca

1. Pressione SELECT para sair do estado idle
2. O display deve mudar de "rAdi" para "F-0"

---

## Resumo Rapido

| Acao | Como Fazer |
|------|------------|
| Avancar frequencia | Pressione SELECT |
| Frequencia errada | Estatica no audio |
| Frequencia certa | Sinal limpo + "GOOd" |
| Vencer | Encontre as 3 frequencias |

### Estados do Display

| Display | Significado |
|---------|-------------|
| "rAdi" | Pronto, pressione SELECT |
| "F-3" | Frequencia 3 |
| "GOOd" | Frequencia certa! |
| "donE" | Puzzle resolvido! |

### Estados dos LEDs

| Cor/Padrao | Significado |
|------------|-------------|
| Azul pulsante | Aguardando inicio |
| Branco + trail | Navegando frequencias |
| Verde pulsante | Frequencia certa! |
| Rainbow | Puzzle resolvido! |

---

*Sintonize a frequencia certa e desvende o misterio...*

*Manual RadioClock - Versao 1.0 para biblioteca PropMaker EscapeRoom*
