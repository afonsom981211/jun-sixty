#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Voice.h"

namespace
{
constexpr int kMaxVoices = 16;
}

juce::AudioProcessorValueTreeState::ParameterLayout SixtyAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto addF = [&] (const char* id, const char* name, float lo, float hi, float def, float centre = 0.0f)
    {
        juce::NormalisableRange<float> r (lo, hi);
        if (centre > 0.0f) r.setSkewForCentre (centre);
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, r, def));
    };
    auto addB = [&] (const char* id, const char* name, bool def)
    {
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def));
    };
    auto addC = [&] (const char* id, const char* name, const juce::StringArray& choices, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { id, 1 }, name, choices, def));
    };
    auto addI = [&] (const char* id, const char* name, int lo, int hi, int def)
    {
        layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { id, 1 }, name, lo, hi, def));
    };

    juce::StringArray sync;
    for (int i = 0; i < sixty::kNumSync; ++i) sync.add (sixty::kSyncNames[i]);
    juce::StringArray chords;
    for (int i = 0; i < sixty::kNumChords; ++i) chords.add (sixty::kChordNames[i]);
    const juce::StringArray dest { "None", "Pitch", "PWM", "Cutoff", "Reson.", "Sub", "Noise", "Level" };

    // DCO
    addC ("range",  "Range",      { "16'", "8'", "4'" }, 1);
    addB ("saw",    "Saw",        true);
    addB ("pulse",  "Pulse",      true);
    addC ("pwMode", "PWM Mode",   { "Man", "LFO", "Env" }, 0);
    addF ("pw",     "Pulse Width", 0.0f, 1.0f, 0.2f);
    addF ("sub",    "Sub",         0.0f, 1.0f, 0.25f);
    addF ("noise",  "Noise",       0.0f, 1.0f, 0.0f);
    addF ("tune",   "Tune",       -50.0f, 50.0f, 0.0f);

    // LFO 1
    addF ("lfoRate",  "LFO Rate",  0.3f, 20.0f, 4.0f, 3.0f);
    addF ("lfoDelay", "LFO Delay", 0.0f, 1.5f, 0.0f, 0.4f);
    addF ("lfoDco",   "LFO > DCO", 0.0f, 1.0f, 0.0f);
    addF ("lfoVcf",   "LFO > VCF", 0.0f, 1.0f, 0.0f);
    addB ("lfoSync",  "LFO Sync",  false);
    addC ("lfoDiv",   "LFO Div",   sync, 4);
    addC ("lfoDelayMode", "LFO Delay Mode", { "Delay", "Rise" }, 0);

    // HPF / Chorus
    addC ("hpf",    "HPF",    { "0", "1", "2", "3" }, 0);
    addC ("chorus", "Chorus", { "Off", "I", "II", "I+II" }, 1);
    addB ("chorusNoise", "Chorus Noise", false);

    // VCF
    addF ("cutoff",    "Cutoff",     20.0f, 20000.0f, 3000.0f, 1000.0f);
    addF ("res",       "Resonance",  0.0f, 1.0f, 0.15f);
    addF ("envAmt",    "Env Amount", -1.0f, 1.0f, 0.4f);
    addF ("keyFollow", "Key Follow", 0.0f, 1.0f, 0.5f);

    // ENV / VCA
    addF ("attack",  "Attack",  0.001f, 3.0f,  0.01f, 0.15f);
    addF ("decay",   "Decay",   0.002f, 12.0f, 0.5f,  0.8f);
    addF ("sustain", "Sustain", 0.0f,   1.0f,  0.6f);
    addF ("release", "Release", 0.002f, 12.0f, 0.4f,  0.8f);
    addC ("vcaMode", "VCA",     { "Env", "Gate", "Env2" }, 0);

    // Saida e performance
    addF ("volume",  "Volume",   0.0f, 1.0f, 0.7f);
    addF ("velSens", "Velocity", 0.0f, 1.0f, 0.3f);
    addF ("drift",   "Drift",    0.0f, 1.0f, 0.2f);
    addF ("bend",    "Bender DCO", 0.0f, 12.0f, 2.0f);
    addF ("bendVcf", "Bender VCF", 0.0f, 1.0f, 0.0f);
    addF ("modDco",  "Mod Wheel DCO", 0.0f, 1.0f, 0.1f);
    addF ("modVcf",  "Mod Wheel VCF", 0.0f, 1.0f, 0.0f);
    addI ("unison",  "Unison",   1, 4, 1);
    addF ("detune",  "Unison Detune", 0.0f, 1.0f, 0.3f);
    addI ("poly",    "Poly",     1, kMaxVoices, 6);
    addF ("glide",   "Portamento", 0.0f, 2.0f, 0.0f, 0.3f);
    addC ("calib",   "Voice Calibration", { "Good", "Avg", "Poor" }, 0);

    // Hold / acordes / arpejador
    addB ("hold",      "Hold",        false);
    addB ("chordOn",   "Chord",       false);
    addC ("chordType", "Chord Type",  chords, 7);
    addB ("arpOn",     "Arp",         false);
    addC ("arpMode",   "Arp Mode",    { "Up", "Up&Dn", "Down" }, 0);
    addC ("arpRange",  "Arp Range",   { "1", "2", "3" }, 0);
    addF ("arpRate",   "Arp Rate",    1.36f, 45.0f, 8.0f, 8.0f);
    addB ("arpSync",   "Arp Sync",    false);
    addC ("arpDiv",    "Arp Div",     sync, 10);

    // Matriz de modulacao (Assign)
    addC ("wheelDest", "Wheel Dest", dest, 0);  addF ("wheelAmt", "Wheel Amt", -1.0f, 1.0f, 0.0f);
    addC ("veloDest",  "Velo Dest",  dest, 0);  addF ("veloAmt",  "Velo Amt",  -1.0f, 1.0f, 0.0f);
    addC ("aftDest",   "Aftertouch Dest", dest, 0); addF ("aftAmt", "Aftertouch Amt", -1.0f, 1.0f, 0.0f);

    // LFO 2
    addB ("lfo2Sync",   "LFO2 Sync",  false);
    addC ("lfo2Div",    "LFO2 Div",   sync, 10);
    addF ("lfo2Rate",   "LFO2 Rate",  0.038f, 30.0f, 1.0f, 3.0f);
    addC ("lfo2Wave",   "LFO2 Wave",  { "Sine", "Tri", "Saw", "Ramp", "Square", "S&H" }, 1);
    addC ("lfo2Retrig", "LFO2 Retrig", { "Off", "Mono", "Poly" }, 0);
    addC ("lfo2Dest1",  "LFO2 Dest 1", dest, 0);  addF ("lfo2Amt1", "LFO2 Amt 1", -1.0f, 1.0f, 0.0f);
    addC ("lfo2Dest2",  "LFO2 Dest 2", dest, 0);  addF ("lfo2Amt2", "LFO2 Amt 2", -1.0f, 1.0f, 0.0f);

    // ENV 2
    addF ("env2A", "Env2 Attack",  0.001f, 3.0f,  0.01f, 0.15f);
    addF ("env2D", "Env2 Decay",   0.002f, 12.0f, 0.5f,  0.8f);
    addF ("env2S", "Env2 Sustain", 0.0f,   1.0f,  0.6f);
    addF ("env2R", "Env2 Release", 0.002f, 12.0f, 0.4f,  0.8f);
    addC ("env2Dest1", "Env2 Dest 1", dest, 0);  addF ("env2Amt1", "Env2 Amt 1", -1.0f, 1.0f, 0.0f);
    addC ("env2Dest2", "Env2 Dest 2", dest, 0);  addF ("env2Amt2", "Env2 Amt 2", -1.0f, 1.0f, 0.0f);

    // Delay
    addB ("delayOn",     "Delay On",    true);
    addF ("delayTime",   "Delay Time",  2.0f, 2000.0f, 250.0f, 300.0f);
    addB ("delaySync",   "Delay Sync",  false);
    addC ("delayDiv",    "Delay Div",   sync, 7);
    addF ("delayFb",     "Delay Feedback", 0.0f, 1.0f, 0.3f);
    addF ("delayStereo", "Delay Stereo",   0.0f, 1.0f, 0.3f);
    addF ("delayMix",    "Delay Dry/Wet",  0.0f, 1.0f, 0.0f);

    // Reverb
    addB ("revOn",    "Reverb On",    true);
    addF ("revSize",  "Reverb Size",  0.0f, 1.0f, 0.643f);
    addF ("revDamp",  "Reverb Damp",  0.0f, 1.0f, 0.6f);
    addF ("revDecay", "Reverb Decay", 0.0f, 1.0f, 0.46f);
    addF ("revPre",   "Reverb Predelay", 0.0f, 0.2f, 0.02f);
    addF ("revMix",   "Reverb Dry/Wet", 0.0f, 1.0f, 0.0f);

    return layout;
}

