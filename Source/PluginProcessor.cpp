#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

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
            const float interval = (fmt == Ms || fmt == Hz) ? 1.0f : (fmt == Gate ? 0.5f : (fmt == Bpm ? 0.1f : 0.001f));
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
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "NOIR", createLayout())
{
    for (int i = 0; i < NumParams; ++i)
        raw[i] = apvts.getRawParameterValue (kParams[i].id);
    for (auto& c : ccMap) c.store (-1);
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
        if (i == Power || i == TunerOn || i >= BpmManual) continue;
        const auto& info = kParams[i];
        if (auto* p = param ((Id) i))
            p->setValueNotifyingHost (p->convertTo0to1 (info.def));
    }
}

NoirProcessor::Order NoirProcessor::defaultOrder()
{
    Order o {};
    for (int i = 0; i < NumSlots; ++i) o[(size_t) i] = i;
    return o;
}

NoirProcessor::Order NoirProcessor::getChainOrder() const
{
    const juce::SpinLock::ScopedLockType lock (orderLock);
    return sharedOrder;
}

void NoirProcessor::setChainOrder (const Order& o)
{
    // must be a permutation of 0..NumSlots-1
    std::array<bool, NumSlots> seen {};
    for (int s : o)
    {
        if (s < 0 || s >= NumSlots || seen[(size_t) s]) return;
        seen[(size_t) s] = true;
    }
    const juce::SpinLock::ScopedLockType lock (orderLock);
    sharedOrder = o;
}

//==============================================================================
void NoirProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    maxBlock = juce::jmax (32, samplesPerBlock);

    gate.each ([&] (auto& e) { e.prepare (sr); });
    comp.each ([&] (auto& e) { e.prepare (sr); });
    wah.each ([&] (auto& e) { e.prepare (sr); });
    pitch.each ([&] (auto& e) { e.prepare (sr); });
    boost.each ([&] (auto& e) { e.prepare (sr); });
    cab.each ([&] (auto& e) { e.prepare (sr); });
    eq.each ([&] (auto& e) { e.prepare (sr); });
    talk.each ([&] (auto& e) { e.prepare (sr); });
    drive.each ([&] (auto& e) { e.prepare (sr * 4.0); });
    amp.each ([&] (auto& e) { e.prepare (sr * 4.0); });

    vocoder.prepare (sr);
    mod1.prepare (sr);
    mod2.prepare (sr);
    lofi.prepare (sr);
    glitch.prepare (sr);
    granular.prepare (sr);
    freeze.prepare (sr);
    shimmer.prepare (sr);
    delay.prepare (sr);
    looper.prepare (sr);
    tuner.prepare (sr);

    osDrive = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    osAmp = std::make_unique<juce::dsp::Oversampling<float>> (
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false);
    osDrive->initProcessing ((size_t) maxBlock);
    osAmp->initProcessing ((size_t) maxBlock);

    juce::dsp::ProcessSpec mono { sr, (juce::uint32) maxBlock, 1 };
    juce::dsp::ProcessSpec stereo { sr, (juce::uint32) maxBlock, 2 };
    for (auto& c : convolution) { c.prepare (mono); c.reset(); }
    reverb.prepare (stereo);
    reverb.reset();
    lastRevType = -1;

    inGainSm.reset (sr, 0.02);   inGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (v (InGain)));
    masterSm.reset (sr, 0.02);   masterSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (v (Master)));
    outGainSm.reset (sr, 0.02);  outGainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (v (OutGain)));
    revMixSm.reset (sr, 0.05);   revMixSm.setCurrentAndTargetValue (v (RevMix));
    muteSm.reset (sr, 0.03);     muteSm.setCurrentAndTargetValue (on (TunerOn) ? 0.0f : 1.0f);

    fPower.prepare (sr);
    for (auto& f : fSlot) f.prepare (sr);
    fAmp.prepare (sr * 4.0, 0.010);

    chainBuf.setSize (2, maxBlock, false, true, true);
    dryBuf.setSize (2, maxBlock, false, true, true);
    wetBuf.setSize (2, maxBlock, false, true, true);
    rawBuf.setSize (1, maxBlock, false, true, true);
    micBuf.setSize (1, maxBlock, false, true, true);
    fadeBuf.setSize (1, maxBlock, false, true, true);
    gainBuf.setSize (1, maxBlock, false, true, true);

    for (int m = 0; m < NumMacros; ++m) lastMacro[m] = v ((Id) (Macro1 + m));
    wasActive = false;
    revIdle = dlyIdle = 3;
    lastLoopMode = 0;
    fPower.snap (on (Power));

    setLatencySamples (juce::roundToInt (osDrive->getLatencyInSamples() + osAmp->getLatencyInSamples()));
}

