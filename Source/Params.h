#pragma once

#include <juce_core/juce_core.h>
#include <iterator>

// Single source of truth for every parameter: the processor, the editor and the presets all use this table.
enum Fmt { Num, Db, Pct, Ms, Gate, Semi, Hz, Bpm };

enum Id
{
    Power, InGain, OutGain, Master,
    GateOn, GateThr,
    CompOn, CompAmt, CompLvl, CompAtk,
    WahOn, WahMode, WahPos, WahQ,
    PitchOn, PitchType, PitchShift, PitchMix,
    BoostOn, BoostType, BoostGain,
    DrvOn, DrvType, DrvDrive, DrvTone, DrvLevel,
    AmpOn, AmpGain, AmpBass, AmpMid, AmpTreble, AmpPres,
    CabOn, CabType,
    EqOn, EqLow, EqLowMid, EqHiMid, EqHigh,
    M1On, M1Type, M1Rate, M1Depth, M1Mix,
    M2On, M2Type, M2Rate, M2Depth, M2Mix,
    LoOn, LoBits, LoRate, LoMix,
    DlyOn, DlyType, DlyTime, DlyFb, DlyMix,
    RevOn, RevType, RevSize, RevMix,
    LoopOn, LoopMode, LoopLevel, LoopSpeed, LoopRev,
    VocOn, VocSens, VocBright, VocMix,
    TbOn, TbVowel, TbAuto, TbRes, TbMix,
    GrOn, GrSize, GrDens, GrPitch, GrSpread, GrMix,
    FrzOn, FrzSize, FrzMix,
    GltOn, GltMode, GltLen, GltAmt, GltMix,
    TunerOn, TunerRef,
    BpmManual, HostSync, DlySync, M1Sync, M2Sync, GltSync, LoopFollow, LoopQuant, LoopTrack, LoopMute1, LoopMute2, LoopMute3, LoopMute4,
    Macro1, Macro2, Macro3, Macro4, Macro5, Macro6, Macro7, Macro8,
    NumParams
};

struct ParamInfo
{
    const char* id;
    const char* name;
    int kind;            // 0 float, 1 bool, 2 choice
    float lo, hi, def;
    const char* choices; // "A|B|C"
    Fmt fmt;
    float skew;
};

#define PF(id, name, lo, hi, def, fmt) { id, name, 0, lo, hi, def, nullptr, fmt, 1.0f }
#define PB(id, name, def)              { id, name, 1, 0.0f, 1.0f, def, nullptr, Num, 1.0f }
#define PC(id, name, choices, def)     { id, name, 2, 0.0f, 0.0f, def, choices, Num, 1.0f }

