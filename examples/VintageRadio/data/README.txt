================================================================================
VintageShortwaveMorseRadio - Arquivos de Dados
================================================================================

Este arquivo contém os recursos de áudio e dados necessários para o puzzle
VintageShortwaveMorseRadio.

--------------------------------------------------------------------------------
ARQUIVOS OBRIGATÓRIOS
--------------------------------------------------------------------------------

1. mensagem.txt
   - Arquivo de texto com a mensagem a ser transmitida em código Morse
   - Máximo 128 caracteres
   - Apenas caracteres: A-Z, 0-9 e espaços
   - Sem caracteres especiais (#, @, <, >, etc.)
   - Exemplo: "PARABENS VOCE RESOLVEU O PUZZLE"

--------------------------------------------------------------------------------
ARQUIVOS OPCIONAIS (Sistema de Sintonia Avançado)
--------------------------------------------------------------------------------

Para o sistema de sintonia de rádio com múltiplas estações, copie estes arquivos
MP3 para o cartão SD (pasta / no LittleFS):

  station1.mp3 - Estação 1: Estática/ruído de fundo
  station2.mp3 - Estação 2: Música ambiente 1
  station3.mp3 - Estação 3: Voz/news
  station4.mp3 - Estação 4: Música ambiente 2
  station5.mp3 - Estação 5: ★ ESTAÇÃO CORRETA (contém dica importante)
  station6.mp3 - Estação 6: Interferência/estática
  station7.mp3 - Estação 7: Música ambiente 3
  station8.mp3 - Estação 8: Voz em outro idioma
  station9.mp3 - Estação 9: Estática final

--------------------------------------------------------------------------------
FORMATO MP3
--------------------------------------------------------------------------------

- Taxa de bits: 128 kbps (recomendado) ou 192 kbps
- Frequência: 44.1 kHz
- Canais: Estéreo ou Mono
- IMPORTANTE: Remover todas as tags ID3 antes de usar

Como remover ID3 tags com ffmpeg:
  ffmpeg -i input.mp3 -map_metadata -1 -c copy output.mp3

--------------------------------------------------------------------------------
ESTRUTURA DO CARTÃO SD
--------------------------------------------------------------------------------

/
├── mensagem.txt          (obrigatório)
├── station1.mp3          (opcional)
├── station2.mp3          (opcional)
├── station3.mp3          (opcional)
├── station4.mp3          (opcional)
├── station5.mp3          (opcional - estação correta)
├── station6.mp3          (opcional)
├── station7.mp3          (opcional)
├── station8.mp3          (opcional)
└── station9.mp3          (opcional)

================================================================================
