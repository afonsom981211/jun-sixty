#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour kAmber   (0xffe7a83c);
const juce::Colour kOrange  (0xffe8743c);
const juce::Colour kCoral   (0xffd9564a);
const juce::Colour kCrimson (0xffc2384e);
const juce::Colour kTeal    (0xff2fa39a);
const juce::Colour kBlue    (0xff3f8fd0);
const juce::Colour kSlate   (0xff8a9bb4);
}

//==============================================================================
ChoiceButtons::ChoiceButtons (APVTS& apvts, const juce::String& id, juce::Colour accent)
    : attachment (*apvts.getParameter (id), [this] (float v) { setSelected ((int) std::lround (v)); })
{
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
    {
        for (int i = 0; i < choice->choices.size(); ++i)
        {
            auto* b = buttons.add (new juce::TextButton (choice->choices[i]));
            b->setColour (juce::TextButton::buttonOnColourId, accent);
            b->onClick = [this, i] { attachment.setValueAsCompleteGesture ((float) i); };
            addAndMakeVisible (b);
        }
    }
    attachment.sendInitialUpdate();
}

void ChoiceButtons::setSelected (int index)
{
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setToggleState (i == index, juce::dontSendNotification);
}

void ChoiceButtons::resized()
{
    int y = 0;
    for (auto* b : buttons)
    {
        b->setBounds (0, y, getWidth(), rowH);
        y += rowH + gap;
    }
}

//==============================================================================
juce::Label* SixtyContent::makeLabel (const juce::String& text)
{
    auto* l = labels.add (new juce::Label ({}, text));
    l->setJustificationType (juce::Justification::centred);
    l->setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    l->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.82f));
    l->setInterceptsMouseClicks (false, false);
    addAndMakeVisible (l);
    return l;
}

void SixtyContent::addFader (Section& sec, const juce::String& id, const juce::String& text, float weight)
{
    auto* f = faders.add (new FaderCtl (proc.apvts, id, sec.colour, this));
    addAndMakeVisible (f);
    sec.items.push_back ({ f, makeLabel (text), Kind::Fader, 0, weight });
}

void SixtyContent::addKnob (Section& sec, const juce::String& id, const juce::String& text, float weight)
{
    auto* k = knobs.add (new KnobCtl (proc.apvts, id, sec.colour, this));
    addAndMakeVisible (k);
    sec.items.push_back ({ k, makeLabel (text), Kind::Knob, 0, weight });
}

ControlColumn* SixtyContent::newColumn()
{
    auto* c = columns.add (new ControlColumn());
    addAndMakeVisible (c);
    return c;
}

void SixtyContent::finishColumn (Section& sec, ControlColumn* c, const juce::String& text, float weight)
{
    sec.items.push_back ({ c, makeLabel (text), Kind::Column, c->totalHeight(), weight });
}

