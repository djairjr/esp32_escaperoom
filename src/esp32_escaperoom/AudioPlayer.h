#pragma once

#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/CoreAudio/AudioEffects/SoundGenerator.h"

// Undefine AudioTools macros that conflict with our constexpr pin names
// audio-tools/PlatformConfig/esp32.h defines:
//   PIN_I2S_BCK=6, PIN_I2S_WS=7, PIN_I2S_DATA_OUT=8, PIN_I2S_DATA_IN=9
//   PIN_I2S_MCK=-1, PIN_I2S_MUTE=-1, PIN_PWM_START=1, PIN_CS=SS, PIN_ADC1=21
#ifdef PIN_I2S_BCK
#undef PIN_I2S_BCK
#endif
#ifdef PIN_I2S_WS
#undef PIN_I2S_WS
#endif
#ifdef PIN_I2S_DATA_OUT
#undef PIN_I2S_DATA_OUT
#endif
#ifdef PIN_I2S_DATA_IN
#undef PIN_I2S_DATA_IN
#endif
#ifdef PIN_I2S_MCK
#undef PIN_I2S_MCK
#endif
#ifdef PIN_I2S_MUTE
#undef PIN_I2S_MUTE
#endif
#ifdef PIN_PWM_START
#undef PIN_PWM_START
#endif
#ifdef PIN_CS
#undef PIN_CS
#endif
#ifdef PIN_ADC1
#undef PIN_ADC1
#endif

#include "PlatformPins.h"

#ifdef OPEN
#undef OPEN
#endif
#ifdef CLOSED
#undef CLOSED
#endif

namespace esp32_escaperoom {

using namespace audio_tools;

class AudioPlayer {
 public:
  enum class StorageBackend : uint8_t {
    SDCard = 0,
    LittleFS = 1,
  };

bool begin(bool sdAvailable, bool littleFsAvailable) {
    if (!ensureAudioOutputReady()) return false;
    
    sdMounted_ = sdAvailable;
    littleFsMounted_ = littleFsAvailable;
    
    // Status log
    if (sdMounted_) {
        Serial.println(F("[AUDIO] SD Card: Available"));
    } else {
        Serial.println(F("[AUDIO] SD Card: Not Available"));
    }
    
    if (littleFsMounted_) {
        Serial.println(F("[AUDIO] LittleFS: Available"));
    } else {
        Serial.println(F("[AUDIO] LittleFS: Not Available"));
    }
    
    begun_ = true;
    mounted_ = (sdMounted_ || littleFsMounted_);
    
    if (littleFsMounted_) {
        backend_ = StorageBackend::LittleFS;
    } else if (sdMounted_) {
        backend_ = StorageBackend::SDCard;
    }
    
    return mounted_;
}

  void setSynchronous(bool sync) {
    synchronous_ = sync;
  }

  void setVolume(uint8_t volume) {
    volumeLevel_ = volume;
	volume_.setVolume(volumeLevel_ / 255.0f);
  }
  uint8_t getVolume() const { return volumeLevel_; }

  bool mounted() const {
    return mounted_;
  }

  StorageBackend backend() const {
    return backend_;
  }

  bool isPlaying() const {
    return playing_;
  }

  // Plays a file once asynchronously (non-blocking)
  // Ideal for background music that does not need to sync with logic
  bool playOnce(const char *path, bool useSD = false) {
    looping_ = false;
    return play(path, useSD, false);
  }

  // Plays a file synchronously (blocks until it finishes)
  // Ideal for short sound effects that need to sync (e.g. tick)
  bool playOnceBlocking(const char *path, bool useSD = false) {
    looping_ = false;
    return play(path, useSD, true);
  }

  // Plays a file in a loop (always asynchronous)
  bool playLoop(const char *path, bool useSD = false) {
    looping_ = true;
    return play(path, useSD, false);
  }

bool play(const char *path, bool useSD = false, bool blocking = false) {
    if (playing_ && currentFile_) {
        String currentName = String(currentFile_.name());
        String newPath = String(path);
        if (newPath.endsWith(currentName) || currentName.endsWith(newPath)) {
            return true;
        }
    }
    stop();

    currentFile_ = useSD ? SD.open(path, "r") : LittleFS.open(path, "r");

    if (!currentFile_) {
        Serial.printf("[AUDIO] FAIL: Cannot open %s on %s\n", path, useSD ? "SD" : "LittleFS");
        return false;
    }

    Serial.printf("[AUDIO] Playing: %s (%s, Loop: %s, Size: %d bytes)\n", path, useSD ? "SD" : "LittleFS", looping_ ? "YES" : "NO", currentFile_.size());

    copier_.begin(decoder_, currentFile_);
    decoder_.begin();
    playing_ = true;
    lastSourceWasSD_ = useSD;

    if (blocking) {
        // Synchronous mode: processes the entire file at once (blocking)
        copier_.copyAll();
        decoder_.end();
        currentFile_.close();
        playing_ = false;
        Serial.println(F("[AUDIO] Blocking playback complete."));
    }

    return true;
}
  bool playTrack(uint8_t folder, uint16_t track) {
    String path = trackPath(folder, track);
    if (path.isEmpty()) {
      Serial.print(F("Track not found: "));
      Serial.print(folder);
      Serial.print(F("/"));
      Serial.println(track);
      return false;
    }
    return play(path.c_str());
  }

  void copy() {
    if (!playing_) {
      return;
    }

    size_t copied = copier_.copy();
    if (copied == 0 && !currentFile_.available()) {
      if (looping_) {
        currentFile_.seek(0);
        decoder_.end();
        decoder_.begin();
        copier_.begin(decoder_, currentFile_);
      } else {
        stop();
      }
    }
  }

