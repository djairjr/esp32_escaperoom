# User Manual - Vintage Shortwave Radio

## For Escape Room Owners

**Version:** 2.0 (PropMaker Edition)  
**Puzzle:** Vintage Radio with FM Tuning and Morse Code  
**Level:** Intermediate  
**Estimated Time:** 10-15 minutes

---

## Puzzle Overview

This is a **vintage shortwave radio** that players must "repair" to receive a secret message. Its operation imitates an old tube radio.

### Physical Components

1. **Transformer** (main part - connects first)
2. **3 Tubes** (connect to the radio to activate the FM)
3. **1 Final Tube** (connects last to receive the message)
4. **Tuning Knob** (rotary encoder)
5. **Radio Box** with indicator LEDs

### Player Objective

1. Turn on the radio (connect the transformer)
2. Insert the 3 tubes to activate the radio system
3. Explore the radio stations by turning the knob
4. Insert the final tube to receive the message in Morse code
5. Write down the message and use it to leave the room

---

## How it Works (For the Player)

### Step by Step of the Experience

**1. Turning on the Radio**
- The player connects the transformer to the box
- The white LED (Power) turns on
- The radio is "on" but still does not work

**2. Inserting the Tubes**
- As each tube is inserted, a blue LED turns on
- The system makes different sounds for each one:
  - **Tube 1:** Short sound (beep beep beep)
  - **Tube 2:** Long sound (beeeep beeeep beeeep)
  - **Tube 3:** The radio "comes alive" - it now makes radio noise!

**3. Tuning the Radio**
- With the 3rd tube inserted, the knob works
- Turning the knob, the player hears different radio stations
- Each station has a different interference/static sound
- The volume increases and decreases as the knob is turned (simulating tuning).

**4. The Solution**
- The player needs to insert the **4th tube (Final Tube)**
- This stops the radio noise and starts the message in Morse code
- The message is transmitted in beeps by the radio
- The players need to decipher the Morse code

**5. Deciphering**
- The message in Morse code is repeated 5 times
- The player writes down the translation (it comes written in the normal alphabet)
- The message contains the password or hint for the next step of the game

---

## Configuration For You (Operator)

### Before the Game

#### 1. The Secret Message (IMPORTANT!)

You can change the message that will be transmitted in Morse code.

**How to do it:**
1. Remove the SD card from the system
2. Open the file `mensagem.txt` on your computer.
3. Write your message (you can use accents and punctuation)
4. Save the file
5. Put the SD card back in the system

**Example messages:**
```
CONGRATULATIONS THE CODE IS 4729

THE DOOR OPENS WITH THE RED CODE

LOOK FOR THE BOX IN THE CLOSET

THE NEXT DIGIT IS 7
```

**Rules:**
- Use short phrases (maximum 256 characters)
- You can use letters A-Z, numbers 0-9 and spaces
- Accents are accepted (they are converted automatically)
- The message is read from the SD card first, then from internal memory

#### 2. The Radio Station Sounds

The radio has **9 stations** that the player can explore. Each one plays a different MP3 file.

**Required files:**
- `station1.mp3` through `station9.mp3`

**Where to put them:**
- **Option 1 (recommended):** SD card at the root (main folder)
- **Option 2:** Internal memory (LittleFS)

**Priority:** If the same file exists on the SD card and in internal memory, the **SD card wins**.

**This allows:** You can have a standard version in internal memory, and put special versions on the SD for specific events (for example, a station with extra hints or customized messages for different groups).

**What kind of audio to use:**
- Sounds of interfering radio (static, hiss)
- Old radio broadcast sounds
- Any MP3 at 128kbps, 44.1kHz, stereo
- Duration: can be short (automatic loop)

**Suggestion:** Use different sounds for each station to create the feeling of "different channels". If you don't know where to get them, you can leave the default files that already come with the system.

### Test Before Use

**Test procedure:**

1. **Reset:** Disconnect all the parts
2. **Turn on:** Connect the transformer
   - The Power LED should turn on (white)
3. **Tube 1:** Connect the first tube
   - Blue LED turns on + short beep sound
4. **Tube 2:** Connect the second tube
   - Blue LED turns on + long beep sound
5. **Tube 3:** Connect the third tube
   - Blue LED turns on + radio starts to "make noise"
   - Turn the knob - you should hear different stations
6. **Tube 4:** Connect the final tube
   - The radio stops making noise
   - It starts transmitting Morse code
   - The message should be clear and repeat 5 times

**If something does not work:**
- Check that all connections are firm
- Restart by disconnecting and reconnecting the transformer
- Check that the SD card is inserted correctly
- Replace the SD card if necessary

---

## Troubleshooting

### Problem: The radio does not turn on

**Possible causes:**
1. Transformer not connected correctly
2. Problem in the electrical connection
3. System locked up

**Solutions:**
- Disconnect and reconnect the transformer firmly
- Check that the Power LED turns on
- If it persists, turn the system completely off and on
- Check that the USB cable is connected (if using USB power)

### Problem: The tubes do not work

**Possible causes:**
1. The tubes need to be connected in order
2. Loose connection
3. System was not reset

**Solutions:**
- Always connect in order: Tube 1 → 2 → 3 → 4
- Press firmly until you hear/see the feedback
- Each tube should turn on its blue LED
- Disconnect the transformer and start again

### Problem: The tuning knob does not work

**Possible causes:**
1. The 3rd tube is not connected
2. Problem in the encoder

**Solutions:**
- Check that the 3rd tube is connected (blue LED on)
- The knob only works AFTER the 3rd tube is inserted
- Turn the knob slowly to test
- Check that the encoder is well connected