//==============================================================================
SixtyContent::SixtyContent (SixtyAudioProcessor& p, std::function<void()> cb)
    : proc (p), advancedCb (std::move (cb)),
      keyboard (p.keyboardState, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    auto& A = proc.apvts;

    mainSecs.resize (10);
    advSecs.resize (5);
    auto init = [] (Section& s, const char* name, juce::Colour c) { s.name = name; s.colour = c; };

    auto& hc  = mainSecs[0]; init (hc,  "HOLD / CHORD", kSlate);
    auto& arp = mainSecs[1]; init (arp, "ARPEGGIO",     kBlue);
    auto& lfo = mainSecs[2]; init (lfo, "LFO",          kAmber);
    auto& dco = mainSecs[3]; init (dco, "DCO",          kOrange);
    auto& hpf = mainSecs[4]; init (hpf, "HPF",          kCoral);
    auto& vcf = mainSecs[5]; init (vcf, "VCF",          kCrimson);
    auto& vca = mainSecs[6]; init (vca, "VCA",          kTeal);
    auto& env = mainSecs[7]; init (env, "ENV",          kTeal);
    auto& cho = mainSecs[8]; init (cho, "CHORUS",       kBlue);
    auto& mst = mainSecs[9]; init (mst, "MASTER",       kSlate);

    auto& asg = advSecs[0];  init (asg, "ASSIGN", kSlate);
    auto& lf2 = advSecs[1];  init (lf2, "LFO 2",  kAmber);
    auto& en2 = advSecs[2];  init (en2, "ENV 2",  kTeal);
    auto& dly = advSecs[3];  init (dly, "DELAY",  kBlue);
    auto& rev = advSecs[4];  init (rev, "REVERB", kBlue);
    init (perf, "PERFORMANCE", kSlate);

    // ---- HOLD / CHORD ----
    {
        auto* c = newColumn();
        c->add (new ToggleCtl (A, "hold", "HOLD", kOrange), 26);
        c->add (new ToggleCtl (A, "chordOn", "CHORD", kOrange), 26);
        finishColumn (hc, c, "KEYS", 1.0f);
    }
    {
        auto* c = newColumn();
        c->add (new ComboCtl (A, "chordType"), 24);
        finishColumn (hc, c, "TYPE", 1.8f);
    }

    // ---- ARPEGGIO ----
    {
        auto* c = newColumn();
        c->add (new ToggleCtl (A, "arpOn", "ON", kBlue), 26);
        finishColumn (arp, c, "ARP", 1.0f);
    }
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "arpMode", kBlue);
        c->add (b, b->getPreferredHeight());
        finishColumn (arp, c, "MODE", 1.0f);
    }
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "arpRange", kBlue);
        c->add (b, b->getPreferredHeight());
        finishColumn (arp, c, "OCT", 1.0f);
    }
    addFader (arp, "arpRate", "RATE");
    {
        auto* c = newColumn();
        c->add (new ToggleCtl (A, "arpSync", "SYNC", kBlue), 26);
        c->add (new ComboCtl (A, "arpDiv"), 24);
        finishColumn (arp, c, "SYNC", 1.6f);
    }

    // ---- LFO ----
    {
        auto* c = newColumn();
        c->add (new ToggleCtl (A, "lfoSync", "SYNC", kAmber), 26);
        c->add (new ComboCtl (A, "lfoDiv"), 24);
        finishColumn (lfo, c, "SYNC", 1.6f);
    }
    addFader (lfo, "lfoRate",  "RATE");
    addFader (lfo, "lfoDelay", "DELAY");
    {
        auto* c = newColumn();
        c->add (new ComboCtl (A, "lfoDelayMode"), 24);
        finishColumn (lfo, c, "MODE", 1.2f);
    }

    // ---- DCO ----
    addFader (dco, "lfoDco", "LFO");
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "range", kOrange);
        c->add (b, b->getPreferredHeight());
        finishColumn (dco, c, "RANGE", 1.0f);
    }
    {
        auto* c = newColumn();
        c->add (new ToggleCtl (A, "saw", "SAW", kOrange), 26);
        c->add (new ToggleCtl (A, "pulse", "PULSE", kOrange), 26);
        finishColumn (dco, c, "WAVE", 1.0f);
    }
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "pwMode", kOrange);
        c->add (b, b->getPreferredHeight());
        finishColumn (dco, c, "PWM", 1.0f);
    }
    addFader (dco, "pw",    "WIDTH");
    addFader (dco, "sub",   "SUB");
    addFader (dco, "noise", "NOISE");

    // ---- HPF / VCF / VCA / ENV ----
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "hpf", kCoral);
        c->add (b, b->getPreferredHeight());
        finishColumn (hpf, c, "FREQ", 1.0f);
    }
    addFader (vcf, "cutoff",    "FREQ");
    addFader (vcf, "res",       "RES");
    addFader (vcf, "envAmt",    "ENV");
    addFader (vcf, "lfoVcf",    "LFO");
    addFader (vcf, "keyFollow", "KYBD");
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "vcaMode", kTeal);
        c->add (b, b->getPreferredHeight());
        finishColumn (vca, c, "MODE", 1.0f);
    }
    addFader (env, "attack",  "A");
    addFader (env, "decay",   "D");
    addFader (env, "sustain", "S");
    addFader (env, "release", "R");

    // ---- CHORUS / MASTER ----
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "chorus", kBlue);
        c->add (b, b->getPreferredHeight());
        c->add (new ToggleCtl (A, "chorusNoise", "NOISE", kBlue), 26);
        finishColumn (cho, c, "MODE", 1.2f);
    }
    addKnob (mst, "tune",   "TUNE");
    addKnob (mst, "volume", "VOLUME");
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "calib", kSlate);
        c->add (b, b->getPreferredHeight());
        finishColumn (mst, c, "VOICES", 1.2f);
    }

    // ---- ASSIGN (matriz de modulacao) ----
    auto destColumn = [&] (Section& sec, const char* destId, const char* amtId, const char* text, float w)
    {
        auto* c = newColumn();
        c->add (new ComboCtl (A, destId), 24);
        c->add (new KnobCtl (A, amtId, sec.colour, this), 48);
        finishColumn (sec, c, text, w);
    };
    destColumn (asg, "wheelDest", "wheelAmt", "WHEEL", 1.8f);
    destColumn (asg, "veloDest",  "veloAmt",  "VELO",  1.8f);
    destColumn (asg, "aftDest",   "aftAmt",   "AFTER", 1.8f);

    // ---- LFO 2 ----
    {
        auto* c = newColumn();
        c->add (new ToggleCtl (A, "lfo2Sync", "SYNC", kAmber), 26);
        c->add (new ComboCtl (A, "lfo2Div"), 24);
        finishColumn (lf2, c, "SYNC", 1.6f);
    }
    addFader (lf2, "lfo2Rate", "RATE");
    {
        auto* c = newColumn();
        c->add (new ComboCtl (A, "lfo2Wave"), 24);
        finishColumn (lf2, c, "WAVE", 1.5f);
    }
    {
        auto* c = newColumn(); auto* b = new ChoiceButtons (A, "lfo2Retrig", kAmber);
        c->add (b, b->getPreferredHeight());
        finishColumn (lf2, c, "RETRIG", 1.2f);
    }
    destColumn (lf2, "lfo2Dest1", "lfo2Amt1", "DEST 1", 1.8f);
    destColumn (lf2, "lfo2Dest2", "lfo2Amt2", "DEST 2", 1.8f);

    // ---- ENV 2 ----
    addFader (en2, "env2A", "A");
    addFader (en2, "env2D", "D");
    addFader (en2, "env2S", "S");
    addFader (en2, "env2R", "R");
    destColumn (en2, "env2Dest1", "env2Amt1", "DEST 1", 1.8f);
    destColumn (en2, "env2Dest2", "env2Amt2", "DEST 2", 1.8f);

    // ---- DELAY / REVERB ----
    {
        auto* c = newColumn();
        c->add (new ToggleCtl (A, "delayOn", "ON", kBlue), 26);
        c->add (new ToggleCtl (A, "delaySync", "SYNC", kBlue), 26);
        finishColumn (dly, c, "ON", 1.0f);
    }
    addFader (dly, "delayTime", "TIME");
    {
        auto* c = newColumn();
        c->add (new ComboCtl (A, "delayDiv"), 24);
        finishColumn (dly, c, "DIV", 1.5f);
    }
    addFader (dly, "delayFb",     "FDBK");
    addFader (dly, "delayStereo", "STEREO");
    addFader (dly, "delayMix",    "DRY/WET");

    {
        auto* c = newColumn();
        c->add (new ToggleCtl (A, "revOn", "ON", kBlue), 26);
        finishColumn (rev, c, "ON", 1.0f);
    }
    addFader (rev, "revSize",  "SIZE");
    addFader (rev, "revDamp",  "DAMP");
    addFader (rev, "revDecay", "DECAY");
    addFader (rev, "revPre",   "PRE");
    addFader (rev, "revMix",   "DRY/WET");

    // ---- PERFORMANCE (grelha 5 x 2) ----
    perf.gridCols = 5;
    addKnob (perf, "bend",    "BEND");
    addKnob (perf, "bendVcf", "BND>VCF");
    addKnob (perf, "modDco",  "MOD>DCO");
    addKnob (perf, "modVcf",  "MOD>VCF");
    addKnob (perf, "glide",   "PORTA");
    addKnob (perf, "unison",  "UNISON");
    addKnob (perf, "detune",  "DETUNE");
    addKnob (perf, "poly",    "POLY");
    addKnob (perf, "drift",   "DRIFT");
    addKnob (perf, "velSens", "VELO");

    // ---- teclado e botao Advanced ----
    addAndMakeVisible (keyboard);
    keyboard.setAvailableRange (36, 96); // 61 teclas
    keyboard.setLowestVisibleKey (36);
    keyboard.setScrollButtonsVisible (false);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, juce::Colour (0xfff2eee2));
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, juce::Colour (0xff17171a));
    keyboard.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, juce::Colour (0xff9a968a));
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, kOrange.withAlpha (0.6f));
    keyboard.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, kOrange.withAlpha (0.25f));

    addAndMakeVisible (advancedBtn);
    advancedBtn.setClickingTogglesState (true);
    advancedBtn.setColour (juce::TextButton::buttonOnColourId, kOrange);
    advancedBtn.onClick = [this]
    {
        advanced = advancedBtn.getToggleState();
        if (advancedCb) advancedCb();
    };
}