void NoirProcessor::resetEffects()
{
    gate.each ([] (auto& e) { e.reset(); });
    wah.each ([] (auto& e) { e.reset(); });
    boost.each ([] (auto& e) { e.reset(); });
    drive.each ([] (auto& e) { e.reset(); });
    amp.each ([] (auto& e) { e.reset(); });
    cab.each ([] (auto& e) { e.reset(); });
    eq.each ([] (auto& e) { e.reset(); });
    talk.each ([] (auto& e) { e.reset(); });
    pitch.each ([this] (auto& e) { e.prepare (sr); });

    vocoder.reset();
    mod1.reset(); mod2.reset(); lofi.reset(); delay.reset();
    granular.reset(); freeze.reset();
    shimmer.prepare (sr);
    glitch.prepare (sr);
    reverb.reset();
    for (auto& c : convolution) c.reset();
    if (osDrive) osDrive->reset();
    if (osAmp) osAmp->reset();
    revHp.reset(); revLp.reset(); springPeak.reset();

    for (int s = 0; s < NumSlots; ++s)
        fSlot[s].snap (on (kSlotOn[s]));
    fAmp.snap (on (AmpOn));
    revIdle = dlyIdle = 3;
}

//==============================================================================
void NoirProcessor::updateParams()
{
    inGainSm.setTargetValue (juce::Decibels::decibelsToGain (v (InGain)));
    masterSm.setTargetValue (juce::Decibels::decibelsToGain (v (Master)));
    outGainSm.setTargetValue (juce::Decibels::decibelsToGain (v (OutGain)));
    revMixSm.setTargetValue (v (RevMix));

    for (int s = 0; s < NumSlots; ++s)
        fSlot[s].set (on (kSlotOn[s]));
    fAmp.set (on (AmpOn));

    gate.each ([&] (auto& e) { e.setThresholdDb (v (GateThr)); });
    comp.each ([&] (auto& e) { e.setParams (v (CompAmt), v (CompLvl), v (CompAtk)); });
    wah.each ([&] (auto& e) { e.setParams ((int) v (WahMode) == 1, v (WahPos), v (WahQ)); });

    pitchType = (int) v (PitchType);
    pitchMix = v (PitchMix);
    {
        float semis = v (PitchShift);
        if (pitchType == 2) semis = -12.0f;             // Octaver
        else if (pitchType == 3) semis = semis / 24.0f; // Detune: +-0.5 semitone
        else if (pitchType == 4) semis = semis * 2.0f;  // Whammy: +-24 semitones
        pitch.each ([&] (auto& e) { e.setSemitones (semis); });
    }

    boost.each ([&] (auto& e) { e.setParams ((int) v (BoostType), v (BoostGain)); });
    drive.each ([&] (auto& e) { e.setParams (on (DrvOn), (int) v (DrvType), v (DrvDrive), v (DrvTone), v (DrvLevel)); });
    amp.each ([&] (auto& e) { e.setParams (v (AmpGain), v (AmpBass), v (AmpMid), v (AmpTreble), v (AmpPres)); });
    cab.each ([&] (auto& e) { e.setVoicing ((int) v (CabType)); });
    eq.each ([&] (auto& e) { e.setParams (v (EqLow), v (EqLowMid), v (EqHiMid), v (EqHigh)); });
    talk.each ([&] (auto& e) { e.setParams (v (TbVowel), v (TbAuto), v (TbRes), v (TbMix)); });

    vocoder.setParams (v (VocSens), v (VocBright));
    vocMix = v (VocMix);

    auto syncHz = [this] (int choice) { const double b = syncBeats (choice); return b > 0.0 ? bpm / 60.0 / b : -1.0; };
    mod1.setParams ((int) v (M1Type), v (M1Rate), v (M1Depth), v (M1Mix), syncHz ((int) v (M1Sync)));
    mod2.setParams ((int) v (M2Type), v (M2Rate), v (M2Depth), v (M2Mix), syncHz ((int) v (M2Sync)));
    lofi.setParams (v (LoBits), v (LoRate), v (LoMix));

    float gltLen = v (GltLen);
    if (const double b = syncBeats ((int) v (GltSync)); b > 0.0)
        gltLen = (float) juce::jlimit (20.0, 500.0, b * 60000.0 / bpm);
    glitch.setParams ((int) v (GltMode), gltLen, v (GltAmt));
    glitchMix = v (GltMix);

    granular.setParams (v (GrSize), 2.0f + v (GrDens) * 58.0f, v (GrPitch), v (GrSpread));
    granMix = v (GrMix);
    freeze.setParams (v (FrzSize), 14.0f, 0.0f, 0.15f, v (FrzSize) * 3.0f);
    frzMix = v (FrzMix);

    float dlyTime = v (DlyTime);
    if (const double b = syncBeats ((int) v (DlySync)); b > 0.0)
        dlyTime = (float) juce::jlimit (40.0, 1000.0, b * 60000.0 / bpm);
    delay.setParams ((int) v (DlyType), dlyTime, v (DlyFb), v (DlyMix));

    loopLevel = juce::Decibels::decibelsToGain (v (LoopLevel));

    // Reverb voicing
    revType = (int) v (RevType);
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
    if (revType == 4) shimmer.setSemitones (12.0f);

    irOn = irLoaded.load();
    if (irOn != wasIR) { for (auto& c : convolution) c.reset(); wasIR = irOn; }
}

