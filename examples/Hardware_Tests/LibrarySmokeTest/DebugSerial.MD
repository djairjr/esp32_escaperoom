
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
  - Found device at 0x20 (PCF857x) - DETECTED (use hw.addPcf8574(0x20) or hw.addPcf8575)
  - Found device at 0x21 (PCF857x) - DETECTED (use hw.addPcf8574(0x21) or hw.addPcf8575)
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
  - System Healthy: YES << INCLUDE A BATTERY VOLTAGE SAMPLE USING THE NEW METHOD FOR LOWPOWER
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

[HARDWARE] Expander added at 0x20 (PCF8575 - 16 channels) [MANUAL]

== propmaker_escaperoom smoke test ==
System Healthy: YES
[HWTEST] Setup Hardware Test Puzzle
Setup complete - tests running in loop
[HWTEST] NeoPixel Internal: RED
[HWTEST] NeoPixel Internal: GREEN
[HWTEST] NeoPixel Internal: BLUE
[HWTEST] NeoPixel Internal: OK
[HWTEST] NeoPixel External: RED
[HWTEST] NeoPixel External: GREEN
[HWTEST] NeoPixel External: BLUE
[HWTEST] NeoPixel External: OK
[HWTEST] PCF857x: OK (read/write test passed)
[HWTEST] LIS3DH: OK (G=10.16)
[HWTEST] PCA9685: SKIP (not detected)
[HWTEST] Audio: OK (file found)
[HWTEST] Audio: /armed.mp3 size=68279 bytes
[HWTEST] Audio: Starting playback...
[AUDIO] Playing: /armed.mp3 (LittleFS, Loop: NO, Size: 68279 bytes)
[AUDIO] Blocking playback complete.
[HWTEST] Audio: playOnce() called
[HWTEST] Audio: stopped
