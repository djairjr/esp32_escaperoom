# Manual do Telefone Escape Room

## O que é este sistema?

Este é um **telefone interativo para jogos de Escape Room**. Seu objetivo principal é **fornecer pistas e informações através de mensagens de áudio** quando os jogadores digitam números específicos.

Existem **dois modelos** disponíveis:
- **Modelo Teclado:** Com teclado numérico moderno (estilo calculadora)
- **Modelo Disco Rotativo:** Com disco rotativo (estilo telefones antigos)

> O sistema pode ser instalado em um telefone já existente ou montado em uma caixa personalizada.

---

## Como funciona o jogo?

### 1. Levante o fone do gancho

Ao levantar o telefone, os jogadores ouvem um **tom de discagem** (o familiar "tu-tu-tu" dos telefones antigos). Isso indica que o sistema está pronto.

### 2. Digite um número

Os jogadores digitam um número no teclado (ou giram o disco, na versão rotary):

- O sistema aceita números de até **8 dígitos**
- Se pararem de digitar por **3 segundos**, o número é processado automaticamente
- Cada dígito discado ou teclado emite o som correspondente (PULSO para disco rotativo ou DTMF para teclado)

### 3. Ouça a mensagem

O telefone toca uma mensagem de áudio correspondente ao número digitado:

| Resultado | O que acontece |
|-----------|----------------|
| **Número válido** | Toca uma mensagem de áudio (pista, instrução, história) |
| **Número não cadastrado** | Toca "esse número não existe" |

> **Exemplo:** Digitar `1234` toca a mensagem associada a esse número.

### 4. Ações especiais (opcional)

O sistema possui uma **saída controlada por relê** que pode ser acionada para executar ações especiais, como:
- Destravar uma fechadura
- Acionar um mecanismo
- Revelar um compartimento

**Para ativar a ação especial:**
Basta salvar um arquivo com extensão `.txt` para um número que já possua o arquivo `.mp3`. Por exemplo, para abrir uma porta ao discar `123`, você precisa ter dois arquivos:
- `123.mp3` — a mensagem de áudio
- `123.txt` — arquivo que indica que o relê deve ser acionado (o conteúdo não importa)

### 5. Desligue o telefone

Para fazer uma nova consulta, os jogadores devem **colocar o fone de volta no gancho**. Isso reinicia o sistema automaticamente.

---

## Para o Organizador do Jogo

### Configurando as mensagens

O sistema funciona como uma **central de áudio interativa**. Você define quais mensagens serão tocadas para cada número.

**Ideias de uso:**

| Cenário | Como usar |
|---------|-----------|
| **Central de pistas** | Números espalhados na sala revelam pistas quando digitados |
| **Telefone da gerência** | Um número "liga para o chefe" e dá a próxima instrução |
| **Linha direta** | Apenas um número específico funciona (ex: emergência) |
| **Narrativa** | Cada número representa um personagem diferente com informações úteis |

---

## Configuração do Cartão de Memória

O sistema usa um **cartão SD** para armazenar as mensagens personalizadas.

### Arquivos no cartão SD

Os sons do sistema (tom de discagem, beeps das teclas, mensagem "número não existe") já vêm configurados de fábrica e não podem ser alterados.

No cartão SD, você só precisa colocar as **mensagens personalizadas** que criar:

| Arquivo | Função |
|---------|--------|
| `[numero].mp3` | Mensagem para cada número (ex: `1234.mp3`) |
| `[numero].txt` | Arquivo que ativa a saída por relê (opcional) |

### Formato dos arquivos de áudio

- **Formato:** MP3
- **Taxa de bits:** 128 kbps (recomendado)
- **Frequência:** 44 kHz
- **Canais:** Estéreo

> **Nota:** Arquivos com tags ID3 podem não funcionar corretamente. Recomenda-se remover as tags antes de usar.

### Formato do cartão SD

- **Sistema de arquivos:** FAT32
- **Capacidade máxima:** 32 GB
- **Tamanho recomendado:** 2-8 GB são suficientes

### Compatibilidade de cartões

**Funcionam bem:**
- Cartões SD de classe 4, 6 ou 10
- Marcas conhecidas (SanDisk, Kingston, Samsung)

**Podem apresentar problemas:**
- Cartões SDXC (acima de 32 GB)
- Cartões de alta velocidade (UHS-I, UHS-II)
- Cartões muito antigos (menos de 1 GB)
- Adaptadores genéricos de microSD para SD

> **Dica:** Se o sistema não reconhecer o cartão, tente outro modelo ou formate novamente em FAT32.

