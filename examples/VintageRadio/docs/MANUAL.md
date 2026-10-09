# Manual do Usuário - Rádio Vintage de Ondas Curtas

## Para Donos de Escape Room

**Versão:** 2.0 (PropMaker Edition)  
**Puzzle:** Rádio Vintage com Sintonia FM e Código Morse  
**Nível:** Intermediário  
**Tempo Estimado:** 10-15 minutos

---

## Visão Geral do Puzzle

Este é um **rádio de ondas curtas vintage** que os jogadores precisam "consertar" para receber uma mensagem secreta. O funcionamento imita um rádio antigo de válvulas.

### Componentes Físicos

1. **Transformador** (peça principal - conecta primeiro)
2. **3 Válvulas** (conectam no rádio para ativar o FM)
3. **1 Válvula Final** (conecta por último para receber a mensagem)
4. **Knob de Sintonia** (rotary encoder)
5. **Caixa do Rádio** com LEDs indicadores

### Objetivo do Jogador

1. Ligar o rádio (conectar transformador)
2. Inserir as 3 válvulas para ativar o sistema de rádio
3. Explorar as estações de rádio girando o knob
4. Inserir a válvula final para receber a mensagem em código Morse
5. Anotar a mensagem e usá-la para sair da sala

---

## Como Funciona (Para o Jogador)

### Passo a Passo da Experiência

**1. Ligando o Rádio**
- O jogador conecta o transformador na caixa
- O LED branco (Power) acende
- O rádio está "ligado" mas ainda não funciona

**2. Inserindo as Válvulas**
- Conforme cada válvula é inserida, um LED azul acende
- O sistema faz sons diferentes para cada uma:
  - **Válvula 1:** Som curto (beep beep beep)
  - **Válvula 2:** Som longo (beeeep beeeep beeeep)
  - **Válvula 3:** O rádio "ganha vida" - agora faz barulho de rádio!

**3. Sintonizando o Rádio**
- Com a 3ª válvula inserida, o knob funciona
- Girando o knob, o jogador ouve diferentes estações de rádio
- Cada estação tem um som de interferência/static diferente
- O volume sobe e desce conforme o knob gira (simulando sintonia)

**4. A Solução**
- O jogador precisa inserir a **4ª válvula (Válvula Final)**
- Isso para o barulho de rádio e inicia a mensagem em código Morse
- A mensagem é transmitida em beeps pelo rádio
- Os jogadores precisam decifrar o código Morse

**5. Decifrando**
- A mensagem em código Morse é repetida 5 vezes
- O jogador anota a tradução (vem escrito no alfabeto normal)
- A mensagem contém a senha ou dica para o próximo passo do jogo

---

## Configuração Para Você (Operador)

### Antes da Partida

#### 1. A Mensagem Secreta (IMPORTANTE!)

Você pode mudar a mensagem que será transmitida em código Morse.

**Como fazer:**
1. Retire o cartão SD do sistema
2. Abra o arquivo `mensagem.txt` no computador
3. Escreva sua mensagem (pode usar acentos e pontuação)
4. Salve o arquivo
5. Recoloque o cartão SD no sistema

**Exemplos de mensagens:**
```
PARABENS A SENHA E 4729

A PORTA ABRE COM O CODIGO VERMELHO

PROCUREM A CAIXA NO ARMARIO

O PROXIMO DIGITO E 7
```

**Regras:**
- Use frases curtas (máximo 256 caracteres)
- Pode usar letras A-Z, números 0-9 e espaços
- Acentos são aceitos (são convertidos automaticamente)
- A mensagem é lida do cartão SD primeiro, depois da memória interna

#### 2. Os Sons das Estações de Rádio

O rádio tem **9 estações** que o jogador pode explorar. Cada uma toca um arquivo MP3 diferente.

**Arquivos necessários:**
- `station1.mp3` até `station9.mp3`

**Onde colocar:**
- **Opção 1 (recomendada):** Cartão SD na raiz (pasta principal)
- **Opção 2:** Memória interna (LittleFS)

**Prioridade:** Se o mesmo arquivo existir no cartão SD e na memória interna, o **cartão SD vence**.

**Isso permite:** Você pode ter uma versão padrão na memória interna, e colocar versões especiais no SD para eventos específicos (por exemplo, uma estação com dicas extras ou mensagens personalizadas para grupos diferentes).

**Que tipo de áudio usar:**
- Sons de rádio interferindo (estática, chiado)
- Sons de transmissão de rádio antiga
- Qualquer MP3 em 128kbps, 44.1kHz, estéreo
- Duração: pode ser curto (loop automático)

