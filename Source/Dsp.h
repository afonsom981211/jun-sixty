#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace sixty
{
constexpr float kTwoPi = 6.28318530717958647692f;

inline float fastTanh (float x) noexcept
{
    x = std::max (-3.0f, std::min (3.0f, x));
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

// Correcao PolyBLEP para descontinuidades de altura 2 na fase 0.
inline float polyBlep (float t, float dt) noexcept
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

//==============================================================================
// Divisoes de tempo para sincronizar com a DAW (em tempos de semínima).
inline constexpr int kNumSync = 13;
inline constexpr const char* kSyncNames[kNumSync] = { "2 bar", "1 bar", "1/2", "1/2T", "1/4", "1/4.", "1/4T",
                                                      "1/8", "1/8.", "1/8T", "1/16", "1/16T", "1/32" };
inline constexpr float kSyncBeats[kNumSync] = { 8.0f, 4.0f, 2.0f, 4.0f / 3.0f, 1.0f, 1.5f, 2.0f / 3.0f,
                                                0.5f, 0.75f, 1.0f / 3.0f, 0.25f, 1.0f / 6.0f, 0.125f };

inline float syncSeconds (int index, double bpm) noexcept
{
    index = std::max (0, std::min (kNumSync - 1, index));
    return (float) (60.0 / std::max (bpm, 1.0) * (double) kSyncBeats[index]);
}

//==============================================================================
// ADSR exponencial (estilo RC, como nos circuitos analogicos).
class Envelope
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void reset() { stage = Stage::Idle; level = 0.0f; }
    void noteOn() { stage = Stage::Attack; }
    void noteOff() { if (stage != Stage::Idle) stage = Stage::Release; }
    bool isIdle() const { return stage == Stage::Idle; }

    void setParams (float a, float d, float s, float r, float sampleRate)
    {
        sustain = s;
        aC = coef (a * sampleRate / 1.79f); // alvo 1.2 -> atinge 1.0 em 'a' segundos
        dC = coef (d * sampleRate / 4.0f);
        rC = coef (r * sampleRate / 4.0f);
    }

    float next() noexcept
    {
        switch (stage)
        {
            case Stage::Idle: break;
            case Stage::Attack:
                level += (1.2f - level) * aC;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                level += (sustain - level) * dC;
                if (level - sustain < 1.0e-4f) stage = Stage::Sustain;
                break;
            case Stage::Sustain:
                level += (sustain - level) * dC;
                break;
            case Stage::Release:
                level -= level * rC;
                if (level < 1.0e-4f) { level = 0.0f; stage = Stage::Idle; }
                break;
        }
        return level;
    }

private:
    static float coef (float tauSamples) { return 1.0f - std::exp (-1.0f / std::max (tauSamples, 1.0f)); }
    Stage stage = Stage::Idle;
    float level = 0.0f, sustain = 1.0f, aC = 0.01f, dC = 0.001f, rC = 0.001f;
};

//==============================================================================
// Filtro passa-baixo de 4 polos (24 dB/oct) com saturacao por estagio e realimentacao.
class LadderFilter
{
public:
    void reset() { y1 = y2 = y3 = y4 = t1 = t2 = t3 = 0.0f; }
    void setCoefficients (float f_, float k_) { f = f_; k = k_; }

    float process (float in) noexcept
    {
        const float x = in - k * y4;
        y1 += f * (fastTanh (x) - t1);  t1 = fastTanh (y1);
        y2 += f * (t1 - t2);            t2 = fastTanh (y2);
        y3 += f * (t2 - t3);            t3 = fastTanh (y3);
        y4 += f * (t3 - fastTanh (y4));
        return y4;
    }

private:
    float f = 0.1f, k = 0.0f;
    float y1 = 0, y2 = 0, y3 = 0, y4 = 0, t1 = 0, t2 = 0, t3 = 0;
};

// Passa-alto de 1 polo (6 dB/oct).
class OnePoleHP
{
public:
    void reset() { lp = 0.0f; }
    void setCutoff (float fc, double sampleRate) { a = 1.0f - std::exp (-kTwoPi * fc / (float) sampleRate); }
    float process (float x) noexcept { lp += a * (x - lp); return x - lp; }
private:
    float lp = 0.0f, a = 0.01f;
};

//==============================================================================
// LFO com varias formas de onda (0 seno, 1 triangulo, 2 serra, 3 rampa, 4 quadrada, 5 sample&hold).
class LfoGen
{
public:
    void seed (std::uint32_t s) { rng = s != 0 ? s : 1u; shValue = noise(); }
    void reset (float startPhase = 0.0f) { phase = startPhase; }

    float tick (float inc, int shape) noexcept
    {
        phase += inc;
        if (phase >= 1.0f) { phase -= std::floor (phase); shValue = noise(); }
        return wave (phase, shape);
    }

    float wave (float ph, int shape) const noexcept
    {
        switch (shape)
        {
            case 0:  return std::sin (kTwoPi * ph);
            case 1:  return 1.0f - 4.0f * std::abs (ph - 0.5f);
            case 2:  return 1.0f - 2.0f * ph;
            case 3:  return 2.0f * ph - 1.0f;
            case 4:  return ph < 0.5f ? 1.0f : -1.0f;
            default: return shValue;
        }
    }

    float phase = 0.0f;

private:
    float noise() noexcept
    {
        rng = rng * 1664525u + 1013904223u;
        return (float) ((rng >> 8) & 0xFFFFu) / 32768.0f - 1.0f;
    }
    float shValue = 0.0f;
    std::uint32_t rng = 22222u;
};

//==============================================================================
// Chorus estilo BBD do Juno: entrada mono, saida estereo, taps com LFO em oposicao.
class JunoChorus
{
public:
    void prepare (double sr)
    {
        sampleRate = (float) sr;
        buf.assign ((size_t) (sr * 0.03) + 8, 0.0f);
        w = 0;
        lpL = lpR = lpL2 = lpR2 = 0.0f;
        lpA = 1.0f - std::exp (-kTwoPi * 7500.0f / sampleRate); // filtros pre/pos BBD (~7 kHz no esquema)
        curRate = 0.5f; curDepth = 1.0f; phase = 0.0f;
    }

    void setMode (int m) { mode = m; } // 0 off, 1 I, 2 II, 3 I+II

    void process (float in, float& outL, float& outR) noexcept
    {
        const int size = (int) buf.size();
        buf[(size_t) w] = in;

        // Esquema (Panel Board B): I = 20 Vpp, periodo ~2 s; II = 20 Vpp, ~1,2 s; I+II = 1,6 Vpp (0,08x a profundidade).
        // Num oscilador triangular amplitude e frequencia andam ligadas: amplitude 12,5x menor => cerca de 10x mais rapido.
        // O manual do Jun-6 V confirma: no I+II as duas linhas deixam de estar em panorama (mono) e o LFO e muito mais rapido.
        float rate = 0.513f, depthMs = 1.0f;
        if (mode == 2) { rate = 0.863f; depthMs = 1.0f; }
        if (mode == 3) { rate = 9.75f;  depthMs = 0.08f; }
        curRate  += (rate - curRate) * 0.0005f;
        curDepth += (depthMs - curDepth) * 0.0005f;

        phase += curRate / sampleRate;
        if (phase >= 1.0f) phase -= 1.0f;
        const float lfo = 1.0f - 4.0f * std::abs (phase - 0.5f);

        const float centre = 3.5f;
        const float dL = (centre + curDepth * lfo) * 0.001f * sampleRate;
        const float dR = (centre - curDepth * lfo) * 0.001f * sampleRate;

        float wl = readFrac (dL, size);
        float wr = readFrac (dR, size);
        lpL += lpA * (wl - lpL);   lpL2 += lpA * (lpL - lpL2);
        lpR += lpA * (wr - lpR);   lpR2 += lpA * (lpR - lpR2);

        w = (w + 1) % size;

        if (mode == 0) { outL = outR = in; return; }
        outL = in * 0.6f + lpL2 * 0.6f;
        outR = in * 0.6f + lpR2 * 0.6f;
    }

private:
    float readFrac (float delay, int size) const noexcept
    {
        float rp = (float) w - delay;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const float fr = rp - (float) i0;
        const int i1 = (i0 + 1) % size;
        return buf[(size_t) (i0 % size)] * (1.0f - fr) + buf[(size_t) i1] * fr;
    }

    std::vector<float> buf;
    int w = 0, mode = 1;
    float sampleRate = 44100.0f, phase = 0.0f, curRate = 0.5f, curDepth = 1.0f;
    float lpL = 0, lpR = 0, lpL2 = 0, lpR2 = 0, lpA = 0.5f;
};

//==============================================================================
// Delay estereo com feedback e dispersao estereo.
class StereoDelay
{
public:
    void prepare (double sr)
    {
        size = (int) (sr * 2.1) + 4;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        w = 0; sL = sR = 0.0f;
    }

    void process (float& l, float& r, float timeSamples, float feedback, float stereo, float mix) noexcept
    {
        const float maxT = (float) (size - 2);
        const float tL = std::min (timeSamples, maxT);
        const float tR = std::min (timeSamples * (1.0f + 0.5f * stereo), maxT);
        sL += (tL - sL) * 0.001f;
        sR += (tR - sR) * 0.001f;

        const float dl = read (bufL, sL);
        const float dr = read (bufR, sR);
        const float fb = std::min (feedback, 0.95f);
        bufL[(size_t) w] = l + fb * ((1.0f - 0.5f * stereo) * dl + 0.5f * stereo * dr);
        bufR[(size_t) w] = r + fb * ((1.0f - 0.5f * stereo) * dr + 0.5f * stereo * dl);
        w = (w + 1) % size;

        l = l * (1.0f - mix) + dl * mix;
        r = r * (1.0f - mix) + dr * mix;
    }

private:
    float read (const std::vector<float>& b, float delay) const noexcept
    {
        float rp = (float) w - delay;
        while (rp < 0.0f) rp += (float) size;
        const int i0 = (int) rp;
        const float fr = rp - (float) i0;
        return b[(size_t) (i0 % size)] * (1.0f - fr) + b[(size_t) ((i0 + 1) % size)] * fr;
    }

    std::vector<float> bufL, bufR;
    int size = 4, w = 0;
    float sL = 0.0f, sR = 0.0f;
};

// Atraso inteiro simples (pre-delay do reverb).
class SimpleDelay
{
public:
    void prepare (int maxSamples) { buf.assign ((size_t) std::max (maxSamples, 1) + 2, 0.0f); w = 0; }
    float process (float in, int delay) noexcept
    {
        const int size = (int) buf.size();
        delay = std::max (0, std::min (delay, size - 1));
        buf[(size_t) w] = in;
        int rp = w - delay;
        if (rp < 0) rp += size;
        const float out = buf[(size_t) rp];
        w = (w + 1) % size;
        return out;
    }
private:
    std::vector<float> buf;
    int w = 0;
};
} // namespace sixty