//==============================================================================
void SixtyContent::drawWood (juce::Graphics& g, juce::Rectangle<int> r)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff5a3422), (float) r.getX(), 0.0f,
                                             juce::Colour (0xff33190f), (float) r.getRight(), 0.0f, false));
    g.fillRect (r);

    juce::Random rnd (r.getX() + 11);
    for (int i = 0; i < 26; ++i)
    {
        const float x = (float) r.getX() + rnd.nextFloat() * (float) r.getWidth();
        g.setColour (juce::Colours::black.withAlpha (0.06f + rnd.nextFloat() * 0.12f));
        g.drawLine (x, (float) r.getY(), x + (rnd.nextFloat() - 0.5f) * 6.0f, (float) r.getBottom(), 1.0f);
    }
}

void SixtyContent::drawSection (juce::Graphics& g, const Section& s) const
{
    auto b = s.bounds.toFloat();
    g.setColour (juce::Colour (0xff323237));
    g.fillRoundedRectangle (b, 4.0f);

    auto head = b.removeFromTop (20.0f);
    g.setColour (s.colour);
    g.fillRoundedRectangle (head, 4.0f);
    g.fillRect (head.withTrimmedTop (10.0f));

    g.setColour (s.colour.getPerceivedBrightness() > 0.62f ? juce::Colours::black : juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (11.5f, juce::Font::bold)));
    g.drawText (s.name, head, juce::Justification::centred);
}