---

## Saída Auxiliar por Relê

O sistema possui uma **saída por relê** para conectar dispositivos externos:

- **Fechaduras elétricas** — destrava uma porta ao digitar o número correto
- **Luzes** — aciona iluminação especial
- **Motores** — ativa mecanismos ou movimentações
- **Solenoides** — abre gavetas ou compartimentos secretos

### Ligação do relê

O cabo de saída possui três fios:
| Cor | Função |
|-----|--------|
| **Azul** | Comum (C) |
| **Preto** | Normalmente Aberto (NA) |
| **Marrom** | Normalmente Fechado (NF) |

> Conecte o fio **azul** ao comum da fechadura, e o **preto** ou **marrom** conforme o tipo de acionamento desejado.

### Como ativar a saída

Para que um número acione o relê, crie dois arquivos com o mesmo nome no cartão SD:
1. `[numero].mp3` — a mensagem de áudio
2. `[numero].txt` — arquivo que indica que deve acionar o relê (o conteúdo não importa)

### Cuidados com fechaduras

**Fonte de alimentação:** Use uma fonte de qualidade para alimentar a fechadura. Fontes ruins ou de baixa capacidade podem causar quedas de tensão que interferem no funcionamento do telefone.

**Proteção do circuito:** Fechaduras e solenoides devem possuir um **diodo flyback** (também chamado de diodo de roda livre) instalado em paralelo com a bobina. Isso protege o circuito do telefone contra picos de tensão gerados quando o relê desliga.

> **Importante:** Sem o diodo flyback adequado, o telefone pode apresentar mal funcionamento ou até mesmo danos permanentes.

## Resolução de problemas

### O telefone não emite som
1. Verifique se o telefone está ligado na energia
2. Confirme que o fone está fora do gancho
3. Aguarde alguns segundos após levantar o fone
4. Verifique se o cartão SD está inserido corretamente

### O teclado/disco não responde
1. Certifique-se de que o fone está fora do gancho
2. Verifique se há som de discagem (indica que está pronto)
3. Tente digitar mais lentamente

### A fechadura não aciona
1. Verifique se os arquivos estão corretos — é necessário ter **ambos** os arquivos `[numero].mp3` e `[numero].txt` para o relê acionar
2. Teste o número e aguarde a mensagem terminar — o relê só aciona ao final da reprodução
3. Verifique a fonte de alimentação da fechadura — fontes fracas podem não fornecer corrente suficiente
4. Ouça o relê — ao acionar, ele faz um **ruído de comutação característico** (estalo mecânico)
5. Confira a ligação dos fios — azul no comum, preto ou marrom conforme o tipo de acionamento

### Som ruim ou distorcido
1. Verifique se há obstáculos bloqueando o alto-falante
2. Verifique a qualidade dos arquivos MP3 — converta novamente se necessário
3. Em caso de chiado constante, verifique a fonte de alimentação

### Mensagens não aparecem
1. Confirme que os arquivos MP3 estão na raiz do cartão SD
2. Verifique se os nomes dos arquivos correspondem exatamente aos números digitados
3. Certifique-se de que o formato é MP3 e não outro formato de áudio

---

## Dicas para uma boa experiência

### Para os jogadores:
- **Escutem atentamente** — as mensagens não se repetem automaticamente
- **Anotem as informações** — pistas importantes podem estar nas mensagens
- **Explorem números** — números aparentemente aleatórios no cenário podem ser úteis
- **Se não souberem, tentem** — números errados não penalizam, apenas dizem "não existe"

### Para o organizador:
- **Teste todos os números** antes do jogo
- **Tenha um backup** das mensagens importantes
- **Prepare o gancho** — certifique-se de que o sensor físico está funcionando bem
- **Use cartões SD confiáveis** — cartões de baixa qualidade podem falhar durante o jogo

---

## Especificações

| Recurso | Descrição |
|---------|-----------|
| **Entrada** | Teclado numérico 4x3 ou disco rotativo |
| **Capacidade** | Até 8 dígitos por número |
| **Timeout** | 3 segundos sem digitar |
| **Áudio** | Mensagens personalizadas em MP3 |
| **Saída opcional** | Pode acionar fechaduras ou dispositivos |
| **Armazenamento** | Cartão SD até 32 GB (FAT32) |

---

## Suporte

Para problemas técnicos ou dúvidas de configuração, consulte a documentação técnica do sistema ou o fabricante.

---

**Versão:** 1.3  
**Data:** 2026-04-23
