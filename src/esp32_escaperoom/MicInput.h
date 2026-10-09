#pragma once

// ============================================================================
// MicInput — note detection via INMP441 (I2S) for AcordeonPuzzle
//
// LOGIC PORTED from Esp32VoiceTuner (https://github.com/Vateva/Esp32VoiceTuner)
// SPDX-FileCopyrightText: Copyright (c) 2025 Vateva
// SPDX-License-Identifier: MIT
// [upstream: src/audioprocessing.cpp, include/config.h]
// Adapted for AcordeonPuzzle (ESP32-C3, standalone project) on 19/09/2026.
//
// WHAT WAS PORTED (the part that solves the problem):
//   - 2nd-order Butterworth IIR filters in cascade (highpass 60 / lowpass 6000)
//   - pitch detection by YIN: coarse pass + refined pass with window
//   - validation by harmonics (2nd/3rd/4th) and choice among candidates
//   - parabolic interpolation for sub-sample precision
//   - EMA smoothing with note-change detection
//   - silence gate on RMS
//
// WHAT WAS DISCARDED (does not serve a sensor):
//   FreeRTOS tasks and queue, GC9A01 display, menu, power management by
//   CPU frequency, note names by scale (the accordion has fixed tuning).
//
// WHY YIN AND NOT FFT: a 512-point FFT at 16 kHz has 31.25 Hz/bin —
// at C4 (262 Hz) that is ~35 cents per bin, too coarse for a musical
// sequence. Time-domain autocorrelation finds the PERIOD of the fundamental.
// And the I2S reading is STEREO with slot selection: 32-bit MONO returns a
// zero-stuffed buffer ([d,0,d,0,...]), which dcRemoval turns into a square
// wave at fs/2 and makes EVERY note be read as B8.
//
// PRESERVED API: begin/end/readSample/detectFrequency/detectNote/getRms/
// setMinAmplitude/isLineStuck remain identical for the PuzzleEngine.
// ============================================================================

#include <Arduino.h>
#include "PlatformPins.h"

namespace esp32_escaperoom {

class MicInput {
public:
    MicInput();
    ~MicInput();

    /// Configures I2S RX (16 kHz, 32-bit, STEREO) and the IIR filters.
    /// true = driver came up. Does NOT prove that the mic delivers data — use getRms().
    bool begin();

    /// Reads a buffer, chooses the active slot, applies the filters and computes RMS.
    bool readSample();

    /// Frequency detected in Hz by YIN. 0 if below the gate.
    float detectFrequency();

    /// Frequency -> MIDI note (60 = C4). -1 if silence/no detection.
    int detectNote();

    /// Last result of detectFrequency() in Hz (0 if silence).
    /// The tolerance in CENTS needs the frequency: converted to integer
    /// MIDI (round) the cents information would already have been lost.
    float getLastFrequency() const { return cachedFrequency_; }

    /// RMS level of the last buffer (scale [-1,1]) — gate magnitude.
    float getRms() const { return rms_; }

    /// Level in dB (upstream reference) — diagnostic.
    float getDbLevel() const { return dbLevel_; }

    /// Overall detection confidence (0-1) — diagnostic.
    float getConfidence() const { return overallConfidence_; }

    /// true if the buffer read came without variation (SD line stuck).
    bool isLineStuck() const { return lineStuck_; }

    /// Index of the I2S slot that carries the audio: 0 = 1st slot, 1 = 2nd slot.
    uint8_t getActiveSlot() const { return activeSlot_; }

    /// Silence threshold on RMS. Calibrate on the bench.
    void setMinAmplitude(float v) { minAmplitude_ = v; }
    float getMinAmplitude() const { return minAmplitude_; }

    /// Turns off I2S (for deep sleep).
    void end();

    /// Function called at the CPU yield points inside the YIN analysis.
    /// It serves so the SKETCH can attend the web server during processing:
    /// loopTask runs at priority 1 and the HTTP handler is only executed
    /// when someone calls it — without this, the requests stay queued during
    /// the whole analysis (measured: /api/sequence 4/4 timeout).
    /// Keeps MicInput decoupled from the webserver: it is just a pointer.
    typedef void (*YieldPump)();
    void setYieldPump(YieldPump fn) { yieldPump_ = fn; }

private:
    static const uint16_t kSamples    = 2048;   // samples per channel (YIN window)
    static const uint32_t kSampleRate = 48000;  // 48 kHz: better resolution

    // ── YIN parameters (from upstream; useful range ~80-1500 Hz at 48 kHz) ──
    static const int YIN_MIN_PERIOD = 32;
    static const int YIN_MAX_PERIOD = 600;
    static const int YIN_COARSE_SAMPLES = 512;
    static const int YIN_MAX_CANDIDATES = 8;
    static constexpr float YIN_COARSE_THRESHOLD = 0.25f;
    static constexpr float YIN_THRESHOLD = 0.15f;
    static constexpr float YIN_COARSE_QUALITY_GATE = 0.5f;

    static const uint8_t MAX_SCALE_NOTES = 12;

    // ── 2nd-order IIR filter (upstream struct) ───────────────────
    struct IIRFilter {
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        bool  initialized;
        IIRFilter() : b0(1), b1(0), b2(0), a1(0), a2(0),
                      x1(0), x2(0), y1(0), y2(0), initialized(false) {}
        void reset() { x1 = x2 = y1 = y2 = 0.0f; initialized = true; }
    };

    // ── state ────────────────────────────────────────────────────────
    void*   rxHandle_ = nullptr;    // i2s_chan_handle_t (opaque)
    bool    initialized_ = false;
    uint8_t activeSlot_ = 0;

    int32_t rawStereo_[kSamples * 2];   // raw stereo reading
    float   samples_[kSamples];         // active slot, filtered
    float   yinBuffer_[YIN_MAX_PERIOD + 1];

    IIRFilter highpass_, lowpass_;

    float rms_ = 0.0f;
    float dbLevel_ = -80.0f;
    float minAmplitude_ = 0.001f;
    float overallConfidence_ = 0.0f;
    bool  lineStuck_ = false;

    // EMA smoothing (from upstream): avoids oscillating between neighboring notes
    float smoothedFrequency_ = 0.0f;
    float lastRawFrequency_ = 0.0f;
    bool  smoothingInit_ = false;
    float cachedFrequency_ = 0.0f;   // result of the last read window
    unsigned long lastReadMs_ = 0;   // read window (governed by TIME)
    // ≈ duration of the audio buffer: 2048 samples / 48 kHz = 42.7 ms.
    // Reading faster than this brings no new sample to the I2S buffer.
    static const unsigned long kLeituraIntervaloMs = 40;
    static YieldPump yieldPump_;     // attends the server at the yields

    // ── ported helpers ──────────────────────────────────────────────
    void  calculateButterworth(float cutoffHz, bool isHighpass, IIRFilter* f);
    void  initFilters();
    float applyIIR(float input, IIRFilter* f);
    void  applyPrefiltering(uint16_t count);
    float calculateDbLevel(float rms);
    int   findCoarsePeriodYIN(int samplesToUse);
    int   findAllCandidates(int searchMin, int searchMax, int* periods, float* values, int maxC);
    float scoreHarmonicContent(int candidatePeriod);
    float calculateFrequencyBias(int period);
    int   selectBestCandidate(int* periods, float* values, int count);
    bool  yinAnalysis(float* outFrequency);
};

}  // namespace esp32_escaperoom
