# Manual do Mastermind - Jogo de Adivinhacao de Cores

Guia para donos de sala, game masters e jogadores.

---

## 1. Visao Geral

### O que e este dispositivo?

O **Mastermind** e a versao eletronica do classico jogo de tabuleiro. O sistema gera uma sequencia secreta de 4 cores. O jogador faz palpites usando o teclado e recebe dicas sobre quantas cores estao na posicao correta (P) e quantas cores existem mas em posicao errada (E). O jogador tem 10 tentativas para acertar.

### Como funciona no jogo?

- Os jogadores encontram um teclado numerico e um display
- Cada tecla de 1 a 6 representa uma cor
- O jogador digita 4 numeros (seu palpite de cores) e confirma
- O display mostra "P=1 E=2" significando: 1 cor na posicao certa, 2 cores certas em posicao errada
- Os LEDs mostram as cores do palpite e o feedback
- O jogador tem 10 tentativas para adivinhar a sequencia correta

### Mapa de Cores

| Tecla | Cor | Cor no LED |
|-------|-----|------------|
| **1** | Vermelho | Vermelho |
| **2** | Azul | Azul |
| **3** | Verde | Verde |
| **4** | Amarelo | Amarelo |
| **5** | Magenta | Rosa/Magenta |
| **6** | Ciano | Azul claro |

---

## 2. Componentes do Jogo

| Componente | Descricao |
|------------|-----------|
| **Teclado numerico** | Teclas 1-6 para cores, * para limpar, # para confirmar |
| **Display 4 digitos** | Mostra status do jogo e feedback P/E |
| **LEDs NeoPixel** | 4 primeiros LEDs mostram as cores do palpite, 4 seguintes mostram feedback |
| **Alto-falante** | Sons de tecla, acerto, erro e vitoria |

---

## 3. Configuracao Inicial

### Ajuste de dificuldade

Edite o arquivo `/mastermind.json`:

```json
{
  "codeLength": 4,
  "maxColors": 6,
  "maxAttempts": 10
}
```

| Parametro | O que faz | Facil | Dificil |
|-----------|-----------|-------|---------|
| `codeLength` | Quantas cores na sequencia | 3 | 6 |
| `maxColors` | Quantas cores disponiveis | 4 | 8 |
| `maxAttempts` | Quantas tentativas | 15 | 6 |

### Combinacoes possiveis

| codeLength | maxColors | Combinacoes |
|------------|-----------|-------------|
| 3 | 4 | 64 |
| 4 | 6 | 1296 |
| 5 | 8 | 32768 |

---

## 4. Como Jogar

### Para o Game Master

1. Defina a dificuldade no JSON
2. Explique o mapa de cores aos jogadores
3. Tecla * limpa o palpite atual
4. Tecla # confirma e envia o palpite

### Para os jogadores

1. **Veja** o display: mostra "CodE" (pronto)
2. **Digite 4 numeros** de 1 a 6 para formar seu palpite
3. **Veja as cores** nos LEDs - cada LED mostra a cor escolhida
4. **Confirme** com a tecla #
5. **Leia o feedback**:
   - **P=1** = 1 cor esta na posicao correta
   - **E=2** = 2 cores estao corretas mas em posicao errada
6. **Use a logica** para deduzir a sequencia correta
7. **Acertou?** O puzzle resolve!
8. **Errou?** Tente de novo (veja quantas tentativas restam)

### Exemplo de jogo

```
Secreto: [2, 4, 1, 3]  (Azul, Amarelo, Vermelho, Verde)

Palpite 1: [1, 2, 3, 4] -> P=0 E=3
  (cores 1,2,4 existem mas em posicao errada; cor 3 nao existe)

Palpite 2: [2, 1, 4, 3] -> P=2 E=2
  (posicoes 1 e 4 corretas; cores 1 e 4 trocadas)

Palpite 3: [2, 4, 1, 3] -> P=4 E=0 -> RESOLVIDO!
```

### Estrategia

- Comece com todas as cores diferentes para descobrir quais estao presentes
- Preste atencao ao P (posicao correta) - significa que aquela cor ja esta no lugar certo
- Use o E (cor existe) para saber quais cores fazerem parte do codigo
- Processo de eliminacao e a chave

---

## 5. O que acontece ao resolver

1. O display mostra "donE"
2. O som de vitoria toca
3. Os LEDs ficam verdes
4. A trava e liberada

### Se esgotar as tentativas

1. O display mostra "noPE"
2. O som de erro toca
3. Os LEDs ficam vermelhos
4. O codigo secreto e revelado no Serial (para debug)

---

## 6. Checklist de Audio

| Arquivo | Quando Toca | Obrigatorio? |
|---------|-------------|--------------|
| `beep.mp3` | Ao pressionar tecla | Sim |
| `magic.mp3` | Codigo descoberto | Sim |
| `error.mp3` | Tentativas esgotadas | Sim |
| `cancel.mp3` | Limpou palpite | Nao |

Formato: MP3, 128kbps, 44.1kHz, stereo, sem ID3 tags.

---

## 7. Solucao de Problemas

### Teclado nao responde

1. Verifique o PCF8575 no endereco 0x20
2. Teste com `hw.test()` via Serial

### Display nao mostra nada

1. Verifique HT16K33 no endereco 0x70
2. Ajuste o brilho no JSON

### As cores do LED nao correspondem

As cores sao aproximadas. O vermelho e FF0000, azul e 0000FF, etc. Ajuste no codigo se necessario.

---

## Resumo Rapido

| Tecla | Funcao |
|-------|--------|
| **1-6** | Seleciona uma cor |
| ***** | Limpa o palpite |
| **#** | Confirma o palpite |

### Entendendo o feedback

| Display | Significado |
|---------|-------------|
| "CodE" | Jogo pronto, aguardando palpite |
| "P2E1" | 2 cores na posicao certa, 1 cor certa em posicao errada |
| "donE" | Puzzle resolvido! |
| "noPE" | Tentativas esgotadas |

---

*Boa sorte! Use a logica...*

*Manual Mastermind - Versao 1.0 para biblioteca PropMaker EscapeRoom*