inline const ParamInfo kParams[NumParams] = {
    PB ("power", "Power", 1.0f),
    PF ("in",    "Input",  -24.0f, 24.0f, 0.0f, Db),
    PF ("out",   "Output", -24.0f, 12.0f, 0.0f, Db),
    PF ("master","Master", -30.0f, 6.0f, -8.0f, Db),

    PB ("gate_on", "Gate On", 1.0f),
    PF ("gate_thr", "Threshold", -90.0f, -30.0f, -72.0f, Gate),

    PB ("comp_on", "Comp On", 0.0f),
    PF ("comp_amt", "Sustain", 0.0f, 1.0f, 0.5f, Pct),
    PF ("comp_lvl", "Level", -12.0f, 12.0f, 0.0f, Db),
    PF ("comp_atk", "Attack", 0.0f, 1.0f, 0.3f, Pct),

    PB ("wah_on", "Wah On", 0.0f),
    PC ("wah_mode", "Wah Mode", "Wah|Auto Wah", 0.0f),
    PF ("wah_pos", "Position", 0.0f, 1.0f, 0.45f, Pct),
    PF ("wah_q", "Resonance", 0.0f, 1.0f, 0.5f, Pct),

    PB ("pitch_on", "Pitch On", 0.0f),
    PC ("pitch_type", "Pitch Type", "Shifter|Harmonizer|Octaver|Detune|Whammy", 1.0f),
    PF ("pitch_shift", "Shift", -12.0f, 12.0f, 7.0f, Semi),
    PF ("pitch_mix", "Mix", 0.0f, 1.0f, 0.6f, Pct),

    PB ("boost_on", "Boost On", 0.0f),
    PC ("boost_type", "Boost Type", "Clean|Mid|Treble|Bass", 0.0f),
    PF ("boost_gain", "Gain", 0.0f, 20.0f, 6.0f, Db),

    PB ("drv_on", "Drive On", 1.0f),
    PC ("drv_type", "Drive Type", "Overdrive|Distortion|Fuzz|Big Muff|Metal|Octave Fuzz", 0.0f),
    PF ("drv_drive", "Drive", 0.0f, 1.0f, 0.35f, Pct),
    PF ("drv_tone", "Tone", 0.0f, 1.0f, 0.5f, Pct),
    PF ("drv_level", "Level", -12.0f, 12.0f, 0.0f, Db),

    PB ("amp_on", "Amp On", 1.0f),
    PF ("amp_gain", "Gain", 0.0f, 10.0f, 5.5f, Num),
    PF ("amp_bass", "Bass", 0.0f, 10.0f, 5.0f, Num),
    PF ("amp_mid", "Mid", 0.0f, 10.0f, 5.0f, Num),
    PF ("amp_treble", "Treble", 0.0f, 10.0f, 5.5f, Num),
    PF ("amp_pres", "Presence", 0.0f, 10.0f, 4.0f, Num),

    PB ("cab_on", "Cab On", 1.0f),
    PC ("cab_type", "Cab", "1x12 Open|2x12 Combo|4x12 Closed", 1.0f),

    PB ("eq_on", "EQ On", 0.0f),
    PF ("eq_low", "Low", -12.0f, 12.0f, 0.0f, Db),
    PF ("eq_lmid", "Low Mid", -12.0f, 12.0f, 0.0f, Db),
    PF ("eq_hmid", "High Mid", -12.0f, 12.0f, 0.0f, Db),
    PF ("eq_high", "High", -12.0f, 12.0f, 0.0f, Db),

    PB ("m1_on", "Mod 1 On", 0.0f),
    PC ("m1_type", "Mod 1 Type", "Chorus|Flanger|Phaser|Tremolo|Vibrato|Rotary|Ring Mod", 0.0f),
    PF ("m1_rate", "Rate", 0.0f, 1.0f, 0.3f, Pct),
    PF ("m1_depth", "Depth", 0.0f, 1.0f, 0.5f, Pct),
    PF ("m1_mix", "Mix", 0.0f, 1.0f, 1.0f, Pct),

    PB ("m2_on", "Mod 2 On", 0.0f),
    PC ("m2_type", "Mod 2 Type", "Chorus|Flanger|Phaser|Tremolo|Vibrato|Rotary|Ring Mod", 2.0f),
    PF ("m2_rate", "Rate", 0.0f, 1.0f, 0.3f, Pct),
    PF ("m2_depth", "Depth", 0.0f, 1.0f, 0.6f, Pct),
    PF ("m2_mix", "Mix", 0.0f, 1.0f, 1.0f, Pct),

    PB ("lo_on", "Lo-Fi On", 0.0f),
    PF ("lo_bits", "Bits", 0.0f, 1.0f, 0.5f, Pct),
    PF ("lo_rate", "Rate", 0.0f, 1.0f, 0.3f, Pct),
    PF ("lo_mix", "Mix", 0.0f, 1.0f, 1.0f, Pct),

    PB ("dly_on", "Delay On", 0.0f),
    PC ("dly_type", "Delay Type", "Digital|Analog|Tape|Slapback|Ping-Pong|Reverse|Multi-Tap", 2.0f),
    { "dly_time", "Time", 0, 40.0f, 1000.0f, 380.0f, nullptr, Ms, 0.5f },
    PF ("dly_fb", "Repeats", 0.0f, 0.9f, 0.3f, Pct),
    PF ("dly_mix", "Mix", 0.0f, 1.0f, 0.3f, Pct),

    PB ("rev_on", "Reverb On", 0.0f),
    PC ("rev_type", "Reverb Type", "Room|Hall|Plate|Spring|Shimmer", 1.0f),
    PF ("rev_size", "Size", 0.0f, 1.0f, 0.5f, Pct),
    PF ("rev_mix", "Mix", 0.0f, 1.0f, 0.3f, Pct),

    PB ("loop_on", "Looper On", 0.0f),
    PC ("loop_mode", "Looper Mode", "Stop|Record|Play|Overdub", 0.0f),
    PF ("loop_level", "Level", -12.0f, 6.0f, 0.0f, Db),
    PC ("loop_speed", "Looper Speed", "Half|Normal|Double", 1.0f),
    PB ("loop_rev", "Reverse", 0.0f),

    PB ("voc_on", "Vocoder On", 0.0f),
    PF ("voc_sens", "Sens", 0.0f, 1.0f, 0.5f, Pct),
    PF ("voc_bright", "Sibilance", 0.0f, 1.0f, 0.4f, Pct),
    PF ("voc_mix", "Mix", 0.0f, 1.0f, 1.0f, Pct),

    PB ("tb_on", "Talk Box On", 0.0f),
    PF ("tb_vowel", "Vowel", 0.0f, 1.0f, 0.3f, Pct),
    PF ("tb_auto", "Auto", 0.0f, 1.0f, 0.0f, Pct),
    PF ("tb_res", "Resonance", 0.0f, 1.0f, 0.5f, Pct),
    PF ("tb_mix", "Mix", 0.0f, 1.0f, 1.0f, Pct),

    PB ("gr_on", "Granular On", 0.0f),
    { "gr_size", "Size", 0, 20.0f, 400.0f, 120.0f, nullptr, Ms, 0.5f },
    PF ("gr_dens", "Density", 0.0f, 1.0f, 0.5f, Pct),
    PF ("gr_pitch", "Pitch", -12.0f, 12.0f, 0.0f, Semi),
    PF ("gr_spread", "Scatter", 0.0f, 1.0f, 0.3f, Pct),
    PF ("gr_mix", "Mix", 0.0f, 1.0f, 0.5f, Pct),

    PB ("frz_on", "Freeze On", 0.0f),
    { "frz_size", "Size", 0, 60.0f, 400.0f, 150.0f, nullptr, Ms, 0.5f },
    PF ("frz_mix", "Level", 0.0f, 1.0f, 0.8f, Pct),

    PB ("glt_on", "Glitch On", 0.0f),
    PC ("glt_mode", "Glitch Mode", "Stutter|Glitch", 0.0f),
    { "glt_len", "Length", 0, 20.0f, 500.0f, 125.0f, nullptr, Ms, 0.5f },
    PF ("glt_amt", "Amount", 0.0f, 1.0f, 0.5f, Pct),
    PF ("glt_mix", "Mix", 0.0f, 1.0f, 1.0f, Pct),

    PB ("tuner_on", "Tuner On", 0.0f),
    PF ("tuner_ref", "A4", 430.0f, 450.0f, 440.0f, Hz),

    PF ("bpm", "Manual BPM", 40.0f, 240.0f, 120.0f, Bpm),
    PB ("host_sync", "Follow Host Tempo", 1.0f),
    PC ("dly_sync", "Delay Sync", "Free|1/2|1/4|1/4.|1/4T|1/8|1/8.|1/8T|1/16|1/16.|1/16T|1/32", 0.0f),
    PC ("m1_sync", "Mod 1 Sync", "Free|1/2|1/4|1/4.|1/4T|1/8|1/8.|1/8T|1/16|1/16.|1/16T|1/32", 0.0f),
    PC ("m2_sync", "Mod 2 Sync", "Free|1/2|1/4|1/4.|1/4T|1/8|1/8.|1/8T|1/16|1/16.|1/16T|1/32", 0.0f),
    PC ("glt_sync", "Glitch Sync", "Free|1/2|1/4|1/4.|1/4T|1/8|1/8.|1/8T|1/16|1/16.|1/16T|1/32", 0.0f),
    PB ("loop_follow", "Follow DAW Transport", 0.0f),
    PB ("loop_quant", "Quantize to Bars", 0.0f),
    PC ("loop_track", "Looper Track", "Track 1|Track 2|Track 3|Track 4", 0.0f),
    PB ("loop_m1", "Looper Mute 1", 0.0f),
    PB ("loop_m2", "Looper Mute 2", 0.0f),
    PB ("loop_m3", "Looper Mute 3", 0.0f),
    PB ("loop_m4", "Looper Mute 4", 0.0f),

    PF ("macro1", "Macro 1", 0.0f, 1.0f, 0.0f, Pct),
    PF ("macro2", "Macro 2", 0.0f, 1.0f, 0.0f, Pct),
    PF ("macro3", "Macro 3", 0.0f, 1.0f, 0.0f, Pct),
    PF ("macro4", "Macro 4", 0.0f, 1.0f, 0.0f, Pct),
    PF ("macro5", "Macro 5", 0.0f, 1.0f, 0.0f, Pct),
    PF ("macro6", "Macro 6", 0.0f, 1.0f, 0.0f, Pct),
    PF ("macro7", "Macro 7", 0.0f, 1.0f, 0.0f, Pct),
    PF ("macro8", "Macro 8", 0.0f, 1.0f, 0.0f, Pct),
};