//==============================================================================
void NoirProcessor::makeStereo()
{
    if (! stereoSig)
    {
        std::memcpy (chanR, chanL, sizeof (float) * (size_t) blockN);
        stereoSig = true;
    }
}

template <class Fn>
void NoirProcessor::monoStage (noir::Fader& f, Fn&& fn)
{
    if (! f.active()) return;

    float* fv = fadeBuf.getWritePointer (0);
    for (int i = 0; i < blockN; ++i) fv[i] = f.next();

    const int chs = stereoSig ? 2 : 1;
    for (int ch = 0; ch < chs; ++ch)
    {
        float* d = ch == 0 ? chanL : chanR;
        for (int i = 0; i < blockN; ++i)
        {
            const float x = d[i];
            d[i] = x + (fn (ch, x) - x) * fv[i];
        }
    }
}

template <class Fn>
void NoirProcessor::stereoStage (noir::Fader& f, Fn&& fn)
{
    if (! f.active()) return;
    makeStereo();

    for (int i = 0; i < blockN; ++i)
    {
        float ol, orr;
        fn (chanL[i], chanR[i], ol, orr);
        const float k = f.next();
        chanL[i] += (ol - chanL[i]) * k;
        chanR[i] += (orr - chanR[i]) * k;
    }
}

void NoirProcessor::applyMaster()
{
    float* g = gainBuf.getWritePointer (0);
    for (int i = 0; i < blockN; ++i) g[i] = masterSm.getNextValue();
    for (int i = 0; i < blockN; ++i) chanL[i] *= g[i];
    if (stereoSig)
        for (int i = 0; i < blockN; ++i) chanR[i] *= g[i];
}

