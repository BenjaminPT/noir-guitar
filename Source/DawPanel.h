#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

// "DAW" tab: tempo / transport sync, MIDI control map and eight macro knobs.
class DawPanel : public juce::Component, private juce::Timer
{
public:
    explicit DawPanel (NoirProcessor&);
    ~DawPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    class MidiModel;
    class LinkModel;
    struct SyncRow
    {
        juce::Label label;
        juce::ComboBox box;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> att;
    };
    struct MacroKnob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void timerCallback() override;
    void refreshLists();
    void card (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);

    NoirProcessor& proc;

    // Tempo
    juce::ToggleButton hostSync { "Follow host tempo" };
    juce::Slider bpmSlider;
    juce::Label bpmLabel, hostInfo;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> hostSyncAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bpmAtt;

    // Sync
    SyncRow syncRows[4];

    // Looper
    juce::ToggleButton loopFollow { "Follow DAW transport (play / stop)" }, loopQuant { "Quantize loop length to bars" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> followAtt, quantAtt;

    // MIDI
    juce::Label midiHelp;
    juce::TextButton removeMidi { "Remove selected" }, clearMidi { "Clear all" };
    std::unique_ptr<MidiModel> midiModel;
    juce::ListBox midiList;

    // Macros
    MacroKnob macros[NoirProcessor::NumMacros];
    juce::ComboBox macroSel;
    juce::Label macroHelp;
    std::unique_ptr<LinkModel> linkModel;
    juce::ListBox linkList;

    juce::Rectangle<int> tempoCard, syncCard, loopCard, midiCard, macroCard;
    int lastVersion = -1, lastMacroSel = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DawPanel)
};