#undef PF
#undef PB
#undef PC

static_assert (std::size (kParams) == NumParams, "parameter table out of sync with enum");

inline const char* paramId (Id i) { return kParams[i].id; }

inline juce::String formatParam (Fmt f, float v)
{
    switch (f)
    {
        case Db:   return juce::String (v, 1) + " dB";
        case Pct:  return juce::String (juce::roundToInt (v * 100.0f)) + "%";
        case Ms:   return juce::String (juce::roundToInt (v)) + " ms";
        case Semi: return (v > 0 ? "+" : "") + juce::String (v, 1) + " st";
        case Hz:   return juce::String (v, 0) + " Hz";
        case Bpm:  return juce::String (v, 1) + " BPM";
        case Gate: return v <= -89.5f ? juce::String ("Off") : juce::String (v, 0) + " dB";
        default:   return juce::String (v, 1);
    }
}

//==============================================================================
// Pedals that take part in the reorderable signal chain (default order = enum order below).
enum Slot
{
    SGate, SComp, SWah, SPitch, SBoost, SDrive, SAmp, SCab, SEq, STalk, SVoc,
    SMod1, SMod2, SLofi, SGlitch, SGran, SFreeze, SDelay, SReverb, SLooper,
    NumSlots
};

inline constexpr Id kSlotOn[NumSlots] = { GateOn, CompOn, WahOn, PitchOn, BoostOn, DrvOn, AmpOn, CabOn, EqOn, TbOn, VocOn,
                                          M1On, M2On, LoOn, GltOn, GrOn, FrzOn, DlyOn, RevOn, LoopOn };

