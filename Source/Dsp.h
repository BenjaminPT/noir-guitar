#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>
#include <vector>

// Small, allocation-free DSP building blocks used by Noir Guitar.
namespace noir
{
inline constexpr double kPi = 3.14159265358979323846;

inline double clampFreq (double f, double sr) noexcept { return juce::jlimit (10.0, sr * 0.45, f); }

// Transposed direct form II biquad (RBJ cookbook), double precision state so that
// low-frequency filters stay clean at 4x oversampled rates.
struct Biquad
{
    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    double z1 = 0.0, z2 = 0.0;

    void reset() noexcept { z1 = z2 = 0.0; }

    inline float process (float in) noexcept
    {
        const double x = (double) in;
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }

    void set (double nb0, double nb1, double nb2, double a0, double na1, double na2) noexcept
    {
        const double inv = 1.0 / a0;
        b0 = nb0 * inv; b1 = nb1 * inv; b2 = nb2 * inv;
        a1 = na1 * inv; a2 = na2 * inv;
    }

    void lowpass (double sr, double f, double q) noexcept
    {
        const double w = 2.0 * kPi * clampFreq (f, sr) / sr;
        const double c = std::cos (w), s = std::sin (w), al = s / (2.0 * q);
        set ((1.0 - c) * 0.5, 1.0 - c, (1.0 - c) * 0.5, 1.0 + al, -2.0 * c, 1.0 - al);
    }

    void highpass (double sr, double f, double q) noexcept
    {
        const double w = 2.0 * kPi * clampFreq (f, sr) / sr;
        const double c = std::cos (w), s = std::sin (w), al = s / (2.0 * q);
        set ((1.0 + c) * 0.5, -(1.0 + c), (1.0 + c) * 0.5, 1.0 + al, -2.0 * c, 1.0 - al);
    }

    void bandpass (double sr, double f, double q) noexcept   // constant 0 dB peak gain
    {
        const double w = 2.0 * kPi * clampFreq (f, sr) / sr;
        const double c = std::cos (w), s = std::sin (w), al = s / (2.0 * q);
        set (al, 0.0, -al, 1.0 + al, -2.0 * c, 1.0 - al);
    }

    void peaking (double sr, double f, double q, double dB) noexcept
    {
        const double A = std::pow (10.0, dB / 40.0);
        const double w = 2.0 * kPi * clampFreq (f, sr) / sr;
        const double c = std::cos (w), s = std::sin (w), al = s / (2.0 * q);
        set (1.0 + al * A, -2.0 * c, 1.0 - al * A, 1.0 + al / A, -2.0 * c, 1.0 - al / A);
    }

    void lowShelf (double sr, double f, double dB) noexcept
    {
        const double A = std::pow (10.0, dB / 40.0);
        const double w = 2.0 * kPi * clampFreq (f, sr) / sr;
        const double c = std::cos (w), s = std::sin (w);
        const double tsa = 2.0 * std::sqrt (A) * (s * 0.5 * std::sqrt (2.0));
        set (A * ((A + 1.0) - (A - 1.0) * c + tsa),
             2.0 * A * ((A - 1.0) - (A + 1.0) * c),
             A * ((A + 1.0) - (A - 1.0) * c - tsa),
             (A + 1.0) + (A - 1.0) * c + tsa,
             -2.0 * ((A - 1.0) + (A + 1.0) * c),
             (A + 1.0) + (A - 1.0) * c - tsa);
    }