SixtyAudioProcessor::SixtyAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
#define SIXTY_X(n) p.n = apvts.getRawParameterValue (#n);
    SIXTY_PARAM_LIST (SIXTY_X)
#undef SIXTY_X

    for (int i = 0; i < kMaxVoices; ++i)
        synth.addVoice (new SixtyVoice (shared, i));
    synth.addSound (new SixtySound());
    synth.setNoteStealingEnabled (true);

    lfo2Gen.seed (12345u);
}

bool SixtyAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SixtyAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    synth.setCurrentPlaybackSampleRate (sampleRate);
    monoBuf.setSize (1, samplesPerBlock);
    lfoBuf.assign ((size_t) samplesPerBlock, 0.0f);
    lfoRawBuf.assign ((size_t) samplesPerBlock, 0.0f);
    lfo2Buf.assign ((size_t) samplesPerBlock, 0.0f);
    revL.assign ((size_t) samplesPerBlock, 0.0f);
    revR.assign ((size_t) samplesPerBlock, 0.0f);

    chorus.prepare (sampleRate);
    delay.prepare (sampleRate);
    preL.prepare ((int) (sampleRate * 0.21));
    preR.prepare ((int) (sampleRate * 0.21));
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    hp.reset();
    engine.reset();

    lfoPhase = 0.0f;
    lfoAgeSamples = 1.0e9;
    heldNotes = 0;
    shared.lastNote = -1;
    trigPositions.clear();    trigPositions.reserve (1024);
    noteOnPositions.clear();  noteOnPositions.reserve (1024);
    gain = p.volume->load();
}

