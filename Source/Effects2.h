#pragma once

#include <atomic>
#include <cstdint>
#include <vector>
#include "Effects.h"

// Second batch of effects: looper, vocoder, talk box, granular / freeze, glitch / stutter, tuner.
namespace noir
{
struct Rng
{
    uint32_t s = 2463534242u;
    inline float next01() noexcept
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (float) (s >> 8) * (1.0f / 16777216.0f);
    }
};

inline int nextPow2 (int n) { int p = 1; while (p < n) p <<= 1; return p; }

//==============================================================================
// Stereo looper: record, play, overdub (multi-layer), half / double speed, reverse, one-level undo.
class Looper
{
public:
    enum Mode { Stop, Record, Play, Overdub };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        maxLen = (int) (45.0 * sr);
        bufL.assign ((size_t) maxLen, 0.0f); bufR.assign ((size_t) maxLen, 0.0f);
        undoL.assign ((size_t) maxLen, 0.0f); undoR.assign ((size_t) maxLen, 0.0f);
        length = 0; posD = 0.0; mode = Stop; undoReady = false; undoCopied = 0; dubbing = false;
    }

    int getMode() const noexcept { return mode; }
    bool hasLoop() const noexcept { return length > 0; }
    float progress() const noexcept { return length > 0 ? (float) (posD / (double) length) : 0.0f; }
    bool canUndo() const noexcept { return undoReady; }

    void clear() noexcept { length = 0; posD = 0.0; mode = Stop; undoReady = false; dubbing = false; }

    void setMode (int m) noexcept
    {
        if (m == mode) return;
        const int old = mode;

        if (m == Record)
        {
            length = 0; posD = 0.0; mode = Record; undoReady = false; dubbing = false;
        }
        else if (old == Record)
        {
            const int len = (int) posD;
            if (len < (int) (0.1 * sr)) { length = 0; mode = Stop; posD = 0.0; return; }
            length = len; posD = 0.0;
            mode = m;
            if (m == Overdub) beginDub();
        }
        else if (m == Overdub)
        {
            if (length == 0) { mode = Stop; return; }
            mode = Overdub;
            beginDub();
        }
        else if (m == Play)
        {
            if (length == 0) { mode = Stop; return; }
            if (old == Stop) posD = 0.0;
            mode = Play;
            dubbing = false;
        }
        else // Stop
        {
            mode = Stop; posD = 0.0; dubbing = false;
        }
    }

    void undo() noexcept
    {
        if (! undoReady || length == 0) return;
        std::copy (undoL.begin(), undoL.begin() + length, bufL.begin());
        std::copy (undoR.begin(), undoR.begin() + length, bufR.begin());
        undoReady = false;
    }

    // speed: 0.5, 1 or 2
    inline void process (float inL, float inR, float level, float speed, bool reverse, float& outL, float& outR) noexcept
    {
        outL = inL; outR = inR;

        if (mode == Record)
        {
            const int i = (int) posD;
            if (i < maxLen)
            {
                bufL[(size_t) i] = inL; bufR[(size_t) i] = inR;
                posD += 1.0;
            }
            else
            {
                length = maxLen; posD = 0.0; mode = Play;
            }
            return;
        }

        if (mode == Stop || length == 0) return;

        const int len = length;
        const int i0 = juce::jlimit (0, len - 1, (int) posD);
        const int i1 = (i0 + 1) % len;
        const float fr = (float) (posD - std::floor (posD));
        const float pl = bufL[(size_t) i0] + fr * (bufL[(size_t) i1] - bufL[(size_t) i0]);
        const float pr = bufR[(size_t) i0] + fr * (bufR[(size_t) i1] - bufR[(size_t) i0]);
        outL = inL + pl * level;
        outR = inR + pr * level;

        if (mode == Overdub)
        {
            if (dubbing && undoCopied < len && ! undoReady)
            {
                undoL[(size_t) i0] = bufL[(size_t) i0];
                undoR[(size_t) i0] = bufR[(size_t) i0];
                if (++undoCopied >= len) undoReady = true;
            }
            bufL[(size_t) i0] += inL;
            bufR[(size_t) i0] += inR;
        }

        posD += reverse ? -(double) speed : (double) speed;
        if (posD >= (double) len) posD -= (double) len;
        if (posD < 0.0) posD += (double) len;
    }

