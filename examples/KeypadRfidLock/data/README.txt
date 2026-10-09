Inclua o arquivo lock.json no LittleFS e os seguintes audios em /sfx/:

Arquivos necessarios:
- /sfx/beep.mp3     (Bip de tecla pressionada)
- /sfx/error.mp3    (Som de erro/acesso negado)
- /sfx/magic.mp3    (Som de acesso liberado)
- /sfx/cancel.mp3   (Som ao resetar/limpar)

Nota: Se /lock.json nao existir no LittleFS, o codigo usa valores padrao
embutidos (code=1234, mode=KEYPAD_OR_RFID, lockPin=10).