    void highShelf (double sr, double f, double dB) noexcept
    {
        const double A = std::pow (10.0, dB / 40.0);
        const double w = 2.0 * kPi * clampFreq (f, sr) / sr;
        const double c = std::cos (w), s = std::sin (w);
        const double tsa = 2.0 * std::sqrt (A) * (s * 0.5 * std::sqrt (2.0));
        set (A * ((A + 1.0) + (A - 1.0) * c + tsa),
             -2.0 * A * ((A - 1.0) + (A + 1.0) * c),
             A * ((A + 1.0) + (A - 1.0) * c - tsa),
             (A + 1.0) - (A - 1.0) * c + tsa,
             2.0 * ((A - 1.0) - (A + 1.0) * c),
             (A + 1.0) - (A - 1.0) * c - tsa);
    }
};

//==============================================================================
// Noise gate with hysteresis, hold and smoothed gain. Threshold <= -89.5 dB = off.
class NoiseGate
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        envRelease = (float) std::exp (-1.0 / (0.010 * sr));
        openCoeff  = 1.0f - (float) std::exp (-1.0 / (0.0007 * sr));
        closeCoeff = 1.0f - (float) std::exp (-1.0 / (0.060 * sr));
        holdSamples = (int) (0.040 * sr);
        reset();
    }

    void reset() noexcept { env = 0.0f; gain = 1.0f; open = true; hold = 0; }

    void setThresholdDb (float db) noexcept
    {
        off = db <= -89.5f;
        openThr = juce::Decibels::decibelsToGain (db);
        closeThr = juce::Decibels::decibelsToGain (db - 4.0f);
    }

    inline float process (float x) noexcept
    {
        if (off) return x;

        const float a = std::abs (x);
        env = a > env ? a : env * envRelease;

        if (env > openThr)       { open = true; hold = holdSamples; }
        else if (env > closeThr) { if (open) hold = holdSamples; }
        else if (hold > 0)       { --hold; }
        else                     { open = false; }

        const float target = open ? 1.0f : 0.0f;
        gain += (target - gain) * (target > gain ? openCoeff : closeCoeff);
        return x * gain;
    }

private:
    double sr = 44100.0;
    float envRelease = 0.99f, openCoeff = 0.1f, closeCoeff = 0.001f;
    float env = 0.0f, gain = 1.0f, openThr = 0.001f, closeThr = 0.0006f;
    int holdSamples = 1000, hold = 0;
    bool open = true, off = true;
};

//==============================================================================
// Two cascaded asymmetric triode-style gain stages, post-preamp tone stack, soft power stage.
// Runs at the oversampled rate.
class AmpStage
{
public:
    void prepare (double osRate)
    {
        sr = osRate;
        inHp.highpass (sr, 70.0, 0.707);
        c1.highpass (sr, 22.0, 0.707);
        c2.highpass (sr, 28.0, 0.707);
        lp1.lowpass (sr, 9500.0, 0.707);
        lp2.lowpass (sr, 11000.0, 0.707);
        g1Sm.reset (sr, 0.03);
        g1Sm.setCurrentAndTargetValue (1.0f);
        for (auto& v : last) v = -1000.0f;
        reset();
    }

    void reset() noexcept
    {
        inHp.reset(); c1.reset(); c2.reset(); lp1.reset(); lp2.reset();
        bass.reset(); mid.reset(); treble.reset(); pres.reset();
    }

    // all values 0..10
    void setParams (float gain, float bassV, float midV, float trebleV, float presenceV)
    {
        g1Sm.setTargetValue (juce::Decibels::decibelsToGain (-6.0f + gain * 4.8f));
        power = 1.0f + gain * 0.12f;

        if (changed (0, bassV))     bass.lowShelf (sr, 110.0, (bassV - 5.0) * 2.4);
        if (changed (1, midV))      mid.peaking (sr, 650.0, 0.7, (midV - 5.0) * 2.4);
        if (changed (2, trebleV))   treble.highShelf (sr, 3200.0, (trebleV - 5.0) * 2.4);
        if (changed (3, presenceV)) pres.highShelf (sr, 5200.0, presenceV * 0.9);
    }

    inline float process (float x) noexcept
    {
        x = inHp.process (x);
        const float g1 = g1Sm.getNextValue();

        float y = std::tanh (x * g1 + bias1) - tb1;
        y = lp1.process (c1.process (y));
        y = std::tanh (y * 3.0f + bias2) - tb2;
        y = lp2.process (c2.process (y)) * 0.8f;

        y = bass.process (y);
        y = mid.process (y);
        y = treble.process (y);
        y = pres.process (y);
        return std::tanh (y * power);
    }

private:
    bool changed (int i, float v) noexcept
    {
        if (std::abs (last[(size_t) i] - v) < 1.0e-4f) return false;
        last[(size_t) i] = v;
        return true;
    }

