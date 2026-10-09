#include "MicInput.h"

// ============================================================================
// LOGIC PORTED from Esp32VoiceTuner (https://github.com/Vateva/Esp32VoiceTuner)
// SPDX-FileCopyrightText: Copyright (c) 2025 Vateva
// SPDX-License-Identifier: MIT
// [upstream: src/audioprocessing.cpp, src/utilities.cpp, include/config.h]
// Adapted for AcordeonPuzzle (ESP32-C3, standalone project) on 19/09/2026.
//
// The ported blocks are marked with "[upstream]" — the DSP logic
// (Butterworth, YIN, harmonics, band bias, EMA) is the upstream's, with
// names and constants preserved. What is ours: STEREO reading with slot
// selection (the upstream uses the legacy API with I2S_CHANNEL_FMT_ONLY_LEFT, which
// does not exist in the i2s_std API of core 3.x) and the public API of MicInput.
// ============================================================================

#include <driver/i2s_std.h>
#include <math.h>

namespace esp32_escaperoom {

// Minimum interval between two YIN analyses.
static const unsigned long YIN_MIN_INTERVAL_MS = 40;

// YIN runs INSIDE loop(), on a single core (ESP32-C3 has 1 core).
// A loop of ~700k iterations without yielding CPU blocks handleAP() and the web
// server STOPS responding — measured: /api/sequence 4/4 timeout with /api/note
// responding in 0.18s (the difference is when the loop manages to spin).
// Every YIN_YIELD_EVERY iterations the processing yields control: delay(0)
// lets the system tasks (lwIP/netif, WiFi) run. It costs little
// clock and keeps the server serving during the analysis.
static const int YIN_YIELD_EVERY = 1024;

// At the yield points: (1) delay(0) returns the tick to the scheduler and
// (2) if the sketch registered a pump, it is called — that is how the
// web server RESPONDS during the analysis. loopTask runs at priority 1;
// the HTTP handler only runs when called, so the pump is what keeps the
// server alive while YIN works.
MicInput::YieldPump MicInput::yieldPump_ = nullptr;

// Maximum latency the web server tolerates during an analysis. The pump does NOT
// run at every yield (that would be ~500 calls per analysis and the cost of
// servicing would dominate); it runs at most once per interval. 5 ms
// keeps the server responsive without competing with the computation.
static const unsigned long PUMP_MIN_INTERVAL_MS = 5;
static unsigned long s_lastPumpMs = 0;

#define YIELD_CPU() do { \
    delay(0); \
    if (MicInput::yieldPump_) { \
      unsigned long now_ = millis(); \
      if (now_ - s_lastPumpMs >= PUMP_MIN_INTERVAL_MS) { \
        s_lastPumpMs = now_; \
        MicInput::yieldPump_(); \
      } \
    } \
  } while (0)

MicInput::MicInput() {}

MicInput::~MicInput() { end(); }

// ============================================================================
// IIR FILTERS (ported)
// ============================================================================

// [upstream] calculateButterworthCoefficients — audioprocessing.cpp L24-52
void MicInput::calculateButterworth(float cutoffHz, bool isHighpass, IIRFilter* filter) {
    if (!filter || cutoffHz <= 0.0f || kSampleRate <= 0.0f) return;

    float omega = 2.0f * M_PI * cutoffHz / (float)kSampleRate;
    float k = tanf(omega / 2.0f);
    float k2 = k * k;
    float sqrt2 = sqrtf(2.0f);
    float norm = 1.0f / (1.0f + sqrt2 * k + k2);

    if (isHighpass) {
        filter->b0 = 1.0f * norm;
        filter->b1 = -2.0f * filter->b0;
        filter->b2 = filter->b0;
    } else {
        filter->b0 = k2 * norm;
        filter->b1 = 2.0f * filter->b0;
        filter->b2 = filter->b0;
    }
    filter->a1 = 2.0f * (k2 - 1.0f) * norm;
    filter->a2 = (1.0f - sqrt2 * k + k2) * norm;
    filter->reset();
}

