KeypadTester - Pinout Detector for Matrix Keypads
============================================================

This example automatically identifies which pins of the PCF8574/PCF8575
are connected to the ROWS and COLUMNS of a matrix keypad.

WHY USE IT?
-------------
When you solder wires directly to the keypad and the PCF module, it is difficult
to know which pins are rows and which are columns. This sketch solves that.

CONNECTIONS
--------
1. Connect 7 keypad pins to 7 pins of the PCF857x (any 7)
2. For PCF8575: use P00-P06 (pins 0-6)
3. For PCF8574: use P0-P6 (pins 0-6)

CODE CONFIGURATION
----------------------
Edit these lines at the beginning of the sketch:

  #define PCF_ADDRESS 0x20      // PCF I2C address
  #define PCF_IS_8575 true      // true for PCF8575, false for PCF8574

  const byte pinoHardware[7] = {0,1,2,3,4,5,6};  // Physical pinout used

HOW TO USE
---------
1. Upload the sketch
2. Open the Serial Monitor (115200 baud)
3. When prompted, press and hold each key
4. Release when indicated
5. At the end, the sketch shows the rowPins[] and colPins[] pinout

RESULT
---------
The sketch displays ready-to-copy code:

  byte rowPins[4] = {X, X, X, X};
  byte colPins[3] = {X, X, X};

  MatrixKeypadPcf keypad(hw.portExpander(), rowPins, 4, colPins, 3, keymap);

VALIDATION
---------
After detecting the pinout, the sketch enters test mode. Press
keys to validate whether they are being read correctly.

NOTES
-----
- 3x4 keypad = 12 keys (1-9, *, 0, #)
- For a 4x4 keypad, adapt the key order in the code
- The PCF8575 must be in 16-bit mode (all pins P00-P15)
