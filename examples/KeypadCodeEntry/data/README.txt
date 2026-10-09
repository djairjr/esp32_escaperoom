Inclua o arquivo keypad.json no LittleFS:

data/keypad.json:
{
  "code": "8671",
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

Arquivos de audio necessarios na pasta raiz do LittleFS ou SD:
- /sfx/beep.mp3     (Bip de tecla pressionada)
- /sfx/error.mp3    (Som de erro/senha incorreta)
- /sfx/magic.mp3    (Som de acerto/sucesso)
- /sfx/cancel.mp3   (Som ao limpar a entrada)

Nota: Se o arquivo keypad.json nao existir, o codigo usa os valores padrao
embutidos (code=8671, lockPin=20).