void MicInput::initFilters() {
    // highpass 60 Hz removes DC and low-frequency noise;
    // lowpass 6000 Hz removes what is above the useful harmonics.
    // Accordion range (C2~65 Hz to C6~1047 Hz) stays preserved.
    calculateButterworth(60.0f,   true,  &highpass_);
    calculateButterworth(6000.0f, false, &lowpass_);
}

// [upstream] applyIIRFilter — audioprocessing.cpp L89-106
float MicInput::applyIIR(float input, IIRFilter* filter) {
    if (!filter || !filter->initialized) return input;
    float output = filter->b0 * input +
                   filter->b1 * filter->x1 +
                   filter->b2 * filter->x2 -
                   filter->a1 * filter->y1 -
                   filter->a2 * filter->y2;
    filter->x2 = filter->x1;
    filter->x1 = input;
    filter->y2 = filter->y1;
    filter->y1 = output;
    return output;
}

// [upstream] applyPrefiltering — audioprocessing.cpp L109-120
void MicInput::applyPrefiltering(uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        float filtered = applyIIR(samples_[i], &highpass_);
        samples_[i] = applyIIR(filtered, &lowpass_);
    }
}

// [upstream] calculateDbLevel — utilities.cpp L112-126
float MicInput::calculateDbLevel(float rms) {
    if (rms < 0.0001f) return -80.0f;
    float db = 20.0f * log10f(rms / 0.1f);   // DB_REFERENCE_LEVEL = 0.1
    if (db < -80.0f) db = -80.0f;
    if (db > 0.0f)   db = 0.0f;
    return db;
}

// ============================================================================
// YIN (ported)
// ============================================================================

// [upstream] findCoarsePeriodYIN — audioprocessing.cpp L254-320
int MicInput::findCoarsePeriodYIN(int samplesToUse) {
    if (samplesToUse > kSamples) samplesToUse = kSamples;
    if (samplesToUse < YIN_COARSE_SAMPLES) samplesToUse = YIN_COARSE_SAMPLES;
    if (samplesToUse > kSamples) return 0;

    int workCoarse = 0;
    for (int tau = YIN_MIN_PERIOD; tau <= YIN_MAX_PERIOD; tau++) {
        float diff = 0.0f;
        int validSamples = samplesToUse - tau;
        if (validSamples < 64) continue;          // YIN_COARSE_MIN_VALID_SAMPLES
        for (int i = 0; i < validSamples; i++) {
            float delta = samples_[i] - samples_[i + tau];
            diff += delta * delta;
            if ((++workCoarse % YIN_YIELD_EVERY) == 0) YIELD_CPU();
        }
        yinBuffer_[tau] = diff;
    }

    // normalization by the accumulated mean
    float runningSum = 0.0f;
    yinBuffer_[YIN_MIN_PERIOD] = 1.0f;
    for (int tau = YIN_MIN_PERIOD + 1; tau <= YIN_MAX_PERIOD; tau++) {
        runningSum += yinBuffer_[tau];
        if (runningSum > 0.0f) yinBuffer_[tau] *= tau / runningSum;
        else                   yinBuffer_[tau] = 1.0f;
    }

    int bestPeriod = 0;
    float bestValue = 1.0f;
    for (int tau = YIN_MIN_PERIOD; tau <= YIN_MAX_PERIOD; tau++) {
        if (yinBuffer_[tau] < YIN_COARSE_THRESHOLD) { bestPeriod = tau; break; }
        if (yinBuffer_[tau] < bestValue) { bestValue = yinBuffer_[tau]; bestPeriod = tau; }
    }
    if (bestPeriod == 0 || bestValue > YIN_COARSE_QUALITY_GATE) return 0;
    return bestPeriod;
}

