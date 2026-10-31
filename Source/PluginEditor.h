#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

// Minimal monochrome look: thin arcs, one warm accent, no skeuomorphism.
class NoirLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Colour bg     { 0xff0e0e10 };
    static inline const juce::Colour panel  { 0xff151518 };
    static inline const juce::Colour line   { 0xff2a2a2f };
    static inline const juce::Colour text   { 0xffe8e6e1 };
    static inline const juce::Colour dim    { 0xff7d7d85 };
    static inline const juce::Colour accent { 0xffe8a33d };

    NoirLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (13.0f); }
    juce::Font getLabelFont (juce::Label&) override { return juce::FontOptions (12.0f); }
};

class NoirEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoirEditor (NoirProcessor&);
    ~NoirEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    struct Section
    {
        juce::String title;
        std::vector<Knob*> knobs;
        juce::Rectangle<int> area;
    };

    Knob& addKnob (const char* paramId, const juce::String& name, Section& s);
    void timerCallback() override;
    void chooseIR();

    NoirProcessor& proc;
    NoirLookAndFeel laf;

    std::vector<std::unique_ptr<Knob>> knobs;
    Section sInput, sDrive, sAmp, sCab, sDelay, sReverb, sOut;

    juce::ToggleButton driveOn, cabOn;
    juce::ComboBox cabType;
    juce::TextButton irButton { "Load IR" }, irClear { "x" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> driveOnAtt, cabOnAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cabTypeAtt;

    std::unique_ptr<juce::FileChooser> chooser;
    float inLevel = 0.0f, outLevel = 0.0f;
    juce::Rectangle<int> meterIn, meterOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirEditor)
};
