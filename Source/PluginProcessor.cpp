#include "PluginProcessor.h"
#include "PluginEditor.h"

using APF = juce::AudioParameterFloat;
using APB = juce::AudioParameterBool;
using APC = juce::AudioParameterChoice;
using NR  = juce::NormalisableRange<float>;

static juce::String dbText (float v, int) { return juce::String (v, 1) + " dB"; }
static juce::String oneDp  (float v, int) { return juce::String (v, 1); }
static juce::String pctTxt (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + "%"; }
static juce::String msTxt  (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; }
static juce::String gateTxt (float v, int) { return v <= -89.5f ? juce::String ("Off") : juce::String (v, 0) + " dB"; }

juce::AudioProcessorValueTreeState::ParameterLayout NoirProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> v;
    auto id = [] (const char* s) { return juce::ParameterID { s, 1 }; };

    v.push_back (std::make_unique<APF> (id (ids::in),     "Input",      NR (-24.0f, 24.0f, 0.1f), 0.0f,  juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));
    v.push_back (std::make_unique<APF> (id (ids::gate),   "Gate",       NR (-90.0f, -30.0f, 0.5f), -72.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (gateTxt)));

    v.push_back (std::make_unique<APB> (id (ids::odOn),   "Drive On",   true));
    v.push_back (std::make_unique<APF> (id (ids::odDrive),"Drive",      NR (0.0f, 1.0f, 0.001f), 0.35f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctTxt)));
    v.push_back (std::make_unique<APF> (id (ids::odTone), "Drive Tone", NR (0.0f, 1.0f, 0.001f), 0.5f,  juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctTxt)));
    v.push_back (std::make_unique<APF> (id (ids::odLevel),"Drive Level",NR (-12.0f, 12.0f, 0.1f), 0.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    v.push_back (std::make_unique<APF> (id (ids::gain),    "Gain",      NR (0.0f, 10.0f, 0.01f), 5.5f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (oneDp)));
    v.push_back (std::make_unique<APF> (id (ids::bass),    "Bass",      NR (0.0f, 10.0f, 0.01f), 5.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (oneDp)));
    v.push_back (std::make_unique<APF> (id (ids::mid),     "Mid",       NR (0.0f, 10.0f, 0.01f), 5.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (oneDp)));
    v.push_back (std::make_unique<APF> (id (ids::treble),  "Treble",    NR (0.0f, 10.0f, 0.01f), 5.5f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (oneDp)));
    v.push_back (std::make_unique<APF> (id (ids::presence),"Presence",  NR (0.0f, 10.0f, 0.01f), 4.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (oneDp)));
    v.push_back (std::make_unique<APF> (id (ids::master),  "Master",    NR (-30.0f, 6.0f, 0.1f), -10.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    v.push_back (std::make_unique<APB> (id (ids::cabOn),   "Cab On",    true));
    v.push_back (std::make_unique<APC> (id (ids::cabType), "Cab",       juce::StringArray { "1x12 Open", "2x12 Combo", "4x12 Closed" }, 1));

    v.push_back (std::make_unique<APF> (id (ids::dlyTime), "Delay Time",NR (40.0f, 1000.0f, 1.0f, 0.5f), 380.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (msTxt)));
    v.push_back (std::make_unique<APF> (id (ids::dlyFb),   "Delay Feedback", NR (0.0f, 0.9f, 0.001f), 0.30f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctTxt)));
    v.push_back (std::make_unique<APF> (id (ids::dlyMix),  "Delay Mix", NR (0.0f, 1.0f, 0.001f), 0.0f,  juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctTxt)));

    v.push_back (std::make_unique<APF> (id (ids::revSize), "Reverb Size", NR (0.0f, 1.0f, 0.001f), 0.5f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctTxt)));
    v.push_back (std::make_unique<APF> (id (ids::revMix),  "Reverb Mix",  NR (0.0f, 1.0f, 0.001f), 0.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (pctTxt)));

    v.push_back (std::make_unique<APF> (id (ids::out),     "Output",    NR (-24.0f, 12.0f, 0.1f), 0.0f, juce::AudioParameterFloatAttributes().withStringFromValueFunction (dbText)));

    return { v.begin(), v.end() };
}