private:
    void beginDub() noexcept { dubbing = true; undoCopied = 0; undoReady = false; }

    double sr = 44100.0, posD = 0.0;
    int maxLen = 0, length = 0, mode = Stop, undoCopied = 0;
    bool undoReady = false, dubbing = false;
    std::vector<float> bufL, bufR, undoL, undoR;
};

//==============================================================================
// 16-band channel vocoder. Modulator = microphone (input 2), carrier = the guitar signal.
class Vocoder
{
public:
    static constexpr int B = 16;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int i = 0; i < B; ++i)
        {
            const double f = 100.0 * std::pow (8000.0 / 100.0, (double) i / (double) (B - 1));
            modBand[i].bandpass (sr, f, 5.0);
            carBand[i].bandpass (sr, f, 5.0);
            env[i] = 0.0f;
        }
        sibHp.highpass (sr, 5500.0, 0.707);
        attack = 1.0f - (float) std::exp (-1.0 / (0.003 * sr));
        release = 1.0f - (float) std::exp (-1.0 / (0.025 * sr));
        sibEnv = 0.0f;
        reset();
    }

    void reset() noexcept
    {
        for (auto& b : modBand) b.reset();
        for (auto& b : carBand) b.reset();
        sibHp.reset();
        for (auto& e : env) e = 0.0f;
        sibEnv = 0.0f;
    }

    void setParams (float sensV, float brightV) { sens = sensV; bright = brightV; }

    inline float process (float mic, float carrier) noexcept
    {
        float out = 0.0f;
        for (int i = 0; i < B; ++i)
        {
            const float e = std::abs (modBand[i].process (mic));
            env[i] += (e - env[i]) * (e > env[i] ? attack : release);
            out += carBand[i].process (carrier) * env[i];
        }

        const float s = std::abs (sibHp.process (mic));
        sibEnv += (s - sibEnv) * (s > sibEnv ? attack : release);
        noise.s ^= noise.s << 13; noise.s ^= noise.s >> 17; noise.s ^= noise.s << 5;
        const float n = (float) (int32_t) noise.s * (1.0f / 2147483648.0f);
        out += n * sibEnv * bright * 3.0f;

        return std::tanh (out * (2.0f + sens * 14.0f));
    }

private:
    double sr = 44100.0;
    Biquad modBand[B], carBand[B], sibHp;
    float env[B] {};
    float attack = 0.1f, release = 0.01f, sens = 0.5f, bright = 0.4f, sibEnv = 0.0f;
    Rng noise;
};

//==============================================================================
// Talk box: three vowel formants (A E I O U) swept by hand or by an auto-mouth LFO.
class TalkBox
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        phase = 0.0;
        ps = 0.3f;
        reset();
    }

    void reset() noexcept { for (int i = 0; i < 3; ++i) ic1[i] = ic2[i] = 0.0f; }

    void setParams (float vowel01, float autoAmt, float res, float mixV)
    {
        vowel = vowel01; autoRate = autoAmt; k = 1.0f / (4.0f + res * 10.0f); mix = mixV;
    }

    inline float process (float x) noexcept
    {
        float pos = vowel;
        if (autoRate > 0.001f)
        {
            phase += 2.0 * kPi * (0.3 + (double) autoRate * 5.7) / sr;
            if (phase > 2.0 * kPi) phase -= 2.0 * kPi;
            pos = 0.5f + 0.5f * (float) std::sin (phase);
        }
        ps += (pos - ps) * 0.003f;

        static const float table[5][3] = {
            { 800.0f, 1150.0f, 2900.0f },   // A
            { 400.0f, 1600.0f, 2700.0f },   // E
            { 270.0f, 2140.0f, 2950.0f },   // I
            { 450.0f,  800.0f, 2830.0f },   // O
            { 325.0f,  700.0f, 2700.0f } }; // U
        static const float gains[3] = { 1.0f, 0.75f, 0.45f };

        const float fp = juce::jlimit (0.0f, 3.999f, ps * 4.0f);
        const int i0 = (int) fp;
        const float fr = fp - (float) i0;

        float y = 0.0f;
        for (int b = 0; b < 3; ++b)
        {
            const float f = table[i0][b] + fr * (table[i0 + 1][b] - table[i0][b]);
            const float g = std::tan ((float) kPi * f / (float) sr);
            const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
            const float v3 = x - ic2[b];
            const float v1 = a1 * ic1[b] + a2 * v3;
            const float v2 = ic2[b] + a2 * ic1[b] + a3 * v3;
            ic1[b] = 2.0f * v1 - ic1[b];
            ic2[b] = 2.0f * v2 - ic2[b];
            y += v1 * k * gains[b];
        }
        y *= 3.0f;
        return x + (y - x) * mix;
    }

