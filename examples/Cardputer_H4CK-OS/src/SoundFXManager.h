#pragma once

#include <M5Cardputer.h>
#include <LittleFS.h>
#include <SD.h>
#include <SPI.h>

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "Config.h"

using namespace audio_tools;

// ============================================================================
// SOUND FX MANAGER — Sound effects via I2S + MP3 (arduino-audio-tools)
// ============================================================================
// Replaces M5Cardputer.Speaker.tone() — PWM doesn't work on the NS4168 I2S.
// Approach identical to the AudioPlayer in PropMakerEscapeRoom.
// All effects are MP3s generated synthetically (ffmpeg).
// ============================================================================

class SoundFXManager {
public:
    enum class StorageBackend : uint8_t {
        SDCard = 0,
        LittleFS = 1,
    };

    void begin(bool sdAvailable = false, bool littleFsAvailable = true) {
        if (!ensureAudioOutputReady()) return;
        sdMounted_ = sdAvailable;
        littleFsMounted_ = littleFsAvailable;
        begun_ = true;
        mounted_ = (sdMounted_ || littleFsMounted_);
        if (littleFsMounted_) backend_ = StorageBackend::LittleFS;
        else if (sdMounted_)  backend_ = StorageBackend::SDCard;
    }

    void setVolume(float vol) {
        volumeLevel_ = vol;
        if (begun_) volume_.setVolume(vol);
    }
    float getVolume() const { return volumeLevel_; }

    bool available() const { return mounted_; }

    // ─── INDIVIDUAL EFFECTS ─────────────────────────────────────────────────
    // All synchronous (blocking): copier_.copyAll() consumes the whole file.

    void keyClick()   { playBlocking("/keyclick.mp3"); }
    void cmdBeep()    { playBlocking("/cmd.mp3"); }
    void errorBeep()  { playBlocking("/error.mp3"); }
    void successBeep(bool big = false) {
        if (!big) {
            playBlocking("/success.mp3");
        } else {
            playBlocking("/success2.mp3");
        }
    }
    void scanBeep() { playBlocking("/scan.mp3"); }
    void transmitBeep(int n = 1) {
        if (n == 1) {
            playBlocking("/transmit1.mp3");
        } else {
            playBlocking("/transmit2.mp3");
        }
    }
    void exploitBeep() { playBlocking("/exploit.mp3"); }

    // ─── GENERIC ─────────────────────────────────────────────────────────────
    // Plays any MP3 synchronously (blocks until it finishes)
    bool playBlocking(const char* path, bool useSD = false) {
        if (!begun_ || !mounted_) return false;
        fs::FS& fs = useSD ? (fs::FS&)SD : (fs::FS&)LittleFS;
        File file = fs.open(path, FILE_READ);
        if (!file) return false;

        Serial.printf("[FX] Playing: %s\n", path);
        copier_.begin(decoder_, file);
        decoder_.begin();
        copier_.copyAll(); // BLOCKING — consumes the whole file
        decoder_.end();
        copier_.end();
        file.close();
        return true;
    }

    // Plays any MP3 asynchronously (non-blocking — call loop() on tick)
    bool playAsync(const char* path, bool useSD = false) {
        if (!begun_ || !mounted_) return false;
        fs::FS& fs = useSD ? (fs::FS&)SD : (fs::FS&)LittleFS;
        currentFile_ = fs.open(path, FILE_READ);
        if (!currentFile_) return false;
        
        Serial.printf("[FX] Playing (async): %s\n", path);
        asyncPlaying_ = true;
        copier_.begin(decoder_, currentFile_);
        decoder_.begin();
        return true;
    }

    void loop() {
        if (!asyncPlaying_) return;
        size_t copied = copier_.copy();
        if (copied == 0 && !currentFile_.available()) {
            decoder_.end();
            copier_.end();
            currentFile_.close();
            asyncPlaying_ = false;
        }
    }

    void stop() {
        if (toneActive_) stopTone();
        if (asyncPlaying_) {
            decoder_.end();
            copier_.end();
            currentFile_.close();
            asyncPlaying_ = false;
        }
    }

    // ─── SINE TONE (fallback for quick beeps, no MP3) ────────────────────────
    void startTone(float freq, uint8_t vol = 128) {
        if (!begun_) return;
        if (asyncPlaying_) stop();

        setVolume(vol / 255.0f);
        AudioInfo info;
        info.channels = 1;
        info.sample_rate = 44100;
        info.bits_per_sample = 16;
        sineGen_.begin(info, freq);
        toneActive_ = true;
    }

    void stopTone() {
        if (!toneActive_) return;
        sineGen_.end();
        toneActive_ = false;
    }

    bool updateTone() {
        if (!toneActive_) return false;
        const size_t bufferSize = 256;
        int16_t buffer[bufferSize];
        for (int i = 0; i < bufferSize; i++) {
            buffer[i] = sineGen_.readSample();
        }
        volume_.write((uint8_t*)buffer, bufferSize * sizeof(int16_t));
        return true;
    }

    bool isPlayingTone() const { return toneActive_; }

private:
    I2SStream i2s_;
    VolumeStream volume_{i2s_};
    MP3DecoderHelix mp3_;
    EncodedAudioStream decoder_{&volume_, &mp3_};
    StreamCopy copier_;
    File currentFile_;

    bool begun_ = false;
    bool mounted_ = false;
    bool sdMounted_ = false;
    bool littleFsMounted_ = true;
    bool asyncPlaying_ = false;
    float volumeLevel_ = 0.08f;
    StorageBackend backend_ = StorageBackend::LittleFS;

    SineWaveGenerator<int16_t> sineGen_;
    bool toneActive_ = false;

    bool ensureAudioOutputReady() {
        if (begun_) return mounted_;

        auto cfg = i2s_.defaultConfig(TX_MODE);
        cfg.pin_bck = I2S_BCLK;
        cfg.pin_ws   = I2S_LRCLK;
        cfg.pin_data = I2S_DATA;
        cfg.buffer_count = 12;
        cfg.buffer_size = 512;
        cfg.channels = 1;      // Mono — NS4168 is mono
        cfg.bits_per_sample = 16;
        cfg.sample_rate = 44100;

        if (!i2s_.begin(cfg)) {
            Serial.println(F("[FX] FAIL: I2S init"));
            begun_ = true;
            mounted_ = false;
            return false;
        }
        Serial.println(F("[FX] I2S OK"));

        volume_.setVolume(volumeLevel_);
        volume_.begin();  // CRITICAL: without begin() the volume = 100%
        Serial.println(F("[FX] VolumeSet"));
        return true;
    }
};
