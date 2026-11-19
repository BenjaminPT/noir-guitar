#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_dsp/juce_dsp.h>
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
    // Palette of the active theme (0 = Pop Stompbox, 1 = Retro Amp). Changed by applyTheme().
    static inline juce::Colour bg { 0xff121212 }, panel { 0xff171717 }, line { 0xff2c2c2c }, text { 0xfff4f1ea },
                               dim { 0xff9a968d }, accent { 0xffff9f6b }, inset { 0xff141414 }, insetLine { 0xff2c2c2c },
                               hdrText { 0xfff4f1ea }, hdrDim { 0xff9a968d }, gold { 0xffff9f6b };
    static inline int theme = 0;
    static inline int style = 0;        // pedal surface: 0 filled, 1 enamel band, 2 flat panel, 3 neon outline
    static inline int knobStyle = 0;    // 0 stomp, 1 brass, 2 arc
    static inline float corner = 16.0f;
    static constexpr int NumThemes = 6;
    static const char* themeName (int t)
    {
        static const char* n[NumThemes] = { "Pop Stompbox  (default)", "Retro Amp", "Pro Studio", "Neon Night", "Swiss Mono", "Aurora" };
        return n[t < 0 ? 0 : (t >= NumThemes ? NumThemes - 1 : t)];
    }
    static juce::Colour onAccent (juce::Colour c) { return c.getPerceivedBrightness() > 0.6f ? juce::Colour (0xff111111) : juce::Colour (0xfff6efde); }

    NoirLookAndFeel();
    void applyTheme (int t);

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

// Window button (minimise / maximise / close) drawn in the plug-in's own header when running standalone.
class WinButton : public juce::Button
{
public:
    explicit WinButton (int k) : juce::Button ("win"), kind (k) { setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
private:
    int kind;   // 0 minimise, 1 maximise, 2 close
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

// Multi-track looper surface (30 tracks, scrollable): waveform per track, draggable trim handles, mute, transport buttons.
class LooperView : public juce::Component
{
public:
    explicit LooperView (NoirProcessor&);
    void refresh();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    juce::Rectangle<int> rowBounds (int t) const;
    juce::Rectangle<int> waveBounds (int t) const;
    int hitHandle (int t, juce::Point<int> pos) const;
    void clampScroll();

    NoirProcessor& proc;
    juce::TextButton rec { "REC" }, play { "PLAY" }, dub { "DUB" }, stop { "STOP" }, undo { "UNDO" }, clear { "CLEAR" }, stopAll { "STOP ALL" };
    juce::ToggleButton rev;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> revAtt;
    juce::Rectangle<int> rows;
    float spin = 0.0f, scroll = 0.0f;
    int selected = 0, drag = 0, dragTrack = 0;     // drag: 0 none, 1 trim start, 2 trim end
};

// Pro-Q style EQ: spectrum analyser, response curve and draggable band nodes (8 bands).
class EqPanel : public juce::Component, private juce::Timer
{
public:
    explicit EqPanel (NoirProcessor&);
    ~EqPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override { hover = -1; repaint(); }
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
    void refreshTheme();

private:
    static constexpr int N = 4096, Cols = 360;

    void timerCallback() override;
    void selectBand (int b);
    int bandAt (juce::Point<float>) const;
    float xForFreq (float f) const;
    float freqForX (float x) const;
    float yForGain (float g) const;
    float gainForY (float y) const;
    float getP (int band, int field) const;
    void setP (int band, int field, float plain);
    bool usesGain (int type) const { return type == noir::EqBell || type == noir::EqLowShelf || type == noir::EqHighShelf; }
    juce::Colour bandColour (int b) const;
    void showMenu (int band);
    void gesture (int band, bool begin);
    juce::Rectangle<float> chipRect (int i) const;
    static juce::String fmtFreq (float f);

    NoirProcessor& proc;
    juce::Rectangle<int> ctl;
    int lastType = -1;
    float startF = 1000.0f, startG = 0.0f;
    juce::Point<float> downPt;
    juce::dsp::FFT fft { 12 };
    juce::dsp::WindowingFunction<float> window { (size_t) N, juce::dsp::WindowingFunction<float>::hann };
    std::vector<float> fftBuf, spec;
    juce::Rectangle<float> graph;
    int selected = 0, hover = -1, dragBand = -1;
    bool analyzerOn = true;
    float phase = 0.0f;

    juce::Label bandLabel, freqLabel, gainLabel, qLabel;
    juce::ComboBox typeBox;
    juce::Slider freqS, gainS, qS;
    juce::ToggleButton onBtn, analyzerBtn;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAtt, gainAtt, qAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EqPanel)
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
    void setAccent (juce::Colour c) { accent = c; applyColours(); repaint(); }
    void applyColours();
    bool noDrag = false;
    int minRowHeight = 0;
    void enablePresetButton (std::function<void()> onClick);

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
    juce::TextButton presetBtn { ".." };
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
    void setColours (std::vector<juce::Colour> c) { colours = std::move (c); repaint(); }
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
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

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
    void setTheme (int t);
    void refreshAccents();
    void buildPanels();
    void rebuildPresetBox();
    void syncPresetSelection();
    void askName (const juce::String& title, const juce::String& message, const juce::String& initial,
                  std::function<void (juce::String)> done);
    void saveChainDialog (bool forceNew);
    void showPresetMenu();
    void showPedalPresetMenu (int slot);
    void showThemeMenu();
    void toast (const juce::String& msg);

    Pedal* makePedal (int slot);

    NoirProcessor& proc;
    NoirLookAndFeel laf;

    std::unique_ptr<ChainStrip> strip;
    juce::Viewport viewport;
    Board board;
    std::vector<std::unique_ptr<Pedal>> pedals;
    Pedal* bySlot[NumSlots] {};
    Pedal *pTuner = nullptr, *pOut = nullptr, *pEq = nullptr;
    std::unique_ptr<EqPanel> eqPanel;
    std::vector<Pedal*> cards;
    Pedal* dragging = nullptr;
    juce::Point<int> grabOffset;
    int boardWidth = 0;

    juce::ComboBox presetBox;
    juce::TextButton saveBtn { "SAVE" }, menuBtn { "..." }, audioBtn { "AUDIO" };
    WinButton winMin { 0 }, winMax { 1 }, winClose { 2 };
    juce::ComponentDragger dragger;
    juce::Array<juce::File> userFiles;
    std::vector<int> presetIds;
    int lastListVersion = -1;
    juce::String toastText;
    float toastTime = 0.0f;
    juce::TextButton tabBoard { "BOARD" }, tabMidi { "MIDI" }, cancelLearn { "CANCEL" }, themeBtn { "THEME" },
                     subCtl { "CONTROLLERS" }, subMacro { "TEMPO & MACROS" };
    int currentSub = 0;
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
