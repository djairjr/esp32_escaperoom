# Manual do Simon Says - Jogo Genius para Escape Room

Um guia completo para donos de sala, game masters e jogadores.

---

## 1. Visão Geral

### O que é este dispositivo?

O **Simon Says** é uma versão do clássico jogo Genius adaptada para Escape Rooms. O dispositivo consiste em uma caixa com 4 botões coloridos, cada um com seu próprio anel de LEDs que acendem em sequência. Os jogadores devem memorizar e repetir a sequência de cores para abrir a caixa.

### Como funciona no jogo?

- Os jogadores encontram o dispositivo na sala (geralmente uma caixa trancada)
- Ao pressionar qualquer botão, o jogo inicia com uma animação de abertura
- O dispositivo mostra uma sequência de cores nos anéis de LED
- Os jogadores devem repetir a sequência apertando os botões na mesma ordem
- Se acertarem, avançam para o próximo nível com uma sequência maior
- Se errarem, o sistema emite som de erro e repete a mesma sequência (sem punição!)
- Ao completar todos os níveis, a caixa se abre automaticamente

> **Diferencial**: Este sistema não pune erros! Se os jogadores errarem, ou demorarem mais de 5 segundos para apertar, o sistema simplesmente repete a sequência atual até que acertem.

---

## 2. Componentes do Jogo

Estes são os elementos que os jogadores interagem durante o jogo:

| Componente | Descrição |
|------------|-----------|
| **Botão Vermelho** | Anel de 12 LEDs vermelhos na borda superior/esquerda |
| **Botão Azul** | Anel de 12 LEDs azuis na borda superior/direita |
| **Botão Amarelo** | Anel de 12 LEDs amarelos na borda inferior/esquerda |
| **Botão Verde** | Anel de 12 LEDs verdes na borda inferior/direita |
| **Fechadura** | Abre automaticamente quando o puzzle é resolvido |
| **Alto-falante** | Emite sons para cada cor e feedback de acerto/erro |

---

## 3. Como Jogar (para os jogadores)

### Iniciando o jogo

1. O dispositivo estará no modo de espera, com todos os LEDs apagados
2. **Pressione qualquer botão** para iniciar
3. Ouça o som de abertura e veja todos os anéis acenderem na sequência: vermelho → azul → amarelo → verde

### Durante o jogo

1. **Observe a sequência** de cores que o dispositivo mostrar
2. **Repita a sequência** apertando os botões na mesma ordem
3. Cada cor pressionada corretamente emitirá seu som característico
4. Se acertar toda a sequência, ouvirá um som de "acerto" e o próximo nível começará

### Se errar ou demorar

- Não se preocupe! Este sistema **não reinicia o jogo** nem aplica punições
- Se errar ou demorar mais de 5 segundos, ouvirá um som de erro
- O dispositivo repetirá a **mesma sequência** novamente
- Continue tentando até acertar!

### Vencendo o jogo

- Complete todos os 7 níveis (sequências cada vez maiores)
- Ao completar o último nível, ouvirá um som de vitória e verá uma animação arco-íris
- A caixa se abrirá automaticamente
- Para resetar o jogo (após remover o conteúdo), pressione e segure qualquer botão por 2 segundos

---

## 4. Configuração e Manutenção

### Para o dono da sala / game master

#### Regras de progressão

O jogo segue uma progressão de dificuldade automática:

| Nível | Velocidade | Descrição |
|-------|------------|-----------|
| 1-2 | 800ms | Lento - início fácil |
| 3-4 | 600-700ms | Médio - velocidade moderada |
| 5-6 | 400-500ms | Rápido - desafio aumenta |
| 7 | 300ms | Muito rápido - nível final |

#### Resetando o jogo após uso

1. Após o puzzle ser resolvido e a caixa aberta
2. Pressione **qualquer botão por 2 segundos** (até ouvir o som de "pronto")
3. O jogo estará resetado e pronto para a próxima partida

> **Dica**: O jogo não reseta automaticamente. Sempre faça o reset após o término da partida para garantir que comece do nível 1.

#### Verificação de som

Se necessário, verifique se os arquivos de áudio estão corretos:

1. Ao ligar, deve tocar o som "ready" (pronto)
2. Ao iniciar, deve tocar o som "begin" (início)
3. Cada botão deve tocar seu som respectivo: red, blue, yellow, green
4. Ao acertar uma sequência completa: som "correct"
5. Ao errar: som "error"
6. Ao vencer: som "victory"

---

## 5. Solução de Problemas

### O dispositivo não liga

- Verifique se a alimentação está conectada corretamente
- Verifique se há energia na fonte de alimentação

### LEDs não acendem