**Sugestão:** Use sons diferentes para cada estação para criar a sensação de "canais diferentes". Se não souber onde conseguir, pode deixar os arquivos padrão que já vêm no sistema.

### Teste Antes de Usar

**Procedimento de teste:**

1. **Reset:** Desconecte todas as peças
2. **Ligue:** Conecte o transformador
   - LED Power deve acender (branco)
3. **Válvula 1:** Conecte a primeira válvula
   - LED azul acende + som de beep curto
4. **Válvula 2:** Conecte a segunda válvula
   - LED azul acende + som de beep longo
5. **Válvula 3:** Conecte a terceira válvula
   - LED azul acende + rádio começa a "fazer barulho"
   - Gire o knob - deve ouvir diferentes estações
6. **Válvula 4:** Conecte a válvula final
   - Rádio para de fazer barulho
   - Começa a transmitir código Morse
   - A mensagem deve ser clara e repetir 5 vezes

**Se algo não funcionar:**
- Verifique se todas as conexões estão firmes
- Reinicie desconectando e reconectando o transformador
- Verifique se o cartão SD está inserido corretamente
- Troque o cartão SD se necessário

---

## Resolução de Problemas

### Problema: O rádio não liga

**Causas possíveis:**
1. Transformador não conectado corretamente
2. Problema na conexão elétrica
3. Sistema travou

**Soluções:**
- Desconecte e reconecte o transformador firmemente
- Verifique se o LED Power acende
- Se persistir, desligue e ligue o sistema completamente
- Verifique se o cabo USB está conectado (se usar alimentação USB)

### Problema: As válvulas não funcionam

**Causas possíveis:**
1. As válvulas precisam ser conectadas em ordem
2. Conexão solta
3. Sistema não foi resetado

**Soluções:**
- Sempre conecte na ordem: Válvula 1 → 2 → 3 → 4
- Pressione firmemente até ouvir/ver o feedback
- Cada válvula deve acender seu LED azul
- Desconecte o transformador e comece de novo

### Problema: O knob de sintonia não funciona

**Causas possíveis:**
1. A 3ª válvula não está conectada
2. Problema no encoder

**Soluções:**
- Verifique se a 3ª válvula está conectada (LED azul aceso)
- O knob só funciona APÓS a 3ª válvula ser inserida
- Gire o knob devagar para testar
- Verifique se o encoder está bem conectado

### Problema: Não ouço as estações de rádio

**Causas possíveis:**
1. Arquivos MP3 ausentes ou com formato errado
2. Volume muito baixo
3. Problema no cartão SD
4. Problema no alto-falante

**Soluções:**
- Verifique se os arquivos `station1.mp3` a `station9.mp3` existem no SD
- Formato deve ser MP3, 128kbps, 44.1kHz
- Tente substituir o cartão SD
- Verifique se o alto-falante está conectado

### Problema: A mensagem em Morse não toca

**Causas possíveis:**
1. Arquivo `mensagem.txt` ausente
2. Arquivo `mensagem.txt` está em branco
3. Cartão SD não está sendo lido

**Soluções:**
- Verifique se `mensagem.txt` existe na raiz do SD
- Abra o arquivo e confirme que tem texto dentro
- Se o arquivo estiver ausente, o sistema usa "SOS" como padrão
- Tente outro cartão SD

### Problema: Som com ruído ou distorcido

**Causas possíveis:**
1. Arquivos MP3 com formato inadequado
2. ID3 tags nos MP3 causando problemas
3. Cartão SD lento ou corrompido

**Soluções:**
- Reconverta os MP3 usando o comando ffmpeg:
  ```
  ffmpeg -i input.mp3 -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 output.mp3
  ```
- Isso remove tags ID3 e padroniza o formato
- Formate o cartão SD em FAT32

### Problema: O rádio trava ou reinicia sozinho

**Causas possíveis:**
1. Fonte de alimentação fraca
2. Transformador foi removido (HARD RESET intencional)
3. Sistema precisa de reset

**Soluções:**
- Use uma fonte de 5V com pelo menos 2A
- O HARD RESET é normal quando remove o transformador - reconecte para continuar
- Se travar, desconecte tudo e reconecte o transformador

---

## Cuidados e Manutenção

### Limpeza
- Limpe as conexões das válvulas com pano seco periodicamente
- Evite umidade no sistema
- Mantenha o knob de sintonia limpo
- Não use produtos químicos diretamente nos conectores

### Armazenamento
- Guarde as válvulas em local seco quando não estiverem em uso
- Evite deixar o transformador conectado por longos períodos sem uso
- Proteja o cartão SD de poeira e umidade

