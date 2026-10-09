# STATUS - Mastermind

## O que foi feito
- Jogo de adivinhação de cores com teclado (teclas 1-6 para cores 1-6)
- 4 posições, feedback P (posição correta) e E (cor certa lugar errado)
- Algoritmo: copia secret+guess, remove exatas primeiro, depois checa cores restantes
- NeoPixel: LEDs 0-3 mostram cores do palpite, LEDs 4-7 mostram P/E
- Display HT16K33: mostra "PxEy" após cada tentativa
- 10 tentativas máximas, configurável codeLength/maxColors/maxAttempts
- Comandos Serial: reset, status
- Configuração via /mastermind.json

## Status de compilação
Sucesso: 253192 bytes (3%), 16216 bytes RAM (6%)

## Decisões pendentes
1. **codeLength = 4:** Dificuldade adequada? 3 para fácil, 5 para difícil?
2. **maxColors = 6:** Usar todas as 6 cores ou menos?
3. **maxAttempts = 10:** Número ideal de tentativas?
4. **lockPin = 20:** Pino correto para a placa?
5. **neoCount = 8:** 4 para palpite + 4 para feedback. Suficiente?
6. **Cores no NeoPixel:** As cores atuais (RGB) são viáveis ou precisa de calibração?
7. **Teclado via PCF8575:** Endereço 0x20 configurado corretamente?
8. **Criar assets de áudio:** beep.mp3, magic.mp3, error.mp3, cancel.mp3

## Configuração
examples/Mastermind/data/mastermind.json

## Assets necessários
- /sfx/beep.mp3 (ao pressionar tecla)
- /sfx/magic.mp3 (puzzle resolvido)
- /sfx/error.mp3 (esgotou tentativas)
- /sfx/cancel.mp3 (limpou palpite)