- Verifique se há arquivo "ready.mp3" no cartão (a presença de áudio indica funcionamento)
- Se o áudio funcionar mas LEDs não: pode haver problema na conexão dos NeoPixel

### Som não funciona

- Verifique se há arquivos MP3 na pasta correta (LittleFS ou SD)
- Verifique se os arquivos estão no formato correto: 44.1kHz, estéreo, 128kbps
- Tente aumentar o volume na configuração (se disponível no menu)

### A caixa não abre ao vencer

- Verifique a conexão do pino de fechadura (PIN_EXTERNAL_POWER)
- Teste manualmente acionando a fechadura durante o jogo

### O jogo está muito difícil para os jogadores

- Lembre-se: este sistema **não reinicia** ao errar, apenas repete a mesma sequência
- Isso permite que jogadores tentem quantas vezes precisarem no mesmo nível
- Se ainda estiver difícil, ajuste o nível máximo no código (padrão: 7 níveis)

---

## 6. Especificações Técnicas

### Hardware necessário

| Componente | Descrição |
|------------|-----------|
| Placa | Adafruit PropMaker Feather RP2040 |
| Expansor | PCF8575 (I2C endereço 0x20) |
| LEDs | 48 LEDs NeoPixel (4 anéis de 12 LEDs cada) |
| Botões | 4 botões momentâneos |
| Áudio | Saída de áudio I2S integrada à PropMaker |
| Fechadura | Solenoide ou relê (controle via PIN_EXTERNAL_POWER) |

### Arquivos de áudio necessários

| Arquivo | Função |
|---------|--------|
| ready.mp3 | Boot do sistema |
| begin.mp3 | Início do jogo |
| red.mp3 | Botão vermelho |
| blue.mp3 | Botão azul |
| yellow.mp3 | Botão amarelo |
| green.mp3 | Botão verde |
| correct.mp3 | Sequência correta |
| error.mp3 | Erro na sequência |
| victory.mp3 | Puzzle resolvido |

**Formato dos arquivos:**
- Taxa de bits: 128 Kbps
- Taxa de amostragem: 44.1 kHz
- Canais: Estéreo (2)
- Sem metadados ID3

Para converter: `ffmpeg -i input.mp3 -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 output.mp3`

### Comunicação (opcional)

- O dispositivo pode se comunicar com outros puzzles via RS485
- ID na rede: 20 (configurável no código)
- Comandos suportados:
  - `ForceSolve`: Força a resolução imediata (abre a caixa)
  - `GlobalEvent`: Reseta o jogo para o estado inicial

---

## 7. Dicas para Game Masters

### Integração na sala

1. **Narrativa**: O Simon Says pode ser apresentado como:
   - Um sistema de segurança que precisa ser desativado
   - Um mecanismo mágico que requer memória e concentração
   - Um dispositivo científico de teste de padrões

2. **Dificuldade**: Como o sistema não reinicia ao errar:
   - O desafio está em memorizar sequências cada vez mais longas
   - A velocidade aumenta a cada nível
   - Considere dar dicas visuais se necessário

3. **Pós-resolução**:
   - A animação arco-íris indica sucesso
   - A caixa abre automaticamente
   - Lembre-se de resetar o dispositivo antes da próxima partida

### Manutenção preventiva

- Verifique as conexões dos botões periodicamente
- Teste todos os sons antes de cada dia de operação
- Mantenha o sistema limpo e protegido da umidade

---

## 8. FAQ (Perguntas Frequentes)

**P: Os jogadores precisam começar do nível 1 toda vez?**
R: Sim, o jogo sempre começa do nível 1 após reset.

**P: O que acontece se o tempo de 5 segundos entre botões acabar?**
R: O sistema considera um erro, toca o som de erro e repete a mesma sequência.

**P: Posso ajustar a velocidade do jogo?**
R: Sim, mas requer modificação no código (função calculateSpeed()). Os valores padrão são otimizados para jogadores.

**P: O sistema pode funcionar sem todos os sons?**
R: Sim, mas a experiência será prejudicada. O jogo funciona sem áudio, mas isso não é recomendado.

**P: Posso mudar o número de níveis?**
R: Sim, altere a constante MAX_LEVELS no código (padrão: 7 níveis).

---

## Referência Rápida

| Situação | Ação |
|----------|------|
| Iniciar jogo | Pressione qualquer botão |
| Jogar | Observe a sequência e repita |
| Errou ou demorou | Sistema repete automaticamente |
| Venceu | Caixa abre, animação arco-íris |
| Resetar | Segure qualquer botão por 2 segundos |
| Problemas | Verifique alimentação e conexões |

---

Documentação gerada automaticamente.
Última atualização: 2026-04-23