private:
    double sr = 44100.0, phase = 0.0;
    float vowel = 0.3f, autoRate = 0.0f, k = 0.1f, mix = 1.0f, ps = 0.3f;
    float ic1[3] {}, ic2[3] {};
};

//==============================================================================
// Granular engine. Used for the Granular synth pedal and (with a frozen region) for Freeze.
class Granular
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        size = nextPow2 ((int) (3.0 * sr));
        mask = size - 1;
        buf.assign ((size_t) size, 0.0f);
        w = 0;
        frozen = false;
        counter = 0.0f;
        for (auto& g : grains) g.on = false;
    }

    void reset() noexcept
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        for (auto& g : grains) g.on = false;
        frozen = false;
    }

    void setParams (float sizeMs, float densityHz, float semis, float spread, float regionMs = 0.0f)
    {
        grainMs = sizeMs; density = juce::jmax (0.5f, densityHz); pitch = semis; scatter = spread;
        region = regionMs > 0.0f ? regionMs : grainMs * 3.0f;
    }

    void setFrozen (bool f) noexcept
    {
        if (f && ! frozen)
        {
            frozen = true;
            fLen = juce::jlimit (256, size / 2, (int) (region * 0.001 * sr));
            fEnd = w;
            fStart = w - fLen;
        }
        else if (! f)
        {
            frozen = false;
        }
    }

    bool busy() const noexcept { for (auto& g : grains) if (g.on) return true; return false; }

    inline void process (float x, bool spawnEnabled, float& oL, float& oR) noexcept
    {
        if (! frozen)
        {
            buf[(size_t) w] = x;
            w = (w + 1) & mask;
        }

        if (spawnEnabled)
        {
            counter -= 1.0f;
            if (counter <= 0.0f)
            {
                spawn();
                counter = (float) (sr / density) * (0.6f + 0.8f * rng.next01());
            }
        }

        float l = 0.0f, r = 0.0f;
        for (auto& g : grains)
        {
            if (! g.on) continue;
            const float ph = (float) g.age / (float) g.len;
            const float s = std::sin ((float) kPi * ph);
            const float win = s * s;

            const long i0 = (long) std::floor (g.pos);
            const float fr = (float) (g.pos - (double) i0);
            const float a = buf[(size_t) (i0 & mask)];
            const float b = buf[(size_t) ((i0 + 1) & mask)];
            const float v = (a + fr * (b - a)) * win;
            l += v * g.gL;
            r += v * g.gR;

            g.pos += g.inc;
            if (++g.age >= g.len) g.on = false;
        }

        const float overlap = density * grainMs * 0.001f;
        const float norm = 1.0f / std::sqrt (juce::jmax (1.0f, overlap));
        oL = l * norm;
        oR = r * norm;
    }

