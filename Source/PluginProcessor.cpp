#include "PluginProcessor.h"
#include "PluginEditor.h"

using NR = juce::NormalisableRange<float>;

juce::AudioProcessorValueTreeState::ParameterLayout NoirProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    for (int i = 0; i < NumParams; ++i)
    {
        const auto& p = kParams[i];
        const juce::ParameterID pid { p.id, 1 };

        if (p.kind == 1)
        {
            params.push_back (std::make_unique<juce::AudioParameterBool> (pid, p.name, p.def > 0.5f));
        }
        else if (p.kind == 2)
        {
            params.push_back (std::make_unique<juce::AudioParameterChoice> (
                pid, p.name, juce::StringArray::fromTokens (p.choices, "|", ""), (int) p.def));
        }
        else
        {
            const auto fmt = p.fmt;
            const float interval = fmt == Ms ? 1.0f : (fmt == Gate ? 0.5f : 0.001f);
            params.push_back (std::make_unique<juce::AudioParameterFloat> (
                pid, p.name, NR (p.lo, p.hi, interval, p.skew), p.def,
                juce::AudioParameterFloatAttributes().withStringFromValueFunction (
                    [fmt] (float val, int) { return formatParam (fmt, val); })));
        }
    }

    return { params.begin(), params.end() };
}

//==============================================================================
NoirProcessor::NoirProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::mono(),   true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NOIR", createLayout())
{
    for (int i = 0; i < NumParams; ++i)
        raw[i] = apvts.getRawParameterValue (kParams[i].id);
}

bool NoirProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto in  = l.getMainInputChannelSet();
    const auto out = l.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (in  != juce::AudioChannelSet::mono() && in  != juce::AudioChannelSet::stereo()) return false;
    return true;
}

void NoirProcessor::setParamValue (Id i, float plainValue)
{
    if (auto* p = param (i))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
}

void NoirProcessor::resetAllParams()
{
    for (int i = 0; i < NumParams; ++i)
    {
        if (i == Power) continue;
        const auto& info = kParams[i];
        if (auto* p = param ((Id) i))
            p->setValueNotifyingHost (p->convertTo0to1 (info.def));
    }
}

void NoirProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (32, samplesPerBlock);

    gate.prepare (sr);
    comp.prepare (sr);
    wah.prepare (sr);
    pitch.prepare (sr);
    shimmer.prepare (sr);
    boost.prepare (sr);
    cab.prepare (sr);
    eq.prepare (sr);
    mod1.prepare (sr);
    mod2.prepare (sr);
    lofi.prepare (sr);
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
    lastRevType = -1;

    inGainSm.reset (sr, 0.02);   inGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (v (InGain)));
    masterSm.reset (sr, 0.02);   masterSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (v (Master)));
    outGainSm.reset (sr, 0.02);  outGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (v (OutGain)));

    for (auto* f : { &fPower, &fGate, &fComp, &fWah, &fPitch, &fBoost, &fCab, &fEq, &fM1, &fM2, &fLo, &fDly, &fRev })
        f->prepare (sr);
    fAmp.prepare (sr * 4.0, 0.010);

    monoBuf.setSize (1, maxBlock, false, true, true);
    dryBuf.setSize (1, maxBlock, false, true, true);
    rawBuf.setSize (1, maxBlock, false, true, true);
    fxBuf.setSize (2, maxBlock, false, true, true);
    wetBuf.setSize (2, maxBlock, false, true, true);

    wasActive = false;
    revIdle = 0;
    fPower.snap (on (Power));

    setLatencySamples (juce::roundToInt (oversampling->getLatencyInSamples()));
}

