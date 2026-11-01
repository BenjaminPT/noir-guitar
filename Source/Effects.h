#pragma once

#include "Dsp.h"

// Additional effects for Noir Guitar. All are original, allocation-free in the audio path.
namespace noir
{
// Smoothed on/off crossfader used for click-free bypass of every pedal.
struct Fader
{
    juce::SmoothedValue<float> sm;
    void prepare (double sr, double seconds = 0.010) { sm.reset (sr, seconds); sm.setCurrentAndTargetValue (0.0f); }
    void set (bool on) { sm.setTargetValue (on ? 1.0f : 0.0f); }
    void snap (bool on) { sm.setCurrentAndTargetValue (on ? 1.0f : 0.0f); }
    float next() { return sm.getNextValue(); }
    bool active() const { return sm.isSmoothing() || sm.getTargetValue() > 0.5f; }
};

inline float hardish (float y) noexcept   // smooth-knee clipper approaching a hard clip
{
    const float y2 = y * y;
    return y / std::sqrt (std::sqrt (1.0f + y2 * y2));
}

//==============================================================================
class Compressor
{
public:
    void prepare (double sampleRate) { sr = sampleRate; env = 0.0f; setParams (0.5f, 0.0f, 0.3f); }

    void setParams (float amount, float levelDb, float attack01)
    {
        thresholdDb = -8.0f - amount * 32.0f;
        const float autoMakeup = -thresholdDb * (1.0f - 1.0f / ratio) * 0.3f;
        makeup = juce::Decibels::decibelsToGain (autoMakeup + levelDb);
        const double attackMs = 0.5 + attack01 * 30.0;
        atkC = (float) std::exp (-1.0 / (0.001 * attackMs * sr));
        relC = (float) std::exp (-1.0 / (0.180 * sr));
    }

    inline float process (float x) noexcept
    {
        const float a = std::abs (x);
        env = a + (a > env ? atkC : relC) * (env - a);
        const float db = juce::Decibels::gainToDecibels (env, -100.0f);
        const float over = db - thresholdDb;
        const float gr = over > 0.0f ? over * (1.0f - 1.0f / ratio) : 0.0f;
        return x * juce::Decibels::decibelsToGain (-gr) * makeup;
    }

private:
    double sr = 44100.0;
    float env = 0.0f, atkC = 0.99f, relC = 0.999f, thresholdDb = -20.0f, makeup = 1.0f;
    const float ratio = 4.0f;
};

//==============================================================================
// Wah (manual position) / Auto-wah (envelope filter) built on a TPT state variable filter.
class Wah
{
public:
    void prepare (double sampleRate) { sr = sampleRate; ic1 = ic2 = 0.0f; env = 0.0f; ps = 0.4f; }
    void reset() noexcept { ic1 = ic2 = 0.0f; env = 0.0f; }

    void setParams (bool autoMode, float pos01, float q01)
    {
        isAuto = autoMode;
        pos = pos01;
        k = 1.0f / (1.5f + q01 * 8.0f);
    }