//==============================================================================
NoirProcessor::NoirProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::mono(),   true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NOIR", createLayout())
{
    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    p.in = get (ids::in);           p.gate = get (ids::gate);       p.odOn = get (ids::odOn);
    p.odDrive = get (ids::odDrive); p.odTone = get (ids::odTone);   p.odLevel = get (ids::odLevel);
    p.gain = get (ids::gain);       p.bass = get (ids::bass);       p.mid = get (ids::mid);
    p.treble = get (ids::treble);   p.presence = get (ids::presence); p.master = get (ids::master);
    p.cabOn = get (ids::cabOn);     p.cabType = get (ids::cabType);
    p.dlyTime = get (ids::dlyTime); p.dlyFb = get (ids::dlyFb);     p.dlyMix = get (ids::dlyMix);
    p.revSize = get (ids::revSize); p.revMix = get (ids::revMix);   p.out = get (ids::out);
}

bool NoirProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in  = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (in  != juce::AudioChannelSet::mono() && in  != juce::AudioChannelSet::stereo()) return false;
    return true;
}

void NoirProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (32, samplesPerBlock);

    gate.prepare (sr);
    cab.prepare (sr);
    delay.prepare (sr);

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    oversampling->initProcessing ((size_t) maxBlock);
    oversampling->reset();
    drive.prepare (sr * 4.0);
    amp.prepare (sr * 4.0);

    juce::dsp::ProcessSpec mono { sr, (juce::uint32) maxBlock, 1 };
    juce::dsp::ProcessSpec stereo { sr, (juce::uint32) maxBlock, 2 };
    convolution.prepare (mono);
    convolution.reset();
    reverb.prepare (stereo);
    reverb.reset();

    inGainSm.reset (sr, 0.02);   inGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p.in->load()));
    masterSm.reset (sr, 0.02);   masterSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p.master->load()));
    outGainSm.reset (sr, 0.02);  outGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p.out->load()));
    revMixSm.reset (sr, 0.05);   revMixSm.setCurrentAndTargetValue (p.revMix->load());

    monoBuf.setSize (1, maxBlock, false, true, true);
    fxBuf.setSize (2, maxBlock, false, true, true);
    wetBuf.setSize (2, maxBlock, false, true, true);

    setLatencySamples (juce::roundToInt (oversampling->getLatencyInSamples()));
}