void NoirProcessor::runSlot (int slot)
{
    const int n = blockN;

    switch (slot)
    {
        case SGate:  monoStage (fSlot[SGate],  [&] (int ch, float x) { return gate[ch].process (x); }); break;
        case SComp:  monoStage (fSlot[SComp],  [&] (int ch, float x) { return comp[ch].process (x); }); break;
        case SWah:   monoStage (fSlot[SWah],   [&] (int ch, float x) { return wah[ch].process (x); }); break;

        case SPitch:
            monoStage (fSlot[SPitch], [&] (int ch, float x)
            {
                const float w = pitch[ch].process (x);
                switch (pitchType)
                {
                    case 1: case 2: return x + w * pitchMix;               // Harmonizer, Octaver
                    case 3:         return x + (w - x) * pitchMix * 0.5f;  // Detune
                    default:        return x + (w - x) * pitchMix;         // Shifter, Whammy
                }
            });
            break;

        case SBoost: monoStage (fSlot[SBoost], [&] (int ch, float x) { return boost[ch].process (x); }); break;

        case SDrive:
            if (drive[0].active())
            {
                float* ptrs[2] = { chanL, chanR };
                const int chs = stereoSig ? 2 : 1;
                juce::dsp::AudioBlock<float> block (ptrs, (size_t) chs, (size_t) n);
                auto up = osDrive->processSamplesUp (block);
                const size_t un = up.getNumSamples();
                for (int c = 0; c < chs; ++c)
                {
                    float* u = up.getChannelPointer ((size_t) c);
                    for (size_t i = 0; i < un; ++i) u[i] = drive[c].process (u[i]);
                }
                osDrive->processSamplesDown (block);
            }
            break;

        case SAmp:
            if (fAmp.active())
            {
                float* ptrs[2] = { chanL, chanR };
                const int chs = stereoSig ? 2 : 1;
                juce::dsp::AudioBlock<float> block (ptrs, (size_t) chs, (size_t) n);
                auto up = osAmp->processSamplesUp (block);
                const size_t un = up.getNumSamples();
                float* u0 = up.getChannelPointer (0);
                float* u1 = chs > 1 ? up.getChannelPointer (1) : nullptr;
                for (size_t i = 0; i < un; ++i)
                {
                    const float fa = fAmp.next();
                    const float s0 = u0[i];
                    u0[i] = s0 + (amp[0].process (s0) - s0) * fa;
                    if (u1 != nullptr)
                    {
                        const float s1 = u1[i];
                        u1[i] = s1 + (amp[1].process (s1) - s1) * fa;
                    }
                }
                osAmp->processSamplesDown (block);
            }
            break;

        case SCab:
        {
            const int chs = stereoSig ? 2 : 1;
            if (fSlot[SCab].active())
            {
                float* fv = fadeBuf.getWritePointer (0);
                for (int i = 0; i < n; ++i) fv[i] = fSlot[SCab].next();

                for (int ch = 0; ch < chs; ++ch)
                {
                    float* d = ch == 0 ? chanL : chanR;
                    float* dry = dryBuf.getWritePointer (ch);
                    std::memcpy (dry, d, sizeof (float) * (size_t) n);
                    if (irOn)
                    {
                        float* one[1] = { d };
                        juce::dsp::AudioBlock<float> block (one, 1, (size_t) n);
                        convolution[ch].process (juce::dsp::ProcessContextReplacing<float> (block));
                    }
                    else
                    {
                        for (int i = 0; i < n; ++i) d[i] = cab[ch].processCab (d[i]);
                    }
                    for (int i = 0; i < n; ++i) d[i] = dry[i] + (d[i] - dry[i]) * fv[i];
                }
            }
            for (int ch = 0; ch < chs; ++ch)
            {
                float* d = ch == 0 ? chanL : chanR;
                for (int i = 0; i < n; ++i) d[i] = cab[ch].processDc (d[i]);
            }
            break;
        }

        case SEq:   monoStage (fSlot[SEq],   [&] (int ch, float x) { return eq[ch].process (x); }); break;
        case STalk: monoStage (fSlot[STalk], [&] (int ch, float x) { return talk[ch].process (x); }); break;

        case SVoc:
            if (fSlot[SVoc].active())
            {
                const float* mic = micBuf.getReadPointer (0);
                for (int i = 0; i < n; ++i)
                {
                    const float fv = fSlot[SVoc].next();
                    const float car = stereoSig ? 0.5f * (chanL[i] + chanR[i]) : chanL[i];
                    const float y = car + (vocoder.process (mic[i], car) - car) * vocMix;
                    chanL[i] += (y - chanL[i]) * fv;
                    if (stereoSig) chanR[i] += (y - chanR[i]) * fv;
                }
            }
            break;

        case SMod1:  stereoStage (fSlot[SMod1], [&] (float a, float b, float& oa, float& ob) { mod1.process (a, b, oa, ob); }); break;
        case SMod2:  stereoStage (fSlot[SMod2], [&] (float a, float b, float& oa, float& ob) { mod2.process (a, b, oa, ob); }); break;
        case SLofi:  stereoStage (fSlot[SLofi], [&] (float a, float b, float& oa, float& ob) { lofi.process (a, b, oa, ob); }); break;

        case SGlitch:
            stereoStage (fSlot[SGlitch], [&] (float a, float b, float& oa, float& ob)
            {
                float wl, wr;
                glitch.process (a, b, wl, wr);
                oa = a + (wl - a) * glitchMix;
                ob = b + (wr - b) * glitchMix;
            });
            break;

        case SGran:
            stereoStage (fSlot[SGran], [&] (float a, float b, float& oa, float& ob)
            {
                float wl, wr;
                granular.process (0.5f * (a + b), true, wl, wr);
                oa = a * (1.0f - granMix) + wl * granMix * 1.5f;
                ob = b * (1.0f - granMix) + wr * granMix * 1.5f;
            });
            break;

        case SFreeze:
        {
            // The buffer keeps recording while the pedal is off so a freeze captures what you just played.
            const bool engaged = on (FrzOn);
            freeze.setFrozen (engaged);
            if (engaged || fSlot[SFreeze].active()) makeStereo();
            for (int i = 0; i < n; ++i)
            {
                const float m = stereoSig ? 0.5f * (chanL[i] + chanR[i]) : chanL[i];
                float wl, wr;
                freeze.process (m, engaged, wl, wr);
                const float fv = fSlot[SFreeze].next();
                if (stereoSig)
                {
                    chanL[i] += wl * frzMix * 1.5f * fv;
                    chanR[i] += wr * frzMix * 1.5f * fv;
                }
            }
            break;
        }

        case SDelay:
            if (fSlot[SDelay].active() || dlyIdle < 3)
            {
                makeStereo();
                float pk = 0.0f;
                for (int i = 0; i < n; ++i)
                {
                    float ol, orr;
                    delay.process (chanL[i], chanR[i], fSlot[SDelay].next(), ol, orr);
                    pk = juce::jmax (pk, std::abs (ol - chanL[i]), std::abs (orr - chanR[i]));
                    chanL[i] = ol;
                    chanR[i] = orr;
                }
                dlyIdle = (fSlot[SDelay].active() || pk > 1.0e-5f) ? 0 : dlyIdle + 1;
            }
            break;

        case SReverb:
            if (fSlot[SReverb].active() || revIdle < 3)
            {
                makeStereo();
                float* wl = wetBuf.getWritePointer (0);
                float* wr = wetBuf.getWritePointer (1);
                for (int i = 0; i < n; ++i)
                {
                    float m = 0.5f * (chanL[i] + chanR[i]) * fSlot[SReverb].next();
                    if (revType == 4) m += 0.6f * shimmer.process (m);
                    m = revHp.process (m);
                    m = springPeak.process (m);
                    m = revLp.process (m);
                    wl[i] = wr[i] = m;
                }

                juce::dsp::AudioBlock<float> wet (wetBuf.getArrayOfWritePointers(), 2, (size_t) n);
                reverb.process (juce::dsp::ProcessContextReplacing<float> (wet));

                float pk = 0.0f;
                for (int i = 0; i < n; ++i)
                {
                    const float rm = revMixSm.getNextValue() * 0.6f;
                    chanL[i] += wl[i] * rm;
                    chanR[i] += wr[i] * rm;
                    pk = juce::jmax (pk, std::abs (wl[i]), std::abs (wr[i]));
                }
                revIdle = (fSlot[SReverb].active() || pk > 1.0e-5f) ? 0 : revIdle + 1;
            }
            break;

        case SLooper:
        {
            if (clearReq.exchange (false)) looper.clear();
            if (undoReq.exchange (false)) looper.undo();

            looper.setQuantize (on (LoopQuant) ? beatsPerBar * 60.0 / bpm * sr : 0.0);

            if (on (LoopFollow) && hostKnown.load())
            {
                if (hostPlaying && ! lastHostPlaying && looper.hasLoop())
                {
                    looper.setMode (noir::Looper::Play);
                    lastLoopMode = noir::Looper::Play;
                    setParamValue (LoopMode, (float) noir::Looper::Play);
                }
                else if (! hostPlaying && lastHostPlaying)
                {
                    looper.setMode (noir::Looper::Stop);
                    lastLoopMode = noir::Looper::Stop;
                    setParamValue (LoopMode, (float) noir::Looper::Stop);
                }
                if (hostPlaying && hostHasPpq)
                    looper.syncTo (hostPpq * 60.0 / bpm + (double) chunkStart / sr);
            }

            const int mode = (int) v (LoopMode);
            if (mode != lastLoopMode) { looper.setMode (mode); lastLoopMode = mode; }

            if (fSlot[SLooper].active())
            {
                makeStereo();
                static const float speeds[3] = { 0.5f, 1.0f, 2.0f };
                const float spd = speeds[juce::jlimit (0, 2, (int) v (LoopSpeed))];
                const bool rev = on (LoopRev);
                for (int i = 0; i < n; ++i)
                {
                    float ol, orr;
                    looper.process (chanL[i], chanR[i], loopLevel, spd, rev, ol, orr);
                    const float k = fSlot[SLooper].next();
                    chanL[i] += (ol - chanL[i]) * k;
                    chanR[i] += (orr - chanR[i]) * k;
                }
            }
            break;
        }

        default: break;
    }
}

