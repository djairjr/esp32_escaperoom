#pragma once

// ============================================================================
// MicInput — deteccao de nota via INMP441 (I2S) para o AcordeonPuzzle
//
// LOGICA PORTADA de Esp32VoiceTuner (https://github.com/Vateva/Esp32VoiceTuner)
// SPDX-FileCopyrightText: Copyright (c) 2025 Vateva
// SPDX-License-Identifier: MIT
// [upstream: src/audioprocessing.cpp, include/config.h]
// Adaptado para o AcordeonPuzzle (ESP32-C3, projeto standalone) em 19/09/2026.
//
// O QUE FOI PORTADO (a parte que resolve o problema):
//   - filtros IIR Butterworth de 2a ordem em cascata (highpass 60 / lowpass 6000)
//   - deteccao de pitch por YIN: passe grosso + passe refinado com janela
//   - validacao por harmonicos (2o/3o/4o) e escolha entre candidatos
//   - interpolacao parabolica para precisao sub-amostra
//   - suavizacao EMA com deteccao de troca de nota
//   - gate de silencio sobre RMS
//
// O QUE FOI DESCARTADO (nao serve a um sensor):
//   tarefas FreeRTOS e fila, display GC9A01, menu, gestao de energia por
//   frequencia de CPU, nomes de nota por escala (o acordeon tem afinacao fixa).
//
// POR QUE YIN E NAO FFT: uma FFT de 512 pontos a 16 kHz tem 31,25 Hz/bin —
// em C4 (262 Hz) isso e' ~35 cents por bin, grosso demais para sequencia
// musical. Autocorrelacao no dominio do tempo acha o PERIODO da fundamental.
// E a leitura do I2S e' STEREO com escolha de slot: MONO de 32 bits devolve
// buffer zero-stuffed ([d,0,d,0,...]), que o dcRemoval transforma em onda
// quadrada em fs/2 e faz TODA nota ser lida como B8.
//
// API PRESERVADA: begin/end/readSample/detectFrequency/detectNote/getRms/
// setMinAmplitude/isLineStuck continuam identicos para o PuzzleEngine.
// ============================================================================

#include <Arduino.h>
#include "PlatformPins.h"

namespace esp32_escaperoom {

class MicInput {
public:
    MicInput();
    ~MicInput();

    /// Configura I2S RX (16 kHz, 32-bit, STEREO) e os filtros IIR.
    /// true = driver subiu. NAO prova que o mic entrega dado — use getRms().
    bool begin();

    /// Le um buffer, escolhe o slot ativo, aplica os filtros e calcula RMS.
    bool readSample();

    /// Frequencia detectada em Hz pelo YIN. 0 se abaixo do gate.
    float detectFrequency();

    /// Frequencia -> MIDI note (60 = C4). -1 se silencio/sem deteccao.
    int detectNote();

    /// Ultimo resultado de detectFrequency() em Hz (0 se silencio).
    /// A tolerancia em CENTS precisa da frequencia: convertida para MIDI
    /// inteiro (round) a informacao de cents ja teria sido perdida.
    float getLastFrequency() const { return cachedFrequency_; }

    /// Nivel RMS do ultimo buffer (escala [-1,1]) — grandeza do gate.
    float getRms() const { return rms_; }

    /// Nivel em dB (referencia do upstream) — diagnostico.
    float getDbLevel() const { return dbLevel_; }

    /// Confianca global da deteccao (0-1) — diagnostico.
    float getConfidence() const { return overallConfidence_; }

    /// true se o buffer lido veio sem variacao (linha SD parada).
    bool isLineStuck() const { return lineStuck_; }

    /// Indice do slot I2S que carrega o audio: 0 = 1o slot, 1 = 2o slot.
    uint8_t getActiveSlot() const { return activeSlot_; }

    /// Limiar de silencio sobre o RMS. Calibrar com a bancada.
    void setMinAmplitude(float v) { minAmplitude_ = v; }
    float getMinAmplitude() const { return minAmplitude_; }

    /// Desliga I2S (para deep sleep).
    void end();

    /// Funcao chamada nos pontos de cedencia de CPU dentro da analise YIN.
    /// Serve para o SKETCH atender o servidor web durante o processamento:
    /// o loopTask roda na prioridade 1 e o handler HTTP so' e' executado
    /// quando alguem o chama — sem isto, os pedidos ficam na fila durante
    /// toda a analise (medido: /api/sequence 4/4 timeout).
    /// Mantem o MicInput desacoplado do webserver: e' so' um ponteiro.
    typedef void (*YieldPump)();
    void setYieldPump(YieldPump fn) { yieldPump_ = fn; }

private:
    static const uint16_t kSamples    = 2048;   // amostras por canal (janela YIN)
    static const uint32_t kSampleRate = 48000;  // 48 kHz: melhor resolucao

    // ── parametros YIN (do upstream; faixa util ~80-1500 Hz a 48 kHz) ──
    static const int YIN_MIN_PERIOD = 32;
    static const int YIN_MAX_PERIOD = 600;
    static const int YIN_COARSE_SAMPLES = 512;
    static const int YIN_MAX_CANDIDATES = 8;
    static constexpr float YIN_COARSE_THRESHOLD = 0.25f;
    static constexpr float YIN_THRESHOLD = 0.15f;
    static constexpr float YIN_COARSE_QUALITY_GATE = 0.5f;

    static const uint8_t MAX_SCALE_NOTES = 12;

    // ── filtro IIR de 2a ordem (struct do upstream) ───────────────────
    struct IIRFilter {
        float b0, b1, b2, a1, a2;
        float x1, x2, y1, y2;
        bool  initialized;
        IIRFilter() : b0(1), b1(0), b2(0), a1(0), a2(0),
                      x1(0), x2(0), y1(0), y2(0), initialized(false) {}
        void reset() { x1 = x2 = y1 = y2 = 0.0f; initialized = true; }
    };

    // ── estado ────────────────────────────────────────────────────────
    void*   rxHandle_ = nullptr;    // i2s_chan_handle_t (opaco)
    bool    initialized_ = false;
    uint8_t activeSlot_ = 0;

    int32_t rawStereo_[kSamples * 2];   // leitura crua stereo
    float   samples_[kSamples];         // slot ativo, filtrado
    float   yinBuffer_[YIN_MAX_PERIOD + 1];

    IIRFilter highpass_, lowpass_;

    float rms_ = 0.0f;
    float dbLevel_ = -80.0f;
    float minAmplitude_ = 0.001f;
    float overallConfidence_ = 0.0f;
    bool  lineStuck_ = false;

    // suavizacao EMA (do upstream): evita oscilar entre notas vizinhas
    float smoothedFrequency_ = 0.0f;
    float lastRawFrequency_ = 0.0f;
    bool  smoothingInit_ = false;
    float cachedFrequency_ = 0.0f;   // resultado da ultima janela de leitura
    unsigned long lastReadMs_ = 0;   // janela de leitura (governada por TEMPO)
    // ≈ duracao do buffer de audio: 2048 amostras / 48 kHz = 42,7 ms.
    // Ler mais rapido que isto nao traz amostra nova ao buffer do I2S.
    static const unsigned long kLeituraIntervaloMs = 40;
    static YieldPump yieldPump_;     // atende o servidor nos yields

    // ── helpers portados ──────────────────────────────────────────────
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