private:
    struct Grain { bool on = false; double pos = 0.0, inc = 1.0; int age = 0, len = 1; float gL = 0.7f, gR = 0.7f; };

    void spawn() noexcept
    {
        Grain* slot = nullptr;
        for (auto& g : grains) if (! g.on) { slot = &g; break; }
        if (slot == nullptr) return;

        const int len = juce::jmax (64, (int) (grainMs * 0.001 * sr));
        const float semi = pitch + (rng.next01() * 2.0f - 1.0f) * scatter * 7.0f;
        const double ratio = std::pow (2.0, (double) semi / 12.0);
        const double span = (double) len * ratio;

        double start;
        if (frozen)
        {
            const double room = juce::jmax (0.0, (double) fLen - span);
            start = (double) fStart + (double) rng.next01() * room;
        }
        else
        {
            const double back = span + 256.0 + (double) rng.next01() * (double) scatter * 0.3 * sr;
            start = (double) w - juce::jmin (back, (double) size - 2.0 * span - 1024.0);
        }

        slot->on = true;
        slot->pos = start;
        slot->inc = ratio;
        slot->age = 0;
        slot->len = len;
        const float pan = rng.next01();
        slot->gL = std::cos (pan * 1.5708f);
        slot->gR = std::sin (pan * 1.5708f);
    }

    static constexpr int MaxGrains = 48;
    double sr = 44100.0;
    int size = 0, mask = 0, w = 0, fStart = 0, fEnd = 0, fLen = 0;
    bool frozen = false;
    float counter = 0.0f, grainMs = 120.0f, density = 10.0f, pitch = 0.0f, scatter = 0.3f, region = 360.0f;
    std::vector<float> buf;
    Grain grains[MaxGrains];
    Rng rng;
};

//==============================================================================
// Stutter (rhythmic slice repeat) and Glitch (random repeat / reverse / half / double / drop-out).
class Glitch
{
public:
    enum Mode { Stutter, GlitchMode };

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        size = nextPow2 ((int) (4.0 * sr));
        mask = size - 1;
        bufL.assign ((size_t) size, 0.0f);
        bufR.assign ((size_t) size, 0.0f);
        w = 0; timer = 0; count = 0; act = Pass;
        sliceLen = base = (int) (0.125 * sr);
        capStart = capEnd = 0;
    }

    void setParams (int m, float lenMs, float amt)
    {
        mode = m;
        base = juce::jmax (256, (int) (lenMs * 0.001 * sr));
        amount = amt;
    }

    inline void process (float inL, float inR, float& oL, float& oR) noexcept
    {
        bufL[(size_t) w] = inL;
        bufR[(size_t) w] = inR;
        w = (w + 1) & mask;

        if (timer >= sliceLen) boundary();
        const int p = timer++;

        float wl = inL, wr = inR;
        if (act != Pass)
        {
            if (act == Mute) { wl = wr = 0.0f; }
            else
            {
                double idx;
                switch (act)
                {
                    case Reverse: idx = (double) (capEnd - 1 - p); break;
                    case Half:    idx = (double) capStart + (double) p * 0.5; break;
                    case Double:  idx = (double) capStart + (double) p * 2.0; break;
                    default:      idx = (double) (capStart + p); break;
                }
                const long i0 = (long) std::floor (idx);
                const float fr = (float) (idx - (double) i0);
                const size_t a = (size_t) (i0 & mask), b = (size_t) ((i0 + 1) & mask);
                wl = bufL[a] + fr * (bufL[b] - bufL[a]);
                wr = bufR[a] + fr * (bufR[b] - bufR[a]);
            }
            const float ramp = 48.0f;
            const float e = juce::jmin (1.0f, (float) p / ramp, (float) (sliceLen - p) / ramp);
            wl *= e; wr *= e;
        }

        oL = wl;
        oR = wr;
    }

