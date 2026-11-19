#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <functional>
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

class NoirProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    NoirProcessor();
    ~NoirProcessor() override { stopTimer(); }

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using juce::AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Noir Guitar"; }
    bool acceptsMidi() const override { return true; }
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
    enum LoopCmd { LcStop, LcRecord, LcPlay, LcOverdub, LcUndo, LcClear, LcStopAll, LcClearAll };
    void loopRequest (int track, int cmd);                    // message thread -> audio thread (lock-free)
    int  selectedLoopTrack() const { return juce::jlimit (0, noir::Looper::NumTracks - 1, (int) raw[LoopTrack]->load()); }
    std::atomic<bool> loopHasData { false };
    std::atomic<int> loopState[noir::Looper::NumTracks] {};                         // 0 empty, 1 stopped, 2 playing, 3 overdub, 4 recording
    std::atomic<bool> loopUndoOk[noir::Looper::NumTracks] {};
    std::atomic<uint32_t> loopMute { 0 };                     // bit t = track t muted
    noir::Looper& loopEngine() { return looper; }

    noir::Tuner& getTuner() { return tuner; }
    const noir::SpectrumTap& getEqTap() const { return eqTap; }
    double getSampleRateHz() const { return sr; }

    // UI theme (0 = Pop, 1 = Retro); stored with the session
    std::atomic<int> uiTheme { 0 };

    // ---- DAW integration ------------------------------------------------------------------------------
    static constexpr int NumMacros = 8, LinksPerMacro = 8;

    // ---- Universal MIDI mapping (any controller: CC knobs / pads / footswitches, notes, program change) ------
    enum MidiType { MtCC, MtNote, MtProgram };
    enum MidiAction { ActParam, ActPresetNext, ActPresetPrev, ActLoopCycle, ActLoopRec, ActLoopPlay, ActLoopDub,
                      ActLoopStop, ActLoopUndo, ActLoopClear, ActTrackNext, ActTrackPrev, ActLoopStopAll, NumActions };
    enum MidiMode { MmAuto, MmToggle, MmMomentary, MmAbsolute };
    static constexpr int MaxBindings = 128;
    struct MidiBinding { int type = -1, channel = 0, number = 0, action = 0, target = -1, mode = 0; };

    static const char* actionName (int a);
    void startLearn (int action, int target, int mode);
    void startMidiLearn (Id id) { startLearn (ActParam, (int) id, MmAuto); }
    void cancelMidiLearn() { learnActive.store (false); }
    bool isLearning() const { return learnActive.load(); }
    int  getLearnTarget() const { return learnActive.load() ? (learnAction.load() == ActParam ? learnTarget.load() : 1000 + learnAction.load()) : -1; }
    int  addBinding (const MidiBinding&);
    bool getBinding (int index, MidiBinding& out) const;
    void removeBinding (int index);
    int  getCcFor (Id id) const;
    void clearMidiFor (Id id);
    void clearAllMidi();
    std::array<int, 128> midiMapSnapshot() const;
    juce::String describeBinding (const MidiBinding&) const;
    static juce::String describeSource (int type, int channel, int number);

    // monitor: last 16 incoming messages (packed type | channel | number | value)
    std::atomic<uint32_t> midiLog[16] {};
    std::atomic<int> midiCount { 0 };

    void loadPreset (int index);
    std::atomic<int> currentPreset { -1 };

    // ---- user presets (plain XML files in Documents/Noir Guitar, so they can be copied between machines)
    static juce::File userRoot();
    static juce::File chainPresetDir();
    static juce::File pedalPresetDir (int slot);
    static juce::File irLibraryDir();
    static juce::Array<juce::File> listFiles (const juce::File& dir, const juce::String& pattern);
    static bool isPresetParam (int id);
    static int  slotOfParam (int id);
    bool saveChainPreset (const juce::String& name);
    bool loadChainPreset (const juce::File& file);
    bool savePedalPreset (int slot, const juce::String& name);
    bool loadPedalPreset (int slot, const juce::File& file);
    void resetPedal (int slot);
    juce::File copyIntoIrLibrary (const juce::File& ir) const;
    juce::String currentUserPreset;        // message thread only; empty when a factory preset / nothing is loaded
    std::atomic<int> presetListVersion { 0 };

    // Set by the standalone window (null inside a plug-in host): lets the editor draw the window buttons itself.
    struct StandaloneHost
    {
        std::function<void()> minimise, toggleMaximise, close, audioSettings, toggleMute;
        std::function<bool()> isMuted;
    };
    StandaloneHost* standalone = nullptr;

    // performance
    std::atomic<float> cpuLoad { 0.0f };
    std::atomic<int> xruns { 0 };

    // Macros: eight automatable knobs, each driving up to eight parameters over a [lo, hi] range.
    struct LinkInfo { int param = -1; float lo = 0.0f, hi = 1.0f; };
    bool assignMacro (int macro, Id param);
    void unassignMacro (int macro, int linkIndex);
    LinkInfo getLink (int macro, int linkIndex) const;
    void setLinkRange (int macro, int linkIndex, float lo, float hi);

    std::atomic<int> mapVersion { 0 };                       // bumped whenever MIDI map / macro links change
    std::atomic<float> hostBpmShown { 0.0f };
    std::atomic<bool> hostPlayingShown { false }, hostKnown { false };

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
    void readHost();
    void handleMidi (juce::MidiBuffer&);
    void applyMacros();
    void timerCallback() override;
    void setParamNormalised (int index, float norm01);   // audio-thread safe: queued, applied on the message thread
    void applyBinding (const MidiBinding&, int type, int value, bool pressed);
    void doLoopCommand (int track, int cmd) noexcept;
    static int choiceCount (int paramIndex);
    bool ccWasDown[128] {};
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

    // DAW state
    struct BindSlot { std::atomic<int> type { -1 }, channel { 0 }, number { 0 }, action { 0 }, target { -1 }, mode { 0 }; };
    BindSlot bindings[MaxBindings];
    std::atomic<bool> learnActive { false };
    std::atomic<int> learnTarget { -1 }, learnAction { 0 }, learnMode { 0 };
    std::atomic<int> pendingPreset { -1 }, presetDelta { 0 };

    struct PendingParam { int index; float value; };
    juce::AbstractFifo paramFifo { 512 };
    PendingParam paramBuf[512];
    juce::AbstractFifo loopFifo { 32 };
    int loopBuf[32] {};
    void pushParam (int index, float norm);
    struct Link { std::atomic<int> param { -1 }; std::atomic<float> lo { 0.0f }, hi { 1.0f }; };
    Link links[NumMacros][LinksPerMacro];
    float lastMacro[NumMacros] {};
    double bpm = 120.0, hostPpq = 0.0, beatsPerBar = 4.0;
    bool hostPlaying = false, lastHostPlaying = false, hostHasPpq = false;

    // Per-chunk working state
    float* chanL = nullptr;
    float* chanR = nullptr;
    int blockN = 0, chunkStart = 0;
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
    Pair<noir::ProEq> eq;
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
    noir::SpectrumTap eqTap;

    noir::Biquad revHp, revLp, springPeak;
    juce::dsp::Reverb reverb;
    juce::dsp::Convolution convolution[2];
    std::unique_ptr<juce::dsp::Oversampling<float>> osDrive, osAmp;

    juce::SmoothedValue<float> inGainSm, masterSm, outGainSm, revMixSm, muteSm;

    juce::AudioBuffer<float> chainBuf, dryBuf, rawBuf, micBuf, fadeBuf, gainBuf, wetBuf;

    juce::String irPath;
    std::atomic<bool> irLoaded { false };
    noir::Limiter limiter;
    bool nanFlag = false;
    bool wasIR = false, wasActive = false;
    int lastRevType = -1, lastLoopMode = 0, revIdle = 3, dlyIdle = 3;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NoirProcessor)
};
