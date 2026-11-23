#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Dsp.h"
#include "SynthShared.h"

class SixtySound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int) override    { return true; }
    bool appliesToChannel (int) override { return true; }
};

// Uma voz: DCO (serra + pulso, com unison; sub e ruido) -> VCF 24 dB -> VCA, com dois envelopes e LFO 2 opcional.
// Valores de modulacao tirados das Service Notes do Juno-60 (ver README).
class SixtyVoice : public juce::SynthesiserVoice
{
public:
    SixtyVoice (SharedState& s, int voiceIndex) : shared (s), index (voiceIndex)
    {
        juce::Random r (voiceIndex * 7919 + 13);
        for (auto& d : disp) d = r.nextFloat() * 2.0f - 1.0f;
        lfo2v.seed ((std::uint32_t) (voiceIndex * 104729 + 7));
    }

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<SixtySound*> (s) != nullptr && index < shared.params.poly;
    }

    void startNote (int midiNote, float vel, juce::SynthesiserSound*, int pitchWheel) override
    {
        const auto& p = shared.params;
        note = midiNote;
        velocity = vel;
        bend = (float) (pitchWheel - 8192) / 8192.0f;

        startPitch = (p.glide > 0.0f && shared.lastNote >= 0) ? (float) shared.lastNote : (float) midiNote;
        shared.lastNote = midiNote;
        glideAge = 0.0f;

        for (auto& ph : phase) ph = rng.nextFloat();
        subState = rng.nextBool();
        lfo2v.reset (0.0f);

        filter.reset();
        env.reset();  env.noteOn();
        env2.reset(); env2.noteOn();
        gate.reset(); gate.setParams (0.002f, 0.002f, 1.0f, 0.006f, (float) getSampleRate()); gate.noteOn();
    }

    void stopNote (float, bool allowTailOff) override
    {
        env.noteOff();
        env2.noteOff();
        gate.noteOff();
        if (! allowTailOff)
            clearCurrentNote();
    }

    void pitchWheelMoved (int v) override { bend = (float) (v - 8192) / 8192.0f; }
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& out, int startSample, int numSamples) override
    {
        if (! isVoiceActive())
            return;

        using namespace sixty;
        const auto& p = shared.params;
        const float* lfo1 = shared.lfo;
        const float* lfo1Raw = shared.lfoRaw;
        const float* lfo2g = shared.lfo2;
        const float sr = (float) getSampleRate();
        float* dst = out.getWritePointer (0);

        // calibracao das vozes: dispersao entre vozes (Good / Avg / Poor)
        const float calAmt = p.calib == 0 ? 0.0f : (p.calib == 1 ? 0.5f : 1.0f);
        const float timeScale = 1.0f + disp[4] * 0.15f * calAmt;
        env.setParams  (p.attack * timeScale, p.decay * timeScale, p.sustain, p.release * timeScale, sr);
        env2.setParams (p.env2A * timeScale,  p.env2D * timeScale,  p.env2S,   p.env2R * timeScale,  sr);

        const float velGain = 1.0f - p.velSens + p.velSens * velocity;
        const float calGain = 1.0f + disp[3] * 0.12f * calAmt;
        const float glideSamples = p.glide * sr;
        const int nUni = std::max (1, std::min (4, p.unison));
        const float uniGain = 1.0f / std::sqrt ((float) nUni);

        for (int i = 0; i < numSamples; ++i)
        {
            const int idx = startSample + i;
            const float env1V = env.next();
            const float env2V = env2.next();
            const float gateV = gate.next();
            const float vca = p.vcaMode == 1 ? gateV : (p.vcaMode == 2 ? env2V : env1V);

            if ((i & 7) == 0) // taxa de controlo
            {
                const float l1 = lfo1[idx];
                const float l1Raw = lfo1Raw[idx];
                const float l2 = p.lfo2Retrig == 2 ? lfo2v.tick (p.lfo2Rate * 8.0f / sr, p.lfo2Shape) : lfo2g[idx];

                // matriz de modulacao (fontes: roda, velocity, aftertouch, LFO 2, ENV 2)
                float acc[kNumDests] = {};
                auto add = [&acc] (const ModSlot& m, float src) { if (m.dest > 0 && m.dest < kNumDests) acc[m.dest] += src * m.amount; };
                add (p.wheel, p.modWheel);  add (p.velo, velocity);  add (p.aft, p.aftertouch);
                add (p.lfo2a, l2);          add (p.lfo2b, l2);
                add (p.env2a, env2V);       add (p.env2b, env2V);

                // portamento (tempo fixo, igual para todas as notas)
                float glideOffset = 0.0f;
                if (glideSamples > 1.0f && glideAge < glideSamples)
                    glideOffset = (startPitch - (float) note) * (1.0f - glideAge / glideSamples);
                glideAge += 8.0f;

                // Oscilador mestre: LFO ate +-300 cents, bender ate +-700 cents (aqui ate 12 semitons), tune +-50 cents.
                const float dcoLfo = std::min (1.0f, p.lfoDco + p.modWheel * p.modDco);
                const float vcfLfo = std::min (1.0f, p.lfoVcf + p.modWheel * p.modVcf);
                const float semis = (float) note + (float) p.rangeSemis + glideOffset
                                    + (p.tuneCents + p.driftCents + disp[0] * 3.0f * calAmt) * 0.01f
                                    + bend * p.bendSemis + l1 * dcoLfo * 3.0f + acc[kDestPitch] * 12.0f;
                const float freq = 440.0f * std::exp2 ((semis - 69.0f) / 12.0f);
                for (int u = 0; u < nUni; ++u)
                {
                    const float spread = nUni > 1 ? ((float) u - 0.5f * (float) (nUni - 1)) / (0.5f * (float) (nUni - 1)) : 0.0f;
                    const float cents = spread * p.detune * 25.0f;
                    dtU[u] = std::min (freq * std::exp2 (cents / 1200.0f) / sr, 0.45f);
                }

                // Largura de pulso: 50% a 95%. O PWM por LFO usa o LFO sem delay (o manual do Jun-6 V confirma).
                float depth = p.pwm;
                if (p.pwmMode == 1)      depth = p.pwm * (0.5f + 0.5f * l1Raw);
                else if (p.pwmMode == 2) depth = p.pwm * env1V;
                depth = std::max (0.0f, std::min (1.0f, depth + acc[kDestPwm]));
                pw = 0.5f + 0.45f * depth;

                subLevel   = std::max (0.0f, std::min (1.0f, p.sub + acc[kDestSub]));
                noiseLevel = std::max (0.0f, std::min (1.0f, p.noise + acc[kDestNoise]));
                levelMod   = std::max (0.0f, 1.0f + acc[kDestLevel]);

                // VCF: LFO ate +-3 oitavas (6 Vp-p), envelope ate ~9 oitavas (~30 kHz), key follow 1 V/oit.
                const float oct = p.envAmt * env1V * 9.0f + l1 * vcfLfo * 3.0f
                                  + p.keyFollow * ((float) note + glideOffset - 60.0f) / 12.0f
                                  + bend * p.bendVcf * 3.0f + acc[kDestCutoff] * 5.0f + disp[1] * 0.12f * calAmt;
                float fc = p.cutoff * std::exp2 (oct);
                fc = std::max (20.0f, std::min (std::min (20000.0f, sr * 0.45f), fc));
                const float f = std::min (0.8f, 1.0f - std::exp (-kTwoPi * fc / sr));
                const float res = std::max (0.0f, std::min (1.0f, p.res + acc[kDestRes] * 0.5f + disp[2] * 0.04f * calAmt));
                filter.setCoefficients (f, res * 4.0f); // res = 1 -> auto-oscilacao (seno)
            }

            float osc = 0.0f;
            for (int u = 0; u < nUni; ++u)
            {
                const float d = dtU[u];
                phase[u] += d;
                if (phase[u] >= 1.0f) { phase[u] -= 1.0f; if (u == 0) subState = ! subState; }
                const float t = phase[u];

                if (p.saw)
                    osc += (2.0f * t - 1.0f) - polyBlep (t, d);
                if (p.pulse)
                {
                    float v = t < pw ? 1.0f : -1.0f;
                    v += polyBlep (t, d);
                    float t2 = t - pw; if (t2 < 0.0f) t2 += 1.0f;
                    v -= polyBlep (t2, d);
                    osc += v;
                }
            }
            osc *= uniGain;

            if (subLevel > 0.0f)
            {
                const float t = phase[0], d = dtU[0];
                float sv = subState ? 1.0f : -1.0f;
                const float dir = t > 0.5f ? (subState ? -1.0f : 1.0f) : (subState ? 1.0f : -1.0f);
                sv += dir * polyBlep (t, d);
                osc += subLevel * sv;
            }
            if (noiseLevel > 0.0f)
                osc += noiseLevel * (rng.nextFloat() * 2.0f - 1.0f);

            const float y = filter.process (osc * 0.4f);
            dst[idx] += y * vca * velGain * levelMod * calGain * 0.45f;

            const bool finished = p.vcaMode == 1 ? gate.isIdle() : (p.vcaMode == 2 ? env2.isIdle() : env.isIdle());
            if (finished)
            {
                clearCurrentNote();
                break;
            }
        }
    }

private:
    SharedState& shared;
    const int index;
    sixty::Envelope env, env2, gate;
    sixty::LadderFilter filter;
    sixty::LfoGen lfo2v;
    juce::Random rng;
    float disp[5] = {};   // dispersao fixa desta voz (calibracao)

    int note = 60;
    float velocity = 1.0f, bend = 0.0f;
    float startPitch = 60.0f, glideAge = 0.0f;
    float phase[4] = {}, dtU[4] = { 0.01f, 0.01f, 0.01f, 0.01f };
    float pw = 0.5f, subLevel = 0.0f, noiseLevel = 0.0f, levelMod = 1.0f;
    bool subState = false;
};