### Backup dos Arquivos
- Mantenha uma cópia dos arquivos `station*.mp3` e `mensagem.txt` no computador
- Se o cartão SD corromper, você pode recriar facilmente
- Teste o sistema semanalmente para garantir funcionamento

---

## Dicas Para Seu Jogo

### Dificuldade

**Fácil:**
- Coloque a mensagem em Morse escrita em algum lugar da sala
- Os jogadores só precisam identificar o padrão
- A 4ª válvula pode estar visível e acessível

**Médio:**
- Forneça uma tabela de código Morse em outro puzzle
- Os jogadores precisam decifrar a mensagem
- A 4ª válvula pode estar escondida ou em outro local

**Difícil:**
- Sem ajuda externa - os jogadores precisam conhecer ou descobrir código Morse
- A mensagem pode ser uma pista criptografada
- A 4ª válvula só aparece depois de resolver outro puzzle

### Tempo

- A transmissão Morse repete **5 vezes**
- Cada ciclo leva alguns segundos
- Tempo total: cerca de 1-2 minutos para ouvir tudo
- Deixe tempo suficiente para os jogadores anotarem
- O sistema FM pode ficar ligado indefinidamente - não há pressa

### Combine com Outros Puzzles

O código Morse pode ser:
- Uma senha numérica para um cadeado
- Coordenadas para encontrar um objeto escondido
- A primeira parte de uma senha maior
- Uma pista sobre outro puzzle na sala
- O código para um cofre eletrônico

---

## Checklist Pré-Jogo

Use esta lista antes de cada grupo de jogadores:

- [ ] Transformador desconectado (posição inicial)
- [ ] Todas as válvulas desconectadas
- [ ] Cartão SD inserido corretamente
- [ ] Teste rápido: conecte o transformador → LED Power acende?
- [ ] Teste: válvula 1 → LED azul + som curto?
- [ ] Teste: válvula 2 → LED azul + som longo?
- [ ] Teste: válvula 3 → LED azul + barulho de rádio?
- [ ] Teste: gire o knob → muda de estação?
- [ ] Teste: válvula 4 → mensagem Morse toca?
- [ ] Mensagem configurada no arquivo `mensagem.txt` está correta?
- [ ] Arquivos de áudio disponíveis (station1-9.mp3)
- [ ] Volume adequado para a sala

---

## Informações Técnicas Básicas

### Especificações

| Item | Especificação |
|------|---------------|
| Alimentação | 5V DC (via USB ou fonte) |
| Consumo | ~500mA típico, 1.5A pico |
| Cartão SD | MicroSD, formato FAT32 |
| Áudio | MP3, 128kbps, 44.1kHz, estéreo |
| Conexões | Plugs banana 4mm |

### Pinos do PCF8575 (para referência técnica)

| Pino | Função |
|------|--------|
| P0 | Transformador |
| P1 | Válvula 1 |
| P2 | Válvula 2 |
| P3 | Válvula 3 |
| P4 | Encoder A |
| P5 | Encoder B |
| P6 | Botão Encoder |
| P7 | Válvula 4 |

---

## Suporte Técnico

Se você encontrar problemas que não consegue resolver:

1. Verifique se todos os cabos estão conectados corretamente
2. Teste com um cartão SD diferente
3. Verifique os logs no monitor serial (se tiver acesso técnico)
4. Consulte o arquivo `ALGORITMO.MD` para detalhes técnicos completos
5. Entre em contato com o fabricante

### Contato

Para suporte técnico ou dúvidas sobre o puzzle:

**GitHub:** https://github.com/djairhector/propmaker_escaperoom

---

## Resumo Rápido

| Componente | Função | Quando Usar |
|------------|--------|-------------|
| Transformador | Liga o sistema | Sempre primeiro |
| Válvula 1 | Ativa som curto | Depois do transformador |
| Válvula 2 | Ativa som longo | Depois da válvula 1 |
| Válvula 3 | Ativa o rádio FM | Depois da válvula 2 |
| Knob | Muda estações | Após válvula 3 |
| Válvula 4 | Recebe mensagem Morse | Última - ativa a solução |

**Arquivos importantes no SD:**
- `mensagem.txt` - A mensagem em código Morse
- `station1.mp3` a `station9.mp3` - Sons das estações

**Prioridade de arquivos:** Cartão SD > Memória interna

---

*Manual para Donos de Escape Room*  
*Sistema PropMaker EscapeRoom - Vintage Radio*  
*Versão 2.0 - Atualizado com sistema de 4 válvulas e prioridade SD Card*
