================================================================================
VintageShortwaveMorseRadio - Data Files
================================================================================

This file contains the audio resources and data needed for the
VintageShortwaveMorseRadio puzzle.

--------------------------------------------------------------------------------
REQUIRED FILES
--------------------------------------------------------------------------------

1. message.txt
   - Text file with the message to be transmitted in Morse code
   - Maximum 128 characters
   - Only characters: A-Z, 0-9 and spaces
   - No special characters (#, @, <, >, etc.)
   - Example: “PARABENS, YOU HAVE SOLVED THE PUZZLE”

--------------------------------------------------------------------------------
OPTIONAL FILES (Advanced Tuning System)
--------------------------------------------------------------------------------

For the radio tuning system with multiple stations, copy these MP3 files
to the SD card (folder / on LittleFS):

  station1.mp3 - Station 1: Static/background noise
  station2.mp3 - Station 2: Ambient music 1
  station3.mp3 - Station 3: Voice/news
  station4.mp3 - Station 4: Ambient music 2
  station5.mp3 - Station 5: ★ CORRECT STATION (contains an important hint)
  station6.mp3 - Station 6: Interference/static
  station7.mp3 - Station 7: Ambient music 3
  station8.mp3 - Station 8: Voice in another language
  station9.mp3 - Station 9: Final static

--------------------------------------------------------------------------------
MP3 FORMAT
--------------------------------------------------------------------------------

- Bit rate: 128 kbps (recommended) or 192 kbps
- Frequency: 44.1 kHz
- Channels: Stereo or Mono
- IMPORTANT: Remove all ID3 tags before use

How to remove ID3 tags with ffmpeg:
  ffmpeg -i input.mp3 -map_metadata -1 -c copy output.mp3

--------------------------------------------------------------------------------
SD CARD STRUCTURE
--------------------------------------------------------------------------------

/
├── mensagem.txt          (Required)
├── station1.mp3          (optional)
├── station2.mp3          (optional)
├── station3.mp3          (optional)
├── station4.mp3          (optional)
├── station5.mp3          (optional - correct station)
├── station6.mp3          (optional)
├── station7.mp3          (optional)
├── station8.mp3          (optional)
└── station9.mp3          (optional)

================================================================================
