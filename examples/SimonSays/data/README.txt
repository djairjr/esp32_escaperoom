ARQUIVOS DE ÁUDIO NECESSÁRIOS
=============================

Coloque os seguintes arquivos MP3 nesta pasta para upload no LittleFS ou SD Card:

FORMATO OBRIGATÓRIO:
--------------------
- Taxa de bits: 128 Kbps
- Taxa de amostragem: 44.1 kHz
- Canais: Estéreo
- Formato: MP3 sem tags ID3

Para converter com ffmpeg:
ffmpeg -y -i input.mp3 -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 output.mp3

ARQUIVOS NECESSÁRIOS:
---------------------

1. ready.mp3
   - Som de sistema pronto
   - Tocado no boot

2. begin.mp3
   - Som de início do jogo
   - Tocado na animação de abertura

3. red.mp3
   - Som do botão vermelho
   - Tocado quando o anel vermelho acende

4. blue.mp3
   - Som do botão azul
   - Tocado quando o anel azul acende

5. yellow.mp3
   - Som do botão amarelo
   - Tocado quando o anel amarelo acende

6. green.mp3
   - Som do botão verde
   - Tocado quando o anel verde acende

7. correct.mp3
   - Som de acerto
   - Tocado ao completar uma sequência

8. error.mp3
   - Som de erro
   - Tocado quando o jogador erra

9. victory.mp3
   - Som de vitória
   - Tocado quando o puzzle é resolvido

DICA: Os sons das cores podem ser notas musicais diferentes
      (ex: Dó, Ré, Mi, Fá) ou efeitos sonoros distintos.

Upload para LittleFS:
- Use o plugin "Arduino ESP32 LittleFS Uploader" na IDE Arduino
- Ou use a ferramenta de linha de comando littlefs-python

NOTA SOBRE ARQUIVOS *_sfx.mp3:
--------------------------------
Os arquivos com sufixo "_sfx.mp3" são os originais em formato incorreto
(24kHz mono 48Kbps). Eles foram mantidos apenas como backup.
Para o funcionamento correto, use APENAS os arquivos convertidos
(.mp3 sem sufixo _sfx) que estão no formato 44.1kHz estéreo 128Kbps.

ARQUIVOS CONVERTIDOS (use estes):
- ready.mp3, begin.mp3, victory.mp3
- red.mp3, blue.mp3, yellow.mp3, green.mp3
- correct.mp3, error.mp3
