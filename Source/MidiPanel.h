#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <memory>
#include "PluginProcessor.h"

// "MIDI" tab: connected devices, live monitor, and a universal assignment table for any controller
// (knobs, pads, footswitches, program-change pedals) -> any effect on/off, knob, preset or looper action.
class MidiPanel : public juce::Component, private juce::Timer
{
public:
    explicit MidiPanel (NoirProcessor&);
    ~MidiPanel() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void visibilityChanged() override;

private:
    class DeviceModel;
    class BindingModel;

    void timerCallback() override;
    void refreshDevices();
    void refreshBindings();
    void updateTargetEnabled();
    void startLearn();
    void addManual();
    void card (juce::Graphics&, juce::Rectangle<int>, const juce::String& title);

    NoirProcessor& proc;

    std::unique_ptr<DeviceModel> deviceModel;
    juce::ListBox deviceList;
    juce::StringArray deviceNames;
    juce::Label deviceNote;

    juce::StringArray logLines;
    int lastCount = 0;
    float activity = 0.0f;

    juce::Label actionLabel, targetLabel, modeLabel, typeLabel, numberLabel;
    juce::ComboBox actionBox, targetBox, modeBox, typeBox;
    juce::Slider numberSlider;
    juce::TextButton learnBtn { "LEARN" }, addBtn { "ADD" };
    juce::Label help;

    std::unique_ptr<BindingModel> bindingModel;
    juce::ListBox bindingList;
    juce::TextButton removeBtn { "Remove selected" }, clearBtn { "Clear all" };

    juce::Rectangle<int> devCard, monCard, asgCard, mapCard;
    int lastVersion = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiPanel)
};