//==============================================================================
void NoirProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn  = getTotalNumInputChannels();
    const int numOut = getTotalNumOutputChannels();
    const int total  = buffer.getNumSamples();

    if (numIn == 0 || osDrive == nullptr)
    {
        buffer.clear();
        return;
    }

    handleMidi (midi);
    applyMacros();
    readHost();

    fPower.set (on (Power));
    const bool tunerOn = on (TunerOn);
    muteSm.setTargetValue (tunerOn ? 0.0f : 1.0f);

    float inPk = 0.0f, outPk = 0.0f;

    // ---- Power off: clean pass-through (with a short crossfade) -----------------------------------
    if (! fPower.active())
    {
        wasActive = false;
        const float* src = buffer.getReadPointer (0);
        for (int i = 0; i < total; ++i)
        {
            inPk = juce::jmax (inPk, std::abs (src[i]));
            if (tunerOn) tuner.push (src[i]);
        }
        for (int i = 0; i < total; ++i)
        {
            const float m = muteSm.getNextValue();
            buffer.getWritePointer (0)[i] *= m;
        }
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        {
            if (ch < numOut) buffer.copyFrom (ch, 0, buffer.getReadPointer (0), total);
            else buffer.clear (ch, 0, total);
        }
        outPk = tunerOn ? 0.0f : inPk;
        if (inPk  > inPeak.load())  inPeak.store (inPk);
        if (outPk > outPeak.load()) outPeak.store (outPk);
        return;
    }

    if (! wasActive)
    {
        resetEffects();
        wasActive = true;
    }

    updateParams();

    {
        const juce::SpinLock::ScopedTryLockType lock (orderLock);
        if (lock.isLocked()) audioOrder = sharedOrder;
    }

    // The master level is applied right after the later of Amp / Cab in the current order.
    int masterAfter = audioOrder[0];
    {
        int best = -1;
        for (int k = 0; k < NumSlots; ++k)
            if (audioOrder[(size_t) k] == SAmp || audioOrder[(size_t) k] == SCab) best = k;
        if (best >= 0) masterAfter = audioOrder[(size_t) best];
    }

    // ---- Audio ------------------------------------------------------------------------------------------
    for (int start = 0; start < total; start += maxBlock)
    {
        const int n = juce::jmin (maxBlock, total - start);
        blockN = n;
        chunkStart = start;
        chanL = chainBuf.getWritePointer (0);
        chanR = chainBuf.getWritePointer (1);
        stereoSig = false;

        float* rawp = rawBuf.getWritePointer (0);
        float* mic = micBuf.getWritePointer (0);

        // Guitar on input 1, microphone (for the vocoder) on input 2.
        const float* src = buffer.getReadPointer (0) + start;
        const float* src2 = numIn >= 2 ? buffer.getReadPointer (1) + start : nullptr;
        for (int i = 0; i < n; ++i)
        {
            rawp[i] = src[i];
            mic[i] = src2 != nullptr ? src2[i] : 0.0f;
            if (tunerOn) tuner.push (src[i]);
            const float x = src[i] * inGainSm.getNextValue();
            inPk = juce::jmax (inPk, std::abs (x));
            chanL[i] = x;
        }

        for (int k = 0; k < NumSlots; ++k)
        {
            const int slot = audioOrder[(size_t) k];
            runSlot (slot);
            if (slot == masterAfter) applyMaster();
        }

        // Output (with the Power crossfade against the clean input, and the tuner mute)
        const float* R = stereoSig ? chanR : chanL;
        float* outL = buffer.getWritePointer (0) + start;
        float* outR = numOut >= 2 ? buffer.getWritePointer (1) + start : nullptr;

        for (int i = 0; i < n; ++i)
        {
            const float g = outGainSm.getNextValue() * muteSm.getNextValue();
            const float p = fPower.next();
            const float a = noir::softLimit (chanL[i] * g);
            const float b = noir::softLimit (R[i] * g);
            const float dry = rawp[i] * g;
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

    lastHostPlaying = hostPlaying;
    loopProgress.store (looper.progress());
    loopHasData.store (looper.hasLoop());
    loopCanUndo.store (looper.canUndo());

    // Meters: the editor reads and resets these.
    if (inPk  > inPeak.load())  inPeak.store (inPk);
    if (outPk > outPeak.load()) outPeak.store (outPk);
}


//==============================================================================
// DAW integration
void NoirProcessor::readHost()
{
    bpm = on (HostSync) ? 0.0 : (double) v (BpmManual);
    hostPlaying = false;
    hostHasPpq = false;
    bool known = false;

    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            known = true;
            hostPlaying = pos->getIsPlaying();
            if (auto b = pos->getBpm(); b.hasValue() && *b > 20.0 && on (HostSync)) bpm = *b;
            if (auto q = pos->getPpqPosition(); q.hasValue()) { hostPpq = *q; hostHasPpq = true; }
            if (auto ts = pos->getTimeSignature(); ts.hasValue() && ts->denominator > 0)
                beatsPerBar = (double) ts->numerator * 4.0 / (double) ts->denominator;
        }
    }

    if (bpm < 20.0) bpm = (double) v (BpmManual);
    hostKnown.store (known);
    hostPlayingShown.store (hostPlaying);
    hostBpmShown.store ((float) bpm);
}

