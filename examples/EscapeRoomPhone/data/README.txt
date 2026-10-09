This phone example uses a hybrid storage system:

Files in LittleFS memory (System effects):
- dialtone.mp3   (Dial tone)
- busy.mp3       (Busy signal)
- naoexiste.mp3  (Invalid number notice)

Files on the SD Card (Dynamic content):
- [number].mp3   (Audio files for each dialed number, e.g. 123.mp3)

Tip: On LittleFS, files must be in the root (without the sfx/ folder).
