#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <memory>
#include "Dsp.h"
#include "Effects.h"
#include "Params.h"

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

    // Parameter helpers (message thread)
    juce::RangedAudioParameter* param (Id i) { return apvts.getParameter (paramId (i)); }
    void setParamValue (Id i, float plainValue);
    void resetAllParams();

    juce::AudioProcessorValueTreeState apvts;

    // Peak meters, written by the audio thread, read-and-cleared by the editor.
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    float v (Id i) const noexcept { return raw[i]->load(); }
    bool on (Id i) const noexcept { return raw[i]->load() > 0.5f; }
    void resetEffects();

    std::atomic<float>* raw[NumParams] {};

    double sr = 44100.0;
    int maxBlock = 512;

    // Faders (click-free on/off for every pedal)
    noir::Fader fPower, fGate, fComp, fWah, fPitch, fBoost, fCab, fEq, fM1, fM2, fLo, fDly, fRev;
    noir::Fader fAmp; // runs at the oversampled rate

    // DSP
    noir::NoiseGate gate;
    noir::Compressor comp;
    noir::Wah wah;
    noir::PitchShifter pitch, shimmer;
    noir::Boost boost;
    noir::DriveStage drive;
    noir::AmpStage amp;
    noir::Cabinet cab;
    noir::Eq4 eq;
    noir::ModFx mod1, mod2;
    noir::LoFi lofi;
    noir::TimeDelay delay;
    noir::Biquad revHp, revLp, springPeak;
    juce::dsp::Reverb reverb;
    juce::dsp::Convolution convolution;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    juce::SmoothedValue<float> inGainSm, masterSm, outGainSm;

    juce::AudioBuffer<float> monoBuf, dryBuf, rawBuf, fxBuf, wetBuf;

    juce::String irPath;
    std::atomic<bool> irLoaded { false };
    bool wasIR = false, wasActive = false;
    int revIdle = 0, lastRevType = -1;
    float revMixTarget = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirProcessor)
};
