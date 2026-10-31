#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

// Minimal dark look; every pedal category gets its own muted accent colour.
class NoirLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Colour bg     { 0xff0c0c0e };
    static inline const juce::Colour panel  { 0xff141417 };
    static inline const juce::Colour line   { 0xff26262b };
    static inline const juce::Colour text   { 0xffe8e6e1 };
    static inline const juce::Colour dim    { 0xff75757d };
    static inline const juce::Colour accent { 0xffe8a33d };

    NoirLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (12.5f); }
    juce::Font getLabelFont (juce::Label&) override { return juce::FontOptions (11.5f); }
};

// One effect "pedal": LED on/off, optional type selector and a row of knobs.
class Pedal : public juce::Component
{
public:
    Pedal (juce::AudioProcessorValueTreeState& apvts, const juce::String& title, juce::Colour accent,
           int onId, int typeId, std::initializer_list<Id> knobIds, const juce::String& subtitle = {});

    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

    int desiredWidth() const;
    std::function<void (juce::Rectangle<int>)> onLayoutBody;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attach;
    };

    juce::AudioProcessorValueTreeState& state;
    juce::String title, subtitle;
    juce::Colour accent;
    int onId, typeId;
    bool isOn = true;

    juce::ToggleButton led;
    juce::ComboBox type;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ledAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pedal)
};

// Signal-flow strip: IN > Gate > Comp > ... > OUT. Click a block to bypass it.
class ChainStrip : public juce::Component
{
public:
    struct Item { juce::String name; int onId; juce::Colour colour; };

    ChainStrip (NoirProcessor& p, std::vector<Item> items);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    juce::Rectangle<float> boxFor (int i) const;
    int indexAt (juce::Point<int>) const;

    NoirProcessor& proc;
    std::vector<Item> items;
    int hover = -1;
};

class NoirEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoirEditor (NoirProcessor&);
    ~NoirEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void chooseIR();
    void setAllEffects (bool on);
    void layoutRow (juce::Rectangle<int> row, std::vector<Pedal*> pedals);

    NoirProcessor& proc;
    NoirLookAndFeel laf;

    std::unique_ptr<ChainStrip> strip;
    std::vector<std::unique_ptr<Pedal>> pedals;
    Pedal *pGate, *pComp, *pWah, *pPitch, *pBoost, *pDrive, *pAmp, *pCab, *pEq, *pMod1, *pMod2, *pLofi, *pDelay, *pReverb, *pOut;

    juce::ComboBox presetBox;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, allOn { "ALL ON" }, allOff { "ALL OFF" };
    juce::ToggleButton power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAtt;

    juce::TextButton irButton { "Load IR" }, irClear { "x" };
    std::unique_ptr<juce::FileChooser> chooser;

    float inLevel = 0.0f, outLevel = 0.0f;
    juce::Rectangle<int> headerArea, meterIn, meterOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirEditor)
};
