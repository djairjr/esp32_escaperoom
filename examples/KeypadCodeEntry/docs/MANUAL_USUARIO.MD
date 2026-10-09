# KeypadCodeEntry — GameMaster Manual

## What is this puzzle?

It is an electronic lock controlled by a numeric keypad. Players must discover a 4-digit code and enter it on the keypad to open a door, drawer or secret compartment.

## How does it work?

1. The control box shows `----` on the display (or the NeoPixel is off), indicating that it is ready
2. The player enters 4 numbers on the keypad
3. If the code is correct, the electric strike opens for 5 seconds and the LED turns green
4. If wrong, the LED blinks red and they can try again (up to 3 times, configurable)
5. If all attempts are wrong, the system locks for 5 seconds and restarts

## What you need to provide

- **Secret code**: a 4-digit number (e.g. 8671, 1945, 2001)
- **Hint for the players**: something in the scenery that leads to this number (a picture on the wall, a note, a book with a marked page)

## How to configure

### From the keypad (local mode)

1. Press `*` to enter configuration mode
2. Enter the current code (factory default: 1234)
3. Enter the new 4-digit code
4. Press `#` to confirm

### From a phone/tablet (Home Assistant)

If the system is connected to the room's WiFi:

1. Open the Home Assistant app
2. Find the device "KeypadCode-ESP32"
3. Change the "Code" field to the new value
4. Change "Max Attempts" if you want more or fewer attempts
5. Use the "Reset" button to restart the puzzle at any time

### From the web portal (AP mode)

If there is no WiFi configured:

1. Turn on the device — it will create a WiFi network called "KeypadCode-ESP32"
2. Connect your phone to this network
3. Open the browser and go to `http://192.168.4.1`
4. Configure the room's WiFi and the MQTT server
5. Save — the device will restart connected to the network

## Precautions

- The electric strike runs on 5V — use a suitable power supply
- The code NEVER appears on the display during the game (only underscores)
- If the power fails, the code returns to the last saved value
- The relay stays active for 5 seconds when opening — enough time to pull a door

## Scenery tips

- Hide the keypad behind a false panel or picture
- Use colored wires to give false clues (the red wire is NOT the detonator!)
- Combine it with a visible timer to increase the tension
- Place a speaker with an "access granted" sound when the code is correct