    inline float process (float x) noexcept
    {
        float target = pos;
        if (isAuto)
        {
            const float a = std::abs (x);
            env = a > env ? a + 0.02f * (env - a) : env * 0.9996f;
            target = juce::jlimit (0.0f, 1.0f, env * (2.0f + pos * 24.0f));
        }
        ps += (target - ps) * (isAuto ? 0.01f : 0.004f);

        const float f = 380.0f * std::pow (6.0f, ps);
        const float g = std::tan ((float) kPi * juce::jmin (f, (float) sr * 0.45f) / (float) sr);
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;

        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        return v1 * k * 4.5f;
    }

private:
    double sr = 44100.0;
    float ic1 = 0.0f, ic2 = 0.0f, env = 0.0f, ps = 0.4f, pos = 0.4f, k = 0.2f;
    bool isAuto = false;
};

//==============================================================================
// Dual-tap crossfading delay-line pitch shifter. Also used for octaver, harmonizer, detune and shimmer.
class PitchShifter
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        window = (float) (0.040 * sr);
        line.prepare ((int) window + 16);
        line.clear();
        phase = 0.0f;
        ratio = target = 1.0f;
    }

    void setSemitones (float s) noexcept { target = std::pow (2.0f, s / 12.0f); }

    inline float process (float x) noexcept
    {
        line.write (x);
        ratio += (target - ratio) * 0.002f;

        phase += (1.0f - ratio) / window;
        phase -= std::floor (phase);
        float p2 = phase + 0.5f;
        if (p2 >= 1.0f) p2 -= 1.0f;

        const float s1 = std::sin ((float) kPi * phase);
        const float w1 = s1 * s1;
        return line.read ((double) (phase * window)) * w1 + line.read ((double) (p2 * window)) * (1.0f - w1);
    }

private:
    double sr = 44100.0;
    float window = 1764.0f, phase = 0.0f, ratio = 1.0f, target = 1.0f;
    DelayLine line;
};

//==============================================================================
class Boost
{
public:
    enum Type { Clean, Mid, Treble, Bass };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        gainSm.reset (sr, 0.02);
        gainSm.setCurrentAndTargetValue (1.0f);
        type = -1;
        setParams (Clean, 6.0f);
    }

    void reset() noexcept { f.reset(); }

    void setParams (int t, float gainDb)
    {
        gainSm.setTargetValue (juce::Decibels::decibelsToGain (gainDb));
        if (t == type) return;
        type = t;
        f.reset();
        switch (t)
        {
            case Mid:    f.peaking (sr, 800.0, 0.9, 8.0); break;
            case Treble: f.highShelf (sr, 2800.0, 8.0); break;
            case Bass:   f.lowShelf (sr, 150.0, 8.0); break;
            default:     f.set (1.0, 0.0, 0.0, 1.0, 0.0, 0.0); break;
        }
    }

    inline float process (float x) noexcept { return f.process (x) * gainSm.getNextValue(); }

private:
    double sr = 44100.0;
    int type = -1;
    Biquad f;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gainSm;
};

//==============================================================================
// Overdrive / Distortion / Fuzz. Runs at the oversampled rate.
class DriveStage
{
public:
    enum Type { Overdrive, Distortion, Fuzz };

    void prepare (double osRate)
    {
        sr = osRate;
        hpOd.highpass (sr, 720.0, 0.707);
        preLp.lowpass (sr, 5200.0, 0.707);
        midBump.peaking (sr, 900.0, 0.8, 3.0);
        hpDist.highpass (sr, 140.0, 0.707);
        hpFuzz.highpass (sr, 60.0, 0.707);
        gainSm.reset (sr, 0.03);
        levelSm.reset (sr, 0.03);
        mixSm.reset (sr, 0.01);
        gainSm.setCurrentAndTargetValue (1.0f);
        levelSm.setCurrentAndTargetValue (1.0f);
        mixSm.setCurrentAndTargetValue (0.0f);
        lastTone = -1.0f;
        lastType = -1;
        reset();
    }

    void reset() noexcept { hpOd.reset(); preLp.reset(); tone.reset(); midBump.reset(); hpDist.reset(); hpFuzz.reset(); }

    bool active() const { return mixSm.isSmoothing() || mixSm.getTargetValue() > 0.5f; }

