REQUIRED AUDIO FILES
=============================

Place the following MP3 files in this folder for upload to LittleFS or the SD Card:

MANDATORY FORMAT:
--------------------
- Bit rate: 128 Kbps
- Sample rate: 44.1 kHz
- Channels: Stereo
- Format: MP3 without ID3 tags

To convert with ffmpeg:
ffmpeg -y -i input.mp3 -acodec libmp3lame -ar 44100 -ac 2 -b:a 128k -map_metadata -1 output.mp3

REQUIRED FILES:
---------------------

1. ready.mp3
   - System ready sound
   - Played on boot

2. begin.mp3
   - Game start sound
   - Played during the opening animation

3. red.mp3
   - Red button sound
   - Played when the red ring lights up

4. blue.mp3
   - Blue button sound
   - Played when the blue ring lights up

5. yellow.mp3
   - Yellow button sound
   - Played when the yellow ring lights up

6. green.mp3
   - Green button sound
   - Played when the green ring lights up

7. correct.mp3
   - Correct sound
   - Played when a sequence is completed

8. error.mp3
   - Error sound
   - Played when the player makes a mistake

9. victory.mp3
   - Victory sound
   - Played when the puzzle is solved

TIP: The color sounds can be different musical notes
      (e.g. Do, Re, Mi, Fa) or distinct sound effects.

Upload to LittleFS:
- Use the "Arduino ESP32 LittleFS Uploader" plugin in the Arduino IDE
- Or use the littlefs-python command-line tool

NOTE ABOUT *_sfx.mp3 FILES:
--------------------------------
Files with the "_sfx.mp3" suffix are the originals in the incorrect format
(24kHz mono 48Kbps). They were kept only as a backup.
For correct operation, use ONLY the converted files
(.mp3 without the _sfx suffix) that are in 44.1kHz stereo 128Kbps format.

CONVERTED FILES (use these):
- ready.mp3, begin.mp3, victory.mp3
- red.mp3, blue.mp3, yellow.mp3, green.mp3
- correct.mp3, error.mp3
