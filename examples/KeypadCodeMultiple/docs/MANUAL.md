# Manual do KeypadCodeMultiple - Senha Numerica Multipla

Guia para donos de sala, game masters e jogadores.

---

## 1. Visao Geral

### O que e este dispositivo?

O **KeypadCodeMultiple** e um puzzle de senha numerica onde o jogador deve digitar um codigo de 4 digitos no teclado para destravar algo. Diferente de puzzles de senha unica, este aceita **multiplos codigos validos**, permitindo que diferentes pistas na sala levem a diferentes codigos que todos funcionam.

### Como funciona no jogo?

- Os jogadores encontram um teclado numerico
- Eles devem digitar 4 digitos e confirmar
- Se o codigo estiver na lista de codigos validos, o puzzle e resolvido
- Se o codigo estiver errado, o display (se houver) mostra erro e podem tentar de novo
- Nao ha limite de tentativas

---

## 2. Componentes do Jogo

| Componente | Descricao |
|------------|-----------|
| **Teclado numerico** | Teclado 4x3 com digitos 0-9, * (limpar) e # (confirmar) |
| **LEDs NeoPixel** | Indicam estado do puzzle (idle, digitando, resolvido, erro) |
| **Display HT16K33** | Mostra digitos digitados e feedback (opcional) |
| **Trava/Rele** | Dispositivo externo que e liberado ao resolver |

---

## 3. Configuracao Inicial

### Codigos padrao

Por padrao, o puzzle aceita tres codigos: **1234**, **5678** e **0000**.

### Como alterar os codigos

Edite o arquivo `/keypadmulti.json` no LittleFS:

```json
{
    "codes": ["1234", "5678", "0000"]
}
```

Adicione ou remova codigos conforme necessario. Cada codigo deve ter exatamente 4 digitos.

### Via Serial

Conecte via USB e use os comandos:
- `codes add 4321` - Adiciona o codigo 4321
- `codes list` - Mostra todos os codigos
- `codes clear` - Remove todos os codigos

---

## 4. Como Jogar

### Para o Game Master

1. Defina os codigos validos antes de comecar a partida
2. Esconda pistas pela sala que levem a diferentes codigos
3. O puzzle esta sempre pronto - basta comecar a digitar

### Para os jogadores

1. Digite um codigo de 4 digitos
2. Pressione **#** para confirmar
3. Se errou, pressione ***** para limpar e comecar de novo
4. Se o codigo estiver correto, o puzzle resolve

### Dicas para criar pistas

- **Pista unica**: Esconda um bilhete com o codigo exato
- **Multiplas pistas**: Cada pista leva a um codigo diferente (todos funcionam)
- **Charada**: "O ano do descobrimento do Brasil" = 1500
- **Operacao matematica**: "7+5, 3+4, 9-1, 2+2" = 1284
- **Relogio**: "Meia-noite e meia" = 0030

---

## 5. O que acontece ao resolver

1. O som de sucesso toca
2. Os LEDs ficam verdes
3. A trava e liberada (se configurada)
4. O display mostra mensagem de sucesso

---

## 6. Parametros do JSON

```json
{
  "codes": ["1234", "5678", "0000"],
  "lockPin": 20,
  "audio": {
    "keypress": "/sfx/beep.mp3",
    "unlock": "/sfx/magic.mp3",
    "denied": "/sfx/error.mp3",
    "cancel": "/sfx/cancel.mp3"
  },
  "neopixel": {
    "count": 8,
    "colorIdle": [5, 5, 20],
    "colorUnlock": [0, 50, 0],
    "colorDenied": [50, 0, 0]
  }
}
```

### Ajuste fino

| Parametro | O que faz |
|-----------|-----------|
| `lockPin` | Pino fisico da trava (GPIO) |
| `neopixel.count` | Quantos LEDs estao conectados |
| `neopixel.colorIdle` | Cor dos LEDs quando aguardando |
| `neopixel.colorUnlock` | Cor dos LEDs ao acertar |
| `neopixel.colorDenied` | Cor dos LEDs ao errar |

---

## 7. Checklist de Audio

| Arquivo | Quando Toca | Obrigatorio? |
|---------|-------------|--------------|
| `beep.mp3` | Ao pressionar tecla | Sim |
| `magic.mp3` | Codigo correto | Sim |
| `error.mp3` | Codigo errado | Nao |
| `cancel.mp3` | Cancelar entrada | Nao |

Formato: MP3, 128kbps, 44.1kHz, stereo, sem ID3 tags.

---

## 8. Solucao de Problemas

### Teclado nao responde

1. Verifique o PCF8575 no endereco 0x20
2. Teste com `hw.test()` via Serial
3. Verifique a solda dos cabos do teclado

### Codigo valido nao resolve

1. Verifique se o codigo esta na lista do JSON
2. Reinicie o dispositivo apos alterar o JSON
3. Use `codes list` via Serial para conferir

### Trava nao libera

1. Verifique o pino do lockPin no JSON
2. Teste o rele com `hw.test()`
3. Verifique a alimentacao da trava

---

## Resumo Rapido

| Acao | Como Fazer |
|------|------------|
| Digitar codigo | Teclas 0-9 |
| Confirmar | Tecla # |
| Limpar | Tecla * |
| Resolver puzzle | Digite um codigo valido e confirme |

---

*Manual KeypadCodeMultiple - Versao 1.0 para biblioteca PropMaker EscapeRoom*