void SixtyContent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0f0f11));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2c30), 0.0f, (float) panelBounds.getY(),
                                             juce::Colour (0xff1c1c1f), 0.0f, (float) panelBounds.getBottom(), false));
    g.fillRect (panelBounds);

    drawWood (g, { 0, 0, panelBounds.getX(), getHeight() });
    drawWood (g, { panelBounds.getRight(), 0, getWidth() - panelBounds.getRight(), getHeight() });

    // faixa de ventilacao + titulo
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    for (int i = 0; i < 64; ++i)
        g.fillRoundedRectangle ((float) (titleBounds.getX() + 22 + i * 6), (float) (titleBounds.getY() + 10), 2.0f, 14.0f, 1.0f);

    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
    g.drawText ("6-VOICE POLYPHONIC SYNTHESIZER",
                titleBounds.getX() + 22, titleBounds.getY() + 28, 400, 18, juce::Justification::centredLeft);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (44.0f, juce::Font::bold | juce::Font::italic)));
    g.drawText ("SIXTY", titleBounds.reduced (24, 0), juce::Justification::centredRight);

    // zona inferior
    g.setColour (juce::Colour (0xff141416));
    g.fillRoundedRectangle (lowerBounds.reduced (6, 4).toFloat(), 6.0f);

    for (const auto& s : mainSecs) drawSection (g, s);
    if (advanced)
        for (const auto& s : advSecs) drawSection (g, s);
    drawSection (g, perf);
}

