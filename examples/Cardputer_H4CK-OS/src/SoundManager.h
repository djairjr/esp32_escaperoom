#pragma once

#include <M5Cardputer.h>
#include <LittleFS.h>
#include <SD.h>

// ============================================================================
// SOUND MANAGER — Native wrapper over M5Cardputer.Speaker (Speaker_Class)
// ============================================================================
// SFX via tone() — zero bytes in LittleFS.
// TapScan via playRaw() streaming with WAV from LittleFS/SD.
// ============================================================================

class SoundManager {
public:
    void begin() {
        speaker_ = &M5Cardputer.Speaker;
        if (!speaker_->isEnabled()) {
            Serial.println("[SND] Speaker not available");
            return;
        }
        speaker_->setVolume(144);
        speaker_->setAllChannelVolume(255);
        ready_ = true;
        Serial.println("[SND] Ready");
    }

    bool isReady() const { return ready_; }

    // ─── SFX via tone() ────────────────────────────────────────────────────
    void keyClick()   { tone(1200, 30); }
    void cmdBeep()    { tone(800,  60); }
    void errorBeep()  { tone(200, 150); }
    void successBeep(){ tone(1000, 80); tone(1400, 80); }
    void scanBeep()   { tone(600,  50); tone(900,  50); }
    void transmitBeep(int n = 1) {
        if (n == 1) { tone(500, 40); tone(700, 40); }
        else        { tone(400, 30); tone(600, 30); tone(800, 30); }
    }
    void exploitBeep(){ tone(300, 100); tone(500, 100); tone(900, 100); }

    // ─── Streaming WAV ─────────────────────────────────────────────────────
    bool streamBegin(const char* path, fs::FS* fs = nullptr) {
        if (!ready_) { Serial.println("[SND] streamBegin: !ready_"); return false; }
        streamEnd();

        fs::FS& filesys = fs ? *fs : (fs::FS&)LittleFS;
        streamFile_ = filesys.open(path, FILE_READ);
        if (!streamFile_) {
            Serial.printf("[SND] streamBegin: can't open %s\n", path);
            return false;
        }
        Serial.printf("[SND] Opened %s (%d bytes)\n", path, streamFile_.size());

        if (!parseWavHeader()) {
            Serial.println("[SND] WAV parse FAILED");
            streamFile_.close();
            streamFile_ = File();
            return false;
        }
        Serial.printf("[SND] WAV: %dHz %d-bit %dch, PCM data=%d bytes\n",
                      streamRate_, streamBits_, streamCh_, streamDataLen_);

        chunkCount_ = 0;
        streamActive_ = true;
        return true;
    }

    bool streamLoop() {
        if (!streamActive_ || !ready_) return false;

        // Channel 0: wait while the queue is full (isPlaying returns 2)
        size_t chState = speaker_->isPlaying(0);
        if (chState == 2) return true;

        if (streamPos_ >= streamDataLen_) {
            // Last chunk already sent — wait for it to drain
            if (chState == 0) {
                Serial.printf("[SND] Stream done: %d chunks\n", chunkCount_);
                streamEnd();
                return false;
            }
            return true;  // still playing the last chunk
        }

        size_t sampleBytes = streamBits_ / 8;
        size_t remaining = (streamDataLen_ - streamPos_) / sampleBytes;
        size_t chunkSamples = (remaining > STREAM_BUF_SAMPLES) ? STREAM_BUF_SAMPLES : remaining;
        size_t chunkBytes = chunkSamples * sampleBytes;
        size_t read = streamFile_.read(streamBuf_, chunkBytes);
        if (read == 0) { streamEnd(); return false; }

        bool stopCurrent = (chunkCount_ == 0);  // true only on the 1st chunk
        bool flgStereo = (streamCh_ == 2);
        bool ok;

        if (streamBits_ == 16) {
            ok = speaker_->playRaw((const int16_t*)streamBuf_, read / 2,
                                   streamRate_, flgStereo, 1, 0, stopCurrent);
        } else {
            ok = speaker_->playRaw((const uint8_t*)streamBuf_, read,
                                   streamRate_, flgStereo, 1, 0, stopCurrent);
        }

        if (!ok) {
            Serial.printf("[SND] playRaw FAILED chunk #%d\n", chunkCount_);
            streamEnd();
            return false;
        }

        if (chunkCount_ == 0)
            Serial.printf("[SND] Chunk#0: %d bytes, rate=%d, ch=%d, bits=%d\n",
                          read, streamRate_, streamCh_, streamBits_);
        chunkCount_++;
        streamPos_ += read;
        return true;
    }

