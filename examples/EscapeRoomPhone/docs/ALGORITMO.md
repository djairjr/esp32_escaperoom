# EscapeRoomPhone — Algoritmo do Jogo

## Visão Geral

Simulador de telefone antigo. O jogador tira o fone do gancho, ouve tom de discagem, disca um número (keypad ou disco rotativo), e ouve uma mensagem de áudio correspondente. Se o número discado tiver um arquivo .txt associado, a fechadura elétrica é destravada.

## Configuração

- **Entrada**: Keypad I2C (PCF8574), Keypad GPIO direto, ou Rotary Dial
- **Hardware**: hookPin (sensor de gancho), lockPin (relé da trava), externalPower
- **Timeout**: DIAL_TIMEOUT_MS=3000 (tempo máximo entre dígitos)
- **Máximo dígitos**: MAX_PHONE_DIGITS=8

## Estados

IDLE → OFF_HOOK → DIALING → PLAYING → BUSY → IDLE (gancho)

## Fluxo do Jogo

1. **Gancho no lugar**: sistema em repouso, qualquer áudio é interrompido
2. **Fone levantado**: toca tom de discagem em loop
3. **Discagem**:
   - Keypad: teclas 0-9, *, # com feedback DTMF
   - Rotary: pulsos decodificados com feedback de pulso
   - Timeout de 3s ou 8 dígitos → processa chamada
4. **Processamento**:
   - Verifica se arquivo `/NUMERO.mp3` existe no SD
   - Se sim: toca o áudio
   - Se houver `/NUMERO.txt`: ativa lockPin (destrava porta)
   - Se não: toca mensagem de número inválido
5. **Pós-áudio**: toca tom de ocupado até fone ser recolocado
6. **Gancho recolocado**: reseta tudo, para qualquer áudio

## Feedback

- Áudio: dialtone, DTMF por tecla, pulso por disco, mensagem de áudio, busy tone
- Não usa NeoPixel nem display (áudio apenas)

## AP Mode + WiFi

Configuração de rede via portal web em 192.168.4.1.