void NoirProcessor::resetEffects()
{
    gate.reset(); wah.reset(); boost.reset(); drive.reset(); amp.reset(); cab.reset(); eq.reset();
    mod1.reset(); mod2.reset(); lofi.reset(); delay.reset();
    pitch.prepare (sr); shimmer.prepare (sr);
    reverb.reset(); convolution.reset();
    if (oversampling) oversampling->reset();
    revHp.reset(); revLp.reset(); springPeak.reset();

    fGate.snap (on (GateOn));   fComp.snap (on (CompOn));   fWah.snap (on (WahOn));
    fPitch.snap (on (PitchOn)); fBoost.snap (on (BoostOn)); fCab.snap (on (CabOn));
    fEq.snap (on (EqOn));       fM1.snap (on (M1On));       fM2.snap (on (M2On));
    fLo.snap (on (LoOn));       fDly.snap (on (DlyOn));     fRev.snap (on (RevOn));
    fAmp.snap (on (AmpOn));
    revIdle = 0;
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

    fPower.set (on (Power));

    float inPk = 0.0f, outPk = 0.0f;

    // ---- Power off: clean pass-through (with a short crossfade) -----------------------------------
    if (! fPower.active())
    {
        wasActive = false;
        const float* src = buffer.getReadPointer (0);
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
            if (ch < numOut) buffer.copyFrom (ch, 0, src, total);
        for (int ch = numOut; ch < buffer.getNumChannels(); ++ch)
            buffer.clear (ch, 0, total);
        for (int i = 0; i < total; ++i) inPk = juce::jmax (inPk, std::abs (src[i]));
        outPk = inPk;
        if (inPk  > inPeak.load())  inPeak.store (inPk);
        if (outPk > outPeak.load()) outPeak.store (outPk);
        return;
    }

    if (! wasActive)
    {
        resetEffects();
        wasActive = true;
    }

    // ---- Parameters (block rate; everything audible is smoothed inside the DSP) ---------------------
    inGainSm.setTargetValue (juce::Decibels::decibelsToGain (v (InGain)));
    masterSm.setTargetValue (juce::Decibels::decibelsToGain (v (Master)));
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (v (OutGain)));

    fGate.set (on (GateOn));   fComp.set (on (CompOn));   fWah.set (on (WahOn));
    fPitch.set (on (PitchOn)); fBoost.set (on (BoostOn)); fCab.set (on (CabOn));
    fEq.set (on (EqOn));       fM1.set (on (M1On));       fM2.set (on (M2On));
    fLo.set (on (LoOn));       fDly.set (on (DlyOn));     fRev.set (on (RevOn));
    fAmp.set (on (AmpOn));

    gate.setThresholdDb (v (GateThr));
    comp.setParams (v (CompAmt), v (CompLvl), v (CompAtk));
    wah.setParams ((int) v (WahMode) == 1, v (WahPos), v (WahQ));

    const int pitchType = (int) v (PitchType);
    const float pitchMix = v (PitchMix);
    {
        float semis = v (PitchShift);
        if (pitchType == 2) semis = -12.0f;             // Octaver
        else if (pitchType == 3) semis = semis / 24.0f; // Detune: +-0.5 semitone
        else if (pitchType == 4) semis = semis * 2.0f;  // Whammy: +-24 semitones
        pitch.setSemitones (semis);
    }

    boost.setParams ((int) v (BoostType), v (BoostGain));
    drive.setParams (on (DrvOn), (int) v (DrvType), v (DrvDrive), v (DrvTone), v (DrvLevel));
    amp.setParams (v (AmpGain), v (AmpBass), v (AmpMid), v (AmpTreble), v (AmpPres));
    cab.setVoicing ((int) v (CabType));
    eq.setParams (v (EqLow), v (EqLowMid), v (EqHiMid), v (EqHigh));
    mod1.setParams ((int) v (M1Type), v (M1Rate), v (M1Depth), v (M1Mix));
    mod2.setParams ((int) v (M2Type), v (M2Rate), v (M2Depth), v (M2Mix));
    lofi.setParams (v (LoBits), v (LoRate), v (LoMix));
    delay.setParams ((int) v (DlyType), v (DlyTime), v (DlyFb), v (DlyMix));

    // Reverb voicing
    const int revType = (int) v (RevType);
    if (revType != lastRevType)
    {
        lastRevType = revType;
        reverb.reset();
        revHp.highpass (sr, revType == 3 ? 300.0 : (revType == 2 || revType == 4 ? 150.0 : 100.0), 0.707);
        revLp.lowpass (sr, revType == 3 ? 4500.0 : 14000.0, 0.707);
        if (revType == 3) springPeak.peaking (sr, 1800.0, 1.0, 6.0);
        else springPeak.set (1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
        revHp.reset(); revLp.reset(); springPeak.reset();
    }
    {
        static const float baseSize[5] = { 0.30f, 0.80f, 0.60f, 0.20f, 0.85f };
        static const float damping[5]  = { 0.60f, 0.35f, 0.20f, 0.30f, 0.30f };
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = juce::jlimit (0.05f, 0.98f, baseSize[revType] + (v (RevSize) - 0.5f) * 0.4f);
        rp.damping = damping[revType];
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        rp.width = 1.0f;
        reverb.setParameters (rp);
    }
    revMixTarget = v (RevMix);
    if (revType == 4) shimmer.setSemitones (12.0f);

    const bool irOn = irLoaded.load();
    if (irOn != wasIR) { convolution.reset(); wasIR = irOn; }

    // ---- Audio ------------------------------------------------------------------------------------------
    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, total - start);
        float* mono = monoBuf.getWritePointer (0);
        float* rawp = rawBuf.getWritePointer (0);

        auto stage = [&] (noir::Fader& f, auto&& fn)
        {
            if (! f.active()) return;
            for (int i = 0; i < n; ++i)
            {
                const float x = mono[i];
                mono[i] = x + (fn (x) - x) * f.next();
            }
        };

        // Guitar comes in on the first input channel.
        const float* src = buffer.getReadPointer (0) + start;
        for (int i = 0; i < n; ++i)
        {
            rawp[i] = src[i];
            const float x = src[i] * inGainSm.getNextValue();
            inPk = juce::jmax (inPk, std::abs (x));
            mono[i] = x;
        }

        stage (fGate,  [&] (float x) { return gate.process (x); });
        stage (fComp,  [&] (float x) { return comp.process (x); });
        stage (fWah,   [&] (float x) { return wah.process (x); });
        stage (fPitch, [&] (float x)
        {
            const float w = pitch.process (x);
            switch (pitchType)
            {
                case 1:  case 2: return x + w * pitchMix;                    // Harmonizer, Octaver
                case 3:          return x + (w - x) * pitchMix * 0.5f;       // Detune
                default:         return x + (w - x) * pitchMix;              // Shifter, Whammy
            }
        });
        stage (fBoost, [&] (float x) { return boost.process (x); });

        // Drive + Amp at 4x
        if (drive.active() || fAmp.active())
        {
            juce::dsp::AudioBlock<float> block (monoBuf.getArrayOfWritePointers(), 1, (size_t) n);
            auto up = oversampling->processSamplesUp (block);
            float* u = up.getChannelPointer (0);
            const size_t un = up.getNumSamples();
            const bool ampActive = fAmp.active();
            for (size_t i = 0; i < un; ++i)
            {
                float s = drive.process (u[i]);
                if (ampActive)
                {
                    const float a = amp.process (s);
                    s += (a - s) * fAmp.next();
                }
                u[i] = s;
            }
            oversampling->processSamplesDown (block);
        }

        // Cabinet (procedural voicing or loaded IR), crossfaded
        if (fCab.active())
        {
            std::memcpy (dryBuf.getWritePointer (0), mono, sizeof (float) * (size_t) n);
            if (irOn)
            {
                juce::dsp::AudioBlock<float> block (monoBuf.getArrayOfWritePointers(), 1, (size_t) n);
                convolution.process (juce::dsp::ProcessContextReplacing<float> (block));
            }
            else
            {
                for (int i = 0; i < n; ++i) mono[i] = cab.processCab (mono[i]);
            }
            const float* dry = dryBuf.getReadPointer (0);
            for (int i = 0; i < n; ++i)
                mono[i] = dry[i] + (mono[i] - dry[i]) * fCab.next();
        }
        for (int i = 0; i < n; ++i) mono[i] = cab.processDc (mono[i]);

        stage (fEq, [&] (float x) { return eq.process (x); });

        // Master -> stereo
        float* l = fxBuf.getWritePointer (0);
        float* r = fxBuf.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            const float x = mono[i] * masterSm.getNextValue();
            l[i] = r[i] = x;
        }

        // Modulation 1 / 2
        auto stereoStage = [&] (noir::Fader& f, auto&& fn)
        {
            if (! f.active()) return;
            for (int i = 0; i < n; ++i)
            {
                float ol, orr;
                fn (l[i], r[i], ol, orr);
                const float k = f.next();
                l[i] += (ol - l[i]) * k;
                r[i] += (orr - r[i]) * k;
            }
        };

        stereoStage (fM1, [&] (float a, float b, float& oa, float& ob) { mod1.process (a, b, oa, ob); });
        stereoStage (fM2, [&] (float a, float b, float& oa, float& ob) { mod2.process (a, b, oa, ob); });
        stereoStage (fLo, [&] (float a, float b, float& oa, float& ob) { lofi.process (a, b, oa, ob); });

        // Delay (input is faded, so the tail spills over when it is switched off)
        for (int i = 0; i < n; ++i)
        {
            float ol, orr;
            delay.process (l[i], r[i], fDly.next(), ol, orr);
            l[i] = ol;
            r[i] = orr;
        }

        // Reverb as a parallel wet path
        float* wl = wetBuf.getWritePointer (0);
        float* wr = wetBuf.getWritePointer (1);
        const bool revRun = fRev.active() || revIdle < 3;
        if (revRun)
        {
            for (int i = 0; i < n; ++i)
            {
                float m = 0.5f * (l[i] + r[i]) * fRev.next();
                if (revType == 4) m += 0.6f * shimmer.process (m);
                m = revHp.process (m);
                m = springPeak.process (m);
                m = revLp.process (m);
                wl[i] = wr[i] = m;
            }

            juce::dsp::AudioBlock<float> wet (wetBuf.getArrayOfWritePointers(), 2, (size_t) n);
            reverb.process (juce::dsp::ProcessContextReplacing<float> (wet));

            float pk = 0.0f;
            for (int i = 0; i < n; ++i) pk = juce::jmax (pk, std::abs (wl[i]), std::abs (wr[i]));
            revIdle = (fRev.active() || pk > 1.0e-5f) ? 0 : revIdle + 1;
        }
        else
        {
            std::fill (wl, wl + n, 0.0f);
            std::fill (wr, wr + n, 0.0f);
        }

        // Output (with the Power crossfade against the clean input)
        const float revMixNow = revMixTarget * 0.6f;
        float* outL = buffer.getWritePointer (0) + start;
        float* outR = numOut >= 2 ? buffer.getWritePointer (1) + start : nullptr;

        for (int i = 0; i < n; ++i)
        {
            const float g = outGainSm.getNextValue();
            const float p = fPower.next();
            const float a = noir::softLimit ((l[i] + wl[i] * revMixNow) * g);
            const float b = noir::softLimit ((r[i] + wr[i] * revMixNow) * g);
            const float dry = rawp[i];
            const float oa = dry + (a - dry) * p;
            const float ob = dry + (b - dry) * p;

            if (outR != nullptr)
            {
                outPk = juce::jmax (outPk, std::abs (oa), std::abs (ob));
                outL[i] = oa;
                outR[i] = ob;
            }
            else
            {
                const float m = 0.5f * (oa + ob);
                outPk = juce::jmax (outPk, std::abs (m));
                outL[i] = m;
            }
        }
    }

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
