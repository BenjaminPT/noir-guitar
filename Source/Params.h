#pragma once

#include <juce_core/juce_core.h>
#include <iterator>

// Single source of truth for every parameter: the processor, the editor and the presets all use this table.
enum Fmt { Num, Db, Pct, Ms, Gate, Semi };

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
    PF ("master","Master", -30.0f, 6.0f, -10.0f, Db),

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
    PC ("drv_type", "Drive Type", "Overdrive|Distortion|Fuzz", 0.0f),
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
    PC ("dly_type", "Delay Type", "Digital|Analog|Tape|Slapback|Ping-Pong", 2.0f),
    { "dly_time", "Time", 0, 40.0f, 1000.0f, 380.0f, nullptr, Ms, 0.5f },
    PF ("dly_fb", "Repeats", 0.0f, 0.9f, 0.3f, Pct),
    PF ("dly_mix", "Mix", 0.0f, 1.0f, 0.3f, Pct),

    PB ("rev_on", "Reverb On", 0.0f),
    PC ("rev_type", "Reverb Type", "Room|Hall|Plate|Spring|Shimmer", 1.0f),
    PF ("rev_size", "Size", 0.0f, 1.0f, 0.5f, Pct),
    PF ("rev_mix", "Mix", 0.0f, 1.0f, 0.3f, Pct),
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
        case Gate: return v <= -89.5f ? juce::String ("Off") : juce::String (v, 0) + " dB";
        default:   return juce::String (v, 1);
    }
}