    void streamEnd() {
        if (streamFile_) streamFile_.close();
        streamFile_ = File();
        streamActive_ = false;
        streamChannel_ = -1;
        streamPos_ = 0;
        chunkCount_ = 0;
    }

    bool isStreaming() const { return streamActive_; }

    void stop() {
        streamEnd();
        if (speaker_) speaker_->stop();
    }

private:
    m5::Speaker_Class* speaker_ = nullptr;
    bool ready_ = false;

    void tone(float freq, uint32_t ms) {
        if (!ready_) return;
        speaker_->tone(freq, ms);
    }

    // ─── WAV streaming internals ────────────────────────────────────────────
    static constexpr size_t STREAM_BUF_SAMPLES = 2048;
    File streamFile_;
    uint8_t streamBuf_[STREAM_BUF_SAMPLES * 4];
    uint32_t streamRate_ = 0;
    uint16_t streamCh_ = 0;
    uint16_t streamBits_ = 0;
    uint32_t streamDataLen_ = 0;
    uint32_t streamPos_ = 0;
    int streamChannel_ = -1;
    int chunkCount_ = 0;
    bool streamActive_ = false;

#pragma pack(push, 1)
    struct WavHdr {
        char riff[4];
        uint32_t fileSize;
        char wave[4];
        char fmt[4];
        uint32_t fmtSize;
        uint16_t audioFormat;
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
    };
#pragma pack(pop)

    bool parseWavHeader() {
        WavHdr h;
        if (streamFile_.read((uint8_t*)&h, sizeof(h)) != sizeof(h)) {
            Serial.println("[SND] parseHdr: short read");
            return false;
        }
        if (memcmp(h.riff, "RIFF", 4) || memcmp(h.wave, "WAVE", 4)) {
            Serial.printf("[SND] parseHdr: bad magic '%.4s'/'%.4s'\n", h.riff, h.wave);
            return false;
        }
        if (h.audioFormat != 1) {
            Serial.printf("[SND] parseHdr: format=%d (expect PCM=1)\n", h.audioFormat);
            return false;
        }
        Serial.printf("[SND] parseHdr: fmt=%d bit=%d ch=%d rate=%d\n",
                      h.audioFormat, h.bitsPerSample, h.numChannels, h.sampleRate);

        // Skip extra bytes of the fmt chunk
        if (h.fmtSize > 16)
            streamFile_.seek(streamFile_.position() + (h.fmtSize - 16));

        // Look for the "data" chunk
        char id[4];
        uint32_t sz;
        for (int attempt = 0; attempt < 20; attempt++) {
            if (streamFile_.read((uint8_t*)id, 4) != 4) {
                Serial.println("[SND] parseHdr: EOF looking for 'data'");
                return false;
            }
            if (streamFile_.read((uint8_t*)&sz, 4) != 4) {
                Serial.println("[SND] parseHdr: EOF reading chunk size");
                return false;
            }
            if (memcmp(id, "data", 4) == 0) break;
            Serial.printf("[SND] parseHdr: skip '%.4s' (%d bytes)\n", id, sz);
            streamFile_.seek(streamFile_.position() + sz);
        }

        streamRate_ = h.sampleRate;
        streamCh_ = h.numChannels;
        streamBits_ = h.bitsPerSample;
        streamDataLen_ = sz;
        streamPos_ = 0;
        return true;
    }
};
