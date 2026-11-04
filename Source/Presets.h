#pragma once

#include <utility>
#include <vector>
#include "PluginProcessor.h"

// Factory chains. Every preset starts from the defaults, then applies its overrides.
// Choice values are indices (see the "choices" strings in Params.h).
struct PresetDef
{
    const char* name;
    std::vector<std::pair<Id, float>> values;
};

inline const std::vector<PresetDef>& factoryPresets()
{
    static const std::vector<PresetDef> list = {
        { "Init  -  Amp only", {} },

        { "Clean Sparkle", {
            { DrvOn, 0 }, { AmpGain, 2.5f }, { AmpTreble, 6.5f },
            { CompOn, 1 }, { CompAmt, 0.4f },
            { M1On, 1 }, { M1Type, 0 }, { M1Rate, 0.25f }, { M1Depth, 0.4f }, { M1Mix, 0.6f },
            { RevOn, 1 }, { RevType, 1 }, { RevMix, 0.25f } } },

        { "Blues Crunch", {
            { DrvOn, 1 }, { DrvType, 0 }, { DrvDrive, 0.3f }, { AmpGain, 4.5f },
            { DlyOn, 1 }, { DlyType, 1 }, { DlyTime, 320 }, { DlyFb, 0.25f }, { DlyMix, 0.2f },
            { RevOn, 1 }, { RevType, 2 }, { RevMix, 0.2f } } },

        { "Classic Rock", {
            { DrvType, 1 }, { DrvDrive, 0.4f }, { DrvTone, 0.55f }, { AmpGain, 6.0f },
            { CabType, 1 }, { RevOn, 1 }, { RevType, 0 }, { RevMix, 0.15f } } },

        { "Modern Metal", {
            { GateThr, -55 }, { BoostOn, 1 }, { BoostType, 1 }, { BoostGain, 8 },
            { DrvType, 4 }, { DrvDrive, 0.55f }, { DrvTone, 0.55f }, { DrvLevel, 0 },
            { AmpGain, 6.5f }, { AmpMid, 4 }, { AmpTreble, 6 }, { AmpPres, 6 }, { CabType, 2 },
            { EqOn, 1 }, { Eb1On, 1 }, { Eb1Gain, 2 }, { Eb6On, 1 }, { Eb6Gain, 2 } } },

        { "Fuzz Face 60s", {
            { DrvType, 2 }, { DrvDrive, 0.7f }, { DrvTone, 0.4f }, { AmpGain, 4 }, { CabType, 0 },
            { M1On, 1 }, { M1Type, 3 }, { M1Rate, 0.35f }, { M1Depth, 0.5f },
            { RevOn, 1 }, { RevType, 3 }, { RevMix, 0.35f } } },

        { "Big Muff Sustain", {
            { DrvType, 3 }, { DrvDrive, 0.55f }, { DrvTone, 0.5f }, { AmpGain, 4 }, { CabType, 1 },
            { DlyOn, 1 }, { DlyType, 2 }, { DlyMix, 0.2f }, { RevOn, 1 }, { RevMix, 0.2f } } },

        { "Singing Lead", {
            { CompOn, 1 }, { CompAmt, 0.4f }, { DrvType, 0 }, { DrvDrive, 0.55f }, { DrvTone, 0.6f },
            { AmpGain, 5.5f }, { AmpMid, 6 }, { CabType, 1 },
            { DlyOn, 1 }, { DlyType, 2 }, { DlyTime, 420 }, { DlyFb, 0.3f }, { DlyMix, 0.25f },
            { RevOn, 1 }, { RevType, 1 }, { RevMix, 0.22f } } },

        { "Funk Wah", {
            { DrvOn, 0 }, { AmpGain, 3.5f }, { CabType, 1 },
            { CompOn, 1 }, { CompAmt, 0.6f },
            { WahOn, 1 }, { WahMode, 1 }, { WahPos, 0.5f }, { WahQ, 0.6f } } },

        { "Psychedelic", {
            { DrvType, 2 }, { DrvDrive, 0.5f },
            { M1On, 1 }, { M1Type, 2 }, { M1Rate, 0.15f }, { M1Depth, 0.8f },
            { M2On, 1 }, { M2Type, 5 }, { M2Rate, 0.3f }, { M2Depth, 0.6f }, { M2Mix, 0.7f },
            { DlyOn, 1 }, { DlyType, 2 }, { DlyFb, 0.45f }, { DlyMix, 0.3f },
            { RevOn, 1 }, { RevType, 1 }, { RevMix, 0.3f } } },

        { "Shoegaze Wash", {
            { DrvType, 3 }, { DrvDrive, 0.6f }, { DrvTone, 0.45f }, { AmpGain, 4.5f },
            { M1On, 1 }, { M1Type, 0 }, { M1Rate, 0.2f }, { M1Depth, 0.8f },
            { PitchOn, 1 }, { PitchType, 1 }, { PitchShift, 12 }, { PitchMix, 0.35f },
            { DlyOn, 1 }, { DlyType, 0 }, { DlyTime, 450 }, { DlyFb, 0.5f }, { DlyMix, 0.35f },
            { RevOn, 1 }, { RevType, 4 }, { RevSize, 0.8f }, { RevMix, 0.6f } } },

        { "Ambient Shimmer", {
            { DrvOn, 0 }, { AmpGain, 2 }, { CompOn, 1 }, { CompAmt, 0.5f },
            { DlyOn, 1 }, { DlyType, 4 }, { DlyTime, 520 }, { DlyFb, 0.5f }, { DlyMix, 0.4f },
            { RevOn, 1 }, { RevType, 4 }, { RevSize, 0.9f }, { RevMix, 0.6f } } },

        { "Surf Twang", {
            { DrvOn, 0 }, { AmpGain, 3 }, { AmpTreble, 7 }, { AmpPres, 6 }, { CabType, 0 },
            { M1On, 1 }, { M1Type, 3 }, { M1Rate, 0.45f }, { M1Depth, 0.5f },
            { RevOn, 1 }, { RevType, 3 }, { RevSize, 0.6f }, { RevMix, 0.6f } } },

        { "Slapback Country", {
            { DrvOn, 0 }, { AmpGain, 3 }, { AmpTreble, 6.5f }, { CompOn, 1 }, { CompAmt, 0.5f },
            { DlyOn, 1 }, { DlyType, 3 }, { DlyTime, 500 }, { DlyFb, 0.1f }, { DlyMix, 0.3f } } },

        { "Lo-Fi Radio", {
            { DrvDrive, 0.3f }, { LoOn, 1 }, { LoBits, 0.55f }, { LoRate, 0.35f }, { LoMix, 0.6f },
            { EqOn, 1 }, { Eb1On, 1 }, { Eb1Gain, -6 }, { Eb6On, 1 }, { Eb6Gain, 4 },
            { RevOn, 1 }, { RevType, 0 }, { RevMix, 0.2f } } },

        { "Octave Fuzz Lead", {
            { PitchOn, 1 }, { PitchType, 2 }, { PitchMix, 0.5f },
            { DrvType, 5 }, { DrvDrive, 0.6f },
            { DlyOn, 1 }, { DlyType, 1 }, { DlyMix, 0.25f } } },

        { "Granular Clouds", {
            { DrvOn, 0 }, { AmpGain, 2 },
            { GrOn, 1 }, { GrSize, 160 }, { GrDens, 0.45f }, { GrSpread, 0.6f }, { GrMix, 0.7f },
            { RevOn, 1 }, { RevType, 4 }, { RevSize, 0.8f }, { RevMix, 0.45f } } },

        { "Talk Box Lead", {
            { DrvType, 1 }, { DrvDrive, 0.5f }, { AmpGain, 6.5f },
            { TbOn, 1 }, { TbVowel, 0.3f }, { TbAuto, 0.35f }, { TbRes, 0.6f },
            { DlyOn, 1 }, { DlyType, 1 }, { DlyMix, 0.2f } } },

        { "Stutter Glitch", {
            { DrvDrive, 0.3f }, { GltOn, 1 }, { GltMode, 0 }, { GltLen, 125 }, { GltAmt, 0.5f },
            { RevOn, 1 }, { RevType, 2 }, { RevMix, 0.15f } } },

        { "Looper Jam", {
            { DrvOn, 0 }, { AmpGain, 3 }, { LoopOn, 1 },
            { DlyOn, 1 }, { DlyType, 2 }, { DlyMix, 0.2f },
            { RevOn, 1 }, { RevType, 1 }, { RevMix, 0.25f } } },

        { "Reverse Dream", {
            { DrvOn, 0 }, { AmpGain, 3 },
            { DlyOn, 1 }, { DlyType, 5 }, { DlyTime, 600 }, { DlyFb, 0.4f }, { DlyMix, 0.45f },
            { RevOn, 1 }, { RevType, 1 }, { RevMix, 0.3f } } },

        { "Multi-Tap Rhythm", {
            { DrvDrive, 0.25f }, { AmpGain, 4.5f },
            { DlyOn, 1 }, { DlyType, 6 }, { DlyTime, 400 }, { DlyFb, 0.3f }, { DlyMix, 0.4f } } },
    };
    return list;
}

inline void applyPreset (NoirProcessor& proc, int index)
{
    const auto& list = factoryPresets();
    if (index < 0 || index >= (int) list.size())
        return;

    proc.resetAllParams();
    proc.setChainOrder (NoirProcessor::defaultOrder());
    for (const auto& kv : list[(size_t) index].values)
        proc.setParamValue (kv.first, kv.second);
}
