#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include <memory>
#include <vector>
#include "PluginProcessor.h"
#include "DawPanel.h"

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
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return juce::FontOptions (11.0f, juce::Font::bold); }
};

// Note / cents display for the tuner pedal.
class TunerView : public juce::Component
{
public:
    void setReading (float freqHz, float refA4);
    void paint (juce::Graphics&) override;

private:
    juce::String note = "--";
    float cents = 0.0f, hz = 0.0f;
    bool valid = false;
};

// Thin progress bar for the looper position.
class LoopBar : public juce::Component
{
public:
    void set (float p, bool has) { if (p != progress || has != hasData) { progress = p; hasData = has; repaint(); } }
    void paint (juce::Graphics&) override;

private:
    float progress = 0.0f;
    bool hasData = false;
};

// One effect "pedal": LED on/off, optional type selector and a row of knobs. Draggable by its title.
class Pedal : public juce::Component
{
public:
    Pedal (juce::AudioProcessorValueTreeState& apvts, const juce::String& title, juce::Colour accent,
           int onId, int typeId, std::initializer_list<Id> knobIds, const juce::String& subtitle = {});

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void refresh();

    int desiredWidth() const;
    void setDesiredWidth (int w) { fixedWidth = w; }

    // Gets the body area; returns the part left over for the knobs.
    std::function<juce::Rectangle<int> (juce::Rectangle<int>)> onLayoutBody;

    int slot = -1;                        // >= 0: takes part in the reorderable chain
    juce::Rectangle<int> targetBounds;    // where the editor wants this card to be
    std::function<void (Pedal&, const juce::MouseEvent&)> onDragStart, onDragMove, onDragEnd;
    std::function<void (Id)> onContextMenu;   // right-click on a control: MIDI learn / macro assignment

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
    int onId, typeId, fixedWidth = -1;
    bool isOn = true, dragArmed = false;

    juce::ToggleButton led;
    juce::ComboBox type;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::pair<juce::Component*, Id>> controls;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ledAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pedal)
};

// Signal-flow strip: IN > ... > OUT in the current chain order. Click a block to bypass it.
class ChainStrip : public juce::Component
{
public:
    ChainStrip (NoirProcessor& p, std::vector<juce::Colour> slotColours);

    void setOrder (const NoirProcessor::Order& o) { order = o; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    juce::Rectangle<float> boxFor (int index) const;   // index 0 = IN, NumSlots + 1 = OUT
    int indexAt (juce::Point<int>) const;

    NoirProcessor& proc;
    std::vector<juce::Colour> colours;
    NoirProcessor::Order order = NoirProcessor::defaultOrder();
    int hover = -1;
};

class NoirEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoirEditor (NoirProcessor&);
    ~NoirEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void chooseIR();
    void setAllEffects (bool on);
    void layoutCards (bool animate);
    void syncOrderFromProcessor();
    void commitOrder();
    void setLoopMode (int mode);
    void showParamMenu (Id id);
    void setTab (int tab);

    Pedal* makePedal (int slot);

    NoirProcessor& proc;
    NoirLookAndFeel laf;

    std::unique_ptr<ChainStrip> strip;
    std::vector<std::unique_ptr<Pedal>> pedals;
    Pedal* bySlot[NumSlots] {};
    Pedal *pTuner = nullptr, *pOut = nullptr;
    std::vector<Pedal*> cards;       // reorderable cards, in chain order
    Pedal* dragging = nullptr;
    juce::Point<int> grabOffset;
    juce::Rectangle<int> gridArea;

    juce::ComboBox presetBox;
    juce::TextButton tabBoard { "BOARD" }, tabDaw { "DAW" }, cancelLearn { "CANCEL" };
    std::unique_ptr<DawPanel> daw;
    int currentTab = 0;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, allOn { "ALL ON" }, allOff { "ALL OFF" }, resetOrder { "RESET ORDER" };
    juce::ToggleButton power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAtt;

    juce::TextButton irButton { "Load IR" }, irClear { "x" };
    juce::TextButton loopRec { "REC" }, loopPlay { "PLAY" }, loopDub { "DUB" }, loopStop { "STOP" }, loopClearBtn { "CLEAR" }, loopUndoBtn { "UNDO" };
    juce::ToggleButton loopRev;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> loopRevAtt;
    LoopBar loopBar;
    TunerView tunerView;
    std::unique_ptr<juce::FileChooser> chooser;

    float inLevel = 0.0f, outLevel = 0.0f;
    juce::Rectangle<int> headerArea, meterIn, meterOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirEditor)
};