    void setParams (bool on, int t, float drive01, float tone01, float levelDb)
    {
        mixSm.setTargetValue (on ? 1.0f : 0.0f);
        type = t;

        float g = 1.0f;
        switch (t)
        {
            case Distortion: g = std::pow (10.0f, 0.7f + drive01 * 2.3f); break;
            case Fuzz:       g = std::pow (10.0f, 0.8f + drive01 * 2.2f); break;
            default:         g = std::pow (10.0f, drive01 * 2.0f); break;
        }
        gainSm.setTargetValue (g);
        levelSm.setTargetValue (juce::Decibels::decibelsToGain (levelDb));

        if (std::abs (tone01 - lastTone) > 1.0e-4f || t != lastType)
        {
            lastTone = tone01;
            lastType = t;
            const double base = t == Fuzz ? 700.0 : (t == Distortion ? 1200.0 : 900.0);
            tone.lowpass (sr, base * std::pow (10.0, (double) tone01 * (t == Overdrive ? 1.0 : 0.9)), 0.707);
        }
    }

    inline float process (float x) noexcept
    {
        const float m  = mixSm.getNextValue();
        const float g  = gainSm.getNextValue();
        const float lv = levelSm.getNextValue();
        float y;

        switch (type)
        {
            case Distortion:
                y = hardish (g * hpDist.process (x));
                y = tone.process (y * 0.65f);
                break;
            case Fuzz:
            {
                float v = g * hpFuzz.process (x);
                v = std::tanh (v + 0.35f) - 0.3364f;
                y = tone.process (juce::jlimit (-0.9f, 1.0f, v * 3.0f) * 0.55f);
                break;
            }
            default:
                y = x + g * hpOd.process (x);
                y = preLp.process (y);
                y = std::tanh (y);
                y = tone.process (y * 0.8f);
                y = midBump.process (y);
                break;
        }
        y *= lv;
        return x + (y - x) * m;
    }

private:
    double sr = 44100.0;
    int type = 0, lastType = -1;
    Biquad hpOd, preLp, tone, midBump, hpDist, hpFuzz;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> gainSm, levelSm;
    juce::SmoothedValue<float> mixSm;
    float lastTone = -1.0f;
};

//==============================================================================
class Eq4
{
public:
    void prepare (double sampleRate) { sr = sampleRate; for (auto& v : last) v = -1000.0f; reset(); }
    void reset() noexcept { for (auto& b : band) b.reset(); }

    void setParams (float low, float lowMid, float highMid, float high)
    {
        if (changed (0, low))     band[0].lowShelf (sr, 100.0, low);
        if (changed (1, lowMid))  band[1].peaking (sr, 420.0, 0.8, lowMid);
        if (changed (2, highMid)) band[2].peaking (sr, 2200.0, 0.8, highMid);
        if (changed (3, high))    band[3].highShelf (sr, 6500.0, high);
    }

    inline float process (float x) noexcept
    {
        for (auto& b : band) x = b.process (x);
        return x;
    }

private:
    bool changed (int i, float v) noexcept
    {
        if (std::abs (last[i] - v) < 1.0e-3f) return false;
        last[i] = v;
        return true;
    }

    double sr = 44100.0;
    Biquad band[4];
    float last[4] {};
};