  void loop() {
    if (toneActive_) {
      copyTone();
    } else {
      copy();
    }
  }

  // Generates a sine tone of a specific frequency (for Morse code, beeps, etc.)
  void tone(uint16_t frequencyHz, uint8_t volume = 128) {
    if (!begun_) return;

    // Stops the current MP3 if playing
    if (playing_) {
      stop();
    }

    // Stops the previous tone if any
    if (toneActive_) {
      noTone();
    }

    // Configures volume
    setVolume(volume);

    // Configures the sine wave generator
    AudioInfo info;
    info.channels = 2;  // Stereo
    info.sample_rate = 44100;
    info.bits_per_sample = 16;
    sineGen_.begin(info, frequencyHz);

    toneActive_ = true;
    toneFreq_ = frequencyHz;

    Serial.printf("[AUDIO] Tone: %d Hz, vol=%d\n", frequencyHz, volume);
  }

  // Stops the sine tone
  void noTone() {
    if (!toneActive_) return;

    sineGen_.end();
    toneActive_ = false;
    toneFreq_ = 0;

    Serial.println(F("[AUDIO] Tone stopped"));
  }

  // Checks whether a tone is being generated
  bool isPlayingTone() const {
    return toneActive_;
  }

  // Returns the current tone frequency (0 if not playing)
  uint16_t getToneFrequency() const {
    return toneFreq_;
  }

  void stop() {
    if (toneActive_) {
      noTone();
    }
    if (!playing_) {
      return;
    }
    copier_.end();
    decoder_.end();
    currentFile_.close();
    playing_ = false;
    Serial.println(F("[AUDIO] Playback stopped."));
  }

private:
  void copyTone() {
    if (!toneActive_) return;

    // Reads samples from the generator and writes to the VolumeStream/I2S
    const size_t bufferSize = 256;
    int16_t buffer[bufferSize];

    // Generates enough samples for 2 channels (stereo)
    for (int i = 0; i < bufferSize; i += 2) {
      int16_t sample = sineGen_.readSample();
      buffer[i] = sample;     // Left channel
      buffer[i+1] = sample;   // Right channel
    }

    // Writes to volume_ (which connects to I2S)
    volume_.write((uint8_t*)buffer, bufferSize * sizeof(int16_t));
  }

  String trackPath(uint8_t folder, uint16_t track) {
    char exactPath[16];
    snprintf(exactPath, sizeof(exactPath), "/%02u/%03u.mp3", folder, track);
    if (exists(exactPath)) {
      return String(exactPath);
    }

    char folderPath[8];
    snprintf(folderPath, sizeof(folderPath), "/%02u", folder);
    File dir = openFile(folderPath);
    if (dir && dir.isDirectory()) {
      char prefix[5];
      snprintf(prefix, sizeof(prefix), "%03u", track);
      while (true) {
        File entry = dir.openNextFile();
        if (!entry) {
          break;
        }
        if (!entry.isDirectory()) {
          String name = String(entry.name());
          String upper = name;
          upper.toUpperCase();
          if (upper.startsWith(prefix) && upper.endsWith(".MP3")) {
            entry.close();
            dir.close();
            return String(folderPath) + "/" + name;
          }
        }
        entry.close();
      }
      dir.close();
    }

    return String();
  }

 private:
  bool ensureAudioOutputReady() {
    if (begun_) {
      return mounted_;
    }

    pinMode(PIN_EXTERNAL_POWER, OUTPUT);
    digitalWrite(PIN_EXTERNAL_POWER, HIGH);

    AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);

    auto cfg = i2s_.defaultConfig(TX_MODE);
    cfg.pin_bck = PIN_I2S_BCLK;
    cfg.pin_ws = PIN_I2S_WS;
    cfg.pin_data = PIN_I2S_DATA;
    cfg.buffer_count = 12;
    cfg.buffer_size = 512;
    cfg.channels = 2;
    cfg.bits_per_sample = 16;
    cfg.sample_rate = 44100;

    if (!i2s_.begin(cfg)) {
      Serial.println(F("[AUDIO] FAIL: I2S Initialization"));
      begun_ = true;
      mounted_ = false;
      return false;
    }
    Serial.println(F("[AUDIO] I2S Hardware Stream: OK"));
    volume_.setVolume(volumeLevel_ / 255.0f);
    return true;
  }

  File openFile(const char *path) {
    if (backend_ == StorageBackend::LittleFS) {
      return LittleFS.open(path, FILE_READ);
    }
    return SD.open(path, FILE_READ);
  }

  bool exists(const char *path) {
    if (backend_ == StorageBackend::LittleFS) {
      return LittleFS.exists(path);
    }
    return SD.exists(path);
  }

  I2SStream i2s_;
  VolumeStream volume_{i2s_};
  MP3DecoderHelix mp3_;
  EncodedAudioStream decoder_{&volume_, &mp3_};
  StreamCopy copier_;
  File currentFile_;
  bool begun_ = false;
  bool mounted_ = false;
  bool playing_ = false;
  bool looping_ = false;
  bool synchronous_ = false;
  uint8_t volumeLevel_ = 128; // 50%
  StorageBackend backend_ = StorageBackend::LittleFS;
  bool lastSourceWasSD_ = false;
  bool sdMounted_ = false;
  bool littleFsMounted_ = false;

  // Sine wave generator for tone() support (Morse code, beeps)
  SineWaveGenerator<int16_t> sineGen_;
  bool toneActive_ = false;
  uint16_t toneFreq_ = 0;
};

}  // namespace esp32_escaperoom