// [upstream] findAllYinCandidates — audioprocessing.cpp L323-360
int MicInput::findAllCandidates(int searchMin, int searchMax, int* periods,
                                float* values, int maxC) {
    int n = 0;
    for (int tau = searchMin + 1; tau < searchMax - 1 && n < maxC; tau++) {
        if (yinBuffer_[tau] < YIN_THRESHOLD &&
            yinBuffer_[tau] < yinBuffer_[tau - 1] &&
            yinBuffer_[tau] < yinBuffer_[tau + 1]) {
            periods[n] = tau;
            values[n]  = yinBuffer_[tau];
            n++;
        }
    }
    if (n == 0) {                       // fallback: global minimum of the window
        float minValue = 1.0f; int minPeriod = 0;
        for (int tau = searchMin; tau <= searchMax; tau++) {
            if (yinBuffer_[tau] < minValue) { minValue = yinBuffer_[tau]; minPeriod = tau; }
        }
        if (minPeriod > 0) { periods[0] = minPeriod; values[0] = minValue; n = 1; }
    }
    return n;
}

// [upstream] scoreHarmonicContent — audioprocessing.cpp L363-412
float MicInput::scoreHarmonicContent(int candidatePeriod) {
    if (candidatePeriod <= 0) return 0.0f;
    const int   harmonics[3] = {2, 3, 4};
    const float weights[3]   = {0.8f, 0.6f, 0.4f};

    float harmonicScore = 0.0f;
    int validHarmonics = 0;
    for (int h = 0; h < 3; h++) {
        int harmonicPeriod = candidatePeriod / harmonics[h];
        if (harmonicPeriod < 16) continue;       // YIN_MIN_HARMONIC_PERIOD
        int validSamples = kSamples - harmonicPeriod;
        if (validSamples < 32) continue;         // YIN_MIN_HARMONIC_VALID_SAMPLES
        float corr = 0.0f, energy = 0.0f;
        for (int i = 0; i < validSamples; i++) {
            float s1 = samples_[i];
            float s2 = samples_[i + harmonicPeriod];
            corr   += s1 * s2;
            energy += s1 * s1;
            if ((i & 1023) == 0) YIELD_CPU();
        }
        if (energy > 0.0f) {
            float normalized = corr / energy;
            if (normalized > 0.1f) {             // YIN_HARMONIC_CORRELATION_THRESHOLD
                harmonicScore += normalized * weights[h];
                validHarmonics++;
            }
        }
    }
    if (validHarmonics >= 2) harmonicScore *= 1.5f;   // YIN_HARMONIC_BONUS
    return harmonicScore;
}

// [upstream] calculateFrequencyBias — audioprocessing.cpp L415-433
float MicInput::calculateFrequencyBias(int period) {
    if (period <= 0) return 0.0f;
    float frequency = (float)kSampleRate / (float)period;
    const float VOCAL_MIN = 80.0f, VOCAL_MAX = 600.0f;
    const float HIGH_MAX = 1000.0f, LOW_MIN = 40.0f;
    if (frequency >= VOCAL_MIN && frequency <= VOCAL_MAX) return 1.0f;
    if (frequency > VOCAL_MAX && frequency <= HIGH_MAX)
        return 1.0f - (frequency - VOCAL_MAX) / (HIGH_MAX - VOCAL_MAX) * 0.3f;
    if (frequency >= LOW_MIN && frequency < VOCAL_MIN)
        return 1.0f - (VOCAL_MIN - frequency) / (VOCAL_MIN - LOW_MIN) * 0.2f;
    return 0.5f;
}

// [upstream] selectBestCandidate — audioprocessing.cpp L436-468
int MicInput::selectBestCandidate(int* periods, float* values, int count) {
    if (count <= 0) return -1;
    int   bestIndex = 0;
    float bestScore = -1e9f;
    for (int i = 0; i < count; i++) {
        float yinScore     = (1.0f - values[i]) * 2.0f;
        float harmonic     = scoreHarmonicContent(periods[i]);
        float freqBias     = calculateFrequencyBias(periods[i]);
        // upstream weights: YIN 0.4, harmonics 0.5, band 0.1
        float total = yinScore * 0.4f + harmonic * 0.5f + freqBias * 0.1f;
        if (total > bestScore) { bestScore = total; bestIndex = i; }
    }
    return bestIndex;
}

