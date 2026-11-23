#pragma once
#include "PluginProcessor.h"
#include <functional>

using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
class SixtyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SixtyLookAndFeel()
    {
        setColourScheme (juce::LookAndFeel_V4::getMidnightColourScheme());
        setColour (juce::TextButton::buttonColourId, juce::Colour (0xff3b3b41));
        setColour (juce::TextButton::textColourOffId, juce::Colour (0xffd0d0d6));
        setColour (juce::TextButton::textColourOnId, juce::Colours::black);
        setColour (juce::Slider::thumbColourId, juce::Colour (0xffe8743c));
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff202024));
        setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff4a4a52));
    }

    int getSliderThumbRadius (juce::Slider&) override { return 9; }

    juce::Font getTextButtonFont (juce::TextButton&, int) override
    {
        return juce::Font (juce::FontOptions (10.0f, juce::Font::bold));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (10.5f));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&,
                               bool highlighted, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = b.getToggleState();
        auto base = on ? b.findColour (juce::TextButton::buttonOnColourId)
                       : b.findColour (juce::TextButton::buttonColourId);
        if (highlighted && ! on) base = base.brighter (0.15f);
        if (down) base = base.darker (0.2f);

        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillRoundedRectangle (r.translated (0.0f, 1.5f), 4.0f);
        g.setColour (base);
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.35f : 0.10f));
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float sliderPos, float minPos, float maxPos,
                           juce::Slider::SliderStyle style, juce::Slider& s) override
    {
        if (style != juce::Slider::LinearVertical)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, w, h, sliderPos, minPos, maxPos, style, s);
            return;
        }

        const float cx = (float) x + (float) w * 0.5f;
        const auto accent = s.findColour (juce::Slider::thumbColourId);

        g.setColour (juce::Colour (0xff0e0e10));
        g.fillRoundedRectangle (cx - 3.0f, (float) y + 6.0f, 6.0f, (float) h - 12.0f, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawRoundedRectangle (cx - 3.0f, (float) y + 6.0f, 6.0f, (float) h - 12.0f, 3.0f, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (0.28f));
        for (int i = 0; i <= 10; ++i)
        {
            const float ty = (float) y + 9.0f + ((float) h - 18.0f) * (float) i / 10.0f;
            g.fillRect (cx - 18.0f, ty, 4.0f, 1.0f);
            g.fillRect (cx + 14.0f, ty, 4.0f, 1.0f);
        }

        juce::Rectangle<float> cap (cx - 11.0f, sliderPos - 9.0f, 22.0f, 18.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (cap.translated (0.0f, 2.0f), 3.0f);
        g.setColour (juce::Colour (0xffe9e5d6));
        g.fillRoundedRectangle (cap, 3.0f);
        g.setColour (accent);
        g.fillRect (cap.getX() + 3.0f, cap.getCentreY() - 1.5f, cap.getWidth() - 6.0f, 3.0f);
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.drawRoundedRectangle (cap, 3.0f, 1.0f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
        const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
        const auto c = b.getCentre();
        const auto accent = s.findColour (juce::Slider::thumbColourId);

        g.setColour (juce::Colours::white.withAlpha (0.30f));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            const juce::Point<float> dir (std::sin (a), -std::cos (a));
            g.drawLine (juce::Line<float> (c + dir * (r * 0.84f), c + dir * (r * 0.98f)), 1.0f);
        }

        const float kr = r * 0.72f;
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - kr, c.y - kr + 2.0f, kr * 2.0f, kr * 2.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff55555c), c.x, c.y - kr,
                                                 juce::Colour (0xff1c1c1f), c.x, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

        const float a = startAngle + pos * (endAngle - startAngle);
        const juce::Point<float> dir (std::sin (a), -std::cos (a));
        g.setColour (accent);
        g.drawLine (juce::Line<float> (c + dir * (kr * 0.25f), c + dir * (kr * 0.92f)), 2.5f);
    }
};

//==============================================================================
// Controlos autonomos: cada um traz o seu attachment (destruido antes do componente base).
class FaderCtl : public juce::Slider
{
public:
    FaderCtl (APVTS& apvts, const juce::String& id, juce::Colour accent, juce::Component* popupParent)
    {
        setSliderStyle (juce::Slider::LinearVertical);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setPopupDisplayEnabled (true, true, popupParent);
        setColour (juce::Slider::thumbColourId, accent);
        att = std::make_unique<APVTS::SliderAttachment> (apvts, id, *this);
    }
private:
    std::unique_ptr<APVTS::SliderAttachment> att;
};

