Include the lock.json file in LittleFS and the following audio files in /sfx/:

Required files:
- /sfx/beep.mp3     (Key press beep)
- /sfx/error.mp3    (Error/access denied sound)
- /sfx/magic.mp3    (Access granted sound)
- /sfx/cancel.mp3   (Reset/clear sound)

Note: If /lock.json does not exist in LittleFS, the code uses the built-in
default values (code=1234, mode=KEYPAD_OR_RFID, lockPin=10).