inline constexpr const char* kSlotShort[NumSlots] = { "Gate", "Comp", "Wah", "Pitch", "Boost", "Drive", "Amp", "Cab", "EQ", "Talk", "Voc",
                                                       "Mod 1", "Mod 2", "Lo-Fi", "Glitch", "Grain", "Freeze", "Delay", "Reverb", "Loop" };

// Number of beats (quarter notes) for each entry of the "Sync" choice lists (0 = free running).
inline double syncBeats (int choice)
{
    static const double b[12] = { 0.0, 2.0, 1.0, 1.5, 2.0 / 3.0, 0.5, 0.75, 1.0 / 3.0, 0.25, 0.375, 1.0 / 6.0, 0.125 };
    return b[juce::jlimit (0, 11, choice)];
}

// Human-readable label that includes the pedal name, e.g. "Drive / Tone".
inline juce::String paramLabel (int i)
{
    struct R { Id first, last; const char* name; };
    static const R ranges[] = {
        { Power, Master, "Global" },   { GateOn, GateThr, "Gate" },    { CompOn, CompAtk, "Comp" },
        { WahOn, WahQ, "Wah" },        { PitchOn, PitchMix, "Pitch" }, { BoostOn, BoostGain, "Boost" },
        { DrvOn, DrvLevel, "Drive" },  { AmpOn, AmpPres, "Amp" },      { CabOn, CabType, "Cab" },
        { EqOn, EqHigh, "EQ" },        { M1On, M1Mix, "Mod 1" },       { M2On, M2Mix, "Mod 2" },
        { LoOn, LoMix, "Lo-Fi" },      { DlyOn, DlyMix, "Delay" },     { RevOn, RevMix, "Reverb" },
        { LoopOn, LoopRev, "Looper" }, { VocOn, VocMix, "Vocoder" },   { TbOn, TbMix, "Talk Box" },
        { GrOn, GrMix, "Granular" },   { FrzOn, FrzMix, "Freeze" },    { GltOn, GltMix, "Glitch" },
        { TunerOn, TunerRef, "Tuner" }, { LoopTrack, LoopMute4, "Looper" }, { BpmManual, Macro8, "DAW" } };

    for (const auto& r : ranges)
        if (i >= (int) r.first && i <= (int) r.last)
            return juce::String (r.name) + " / " + kParams[i].name;
    return kParams[i].name;
}