class KnobCtl : public juce::Slider
{
public:
    KnobCtl (APVTS& apvts, const juce::String& id, juce::Colour accent, juce::Component* popupParent)
    {
        setSliderStyle (juce::Slider::RotaryVerticalDrag);
        setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        setPopupDisplayEnabled (true, true, popupParent);
        setColour (juce::Slider::thumbColourId, accent);
        att = std::make_unique<APVTS::SliderAttachment> (apvts, id, *this);
    }
private:
    std::unique_ptr<APVTS::SliderAttachment> att;
};

class ComboCtl : public juce::ComboBox
{
public:
    ComboCtl (APVTS& apvts, const juce::String& id)
    {
        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
            addItemList (choice->choices, 1);
        att = std::make_unique<APVTS::ComboBoxAttachment> (apvts, id, *this);
    }
private:
    std::unique_ptr<APVTS::ComboBoxAttachment> att;
};

class ToggleCtl : public juce::TextButton
{
public:
    ToggleCtl (APVTS& apvts, const juce::String& id, const juce::String& text, juce::Colour accent)
        : juce::TextButton (text)
    {
        setClickingTogglesState (true);
        setColour (juce::TextButton::buttonOnColourId, accent);
        att = std::make_unique<APVTS::ButtonAttachment> (apvts, id, *this);
    }
private:
    std::unique_ptr<APVTS::ButtonAttachment> att;
};

// Botoes de escolha exclusiva ligados a um AudioParameterChoice.
class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons (APVTS& apvts, const juce::String& id, juce::Colour accent);
    void resized() override;
    int getPreferredHeight() const { return buttons.size() * (rowH + gap) - gap; }

private:
    void setSelected (int index);
    static constexpr int rowH = 26, gap = 4;
    juce::OwnedArray<juce::TextButton> buttons;
    juce::ParameterAttachment attachment; // declarado depois dos botoes: destruido primeiro
};

// Coluna vertical de varios controlos.
class ControlColumn : public juce::Component
{
public:
    void add (juce::Component* c, int height)
    {
        parts.add (c);
        heights.push_back (height);
        addAndMakeVisible (c);
    }

    int totalHeight() const
    {
        int t = 0;
        for (int h : heights) t += h + gap;
        return std::max (0, t - gap);
    }

    void resized() override
    {
        int y = 0;
        for (int i = 0; i < parts.size(); ++i)
        {
            parts[i]->setBounds (0, y, getWidth(), heights[(size_t) i]);
            y += heights[(size_t) i] + gap;
        }
    }

private:
    static constexpr int gap = 4;
    juce::OwnedArray<juce::Component> parts;
    std::vector<int> heights;
};

//==============================================================================
// Todo o painel, desenhado num espaco fixo (kBaseW x baseHeight) e escalado pelo editor.
class SixtyContent : public juce::Component
{
public:
    static constexpr int kBaseW = 1600;
    static int baseHeight (bool advanced) { return 56 + 250 + 190 + (advanced ? 214 : 0); }

    SixtyContent (SixtyAudioProcessor&, std::function<void()> onAdvancedChanged);

    bool isAdvanced() const { return advanced; }
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    enum class Kind { Fader, Knob, Column };
    struct Item { juce::Component* comp; juce::Label* label; Kind kind; int height; float weight; };
    struct Section { juce::String name; juce::Colour colour; std::vector<Item> items; juce::Rectangle<int> bounds; int gridCols = 0; };

    juce::Label* makeLabel (const juce::String& text);
    void addFader (Section&, const juce::String& id, const juce::String& text, float weight = 1.0f);
    void addKnob  (Section&, const juce::String& id, const juce::String& text, float weight = 1.0f);
    ControlColumn* newColumn();
    void finishColumn (Section&, ControlColumn*, const juce::String& text, float weight);

    void layoutRow (std::vector<Section>&, juce::Rectangle<int>);
    void layoutSection (Section&, float unit);
    void drawSection (juce::Graphics&, const Section&) const;
    static void drawWood (juce::Graphics&, juce::Rectangle<int>);

    SixtyAudioProcessor& proc;
    std::function<void()> advancedCb;
    bool advanced = false;
    juce::TextButton advancedBtn { "ADVANCED" };

    juce::OwnedArray<FaderCtl> faders;
    juce::OwnedArray<KnobCtl> knobs;
    juce::OwnedArray<ControlColumn> columns;
    juce::OwnedArray<juce::Label> labels;
    std::vector<Section> mainSecs, advSecs;
    Section perf;
    juce::MidiKeyboardComponent keyboard;

    juce::Rectangle<int> panelBounds, titleBounds, lowerBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SixtyContent)
};

//==============================================================================
class SixtyAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SixtyAudioProcessorEditor (SixtyAudioProcessor&);
    ~SixtyAudioProcessorEditor() override;

    void resized() override;

private:
    int heightForWidth (int w) const;
    void applyLimits();

    SixtyLookAndFeel laf;
    SixtyContent content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SixtyAudioProcessorEditor)
};