//==============================================================================
void NoirProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int total  = buffer.getNumSamples();

    if (numIn == 0 || oversampling == nullptr)
    {
        buffer.clear();
        return;
    }

    // Parameters (block-rate; everything that matters is smoothed inside the DSP).
    inGainSm.setTargetValue (juce::Decibels::decibelsToGain (p.in->load()));
    masterSm.setTargetValue (juce::Decibels::decibelsToGain (p.master->load()));
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (p.out->load()));
    revMixSm.setTargetValue (p.revMix->load());

    gate.setThresholdDb (p.gate->load());
    drive.setParams (p.odOn->load() > 0.5f, p.odDrive->load(), p.odTone->load(), p.odLevel->load());
    amp.setParams (p.gain->load(), p.bass->load(), p.mid->load(), p.treble->load(), p.presence->load());
    cab.setVoicing ((int) p.cabType->load());
    delay.setParams (p.dlyTime->load(), p.dlyFb->load(), p.dlyMix->load());

    const float room = p.revSize->load();
    juce::dsp::Reverb::Parameters rp;
    rp.roomSize = 0.25f + 0.7f * room;
    rp.damping = 0.55f;
    rp.wetLevel = 1.0f;
    rp.dryLevel = 0.0f;
    rp.width = 1.0f;
    reverb.setParameters (rp);

    const bool useCab = p.cabOn->load() > 0.5f;
    const bool useIR  = useCab && irLoaded.load();
    if (useIR != wasIR) { convolution.reset(); wasIR = useIR; }

    float inPk = 0.0f, outPk = 0.0f;

    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, total - start);
        float* mono = monoBuf.getWritePointer (0);

        // Guitar comes in on the first input channel.
        const float* src = buffer.getReadPointer (0) + start;
        for (int i = 0; i < n; ++i)
        {
            const float x = src[i] * inGainSm.getNextValue();
            inPk = juce::jmax (inPk, std::abs (x));
            mono[i] = gate.process (x);
        }

        // Drive + amp at 4x
        {
            juce::dsp::AudioBlock<float> block (monoBuf.getArrayOfWritePointers(), 1, (size_t) n);
            auto up = oversampling->processSamplesUp (block);
            float* u = up.getChannelPointer (0);
            const size_t un = up.getNumSamples();
            for (size_t i = 0; i < un; ++i)
                u[i] = amp.process (drive.process (u[i]));
            oversampling->processSamplesDown (block);
        }

        // Cabinet
        if (useIR)
        {
            juce::dsp::AudioBlock<float> block (monoBuf.getArrayOfWritePointers(), 1, (size_t) n);
            convolution.process (juce::dsp::ProcessContextReplacing<float> (block));
            for (int i = 0; i < n; ++i) mono[i] = cab.processDc (mono[i]);
        }
        else if (useCab)
        {
            for (int i = 0; i < n; ++i) mono[i] = cab.processCab (mono[i]);
        }
        else
        {
            for (int i = 0; i < n; ++i) mono[i] = cab.processDc (mono[i]);
        }

        // Master + delay -> stereo
        float* l = fxBuf.getWritePointer (0);
        float* r = fxBuf.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float x = mono[i] * masterSm.getNextValue();
            delay.process (x, l[i], r[i]);
        }

        // Reverb as a parallel wet path
        for (int ch = 0; ch < 2; ++ch)
            wetBuf.copyFrom (ch, 0, fxBuf, ch, 0, n);
        {
            juce::dsp::AudioBlock<float> wet (wetBuf.getArrayOfWritePointers(), 2, (size_t) n);
            reverb.process (juce::dsp::ProcessContextReplacing<float> (wet));
        }

        const float* wl = wetBuf.getReadPointer (0);
        const float* wr = wetBuf.getReadPointer (1);

        if (numOut >= 2)
        {
            float* outL = buffer.getWritePointer (0) + start;
            float* outR = buffer.getWritePointer (1) + start;
            for (int i = 0; i < n; ++i)
            {
                const float rm = revMixSm.getNextValue() * 0.6f;
                const float g = outGainSm.getNextValue();
                const float a = noir::softLimit ((l[i] + wl[i] * rm) * g);
                const float b = noir::softLimit ((r[i] + wr[i] * rm) * g);
                outPk = juce::jmax (outPk, std::abs (a), std::abs (b));
                outL[i] = a;
                outR[i] = b;
            }
        }
        else
        {
            float* outM = buffer.getWritePointer (0) + start;
            for (int i = 0; i < n; ++i)
            {
                const float rm = revMixSm.getNextValue() * 0.6f;
                const float g = outGainSm.getNextValue();
                const float a = noir::softLimit (0.5f * ((l[i] + r[i]) + (wl[i] + wr[i]) * rm) * g);
                outPk = juce::jmax (outPk, std::abs (a));
                outM[i] = a;
            }
        }
    }

    // Clear any extra input-only channels
    for (int ch = numOut; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, total);

    // Meters: the editor reads and resets these.
    if (inPk  > inPeak.load())  inPeak.store (inPk);
    if (outPk > outPeak.load()) outPeak.store (outPk);
}

//==============================================================================
void NoirProcessor::loadIR (const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    convolution.loadImpulseResponse (file,
                                     juce::dsp::Convolution::Stereo::no,
                                     juce::dsp::Convolution::Trim::yes,
                                     0,
                                     juce::dsp::Convolution::Normalise::yes);
    irPath = file.getFullPathName();
    irLoaded.store (true);
}

void NoirProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("irPath", irPath, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void NoirProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            const auto path = tree.getProperty ("irPath").toString();
            apvts.replaceState (tree);

            if (path.isNotEmpty())
                loadIR (juce::File (path));
        }
    }
}

juce::AudioProcessorEditor* NoirProcessor::createEditor() { return new NoirEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NoirProcessor(); }