// [upstream] yinAnalysis — audioprocessing.cpp L471-584 (without the upstream's
// timing/debug/display parts)
bool MicInput::yinAnalysis(float* outFrequency) {
    int hinted = findCoarsePeriodYIN(kSamples);
    if (hinted == 0) return false;

    int searchMin = (int)(hinted * 0.60f);   // window of +-40% (YIN_SEARCH_WINDOW)
    int searchMax = (int)(hinted * 1.40f);
    if (searchMin < YIN_MIN_PERIOD) searchMin = YIN_MIN_PERIOD;
    if (searchMax > YIN_MAX_PERIOD) searchMax = YIN_MAX_PERIOD;

    int workRef = 0;
    for (int tau = searchMin; tau <= searchMax; tau++) {
        float diff = 0.0f;
        for (int i = 0; i < kSamples - tau; i++) {
            float delta = samples_[i] - samples_[i + tau];
            diff += delta * delta;
            // periodically yields the CPU: keeps the webserver serving
            if ((++workRef % YIN_YIELD_EVERY) == 0) YIELD_CPU();
        }
        yinBuffer_[tau] = diff;
    }
    float runningSum = 0.0f;
    yinBuffer_[searchMin] = 1.0f;
    for (int tau = searchMin + 1; tau <= searchMax; tau++) {
        runningSum += yinBuffer_[tau];
        if (runningSum > 0.0f) yinBuffer_[tau] *= tau / runningSum;
        else                   yinBuffer_[tau] = 1.0f;
    }

    int   periods[YIN_MAX_CANDIDATES];
    float values[YIN_MAX_CANDIDATES];
    int count = findAllCandidates(searchMin, searchMax, periods, values, YIN_MAX_CANDIDATES);
    if (count == 0) return false;

    int best = selectBestCandidate(periods, values, count);
    if (best < 0) return false;

    int bestPeriod = periods[best];

    // [upstream] parabolic interpolation — sub-sample precision
    float betterPeriod;
    if (bestPeriod > YIN_MIN_PERIOD && bestPeriod < YIN_MAX_PERIOD) {
        float ym = yinBuffer_[bestPeriod - 1];
        float y0 = yinBuffer_[bestPeriod];
        float yp = yinBuffer_[bestPeriod + 1];
        float den = 2.0f * (2.0f * y0 - yp - ym);
        float p = (den != 0.0f) ? (yp - ym) / den : 0.0f;
        betterPeriod = (float)bestPeriod + p;
    } else {
        betterPeriod = (float)bestPeriod;
    }
    if (betterPeriod <= 0.0f) return false;

    *outFrequency = (float)kSampleRate / betterPeriod;
    return true;
}

// ============================================================================
// I2S + PUBLIC API
// ============================================================================

bool MicInput::begin() {
    if (initialized_) return true;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num  = 8;
    chan_cfg.dma_frame_num = 240;

    i2s_chan_handle_t rx = nullptr;
    esp_err_t err = i2s_new_channel(&chan_cfg, nullptr, &rx);
    if (err != ESP_OK) {
        Serial.printf("[MicInput] i2s_new_channel failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // STEREO on purpose. In 32-bit MONO the RX buffer comes with
    // zero-stuffing ([d,0,d,0,...]) and any step that subtracts the mean
    // turns that into a square wave at fs/2 — the detection then reads
    // the same high note every time. Reading stereo and choosing ONE slot, the
    // stuffing does not exist by construction.
    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG((uint32_t)kSampleRate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                        I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)PIN_I2S_BCLK,
            .ws   = (gpio_num_t)PIN_I2S_WS,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)PIN_I2S_DATA,
            .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false},
        },
    };

    if ((err = i2s_channel_init_std_mode(rx, &std_cfg)) != ESP_OK) {
        Serial.printf("[MicInput] init_std_mode failed: %s\n", esp_err_to_name(err));
        i2s_del_channel(rx);
        return false;
    }
    if ((err = i2s_channel_enable(rx)) != ESP_OK) {
        Serial.printf("[MicInput] channel_enable failed: %s\n", esp_err_to_name(err));
        i2s_del_channel(rx);
        return false;
    }

    rxHandle_ = rx;
    initFilters();
    initialized_ = true;
    Serial.printf("[MicInput] INMP441 ready: %lu Hz, 32-bit, STEREO + IIR filters (YIN)\n",
                  (unsigned long)kSampleRate);
    Serial.printf("[MicInput] RMS gate = %.4f | HP 60 Hz / LP 6000 Hz | window %u samples\n",
                  minAmplitude_, (unsigned)kSamples);
    return true;
}