### Problem: I don't hear the radio stations

**Possible causes:**
1. Missing MP3 files or wrong format
2. Volume is too low.
3. Problem on the SD card
4. Problem in the speaker

**Solutions:**
- Check that the files `station1.mp3` through `station9.mp3` exist on the SD
- Format must be MP3, 128kbps, 44.1kHz
- Try replacing the SD card
- Check that the speaker is connected

### Problem: The Morse message does not play

**Possible causes:**
1. The file `mensagem.txt` is missing.
2. The file `mensagem.txt` is empty.
3. SD card is not being read

**Solutions:**
- Make sure that `mensagem.txt` exists at the root of the SD card.
- Open the file and confirm that it has text inside
- If the file is missing, the system uses "SOS" as default
- Try another SD card

### Problem: Sound with noise or distorted

**Possible causes:**
1. MP3 files with inadequate format
2. ID3 tags in the MP3s causing problems
3. Slow or corrupted SD card

**Solutions:**
- Reconvert the MP3s using the ffmpeg command:
  ```
  ffmpeg -i input.mp3 -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 output.mp3
  ```
- This removes ID3 tags and standardizes the format
- Format the SD card in FAT32

### Problem: The radio freezes or restarts by itself

**Possible causes:**
1. Weak power supply
2. Transformer was removed (intentional HARD RESET)
3. System needs a reset

**Solutions:**
- Use a 5V supply with at least 2A
- The HARD RESET is normal when you remove the transformer - reconnect to continue
- If it freezes, disconnect everything and reconnect the transformer

---

## Care and Maintenance

### Cleaning
- Clean the tube connections with a dry cloth periodically
- Avoid humidity in the system
- Keep the tuning knob clean
- Do not use chemical products directly on the connectors

### Storage
- Store the tubes in a dry place when not in use
- Avoid leaving the transformer connected for long periods without use
- Protect the SD card from dust and humidity

### File Backup
- Keep a copy of the files `station*.mp3` and `mensagem.txt` on your computer.
- If the SD card gets corrupted, you can recreate them easily
- Test the system weekly to ensure it works

---

## Tips For Your Game

### Difficulty

**Easy:**
- Place the message in Morse written somewhere in the room
- The players only need to identify the pattern
- The 4th tube can be visible and accessible

**Medium:**
- Provide a Morse code table in another puzzle
- The players need to decipher the message
- The 4th tube can be hidden or in another location

**Hard:**
- No external help - the players need to know or discover Morse code
- The message can be an encrypted clue
- The 4th tube only appears after solving another puzzle

### Time

- The Morse transmission repeats **5 times**
- Each cycle takes a few seconds
- Total time: about 1-2 minutes to hear everything
- Leave enough time for the players to write it down
- The FM system can stay on indefinitely - there is no rush

### Combine with Other Puzzles

The Morse code can be:
- A numeric password for a padlock
- Coordinates to find a hidden object
- The first part of a larger password
- A hint about another puzzle in the room
- The code for an electronic safe

---

## Pre-Game Checklist

Use this list before each group of players:

- [ ] Transformer disconnected (initial position)
- [ ] All tubes disconnected
- [ ] SD card inserted correctly
- [ ] Quick test: connect the transformer → Power LED turns on?
- [ ] Test: tube 1 → blue LED + short sound?
- [ ] Test: tube 2 → blue LED + long sound?
- [ ] Test: tube 3 → blue LED + radio noise?
- [ ] Test: turn the knob → changes station?
- [ ] Test: tube 4 → Morse message plays?
- [ ] Is the message configured in the `mensagem.txt` file correct?
- [ ] Audio files available (station1-9.mp3)
- [ ] A volume that’s suitable for the room

---

## Basic Technical Information

### Specifications

| Item | Specification |
|------|---------------|
| Power | 5V DC (via USB or supply) |
| Consumption | ~500mA typical, 1.5A peak |
| SD Card | MicroSD, FAT32 format |
| Audio | MP3, 128kbps, 44.1kHz, stereo |
| Connections | 4mm banana plugs |

### PCF8575 Pins (for technical reference)

| Pin | Function |
|------|--------|
| P0 | Transformer |
| P1 | Tube 1 |
| P2 | Tube 2 |
| P3 | Tube 3 |
| P4 | Encoder A |
| P5 | Encoder B |
| P6 | Encoder Button |
| P7 | Tube 4 |

---

## Technical Support

If you encounter problems that you cannot solve:

1. Check that all cables are connected correctly
2. Test with a different SD card
3. Check the logs on the serial monitor (if you have technical access)
4. Refer to the file `ALGORITMO.MD` for complete technical details.
5. Contact the manufacturer

### Contact

For technical support or questions about the puzzle:

**GitHub:** https://github.com/djairhector/propmaker_escaperoom

---

## Quick Summary

| Component | Function | When to Use |
|------------|--------|-------------|
| Transformer | Turns on the system | Always first |
| Tube 1 | Activates short sound | After the transformer |
| Tube 2 | Activates long sound | After tube 1 |
| Tube 3 | Activates the FM radio | After tube 2 |
| Knob | Changes stations | After tube 3 |
| Tube 4 | Receives Morse message | Last - activates the solution |

**Important files on the SD:**
- `mensagem.txt` – The message in Morse code
- `station1.mp3` through `station9.mp3` - Station sounds

**File priority:** SD Card > Internal memory

---

*Manual for Escape Room Owners*  
*PropMaker EscapeRoom System - Vintage Radio*  
*Version 2.0 - Updated with the 4-tube system and SD Card priority*
