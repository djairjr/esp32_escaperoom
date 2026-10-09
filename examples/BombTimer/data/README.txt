Include the following MP3 files in LittleFS memory (via Arduino Pico LittleFS Uploader):

REQUIRED SOUNDS:
- tick.mp3           Tick-tock every second (can be disabled in the "tic" menu)
- armed.mp3          Plays when arming the bomb (DETONATE button in Idle)
- boom.mp3           Plays on detonation
- success.mp3        Plays on successful defusal
- keypress.mp3       Short beep on each key press and menu button
- error.mp3          Plays on wrong code, shake-pause or critical error
- menu_confirm.mp3   Confirms selection in the configuration menu

CONFIGURATION (bomb.json - also in this folder):
- ctdn   : Duration in seconds (e.g. 600 = 10 minutes)
- mode   : 0=Keypad, 1=Single wire, 2=Wire order, 3=RS485 network
- code   : 4-digit code for Keypad mode
- ordr   : Wire order (e.g. 1234 = wires in sequence 1, 2, 3, 4)
- detW   : Detonation wire (0=none, 1=random, 2-5=wires 1-4)
- dffW   : Defuse wire (same mapping)
- spdW   : Double-speed wire (0=none, 1=random, 2-5)
- psW    : Temporary pause wire
- vol    : Volume (0-30)
- bri    : Display brightness (0-15)
- tdEt   : Detonation relay pulse duration in seconds
- tdEF   : Defuse relay pulse duration in seconds
- rs485  : 0=disabled, 1=active
- netC   : Number of puzzles on the network for defuse via mode 3
- shak   : 0=none, 1=double speed, 2=pause, 3=detonate
- tic    : 0=silent, 1=plays tick.mp3 every second

Make sure the bomb.json (also in this folder) is configured correctly before uploading.