//==============================================================================
// One modulation slot: Chorus, Flanger, Phaser, Tremolo, Vibrato, Rotary speaker, Ring modulator.
class ModFx
{
public:
    enum Type { Chorus, Flanger, Phaser, Tremolo, Vibrato, Rotary, RingMod, NumTypes };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        dlL.prepare ((int) (sr * 0.05) + 16);
        dlR.prepare ((int) (sr * 0.05) + 16);
        xoLow.lowpass (sr, 800.0, 0.707);
        type = -1;
        phase = phaseD = 0.0;
        reset();
        setParams (Chorus, 0.3f, 0.5f, 1.0f);
    }

    void reset() noexcept
    {
        dlL.clear(); dlR.clear();
        fbL = fbR = 0.0f;
        for (auto& v : apL) v = 0.0f;
        for (auto& v : apR) v = 0.0f;
        apFbL = apFbR = 0.0f;
        xoLow.reset();
    }

    void setParams (int t, float rate01, float depthV, float mixV)
    {
        if (t != type) { type = t; reset(); }
        static const double lo[NumTypes] = { 0.1, 0.05, 0.05, 1.0, 2.0, 0.5, 40.0 };
        static const double hi[NumTypes] = { 4.0, 2.0, 4.0, 12.0, 8.0, 7.0, 2000.0 };
        rate = lo[t] * std::pow (hi[t] / lo[t], (double) rate01);
        depth = depthV;
        mix = mixV;
    }

    inline void process (float inL, float inR, float& outL, float& outR) noexcept
    {
        phase += 2.0 * kPi * rate / sr;
        if (phase > 2.0 * kPi) phase -= 2.0 * kPi;
        const float sL = (float) std::sin (phase);
        const float sR = (float) std::sin (phase + 1.5708);
        float pL = inL, pR = inR;

        switch (type)
        {
            case Chorus:
            {
                dlL.write (inL); dlR.write (inR);
                const double dL = (0.011 + 0.0055 * depth * sL) * sr;
                const double dR = (0.011 + 0.0055 * depth * sR) * sr;
                pL = (inL + dlL.read (dL)) * 0.7071f;
                pR = (inR + dlR.read (dR)) * 0.7071f;
                break;
            }
            case Flanger:
            {
                dlL.write (inL + fbL * 0.55f); dlR.write (inR + fbR * 0.55f);
                const double dL = (0.0008 + 0.0035 * depth * (0.5 + 0.5 * sL)) * sr;
                const double dR = (0.0008 + 0.0035 * depth * (0.5 + 0.5 * sR)) * sr;
                const float wL = dlL.read (dL), wR = dlR.read (dR);
                fbL = wL; fbR = wR;
                pL = (inL + wL) * 0.7071f;
                pR = (inR + wR) * 0.7071f;
                break;
            }
            case Phaser:
            {
                const float mL = (float) depth * (0.5f + 0.5f * sL) + (1.0f - (float) depth) * 0.5f;
                const float mR = (float) depth * (0.5f + 0.5f * sR) + (1.0f - (float) depth) * 0.5f;
                const float yL = allpass (apL, apFbL, inL, mL);
                const float yR = allpass (apR, apFbR, inR, mR);
                pL = (inL + yL) * 0.7071f;
                pR = (inR + yR) * 0.7071f;
                break;
            }
            case Tremolo:
                pL = inL * (1.0f - (float) depth * 0.5f * (1.0f + sL));
                pR = inR * (1.0f - (float) depth * 0.5f * (1.0f + sL));
                break;
            case Vibrato:
            {
                dlL.write (inL); dlR.write (inR);
                const double dL = (0.004 + 0.0035 * depth * sL) * sr;
                const double dR = (0.004 + 0.0035 * depth * sR) * sr;
                pL = dlL.read (dL);
                pR = dlR.read (dR);
                break;
            }
            case Rotary:
            {
                phaseD += 2.0 * kPi * rate * 0.78 / sr;
                if (phaseD > 2.0 * kPi) phaseD -= 2.0 * kPi;
                const float m = 0.5f * (inL + inR);
                const float low = xoLow.process (m);
                const float high = m - low;
                dlL.write (high); dlR.write (low);
                const float sh = (float) std::sin (phase), ch = (float) std::cos (phase);
                const float sd = (float) std::sin (phaseD), cd = (float) std::cos (phaseD);
                const float hornD = dlL.read ((0.0008 + 0.0006 * depth * sh) * sr);
                const float drumD = dlR.read ((0.0010 + 0.0008 * depth * sd) * sr);
                const float d = (float) depth;
                pL = hornD * 0.7f * (1.0f + 0.8f * d * ch) + drumD * 0.7f * (1.0f + 0.6f * d * cd);
                pR = hornD * 0.7f * (1.0f - 0.8f * d * ch) + drumD * 0.7f * (1.0f - 0.6f * d * cd);
                break;
            }
            default: // RingMod
            {
                const float c = (1.0f - (float) depth) + (float) depth * sL;
                pL = inL * c;
                pR = inR * c;
                break;
            }
        }

        outL = inL + (pL - inL) * mix;
        outR = inR + (pR - inR) * mix;
    }