bool MicInput::readSample() {
    if (!initialized_ || !rxHandle_) return false;

    size_t bytesRead = 0;
    // kSamples frames * 2 slots * 4 bytes. @48 kHz = 42.7 ms of audio.
    //
    // SHORT TIMEOUT: it used to be 1000 ms. If I2S did not deliver (mic
    // disconnected, missing clock), the loop got STUCK for up to 1 s — during
    // that time the web server served NOTHING. 50 ms is larger than the
    // 42.7 ms of audio, so a normal read always completes, and an
    // abnormal read fails fast instead of freezing the board.
    esp_err_t err = i2s_channel_read((i2s_chan_handle_t)rxHandle_, rawStereo_,
                                     sizeof(rawStereo_), &bytesRead, 50);
    if (err != ESP_OK) { if (MicInput::yieldPump_) MicInput::yieldPump_(); return false; }

    size_t frames = bytesRead / (sizeof(int32_t) * 2);
    if (frames == 0) return false;

    // ── chooses the slot that carries the audio, by energy, with hysteresis ──
    double e0 = 0.0, e1 = 0.0;
    for (size_t f = 0; f < frames; f++) {
        double a = (double)rawStereo_[2 * f];
        double b = (double)rawStereo_[2 * f + 1];
        e0 += a * a;
        e1 += b * b;
    }
    if (activeSlot_ == 0) { if (e1 > e0 * 4.0) activeSlot_ = 1; }
    else                  { if (e0 > e1 * 4.0) activeSlot_ = 0; }

    // ── copies ONLY the active slot and normalizes (24-bit in a 32-bit slot) ────
    double acc = 0.0;
    bool allEqual = (frames > 1);
    int32_t first = rawStereo_[activeSlot_];
    for (size_t f = 0; f < frames; f++) {
        int32_t v = rawStereo_[2 * f + activeSlot_];
        float s = (float)v / 2147483648.0f;
        samples_[f] = s;
        acc += (double)s * (double)s;
        if (v != first) allEqual = false;
    }
    for (size_t f = frames; f < kSamples; f++) samples_[f] = 0.0f;

    rms_ = (float)sqrt(acc / (double)frames);
    lineStuck_ = allEqual;
    // (there is no cache flag here: what governs the read cadence is the
    //  TIME window in detectFrequency — see the long comment there)

    // ── filtering (ported) and level in dB ─────────────────────────────
    if (rms_ >= minAmplitude_) {
        applyPrefiltering((uint16_t)frames);
        // RMS after filtering: this is the signal that YIN analyzes
        double acc2 = 0.0;
        for (size_t f = 0; f < frames; f++) acc2 += (double)samples_[f] * (double)samples_[f];
        rms_ = (float)sqrt(acc2 / (double)frames);
    }
    dbLevel_ = calculateDbLevel(rms_);
    return true;
}

