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

Starting PCF8575 at 0x20
PCF857x OK

=== PINOUT DETECTION ===
Follow the instructions for each key.

Press and HOLD the key '1'...

========================================
  KEYPAD TESTER - PropMaker EscapeRoom
========================================

[HARDWARE] =========================================
[HARDWARE] By Nicolau dos Brinquedos – 2026
[HARDWARE] PropMaker Escape Room Engine Initializing
[HARDWARE] =========================================

[HARDWARE] =========================================
[HARDWARE]        Starting Diagnostic Boot
[HARDWARE] =========================================
[HARDWARE] Init I2C Bus
[HARDWARE] Scanning I2C Bus and initializing devices...
  - Found device at 0x18 (LIS3DH Accelerometer) - INITIALIZED
  - Found device at 0x20 (PCF8575 - 16 channels) - DETECTED << Correct detection. Very good.
  - Found device at 0x48 (ADS1115 I2C) - DETECTED
  - Found device at 0x70 (HT16K33 Display @0x70) - INITIALIZED
  - ADS1115 (I2C): INITIALIZED
[HARDWARE] =========================================
[HARDWARE] Hardware Detection Summary:
  - Port Expander (PCF857x): FOUND << Here it should show the correct name (PCF8575 or PCF8574)
  - LIS3DH Accelerometer: FOUND
  - PCA9685 PWM Driver: NOT FOUND
  - HT16K33 Display: FOUND
  - RTC PCF8563: NOT FOUND
  - OLED SSD1306: NOT FOUND
  - ADC (ADS1115): FOUND
  - System Healthy: YES
[HARDWARE] =========================================
[HARDWARE] Initializing Port Expander: PCF8575 (16 channels)
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
  [FILE] tick.mp3              (17180 bytes)
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

Starting PCF8575 at 0x20
PCF857x OK << Here it should show the correct name, or just OK after the String above.

=== PINOUT DETECTION ===
Follow the instructions for each key.

Press and HOLD the key '1'... << Does not advance from this point, that is, it does not detect. 

Check how it was done in the example <local keypad pinout detection project>
without using the Library and replicate the same logic.