private:
    // six first-order all-pass stages swept between ~200 Hz and ~3.2 kHz
    inline float allpass (float* z, float& fb, float x, float m) noexcept
    {
        const double f = 200.0 * std::pow (16.0, (double) m);
        const double t = std::tan (kPi * f / sr);
        const float a = (float) ((t - 1.0) / (t + 1.0));
        float y = x + fb * 0.4f;
        for (int i = 0; i < 6; ++i)
        {
            const float o = a * y + z[i];
            z[i] = y - a * o;
            y = o;
        }
        fb = y;
        return y;
    }

    double sr = 44100.0, phase = 0.0, phaseD = 0.0, rate = 1.0, depth = 0.5;
    float mix = 1.0f;
    int type = -1;
    DelayLine dlL, dlR;
    float fbL = 0.0f, fbR = 0.0f, apL[6] {}, apR[6] {}, apFbL = 0.0f, apFbR = 0.0f;
    Biquad xoLow;
};

//==============================================================================
class LoFi
{
public:
    void prepare (double) { reset(); }
    void reset() noexcept { holdL = holdR = 0.0f; counter = 0; }

    void setParams (float bits01, float rate01, float mixV)
    {
        bits = 16.0f - bits01 * 13.0f;                     // 16 .. 3 bits
        step = 1.0f / (std::pow (2.0f, bits - 1.0f));
        factor = 1 + (int) (rate01 * 31.0f);               // 1 .. 32 sample hold
        mix = mixV;
    }

    inline void process (float inL, float inR, float& outL, float& outR) noexcept
    {
        if (counter <= 0)
        {
            holdL = std::round (inL / step) * step;
            holdR = std::round (inR / step) * step;
            counter = factor;
        }
        --counter;
        outL = inL + (holdL - inL) * mix;
        outR = inR + (holdR - inR) * mix;
    }

private:
    float bits = 16.0f, step = 0.0f, holdL = 0.0f, holdR = 0.0f, mix = 0.0f;
    int factor = 1, counter = 0;
};

