#pragma once

// Destinos da matriz de modulacao (indices usados nos parametros "...Dest").
enum ModDest { kDestNone = 0, kDestPitch, kDestPwm, kDestCutoff, kDestRes, kDestSub, kDestNoise, kDestLevel, kNumDests };

struct ModSlot
{
    int dest = 0;
    float amount = 0.0f;
};

// Parametros lidos uma vez por bloco pelo processador e partilhados pelas vozes.
struct SynthParams
{
    int   rangeSemis = 0;
    int   pwmMode = 0;            // 0 manual, 1 LFO, 2 envelope
    int   vcaMode = 0;            // 0 env, 1 gate, 2 env2
    bool  saw = true, pulse = true;
    float pwm = 0.2f, sub = 0.25f, noise = 0.0f, tuneCents = 0.0f;
    float lfoDco = 0.0f, lfoVcf = 0.0f;
    float cutoff = 3000.0f, res = 0.15f, envAmt = 0.4f, keyFollow = 0.5f;
    float attack = 0.01f, decay = 0.5f, sustain = 0.6f, release = 0.4f;
    float env2A = 0.01f, env2D = 0.5f, env2S = 0.6f, env2R = 0.4f;
    float velSens = 0.3f;
    float modWheel = 0.0f, aftertouch = 0.0f;
    float bendSemis = 2.0f, bendVcf = 0.0f, modDco = 0.1f, modVcf = 0.0f;
    float driftCents = 0.0f;      // deriva do oscilador mestre, comum a todas as vozes
    int   unison = 1;             // 1..4 osciladores empilhados por nota
    float detune = 0.3f;
    int   poly = 6;               // numero maximo de vozes
    float glide = 0.0f;           // portamento (s)
    int   calib = 0;              // 0 Good, 1 Avg, 2 Poor
    int   lfo2Shape = 1, lfo2Retrig = 0;
    float lfo2Rate = 1.0f;
    ModSlot wheel, velo, aft, lfo2a, lfo2b, env2a, env2b;
};

struct SharedState
{
    SynthParams params;
    const float* lfo = nullptr;    // LFO 1 com delay (vibrato e filtro)
    const float* lfoRaw = nullptr; // LFO 1 sem delay (PWM)
    const float* lfo2 = nullptr;   // LFO 2 global (modos Off e Mono)
    int lastNote = -1;             // ultima nota tocada (portamento)
};
