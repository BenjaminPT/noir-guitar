#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include "Dsp.h"
#include "Effects.h"
#include "Effects2.h"
#include "Params.h"

// Two instances of a mono effect: one per channel, so a mono effect placed after a stereo one still works.
template <class T>
struct Pair
{
    T c[2];
    T& operator[] (int i) noexcept { return c[i]; }
    template <class F> void each (F&& f) { f (c[0]); f (c[1]); }
};

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

    // Signal-chain order (a permutation of the Slot enum)
    using Order = std::array<int, NumSlots>;
    static Order defaultOrder();
    Order getChainOrder() const;
    void setChainOrder (const Order& o);

    // Looper controls (message thread -> audio thread)
    void loopClear() { clearReq.store (true); }
    void loopUndo()  { undoReq.store (true); }
    std::atomic<float> loopProgress { 0.0f };
    std::atomic<bool> loopHasData { false }, loopCanUndo { false };

    noir::Tuner& getTuner() { return tuner; }

    juce::AudioProcessorValueTreeState apvts;

    // Peak meters, written by the audio thread, read-and-cleared by the editor.
    std::atomic<float> inPeak { 0.0f }, outPeak { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    float v (Id i) const noexcept { return raw[i]->load(); }
    bool on (Id i) const noexcept { return raw[i]->load() > 0.5f; }
    void resetEffects();
    void updateParams();
    void runSlot (int slot);
    void makeStereo();
    void applyMaster();

    template <class Fn> void monoStage (noir::Fader& f, Fn&& fn);
    template <class Fn> void stereoStage (noir::Fader& f, Fn&& fn);

    std::atomic<float>* raw[NumParams] {};

    double sr = 44100.0;
    int maxBlock = 512;

    // Chain order (shared with the editor under a spin lock; the audio thread uses a cached copy)
    mutable juce::SpinLock orderLock;
    Order sharedOrder = defaultOrder(), audioOrder = defaultOrder();

    // Per-chunk working state
    float* chanL = nullptr;
    float* chanR = nullptr;
    int blockN = 0;
    bool stereoSig = false;

    // Block-rate parameter copies used by the slot lambdas
    int pitchType = 0, revType = 0;
    float pitchMix = 1.0f, glitchMix = 1.0f, granMix = 0.5f, frzMix = 0.8f, vocMix = 1.0f, loopLevel = 1.0f, revMixTarget = 0.0f;
    bool irOn = false;

    // Faders (click-free on/off for every pedal)
    noir::Fader fPower, fSlot[NumSlots];
    noir::Fader fAmp; // runs at the oversampled rate

    // DSP: mono effects exist once per channel
    Pair<noir::NoiseGate> gate;
    Pair<noir::Compressor> comp;
    Pair<noir::Wah> wah;
    Pair<noir::PitchShifter> pitch;
    Pair<noir::Boost> boost;
    Pair<noir::DriveStage> drive;
    Pair<noir::AmpStage> amp;
    Pair<noir::Cabinet> cab;
    Pair<noir::Eq4> eq;
    Pair<noir::TalkBox> talk;

    noir::Vocoder vocoder;
    noir::ModFx mod1, mod2;
    noir::LoFi lofi;
    noir::Glitch glitch;
    noir::Granular granular, freeze;
    noir::PitchShifter shimmer;
    noir::TimeDelay delay;
    noir::Looper looper;
    noir::Tuner tuner;

    noir::Biquad revHp, revLp, springPeak;
    juce::dsp::Reverb reverb;
    juce::dsp::Convolution convolution[2];
    std::unique_ptr<juce::dsp::Oversampling<float>> osDrive, osAmp;

    juce::SmoothedValue<float> inGainSm, masterSm, outGainSm, revMixSm, muteSm;

    juce::AudioBuffer<float> chainBuf, dryBuf, rawBuf, micBuf, fadeBuf, gainBuf, wetBuf;

    juce::String irPath;
    std::atomic<bool> irLoaded { false }, clearReq { false }, undoReq { false };
    bool wasIR = false, wasActive = false;
    int lastRevType = -1, lastLoopMode = 0, revIdle = 3, dlyIdle = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirProcessor)
};