//==============================================================================
// Delay: Digital, Analog, Tape, Slapback, Ping-Pong. Input is faded so tails spill over when switched off.
class TimeDelay
{
public:
    enum Type { Digital, Analog, Tape, Slapback, PingPong, Reverse, MultiTap };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        lineL.prepare ((int) (sr * 1.7) + 16);
        lineR.prepare ((int) (sr * 1.7) + 16);
        timeSm.reset (sr, 0.12);
        mixSm.reset (sr, 0.03);
        timeSm.setCurrentAndTargetValue (350.0f);
        mixSm.setCurrentAndTargetValue (0.0f);
        type = -1;
        lfo = 0.0;
        reset();
        setParams (Digital, 380.0f, 0.3f, 0.0f);
    }

    void reset() noexcept
    {
        lineL.clear(); lineR.clear();
        hpL.reset(); hpR.reset(); lpL.reset(); lpR.reset();
    }

    void setParams (int t, float timeMs, float fb, float mixV)
    {
        if (t != type)
        {
            type = t;
            double lp = 9000.0, hp = 60.0;
            switch (t)
            {
                case Analog:   lp = 3200.0; hp = 150.0; modDepth = 0.0006; modRate = 0.6; sat = true;  break;
                case Tape:     lp = 3800.0; hp = 200.0; modDepth = 0.0015; modRate = 0.45; sat = true; break;
                case Slapback: lp = 6000.0; hp = 120.0; modDepth = 0.0;    modRate = 0.0; sat = false; break;
                case PingPong: lp = 5000.0; hp = 120.0; modDepth = 0.0004; modRate = 0.5; sat = false; break;
                case Reverse:  lp = 5500.0; hp = 120.0; modDepth = 0.0; modRate = 0.0; sat = true;  break;
                case MultiTap: lp = 6500.0; hp = 120.0; modDepth = 0.0; modRate = 0.0; sat = false; break;
                default:       modDepth = 0.0; modRate = 0.0; sat = false; break;
            }
            lpL.lowpass (sr, lp, 0.707); lpR.lowpass (sr, lp, 0.707);
            hpL.highpass (sr, hp, 0.707); hpR.highpass (sr, hp, 0.707);
            lfoInc = 2.0 * kPi * modRate / sr;
            revPhase = 0.0;
        }

        float tm = timeMs;
        feedback = fb;
        if (t == Slapback)
        {
            tm = 60.0f + (timeMs - 40.0f) / 960.0f * 80.0f;
            feedback = juce::jmin (fb, 0.25f);
        }
        timeSm.setTargetValue (tm);
        mixSm.setTargetValue (mixV);
    }

    inline void process (float inL, float inR, float fade, float& outL, float& outR) noexcept
    {
        const double base = (double) timeSm.getNextValue() * 0.001 * sr;
        const float mixV = mixSm.getNextValue();
        const double mod = modDepth * sr;

        lfo += lfoInc;
        if (lfo > 2.0 * kPi) lfo -= 2.0 * kPi;

        float dL, dR;
        if (type == Reverse)
        {
            // pitch-shifter style crossfaded read head running backwards at 1x
            const double W = juce::jmax (64.0, base);
            revPhase += 2.0 / W;
            revPhase -= std::floor (revPhase);
            const double p2 = revPhase + 0.5 - std::floor (revPhase + 0.5);
            const float s1 = (float) std::sin (kPi * revPhase);
            const float w1 = s1 * s1;
            dL = lineL.read (revPhase * W) * w1 + lineL.read (p2 * W) * (1.0f - w1);
            dR = lineR.read (revPhase * W) * w1 + lineR.read (p2 * W) * (1.0f - w1);
        }
        else if (type == MultiTap)
        {
            static const double tl[4] = { 0.50, 0.75, 1.00, 1.50 }, tr[4] = { 0.375, 0.625, 0.875, 1.25 };
            static const float gl[4] = { 0.55f, 0.50f, 0.80f, 0.45f }, gr[4] = { 0.50f, 0.55f, 0.75f, 0.50f };
            dL = dR = 0.0f;
            for (int i = 0; i < 4; ++i)
            {
                dL += gl[i] * lineL.read (base * tl[i]);
                dR += gr[i] * lineR.read (base * tr[i]);
            }
            dL *= 0.6f; dR *= 0.6f;
        }
        else
        {
            dL = lineL.read (base + mod * std::sin (lfo));
            dR = lineR.read (base + mod * std::sin (lfo + 2.1));
        }

        float fbL = lpL.process (hpL.process (type == PingPong ? dR : dL));
        float fbR = lpR.process (hpR.process (type == PingPong ? dL : dR));
        if (sat) { fbL = std::tanh (fbL); fbR = std::tanh (fbR); }
        fbL *= feedback; fbR *= feedback;

        if (type == PingPong)
        {
            lineL.write ((inL + inR) * 0.5f * fade + fbL);
            lineR.write (fbR);
        }
        else
        {
            lineL.write (inL * fade + fbL);
            lineR.write (inR * fade + fbR);
        }

        outL = inL + dL * mixV;
        outR = inR + dR * mixV;
    }

private:
    double sr = 44100.0, lfo = 0.0, lfoInc = 0.0, modDepth = 0.0, modRate = 0.0, revPhase = 0.0;
    int type = -1;
    bool sat = false;
    DelayLine lineL, lineR;
    Biquad hpL, hpR, lpL, lpR;
    juce::SmoothedValue<float> timeSm, mixSm;
    float feedback = 0.3f;
};
} // namespace noir
