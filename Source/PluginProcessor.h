#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Dsp.h"
#include "NoteEngine.h"
#include "SynthShared.h"

// Lista de parametros: o nome do membro e igual ao ID do parametro.
#define SIXTY_PARAM_LIST(X) \
    X(range) X(saw) X(pulse) X(pwMode) X(pw) X(sub) X(noise) X(tune) \
    X(lfoRate) X(lfoDelay) X(lfoDco) X(lfoVcf) X(lfoSync) X(lfoDiv) X(lfoDelayMode) \
    X(hpf) X(chorus) X(chorusNoise) \
    X(cutoff) X(res) X(envAmt) X(keyFollow) \
    X(attack) X(decay) X(sustain) X(release) X(vcaMode) \
    X(volume) X(velSens) X(drift) X(bend) X(bendVcf) X(modDco) X(modVcf) \
    X(unison) X(detune) X(poly) X(glide) X(calib) \
    X(hold) X(chordOn) X(chordType) X(arpOn) X(arpMode) X(arpRange) X(arpRate) X(arpSync) X(arpDiv) \
    X(wheelDest) X(wheelAmt) X(veloDest) X(veloAmt) X(aftDest) X(aftAmt) \
    X(lfo2Sync) X(lfo2Div) X(lfo2Rate) X(lfo2Wave) X(lfo2Retrig) X(lfo2Dest1) X(lfo2Amt1) X(lfo2Dest2) X(lfo2Amt2) \
    X(env2A) X(env2D) X(env2S) X(env2R) X(env2Dest1) X(env2Amt1) X(env2Dest2) X(env2Amt2) \
    X(delayOn) X(delayTime) X(delaySync) X(delayDiv) X(delayFb) X(delayStereo) X(delayMix) \
    X(revOn) X(revSize) X(revDamp) X(revDecay) X(revPre) X(revMix)

class SixtyAudioProcessor : public juce::AudioProcessor
{
public:
    SixtyAudioProcessor();
    ~SixtyAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Sixty"; }
    bool acceptsMidi() const override  { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState; // teclado no ecra (util no Standalone)

private:
    void updateSharedParams();

    struct Ptrs
    {
#define SIXTY_X(n) std::atomic<float>* n = nullptr;
        SIXTY_PARAM_LIST (SIXTY_X)
#undef SIXTY_X
    } p;

    SharedState shared;
    juce::Synthesiser synth;
    juce::AudioBuffer<float> monoBuf;
    std::vector<float> lfoBuf, lfoRawBuf, lfo2Buf, revL, revR;
    float lfoPhase = 0.0f, modWheel = 0.0f, aftertouch = 0.0f, gain = 0.0f;
    float driftPh1 = 0.0f, driftPh2 = 0.37f;
    double lfoAgeSamples = 1.0e9; // tempo desde o ultimo disparo do LFO 1 (delay global)
    int heldNotes = 0;
    std::vector<int> trigPositions, noteOnPositions;

    sixty::LfoGen lfo2Gen;
    sixty::NoteEngine engine;
    juce::MidiBuffer processedMidi;

    sixty::OnePoleHP hp;
    sixty::JunoChorus chorus;
    sixty::StereoDelay delay;
    sixty::SimpleDelay preL, preR;
    juce::Reverb reverb;
    juce::Random noiseRnd;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SixtyAudioProcessor)
};
