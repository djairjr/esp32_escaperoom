# KeypadCodeEntry — Manual do GameMaster

## O que é este puzzle?

É uma fechadura eletrônica controlada por um teclado numérico. Os jogadores precisam descobrir um código de 4 dígitos e digitá-lo no teclado para abrir uma porta, gaveta ou compartimento secreto.

## Como funciona?

1. A caixa de controle exibe `----` no visor (ou no NeoPixel apagado), indicando que está pronta
2. O jogador digita 4 números no teclado
3. Se acertar o código, a trava elétrica abre por 5 segundos e o LED fica verde
4. Se errar, o LED pisca vermelho e pode tentar novamente (até 3 vezes, configurável)
5. Se errar todas as tentativas, o sistema bloqueia por 5 segundos e reinicia

## O que você precisa fornecer

- **Código secreto**: um número de 4 dígitos (ex: 8671, 1945, 2001)
- **Dica para os jogadores**: algo no cenário que leve a esse número (um quadro na parede, um bilhete, um livro com página marcada)

## Como configurar

### Pelo teclado (modo local)

1. Pressione `*` para entrar no modo de configuração
2. Digite o código atual (padrão de fábrica: 1234)
3. Digite o novo código de 4 dígitos
4. Pressione `#` para confirmar

### Pelo celular/tablet (Home Assistant)

Se o sistema estiver conectado ao WiFi da sala:

1. Abra o aplicativo Home Assistant
2. Encontre o dispositivo "KeypadCode-ESP32"
3. Altere o campo "Código" para o novo valor
4. Altere "Máx Tentativas" se quiser mais ou menos tentativas
5. Use o botão "Resetar" para reiniciar o puzzle a qualquer momento

### Pelo portal web (modo AP)

Se não houver WiFi configurado:

1. Ligue o dispositivo — ele criará uma rede WiFi chamada "KeypadCode-ESP32"
2. Conecte seu celular a essa rede
3. Abra o navegador e acesse `http://192.168.4.1`
4. Configure o WiFi da sala e o servidor MQTT
5. Salve — o dispositivo reiniciará conectado à rede

## Cuidados

- A trava elétrica funciona com 5V — use fonte adequada
- O código NUNCA aparece no visor durante o jogo (apenas underscores)
- Se a energia cair, o código volta ao último valor salvo
- O relé fica ativo por 5 segundos ao abrir — tempo suficiente para puxar uma porta

## Dicas para o cenário

- Esconda o teclado atrás de um painel falso ou quadro
- Use fios coloridos para dar pistas falsas (fio vermelho NÃO é o detonador!)
- Combine com um timer visível para aumentar a tensão
- Coloque um speaker com som de "acesso concedido" ao acertar o código
