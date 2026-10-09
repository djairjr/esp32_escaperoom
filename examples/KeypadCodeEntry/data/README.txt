Include the keypad.json file in LittleFS:

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

Audio files required in the root folder of LittleFS or SD:
- /sfx/beep.mp3     (Key press beep)
- /sfx/error.mp3    (Error/wrong password sound)
- /sfx/magic.mp3    (Correct/success sound)
- /sfx/cancel.mp3   (Sound when clearing the entry)

Note: If the keypad.json file does not exist, the code uses the built-in
default values (code=8671, lockPin=20).