private:
    enum Act { Pass, Repeat, Reverse, Half, Double, Mute };

    void boundary() noexcept
    {
        timer = 0;
        ++count;
        const Act prev = act;

        if (mode == Stutter)
        {
            sliceLen = base;
            const int repeats = 1 + (int) std::round (amount * 6.0f);
            act = (count % 8) >= (8 - repeats) ? Repeat : Pass;
            if (act == Repeat && prev != Repeat)
            {
                capEnd = w;
                capStart = w - sliceLen;
            }
        }
        else
        {
            const float pick = rng.next01();
            sliceLen = pick < 0.25f ? juce::jmax (128, base / 2) : (pick < 0.8f ? base : base * 2);
            if (rng.next01() < amount * 0.85f)
            {
                const float r = rng.next01();
                act = r < 0.30f ? Repeat : r < 0.55f ? Reverse : r < 0.72f ? Half : r < 0.87f ? Double : Mute;
                capEnd = w;
                capStart = w - sliceLen;
            }
            else act = Pass;
        }
    }

    double sr = 44100.0;
    int size = 0, mask = 0, w = 0, timer = 0, count = 0, sliceLen = 6000, base = 6000, capStart = 0, capEnd = 0, mode = 0;
    float amount = 0.5f;
    Act act = Pass;
    std::vector<float> bufL, bufR;
    Rng rng;
};

//==============================================================================
// Pitch detector (YIN). The audio thread only pushes decimated samples; analysis runs on the message thread.
class Tuner
{
public:
    struct Result { float freq = 0.0f; float clarity = 0.0f; float level = 0.0f; };

    void prepare (double sampleRate)
    {
        dec = juce::jmax (1, (int) std::lround (sampleRate / 22000.0));
        rate = sampleRate / (double) dec;
        acc = 0.0f; cnt = 0;
        wi.store (0);
        ring.assign ((size_t) ringSize, 0.0f);
    }

    inline void push (float x) noexcept
    {
        acc += x;
        if (++cnt >= dec)
        {
            const int i = wi.load (std::memory_order_relaxed);
            ring[(size_t) (i & (ringSize - 1))] = acc / (float) dec;
            wi.store (i + 1, std::memory_order_release);
            acc = 0.0f; cnt = 0;
        }
    }

    Result analyse() const
    {
        Result res;
        const int W = 1024;
        const int maxLag = juce::jmin (480, (int) (rate / 60.0));
        const int minLag = juce::jmax (8, (int) (rate / 1000.0));
        const int need = W + maxLag + 4;

        const int end = wi.load (std::memory_order_acquire);
        if (end < need || ring.empty()) return res;

        std::vector<float> x ((size_t) need);
        for (int i = 0; i < need; ++i)
            x[(size_t) i] = ring[(size_t) ((end - need + i) & (ringSize - 1))];

        double e = 0.0;
        for (int i = 0; i < W; ++i) e += (double) x[(size_t) i] * x[(size_t) i];
        res.level = (float) std::sqrt (e / W);
        if (res.level < 0.004f) return res;

        std::vector<float> d ((size_t) maxLag + 1, 0.0f);
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            double s = 0.0;
            for (int j = 0; j < W; ++j)
            {
                const float df = x[(size_t) j] - x[(size_t) (j + tau)];
                s += (double) df * df;
            }
            d[(size_t) tau] = (float) s;
        }

        double run = 0.0;
        d[0] = 1.0f;
        for (int tau = 1; tau <= maxLag; ++tau)
        {
            run += d[(size_t) tau];
            d[(size_t) tau] = run > 0.0 ? d[(size_t) tau] * (float) tau / (float) run : 1.0f;
        }

        int best = -1;
        for (int tau = minLag; tau < maxLag; ++tau)
        {
            if (d[(size_t) tau] < 0.15f)
            {
                while (tau + 1 < maxLag && d[(size_t) tau + 1] < d[(size_t) tau]) ++tau;
                best = tau;
                break;
            }
        }
        if (best < 0) return res;

        float t = (float) best;
        if (best > 1 && best < maxLag)
        {
            const float a = d[(size_t) best - 1], b = d[(size_t) best], c = d[(size_t) best + 1];
            const float den = a - 2.0f * b + c;
            if (std::abs (den) > 1.0e-9f) t += 0.5f * (a - c) / den;
        }

        res.freq = (float) (rate / (double) t);
        res.clarity = 1.0f - d[(size_t) best];
        return res;
    }

private:
    static constexpr int ringSize = 4096;
    int dec = 2, cnt = 0;
    double rate = 22050.0;
    float acc = 0.0f;
    std::atomic<int> wi { 0 };
    std::vector<float> ring;
};
} // namespace noir
