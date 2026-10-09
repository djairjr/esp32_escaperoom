#pragma once

#include <M5Cardputer.h>
#include <SD.h>
#include <SPI.h>
#include <LittleFS.h>

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "Config.h"

using namespace audio_tools;

#define MAX_TAP_FILES  24

class TapScanner {
public:
    struct TapFileInfo {
        char name[32];
        bool onLittleFS;
        bool onSD;
    };

    void begin() {
        Serial.println("[TAP] Scanning for audio recordings...");
        sdMounted_ = SD.cardSize() > 0;
        fileCount_ = 0;

        scanRoot(LittleFS, false);
        if (sdMounted_) scanRoot(SD, true);

        Serial.printf("[TAP] %d recordings total\n", fileCount_);
    }

    void listChannels(void (*print)(const char*)) {
        if (fileCount_ == 0) {
            print("[HACK] No vulnerable devices found on this network.");
            return;
        }
        print("[HACK] Scanning cellular frequencies...");
        char buf[64];
        snprintf(buf, sizeof(buf), "[HACK] %d active channels found.", fileCount_);
        print(buf);
        print("");
        for (int i = 0; i < fileCount_; i++) {
            snprintf(buf, sizeof(buf), "%d  %s", i + 1, files_[i].name);
            print(buf);
        }
        print("");
        print("Usage: 'tapscan <name>' or 'tapscan <num>'");
    }

    bool playByName(const char* arg, void (*print)(const char*)) {
        int num = atoi(arg);
        if (num >= 1 && num <= fileCount_)
            return playFile(num - 1, print);
        for (int i = 0; i < fileCount_; i++) {
            if (strcasecmp(files_[i].name, arg) == 0)
                return playFile(i, print);
        }
        char buf[64];
        snprintf(buf, sizeof(buf), "[HACK] No signal on '%s'.", arg);
        print(buf);
        return false;
    }

    bool isAvailable() { return fileCount_ > 0; }
    int fileCount() { return fileCount_; }

private:
    TapFileInfo files_[MAX_TAP_FILES];
    int fileCount_ = 0;
    int sdCount_ = 0;
    bool sdMounted_ = false;

    I2SStream i2s_;
    MP3DecoderHelix decoder_;
    VolumeStream volume_{i2s_};
    EncodedAudioStream player_{&volume_, &decoder_};
    StreamCopy copier_;
    bool i2sReady_ = false;

    void scanRoot(fs::FS& fs, bool isSD) {
        File dir = fs.open("/");
        if (!dir || !dir.isDirectory()) return;
        File f = dir.openNextFile();
        while (f) {
            if (!f.isDirectory()) {
                const char* fp = f.name();
                int fl = strlen(fp);
                if (fl <= 4 || strcasecmp(fp + fl - 4, ".mp3") != 0) { f = dir.openNextFile(); continue; }
                const char* bn = fp;
                for (const char* p = fp; *p; p++) if (*p == '/') bn = p + 1;
                int nl = strlen(bn) - 4; if (nl > 31) nl = 31;
                char name[32]; memcpy(name, bn, nl); name[nl] = '\0';

                if (isSD) {
                    bool found = false;
                    for (int i = 0; i < fileCount_; i++) {
                        if (strcmp(files_[i].name, name) == 0) { files_[i].onSD = true; found = true; sdCount_++; break; }
                    }
                    if (!found) { strlcpy(files_[fileCount_].name, name, 32); files_[fileCount_].onLittleFS = false; files_[fileCount_].onSD = true; sdCount_++; fileCount_++; }
                } else {
                    strlcpy(files_[fileCount_].name, name, 32);
                    files_[fileCount_].onLittleFS = true;
                    files_[fileCount_].onSD = false;
                    fileCount_++;
                }
            }
            f = dir.openNextFile();
        }
        dir.close();
    }

    bool playFile(int idx, void (*print)(const char*)) {
        if (!ensureI2S()) { print("[HACK] Signal interception FAILED!"); return false; }

        char path[40];
        snprintf(path, sizeof(path), "/%s.mp3", files_[idx].name);
        char buf[64];
        snprintf(buf, sizeof(buf), "[HACK] Intercepting %s...", files_[idx].name);
        print(buf);

        fs::FS& fs = files_[idx].onSD ? (fs::FS&)SD : (fs::FS&)LittleFS;
        File file = fs.open(path, FILE_READ);

        // Fallback se não abriu
        if (!file) {
            File dir = fs.open("/");
            if (dir) {
                File f = dir.openNextFile();
                while (f) {
                    if (!f.isDirectory()) {
                        const char* fp = f.name();
                        const char* bn = fp;
                        for (const char* p = fp; *p; p++) if (*p == '/') bn = p + 1;
                        const char* dot = nullptr;
                        for (int ci = 0; bn[ci]; ci++) { if (bn[ci] == '.') { dot = bn + ci; break; } }
                        if (dot) {
                            int bl = dot - bn;
                            if (bl == (int)strlen(files_[idx].name) && strncasecmp(bn, files_[idx].name, bl) == 0)
                                { file = f; break; }
                        }
                    }
                    f = dir.openNextFile();
                }
                dir.close();
            }
        }

        if (!file) { print("[HACK] Signal lost on this frequency."); return false; }

        Serial.printf("[TAP] Playing %s (%d bytes)\n", path, file.size());
        print("(Press any key to stop)");

        player_.begin();
        copier_.begin(player_, file);
        while (copier_.copy() > 0) {
            M5Cardputer.update();
            if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
                M5Cardputer.Keyboard.updateKeysState();
                break;
            }
            delay(1);
        }
        copier_.end();
        player_.end();
        file.close();
        return true;
    }

    bool ensureI2S() {
        if (i2sReady_) return true;
        auto cfg = i2s_.defaultConfig(TX_MODE);
        cfg.pin_bck = I2S_BCLK;
        cfg.pin_ws = I2S_LRCLK;
        cfg.pin_data = I2S_DATA;
        cfg.buffer_count = 12;
        cfg.buffer_size = 512;
        cfg.channels = 2;
        cfg.bits_per_sample = 16;
        cfg.sample_rate = 44100;
        AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Warning);
        if (!i2s_.begin(cfg)) { Serial.println("[TAP] I2S init FAILED"); return false; }
        // NÃO chamar volume_.begin() — PropMaker funciona sem isso
        volume_.setVolume(0.08f);
        i2sReady_ = true;
        return true;
    }
};