void NoirProcessor::setParamNormalised (int index, float norm01)
{
    if (index < 0 || index >= NumParams) return;
    if (auto* p = param ((Id) index))
        p->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, norm01));
}

void NoirProcessor::handleMidi (juce::MidiBuffer& midi)
{
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();

        if (m.isController())
        {
            const int cc = m.getControllerNumber();
            const float val = (float) m.getControllerValue() / 127.0f;

            const int learn = learnTarget.exchange (-1);
            if (learn >= 0)
            {
                for (auto& c : ccMap) if (c.load() == learn) c.store (-1);   // one CC per control
                ccMap[cc].store (learn);
                mapVersion.fetch_add (1);
            }

            const int target = ccMap[cc].load();
            if (target < 0) continue;

            const auto& info = kParams[target];
            if (info.kind == 1)      setParamNormalised (target, val >= 0.5f ? 1.0f : 0.0f);
            else if (info.kind == 2)
            {
                const int n = juce::StringArray::fromTokens (info.choices, "|", "").size();
                setParamNormalised (target, n > 1 ? std::round (val * (float) (n - 1)) / (float) (n - 1) : 0.0f);
            }
            else                     setParamNormalised (target, val);
        }
        else if (m.isProgramChange())
        {
            pendingPreset.store (m.getProgramChangeNumber());
            triggerAsyncUpdate();
        }
    }
    midi.clear();
}