float MicInput::detectFrequency() {
    unsigned long agora = millis();

    // ── READ WINDOW GOVERNED BY TIME ────────────────────────────
    // The loop calls detectNote() AND the PuzzleEngine calls detectFrequency() in
    // the SAME iteration. This window avoids reading/re-analyzing the same instant twice.
    //
    // The window is TIME-based, not a flag that the read itself has to clear.
    // The previous version had exactly this circular defect:
    //     detectFrequency(): if (cacheValid_) return cache;   // does not read the mic
    //     readSample():      cacheValid_ = false;             // the only one that clears it
    // readSample() was only called when cacheValid_ was false, and cacheValid_
    // was only cleared by readSample() -> the mic was read ONCE and never again.
    // Measured symptom: the note got stuck on the first value forever (on screen,
    // F2 fixed while the file played C4, E4, G4...).
    //
    // kLeituraIntervaloMs ≈ duration of the audio buffer (2048 samples at 48 kHz
    // = 42.7 ms). Reading faster than this brings no new sample.
    if (lastReadMs_ != 0 && (agora - lastReadMs_) < kLeituraIntervaloMs) {
        return cachedFrequency_;
    }
    lastReadMs_ = agora;

    // ── READ ALWAYS ───────────────────────────────────────────────────
    // This is the ONLY call that knows whether the sound started or stopped: it measures the
    // RMS of the buffer. Without a read there is no way to detect the silence->
    // sound transition, which is exactly what triggers the detection.
    if (!readSample()) {
        cachedFrequency_ = 0.0f;
        overallConfidence_ = 0.0f;
        return 0.0f;
    }

    // ── GATE: YIN only runs WITH SOUND ─────────────────────────────────────
    // The read above is cheap (I2S + RMS). YIN costs ~1M float operations
    // and is what would starve the web server. Without sound there is nothing to analyze:
    // it returns SILENCE (0), which is also how the consumer knows that the note
    // ended and can release the sustained-note lock.
    if (rms_ < minAmplitude_) {
        overallConfidence_ = 0.0f;
        cachedFrequency_ = 0.0f;
        return 0.0f;
    }

    // ── ANALYSIS (only with sound present) ────────────────────────────────
    float freq = 0.0f;
    if (!yinAnalysis(&freq)) {
        overallConfidence_ = 0.0f;
        cachedFrequency_ = 0.0f;
        return 0.0f;
    }

    // ── EMA smoothing with note-change detection (from upstream) ───────
    if (!smoothingInit_) {
        smoothedFrequency_ = freq;
        smoothingInit_ = true;
    } else {
        float diff = fabsf(freq - smoothedFrequency_);
        float rel  = (smoothedFrequency_ > 0.0f) ? diff / smoothedFrequency_ : 1.0f;
        if (rel > 0.06f) {               // ~1 semitone: note change, adopt directly
            smoothedFrequency_ = freq;
        } else {
            // smaller alpha = smoother; 0.5 is a stable middle ground for C3
            smoothedFrequency_ = 0.5f * freq + 0.5f * smoothedFrequency_;
        }
    }
    lastRawFrequency_ = freq;

    // Signal confidence: scale calibrated on THIS microphone. The upstream uses
    // SIGNAL_CONFIDENCE_GOOD_LEVEL = 0.1 because its mic sits at ~15 cm; here
    // silence measures 0.0003-0.0006 and a note 0.002-0.011 (measured on the board on
    // 19/09/2026). With 0.1 as ceiling, a perfectly good note would give
    // signalConf ~0.1 — the number exists but informs nothing.
    const float GOOD_LEVEL = 0.01f;      // ~3x the minimum level observed with a note
    float signalConf = (rms_ >= GOOD_LEVEL) ? 1.0f
                     : (rms_ - minAmplitude_) / (GOOD_LEVEL - minAmplitude_);
    if (signalConf < 0.0f) signalConf = 0.0f;
    if (signalConf > 1.0f) signalConf = 1.0f;
    float agree = 1.0f - fminf(1.0f, fabsf(freq - smoothedFrequency_) / (smoothedFrequency_ + 1e-6f));
    overallConfidence_ = 0.6f * signalConf + 0.4f * agree;

    cachedFrequency_ = smoothedFrequency_;
    return cachedFrequency_;
}

int MicInput::detectNote() {
    float freq = detectFrequency();
    if (freq <= 0.0f) return -1;
    int midi = (int)roundf(12.0f * log2f(freq / 440.0f) + 69.0f);
    if (midi < 12 || midi > 108) return -1;      // outside the useful musical range
    return midi;
}

void MicInput::end() {
    if (rxHandle_) {
        i2s_channel_disable((i2s_chan_handle_t)rxHandle_);
        i2s_del_channel((i2s_chan_handle_t)rxHandle_);
        rxHandle_ = nullptr;
    }
    initialized_ = false;
    smoothingInit_ = false;
}

}  // namespace esp32_escaperoom