void SixtyAudioProcessor::updateSharedParams()
{
    auto& s = shared.params;
    s.rangeSemis = (static_cast<int> (p.range->load()) - 1) * 12;
    s.saw       = p.saw->load() > 0.5f;
    s.pulse     = p.pulse->load() > 0.5f;
    s.pwmMode   = (int) p.pwMode->load();
    s.vcaMode   = (int) p.vcaMode->load();
    s.pwm       = p.pw->load();
    s.sub       = p.sub->load();
    s.noise     = p.noise->load();
    s.tuneCents = p.tune->load();
    s.lfoDco    = p.lfoDco->load();
    s.lfoVcf    = p.lfoVcf->load();
    s.cutoff    = p.cutoff->load();
    s.res       = p.res->load();
    s.envAmt    = p.envAmt->load();
    s.keyFollow = p.keyFollow->load();
    s.attack    = p.attack->load();
    s.decay     = p.decay->load();
    s.sustain   = p.sustain->load();
    s.release   = p.release->load();
    s.env2A     = p.env2A->load();
    s.env2D     = p.env2D->load();
    s.env2S     = p.env2S->load();
    s.env2R     = p.env2R->load();
    s.velSens   = p.velSens->load();
    s.bendSemis = p.bend->load();
    s.bendVcf   = p.bendVcf->load();
    s.modDco    = p.modDco->load();
    s.modVcf    = p.modVcf->load();
    s.unison    = (int) std::lround (p.unison->load());
    s.detune    = p.detune->load();
    s.poly      = (int) std::lround (p.poly->load());
    s.glide     = p.glide->load();
    s.calib     = (int) p.calib->load();
    s.lfo2Shape  = (int) p.lfo2Wave->load();
    s.lfo2Retrig = (int) p.lfo2Retrig->load();
    s.modWheel  = modWheel;
    s.aftertouch = aftertouch;

    s.wheel = { (int) p.wheelDest->load(), p.wheelAmt->load() };
    s.velo  = { (int) p.veloDest->load(),  p.veloAmt->load() };
    s.aft   = { (int) p.aftDest->load(),   p.aftAmt->load() };
    s.lfo2a = { (int) p.lfo2Dest1->load(), p.lfo2Amt1->load() };
    s.lfo2b = { (int) p.lfo2Dest2->load(), p.lfo2Amt2->load() };
    s.env2a = { (int) p.env2Dest1->load(), p.env2Amt1->load() };
    s.env2b = { (int) p.env2Dest2->load(), p.env2Amt2->load() };
}

void SixtyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const float sr = (float) getSampleRate();
    buffer.clear();

    if (monoBuf.getNumSamples() < n) monoBuf.setSize (1, n, false, false, true);
    if ((int) lfoBuf.size() < n)
    {
        lfoBuf.resize ((size_t) n);  lfoRawBuf.resize ((size_t) n);  lfo2Buf.resize ((size_t) n);
        revL.resize ((size_t) n);    revR.resize ((size_t) n);
    }

    keyboardState.processNextMidiBuffer (midi, 0, n, true);

    // Tempo da DAW (para sincronizacoes)
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    // Hold / acordes / arpejador
    sixty::EngineParams ep;
    ep.hold      = p.hold->load() > 0.5f;
    ep.chordOn   = p.chordOn->load() > 0.5f;
    ep.chordType = (int) p.chordType->load();
    ep.arpOn     = p.arpOn->load() > 0.5f;
    ep.arpMode   = (int) p.arpMode->load();
    ep.arpRange  = 1 + (int) p.arpRange->load();
    ep.stepSamples = p.arpSync->load() > 0.5f
                       ? (double) sixty::syncSeconds ((int) p.arpDiv->load(), bpm) * (double) sr
                       : (double) sr / (double) std::max (0.1f, p.arpRate->load());
    processedMidi.clear();
    engine.process (midi, processedMidi, n, ep);

    // Eventos que interessam aos LFOs e aos controlos
    trigPositions.clear();
    noteOnPositions.clear();
    for (const auto meta : processedMidi)
    {
        const auto msg = meta.getMessage();
        if (msg.isController() && msg.getControllerNumber() == 1)
            modWheel = (float) msg.getControllerValue() / 127.0f;
        else if (msg.isChannelPressure())
            aftertouch = (float) msg.getChannelPressureValue() / 127.0f;
        else if (msg.isAftertouch())
            aftertouch = (float) msg.getAfterTouchValue() / 127.0f;
        else if (msg.isNoteOn())
        {
            if (noteOnPositions.size() < 1024) noteOnPositions.push_back (meta.samplePosition);
            if (heldNotes == 0 && trigPositions.size() < 1024) trigPositions.push_back (meta.samplePosition);
            ++heldNotes;
        }
        else if (msg.isNoteOff())
            heldNotes = juce::jmax (0, heldNotes - 1);
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
            heldNotes = 0;
    }

    updateSharedParams();

    // Deriva lenta do oscilador mestre (comum a todas as vozes, como no DCO do Juno-60)
    driftPh1 += 0.11f * (float) n / sr; if (driftPh1 >= 1.0f) driftPh1 -= 1.0f;
    driftPh2 += 0.27f * (float) n / sr; if (driftPh2 >= 1.0f) driftPh2 -= 1.0f;
    shared.params.driftCents = (std::sin (sixty::kTwoPi * driftPh1) * 0.6f + std::sin (sixty::kTwoPi * driftPh2) * 0.4f)
                               * p.drift->load() * 6.0f;

    // LFO 1 (triangular, global). A versao "raw" (sem delay) alimenta o PWM; a outra o vibrato e o filtro.
    const float lfoHz = p.lfoSync->load() > 0.5f
                          ? 1.0f / std::max (0.001f, sixty::syncSeconds ((int) p.lfoDiv->load(), bpm))
                          : p.lfoRate->load();
    const float inc = lfoHz / sr;
    const float delaySec = p.lfoDelay->load();
    const double delaySamples = (double) delaySec * sr;
    const bool riseOnly = p.lfoDelayMode->load() > 0.5f;
    const double rampSamples = riseOnly ? std::max (1.0, delaySamples)
                                        : std::max (0.02 * sr, delaySamples / 3.0);

    // LFO 2 (varias formas de onda, retrigger Off / Mono / Poly)
    const float lfo2Hz = p.lfo2Sync->load() > 0.5f
                           ? 1.0f / std::max (0.001f, sixty::syncSeconds ((int) p.lfo2Div->load(), bpm))
                           : p.lfo2Rate->load();
    shared.params.lfo2Rate = lfo2Hz;
    const float inc2 = lfo2Hz / sr;
    const int shape2 = shared.params.lfo2Shape;
    const bool mono2 = shared.params.lfo2Retrig == 1;

    size_t trigIdx = 0, noteIdx = 0;
    for (int i = 0; i < n; ++i)
    {
        while (trigIdx < trigPositions.size() && trigPositions[trigIdx] <= i) { lfoAgeSamples = 0.0; ++trigIdx; }
        while (noteIdx < noteOnPositions.size() && noteOnPositions[noteIdx] <= i)
        {
            if (mono2) lfo2Gen.reset (0.0f);
            ++noteIdx;
        }

        lfoPhase += inc;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
        const float tri = 1.0f - 4.0f * std::abs (lfoPhase - 0.5f);

        float g = 1.0f;
        if (riseOnly)
            g = delaySamples > 1.0 ? (float) juce::jlimit (0.0, 1.0, lfoAgeSamples / rampSamples) : 1.0f;
        else if (delaySamples > 1.0)
            g = (float) juce::jlimit (0.0, 1.0, (lfoAgeSamples - delaySamples) / rampSamples);
        lfoAgeSamples += 1.0;

        lfoRawBuf[(size_t) i] = tri;
        lfoBuf[(size_t) i] = tri * g;
        lfo2Buf[(size_t) i] = lfo2Gen.tick (inc2, shape2);
    }
    shared.lfo = lfoBuf.data();
    shared.lfoRaw = lfoRawBuf.data();
    shared.lfo2 = lfo2Buf.data();

    monoBuf.clear();
    synth.renderNextBlock (monoBuf, processedMidi, 0, n);

    // --- cadeia de saida: HPF -> chorus -> delay -> reverb -> volume ---
    // Do esquema: C 22 nF / 10 nF / 4,7 nF sobre ~31 kohm (470k || 33k) -> ~235 / ~520 / ~1100 Hz. Posicao 0 = directo.
    static constexpr float hpfFreq[4] = { 0.0f, 235.0f, 516.0f, 1098.0f };
    const int hpIdx = juce::jlimit (0, 3, (int) p.hpf->load());
    if (hpIdx > 0) hp.setCutoff (hpfFreq[hpIdx], getSampleRate());
    const int chorusMode = (int) p.chorus->load();
    chorus.setMode (chorusMode);
    const bool chorusHiss = p.chorusNoise->load() > 0.5f && chorusMode > 0;

    const float delaySamplesFx = p.delaySync->load() > 0.5f
                                   ? sixty::syncSeconds ((int) p.delayDiv->load(), bpm) * sr
                                   : p.delayTime->load() * 0.001f * sr;
    const float delayFb = p.delayFb->load(), delaySt = p.delayStereo->load();
    const float delayMix = p.delayOn->load() > 0.5f ? p.delayMix->load() : 0.0f;

    const float* mono = monoBuf.getReadPointer (0);
    float* l = buffer.getWritePointer (0);
    float* r = buffer.getWritePointer (1);

    for (int i = 0; i < n; ++i)
    {
        float x = mono[i];
        x = hpIdx > 0 ? hp.process (x) : (hp.reset(), x);
        float cl, cr;
        chorus.process (x, cl, cr);
        if (chorusHiss)
        {
            cl += (noiseRnd.nextFloat() * 2.0f - 1.0f) * 0.0006f;
            cr += (noiseRnd.nextFloat() * 2.0f - 1.0f) * 0.0006f;
        }
        if (delayMix > 0.0005f)
            delay.process (cl, cr, delaySamplesFx, delayFb, delaySt, delayMix);
        l[i] = cl;
        r[i] = cr;
    }

    const float revMix = p.revOn->load() > 0.5f ? p.revMix->load() : 0.0f;
    if (revMix > 0.0005f)
    {
        juce::Reverb::Parameters rp;
        rp.roomSize   = juce::jlimit (0.0f, 1.0f, 0.2f + 0.8f * (0.5f * p.revSize->load() + 0.5f * p.revDecay->load()));
        rp.damping    = p.revDamp->load();
        rp.wetLevel   = 0.33f;
        rp.dryLevel   = 0.0f;
        rp.width      = 1.0f;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);

        const int pre = (int) (p.revPre->load() * sr);
        for (int i = 0; i < n; ++i)
        {
            revL[(size_t) i] = preL.process (l[i], pre);
            revR[(size_t) i] = preR.process (r[i], pre);
        }
        reverb.processStereo (revL.data(), revR.data(), n);
        for (int i = 0; i < n; ++i)
        {
            l[i] = l[i] * (1.0f - revMix) + revL[(size_t) i] * revMix;
            r[i] = r[i] * (1.0f - revMix) + revR[(size_t) i] * revMix;
        }
    }

    const float targetGain = p.volume->load() * 1.2f;
    for (int i = 0; i < n; ++i)
    {
        gain += (targetGain - gain) * 0.002f;
        l[i] = sixty::fastTanh (l[i] * gain);
        r[i] = sixty::fastTanh (r[i] * gain);
    }
}

juce::AudioProcessorEditor* SixtyAudioProcessor::createEditor() { return new SixtyAudioProcessorEditor (*this); }

void SixtyAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void SixtyAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new SixtyAudioProcessor(); }