void NoirProcessor::handleAsyncUpdate()
{
    const int p = pendingPreset.exchange (-1);
    if (p >= 0) applyPreset (*this, p);
}

void NoirProcessor::applyMacros()
{
    for (int m = 0; m < NumMacros; ++m)
    {
        const float mv = v ((Id) (Macro1 + m));
        if (std::abs (mv - lastMacro[m]) < 1.0e-4f) continue;
        lastMacro[m] = mv;

        for (auto& l : links[m])
        {
            const int p = l.param.load();
            if (p >= 0) setParamNormalised (p, l.lo.load() + (l.hi.load() - l.lo.load()) * mv);
        }
    }
}

int NoirProcessor::getCcFor (Id id) const
{
    for (int c = 0; c < 128; ++c) if (ccMap[c].load() == (int) id) return c;
    return -1;
}

void NoirProcessor::clearMidiFor (Id id)
{
    for (auto& c : ccMap) if (c.load() == (int) id) c.store (-1);
    mapVersion.fetch_add (1);
}

void NoirProcessor::clearAllMidi()
{
    for (auto& c : ccMap) c.store (-1);
    mapVersion.fetch_add (1);
}

std::array<int, 128> NoirProcessor::midiMapSnapshot() const
{
    std::array<int, 128> a {};
    for (int i = 0; i < 128; ++i) a[(size_t) i] = ccMap[i].load();
    return a;
}

bool NoirProcessor::assignMacro (int macro, Id param)
{
    if (macro < 0 || macro >= NumMacros || param >= Macro1) return false;
    int freeSlot = -1;
    for (int i = 0; i < LinksPerMacro; ++i)
    {
        const int p = links[macro][i].param.load();
        if (p == (int) param) return false;
        if (p < 0 && freeSlot < 0) freeSlot = i;
    }
    if (freeSlot < 0) return false;

    auto& l = links[macro][freeSlot];
    l.lo.store (0.0f); l.hi.store (1.0f);
    l.param.store ((int) param);
    mapVersion.fetch_add (1);
    return true;
}