//==============================================================================
void SixtyContent::layoutSection (Section& s, float unit)
{
    auto body = s.bounds.withTrimmedTop (22).reduced (0, 4);

    if (s.gridCols > 0)
    {
        const int cols = s.gridCols;
        const int rows = ((int) s.items.size() + cols - 1) / cols;
        const int cw = body.getWidth() / cols, ch = body.getHeight() / rows;

        for (size_t i = 0; i < s.items.size(); ++i)
        {
            auto& it = s.items[i];
            juce::Rectangle<int> cell (body.getX() + (int) (i % (size_t) cols) * cw,
                                       body.getY() + (int) (i / (size_t) cols) * ch, cw, ch);
            it.label->setBounds (cell.removeFromTop (14));
            const int size = std::min (cell.getWidth() - 6, cell.getHeight() - 2);
            it.comp->setBounds (cell.getCentreX() - size / 2, cell.getY() + 1, size, size);
        }
        return;
    }

    for (auto& it : s.items)
    {
        auto col = body.removeFromLeft (juce::roundToInt (unit * it.weight));
        it.label->setBounds (col.removeFromTop (16));

        switch (it.kind)
        {
            case Kind::Fader:
                it.comp->setBounds (col.reduced (2, 2));
                break;
            case Kind::Knob:
            {
                const int size = std::min (col.getWidth() - 8, 54);
                it.comp->setBounds (col.getCentreX() - size / 2,
                                    col.getY() + std::max (4, (col.getHeight() - size) / 2), size, size);
                break;
            }
            case Kind::Column:
                it.comp->setBounds (col.getX() + 2, col.getY() + 2, col.getWidth() - 4, it.height);
                break;
        }
    }
}

void SixtyContent::layoutRow (std::vector<Section>& secs, juce::Rectangle<int> area)
{
    const int gap = 5;
    float total = 0.0f;
    for (auto& s : secs)
        for (auto& it : s.items)
            total += it.weight;

    const float unit = ((float) area.getWidth() - (float) gap * (float) (secs.size() - 1)) / total;

    for (auto& s : secs)
    {
        float w = 0.0f;
        for (auto& it : s.items) w += it.weight;

        s.bounds = area.removeFromLeft (juce::roundToInt (unit * w));
        area.removeFromLeft (gap);
        layoutSection (s, unit);
    }
}

void SixtyContent::resized()
{
    auto r = getLocalBounds();
    const int cheek = 34;
    r.removeFromLeft (cheek);
    r.removeFromRight (cheek);
    panelBounds = r;
    titleBounds = r.removeFromTop (56);
    lowerBounds = r.removeFromBottom (190);

    juce::Rectangle<int> advArea;
    if (advanced)
        advArea = r.removeFromTop (214).reduced (10, 4);

    layoutRow (mainSecs, r.reduced (10, 4));
    if (advanced)
        layoutRow (advSecs, advArea);

    for (auto& s : advSecs)
        for (auto& it : s.items)
        {
            it.comp->setVisible (advanced);
            it.label->setVisible (advanced);
        }

    auto lower = lowerBounds.reduced (12, 10);
    perf.bounds = lower.removeFromLeft (340);
    layoutSection (perf, 0.0f);

    lower.removeFromLeft (10);
    keyboard.setKeyWidth ((float) lower.getWidth() / 36.0f);
    keyboard.setBounds (lower);

    advancedBtn.setBounds (titleBounds.getRight() - 24 - 300 - 120, titleBounds.getY() + 14, 110, 28);
}

//==============================================================================
SixtyAudioProcessorEditor::SixtyAudioProcessorEditor (SixtyAudioProcessor& p)
    : AudioProcessorEditor (&p),
      content (p, [this]
               {
                   applyLimits();
                   setSize (getWidth(), heightForWidth (getWidth()));
               })
{
    setLookAndFeel (&laf);
    addAndMakeVisible (content);

    setResizable (true, true);
    applyLimits();
    setSize (1280, heightForWidth (1280));
}

SixtyAudioProcessorEditor::~SixtyAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

int SixtyAudioProcessorEditor::heightForWidth (int w) const
{
    return juce::roundToInt ((float) w * (float) SixtyContent::baseHeight (content.isAdvanced())
                             / (float) SixtyContent::kBaseW);
}

void SixtyAudioProcessorEditor::applyLimits()
{
    setResizeLimits (800, heightForWidth (800), 2000, heightForWidth (2000));
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) SixtyContent::kBaseW / (double) SixtyContent::baseHeight (content.isAdvanced()));
}

void SixtyAudioProcessorEditor::resized()
{
    const float scale = (float) getWidth() / (float) SixtyContent::kBaseW;
    content.setTransform (juce::AffineTransform::scale (scale));
    content.setBounds (0, 0, SixtyContent::kBaseW, SixtyContent::baseHeight (content.isAdvanced()));
}
