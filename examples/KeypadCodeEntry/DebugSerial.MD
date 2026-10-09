
[HARDWARE] =========================================
[HARDWARE]    By   Nicolau dos Brinquedos - 2026
[HARDWARE] PropMaker Escape Room Engine Initializing
[HARDWARE] =========================================

[HARDWARE] =========================================
[HARDWARE]        Starting Diagnostic Boot
[HARDWARE] =========================================
[HARDWARE] Init I2C Bus
[HARDWARE] Scanning I2C Bus and initializing devices...
  - Found device at 0x18 (LIS3DH Accelerometer) - INITIALIZED
  - Found device at 0x20 (PCF857x) - DETECTED (use hw.addPcf8574(0x20) ou hw.addPcf8575)
  - Found device at 0x70 (HT16K33 Display @0x70) - INITIALIZED
[HARDWARE] =========================================
[HARDWARE] Hardware Detection Summary:
  - Port Expander (PCF857x): FOUND
  - LIS3DH Accelerometer: FOUND
  - PCA9685 PWM Driver: NOT FOUND
  - HT16K33 Display: FOUND
  - RTC PCF8563: NOT FOUND
  - OLED SSD1306: NOT FOUND
  - ADC (ADS1115): NOT FOUND
  - System Healthy: YES
[HARDWARE] =========================================
[HARDWARE] Init SPI Bus
[HARDWARE] External Power: ON
[HARDWARE] CHECKING FILE SYSTEMS:
[HARDWARE] LittleFS: MOUNTED
[HARDWARE] Scanning LittleFS directory: /
  [FILE] README.txt            (1585 bytes)
  [FILE] armed.mp3             (68279 bytes)
  [FILE] bomb.json             (218 bytes)
  [FILE] boom.mp3              (1128045 bytes)
  [FILE] error.mp3             (16947 bytes)
  [FILE] keypress.mp3          (17180 bytes)
  [FILE] sucess.mp3            (42597 bytes)
  [FILE] tick.mp3              (10493 bytes)
[HARDWARE] Found 8 entries in LittleFS.
[HARDWARE] SD Card: FAIL (Check card insertion)
[HARDWARE] Audio Amplifier (I2S): [AUDIO] I2S Hardware Stream: OK
[AUDIO] SD Card: Not Available
[AUDIO] LittleFS: Available
READY
[AUDIO] Storage: SD=NO, LittleFS=YES
[HARDWARE] NeoPixel Interno: Initialized
[HARDWARE] NeoPixel Externo: Initialized
[HARDWARE] Servo: ATTACHED
[HARDWARE] -----------------------------------------
[HARDWARE]       ENGINE INITIALIZATION COMPLETE
[HARDWARE] =========================================

[HARDWARE] Expander added at 0x20 (PCF8575 - 16 channels) [MANUAL]
[KEYPAD] Aguardando senha...
[PUZZLE 12] State Change: Initialising -> Idle
[PUZZLE 12] Manual Start Sequence (run() call)
[PUZZLE 12] State Change: Idle -> Running
[KEYPAD] Pressionado: 1
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes) << Corigir o código para usar esses arquivos direto na raiz do LittleFS
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 2
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 3
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 4
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] SENHA INCORRETA.
[AUDIO] Playing: /armed.mp3 (LittleFS, Loop: NO, Size: 68279 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 8
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 6
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 7
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 4
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] SENHA INCORRETA.
[AUDIO] Playing: /armed.mp3 (LittleFS, Loop: NO, Size: 68279 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 8
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 7
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 1
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 7
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] SENHA INCORRETA.
[AUDIO] Playing: /armed.mp3 (LittleFS, Loop: NO, Size: 68279 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 8
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 6
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 7
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] Pressionado: 1
[AUDIO] Playing: /tick.mp3 (LittleFS, Loop: NO, Size: 10493 bytes)
[AUDIO] Blocking playback complete.
[KEYPAD] ACERTOU!
[AUDIO] Playing: /sucess.mp3 (LittleFS, Loop: NO, Size: 42597 bytes)
[AUDIO] Blocking playback complete.
[PUZZLE 12] State Change: Running -> Solved