    double sr = 44100.0;
    Biquad inHp, c1, c2, lp1, lp2, bass, mid, treble, pres;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> g1Sm;
    float power = 1.0f;
    float last[4] {};
    const float bias1 = 0.15f, bias2 = 0.10f;
    const float tb1 = std::tanh (0.15f), tb2 = std::tanh (0.10f);
};

//==============================================================================
// Procedural speaker cabinet voicings (not modelled on any specific product).
class Cabinet
{
public:
    struct Voicing { double hp, bumpF, bumpDb, dipF, dipDb, peakF, peakDb, lpF; };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        dc.highpass (sr, 25.0, 0.707);
        current = -1;
        setVoicing (1);
        reset();
    }

    void reset() noexcept
    {
        dc.reset(); hp.reset(); bump.reset(); dip.reset(); peak.reset(); lp1.reset(); lp2.reset();
    }

    void setVoicing (int v)
    {
        static const Voicing table[3] = {
            //  hp    bumpF bumpDb  dipF  dipDb peakF peakDb  lpF
            {  90.0, 120.0,  2.5,  420.0, -1.5, 2400.0, 3.5, 5600.0 },   // 1x12 open
            {  80.0, 105.0,  3.0,  350.0, -2.0, 2800.0, 3.0, 5200.0 },   // 2x12 combo
            {  70.0,  95.0,  4.0,  500.0, -2.5, 3200.0, 2.5, 4700.0 }    // 4x12 closed
        };

        v = juce::jlimit (0, 2, v);
        if (v == current) return;
        current = v;
        const auto& t = table[v];
        hp.highpass (sr, t.hp, 0.707);
        bump.peaking (sr, t.bumpF, 0.9, t.bumpDb);
        dip.peaking (sr, t.dipF, 1.0, t.dipDb);
        peak.peaking (sr, t.peakF, 1.1, t.peakDb);
        lp1.lowpass (sr, t.lpF, 0.5412);   // 4th order Butterworth split into two biquads
        lp2.lowpass (sr, t.lpF, 1.3066);
    }

    inline float processCab (float x) noexcept
    {
        x = hp.process (x);
        x = bump.process (x);
        x = dip.process (x);
        x = peak.process (x);
        x = lp1.process (x);
        return lp2.process (x);
    }

    inline float processDc (float x) noexcept { return dc.process (x); }

private:
    double sr = 44100.0;
    int current = -1;
    Biquad dc, hp, bump, dip, peak, lp1, lp2;
};

//==============================================================================
// Simple delay line with linear interpolation, power-of-two length.
class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        int n = 1;
        while (n < maxSamples + 8) n <<= 1;
        buf.assign ((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }

    void clear() noexcept { std::fill (buf.begin(), buf.end(), 0.0f); }

    inline void write (float x) noexcept { buf[(size_t) w] = x; w = (w + 1) & mask; }

    // d = 0 returns the most recently written sample
    inline float read (double d) const noexcept
    {
        const double pos = (double) (w - 1) - d;
        const double fl = std::floor (pos);
        const float frac = (float) (pos - fl);
        const int i0 = (int) fl;
        const float a = buf[(size_t) (i0 & mask)];
        const float b = buf[(size_t) ((i0 + 1) & mask)];
        return a + frac * (b - a);
    }

private:
    std::vector<float> buf;
    int mask = 0, w = 0;
};

// Gentle safety limiter so a wrong knob never hurts anyone's ears.
inline float softLimit (float x) noexcept
{
    const float a = std::abs (x);
    if (a <= 0.9f) return x;
    const float y = 0.9f + 0.1f * std::tanh ((a - 0.9f) * 10.0f);
    return x < 0.0f ? -y : y;
}
} // namespace noir
