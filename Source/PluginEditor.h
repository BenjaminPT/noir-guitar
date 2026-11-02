#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <functional>
#include <memory>
#include <vector>
#include "PluginProcessor.h"
#include "DawPanel.h"
#include "MidiPanel.h"

// Flat, colourful "stompbox" look: dark bodies, bold accent-coloured headers, solid knobs.
class NoirLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static inline const juce::Colour bg     { 0xff0b0b0d };
    static inline const juce::Colour panel  { 0xff131316 };
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
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::FontOptions (12.5f); }
    juce::Font getLabelFont (juce::Label&) override { return juce::FontOptions (11.0f); }
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

// Level meter with signal-present indicator, peak hold and clip lamp.
class LevelMeter : public juce::Component
{
public:
    explicit LevelMeter (const juce::String& n) : name (n) {}
    void push (float peak, float dt);     // linear peak since last call
    void paint (juce::Graphics&) override;
    bool hasSignal() const { return sinceSignal < 1.5f; }

private:
    juce::String name;
    float level = 0.0f, hold = 0.0f, holdTime = 0.0f, sinceSignal = 99.0f, clipTime = 99.0f;
};

// Four-track looper surface: track rows (select / state / progress / mute) and transport buttons.
class LooperView : public juce::Component
{
public:
    explicit LooperView (NoirProcessor&);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<int> rowBounds (int t) const;

    NoirProcessor& proc;
    juce::TextButton rec { "REC" }, play { "PLAY" }, dub { "DUB" }, stop { "STOP" }, undo { "UNDO" }, clear { "CLEAR" }, stopAll { "STOP ALL" };
    juce::ToggleButton rev;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> revAtt;
    juce::Rectangle<int> rows;
    float spin = 0.0f;
    int selected = 0;
};

// One effect "pedal": header, LED, optional type selector and a row of knobs.
// Click anywhere on its surface to switch it on or off; drag the header to reorder.
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
    void mouseEnter (const juce::MouseEvent&) override { hover = true; }
    void mouseExit (const juce::MouseEvent&) override { hover = false; }
    void refresh();
    bool tick (float dt, float signal);     // animation step; returns true when something moved

    int desiredWidth() const;
    void setDesiredWidth (int w) { fixedWidth = w; }
    bool isEnabled() const { return isOn; }
    void toggle();

    std::function<juce::Rectangle<int> (juce::Rectangle<int>)> onLayoutBody;

    int slot = -1;
    juce::Rectangle<int> targetBounds;
    std::function<void (Pedal&, const juce::MouseEvent&)> onDragStart, onDragMove, onDragEnd;
    std::function<void (Id)> onContextMenu;

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
    bool isOn = true, dragArmed = false, dragging = false, hover = false;
    float onAmt = 1.0f, flash = 0.0f, phase = 0.0f, sig = 0.0f;
    juce::Point<int> downPos;

    juce::ToggleButton led;
    juce::ComboBox type;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::pair<juce::Component*, Id>> controls;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ledAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Pedal)
};

// Signal-flow strip: IN > ... > OUT in the current chain order, with flowing dots. Click a block to bypass it.
class ChainStrip : public juce::Component
{
public:
    ChainStrip (NoirProcessor& p, std::vector<juce::Colour> slotColours);

    void setOrder (const NoirProcessor::Order& o) { order = o; repaint(); }
    void tick (float dt, float signal) { phase = std::fmod (phase + dt * (0.35f + 1.2f * signal), 1.0f); repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }

private:
    juce::Rectangle<float> boxFor (int index) const;
    int indexAt (juce::Point<int>) const;

    NoirProcessor& proc;
    std::vector<juce::Colour> colours;
    NoirProcessor::Order order = NoirProcessor::defaultOrder();
    int hover = -1;
    float phase = 0.0f;
};

class NoirEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit NoirEditor (NoirProcessor&);
    ~NoirEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Board : public juce::Component
    {
        std::function<void (juce::Graphics&)> onPaint;
        void paint (juce::Graphics& g) override { if (onPaint) onPaint (g); }
    };

    void timerCallback() override;
    void animTick();
    void chooseIR();
    void setAllEffects (bool on);
    void layoutCards (bool animate);
    void syncOrderFromProcessor();
    void commitOrder();
    void showParamMenu (Id id);
    void setTab (int tab);

    Pedal* makePedal (int slot);

    NoirProcessor& proc;
    NoirLookAndFeel laf;

    std::unique_ptr<ChainStrip> strip;
    juce::Viewport viewport;
    Board board;
    std::vector<std::unique_ptr<Pedal>> pedals;
    Pedal* bySlot[NumSlots] {};
    Pedal *pTuner = nullptr, *pOut = nullptr;
    std::vector<Pedal*> cards;
    Pedal* dragging = nullptr;
    juce::Point<int> grabOffset;
    int boardWidth = 0;

    juce::ComboBox presetBox;
    juce::TextButton tabBoard { "BOARD" }, tabMidi { "MIDI" }, tabDaw { "DAW" }, cancelLearn { "CANCEL" };
    std::unique_ptr<DawPanel> daw;
    std::unique_ptr<MidiPanel> midi;
    int currentTab = 0;
    juce::TextButton prevPreset { "<" }, nextPreset { ">" }, allOn { "ALL ON" }, allOff { "ALL OFF" }, resetOrder { "RESET ORDER" };
    juce::ToggleButton power;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> powerAtt;

    juce::TextButton irButton { "Load IR" }, irClear { "x" };
    std::unique_ptr<LooperView> looperView;
    TunerView tunerView;
    std::unique_ptr<juce::FileChooser> chooser;

    LevelMeter meterIn { "IN" }, meterOut { "OUT" };
    juce::Rectangle<int> headerArea, perfArea, waveArea;
    juce::String perfText;

    std::unique_ptr<juce::VBlankAttachment> vblank;
    double lastFrame = 0.0;
    float wavePhase = 0.0f, outSmooth = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirEditor)
};