void NoirProcessor::unassignMacro (int macro, int linkIndex)
{
    if (macro < 0 || macro >= NumMacros || linkIndex < 0 || linkIndex >= LinksPerMacro) return;
    links[macro][linkIndex].param.store (-1);
    mapVersion.fetch_add (1);
}

NoirProcessor::LinkInfo NoirProcessor::getLink (int macro, int linkIndex) const
{
    const auto& l = links[macro][linkIndex];
    return { l.param.load(), l.lo.load(), l.hi.load() };
}

void NoirProcessor::setLinkRange (int macro, int linkIndex, float lo, float hi)
{
    auto& l = links[macro][linkIndex];
    l.lo.store (juce::jlimit (0.0f, 1.0f, lo));
    l.hi.store (juce::jlimit (0.0f, 1.0f, hi));
}

//==============================================================================
void NoirProcessor::loadIR (const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    for (auto& c : convolution)
        c.loadImpulseResponse (file,
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

    juce::StringArray parts;
    for (int s : getChainOrder()) parts.add (juce::String (s));
    state.setProperty ("order", parts.joinIntoString (","), nullptr);

    juce::StringArray midiParts, macroParts;
    for (int c = 0; c < 128; ++c)
        if (ccMap[c].load() >= 0) midiParts.add (juce::String (c) + ":" + juce::String (ccMap[c].load()));
    for (int m = 0; m < NumMacros; ++m)
        for (int i = 0; i < LinksPerMacro; ++i)
        {
            const auto l = getLink (m, i);
            if (l.param >= 0)
                macroParts.add (juce::String (m) + ":" + juce::String (l.param) + ":" + juce::String (l.lo, 4) + ":" + juce::String (l.hi, 4));
        }
    state.setProperty ("midimap", midiParts.joinIntoString (","), nullptr);
    state.setProperty ("macros", macroParts.joinIntoString (","), nullptr);

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
            const auto orderStr = tree.getProperty ("order").toString();
            const auto midiStr = tree.getProperty ("midimap").toString();
            const auto macroStr = tree.getProperty ("macros").toString();
            apvts.replaceState (tree);

            for (auto& c : ccMap) c.store (-1);
            for (auto& row : links) for (auto& l : row) l.param.store (-1);
            for (const auto& item : juce::StringArray::fromTokens (midiStr, ",", ""))
            {
                const auto kv = juce::StringArray::fromTokens (item, ":", "");
                if (kv.size() == 2 && kv[0].getIntValue() >= 0 && kv[0].getIntValue() < 128
                    && kv[1].getIntValue() >= 0 && kv[1].getIntValue() < NumParams)
                    ccMap[kv[0].getIntValue()].store (kv[1].getIntValue());
            }
            for (const auto& item : juce::StringArray::fromTokens (macroStr, ",", ""))
            {
                const auto f = juce::StringArray::fromTokens (item, ":", "");
                if (f.size() == 4 && assignMacro (f[0].getIntValue(), (Id) f[1].getIntValue()))
                {
                    for (int i = 0; i < LinksPerMacro; ++i)
                        if (links[f[0].getIntValue()][i].param.load() == f[1].getIntValue())
                            setLinkRange (f[0].getIntValue(), i, f[2].getFloatValue(), f[3].getFloatValue());
                }
            }
            for (int m = 0; m < NumMacros; ++m) lastMacro[m] = v ((Id) (Macro1 + m));
            mapVersion.fetch_add (1);

            // never start a session recording or with the tuner muting
            setParamValue (LoopMode, 0.0f);
            setParamValue (TunerOn, 0.0f);

            Order o = defaultOrder();
            const auto toks = juce::StringArray::fromTokens (orderStr, ",", "");
            if (toks.size() == NumSlots)
                for (int i = 0; i < NumSlots; ++i) o[(size_t) i] = toks[i].getIntValue();
            setChainOrder (o);

            if (path.isNotEmpty())
                loadIR (juce::File (path));
        }
    }
}

juce::AudioProcessorEditor* NoirProcessor::createEditor() { return new NoirEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new NoirProcessor(); }
