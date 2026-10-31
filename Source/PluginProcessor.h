#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include "Dsp.h"

namespace ids
{
inline constexpr const char* in = "in";
inline constexpr const char* gate = "gate";
inline constexpr const char* odOn = "odOn";
inline constexpr const char* odDrive = "odDrive";
inline constexpr const char* odTone = "odTone";
inline constexpr const char* odLevel = "odLevel";
inline constexpr const char* gain = "gain";
inline constexpr const char* bass = "bass";
inline constexpr const char* mid = "mid";
inline constexpr const char* treble = "treble";
inline constexpr const char* presence = "presence";
inline constexpr const char* master = "master";
inline constexpr const char* cabOn = "cabOn";
inline constexpr const char* cabType = "cabType";
inline constexpr const char* dlyTime = "dlyTime";
inline constexpr const char* dlyFb = "dlyFb";
inline constexpr const char* dlyMix = "dlyMix";
inline constexpr const char* revSize = "revSize";
inline constexpr const char* revMix = "revMix";
inline constexpr const char* out = "out";
} // namespace ids

class NoirProcessor : public juce::AudioProcessor
{
public:
    NoirProcessor();
    ~NoirProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Noir Guitar"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Cabinet impulse response (message thread only)
    void loadIR (const juce::File& file);
    void clearIR() { irPath.clear(); irLoaded.store (false); }
    juce::String getIRName() const { return irPath.isEmpty() ? juce::String() : juce::File (irPath).getFileNameWithoutExtension(); }
    bool hasIR() const { return irLoaded.load(); }

    juce::AudioProcessorValueTreeState apvts;

    // Peak meters, written by the audio thread, read-and-cleared by the editor.
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    struct Params
    {
        std::atomic<float> *in, *gate, *odOn, *odDrive, *odTone, *odLevel, *gain, *bass, *mid, *treble,
                           *presence, *master, *cabOn, *cabType, *dlyTime, *dlyFb, *dlyMix, *revSize,
                           *revMix, *out;
    } p {};

    double sr = 44100.0;
    int maxBlock = 512;

    // DSP
    noir::NoiseGate gate;
    noir::DriveStage drive;
    noir::AmpStage amp;
    noir::Cabinet cab;
    noir::StereoDelay delay;
    juce::dsp::Reverb reverb;
    juce::dsp::Convolution convolution;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    juce::SmoothedValue<float> inGainSm, masterSm, revMixSm, outGainSm;

    juce::AudioBuffer<float> monoBuf, fxBuf, wetBuf;

    juce::String irPath;
    std::atomic<bool> irLoaded { false };
    bool wasIR = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirProcessor)
};
