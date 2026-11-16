#pragma once
#include <JuceHeader.h>
#include "../Params.h"
#include "Wavetables.h"

namespace mc::dsp
{
constexpr float kPi = juce::MathConstants<float>::pi;
constexpr float kTwoPi = juce::MathConstants<float>::twoPi;

inline float fastTanh (float x)
{
    x = juce::jlimit (-4.5f, 4.5f, x);
    const float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
inline float dbToGain (float db, float floorDb = -59.9f) { return db <= floorDb ? 0.0f : std::pow (10.0f, db * 0.05f); }
inline float noteHz (float note) { return 440.0f * std::exp2 ((note - 69.0f) / 12.0f); }
inline float frac (float x) { return x - std::floor (x); }
inline void panGains (float pan, float& l, float& r)
{
    const float a = (juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * 0.25f * kPi;
    l = std::cos (a) * 1.41421356f;
    r = std::sin (a) * 1.41421356f;
}
inline float clampf (float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

struct OnePole
{
    float s = 0;
    inline float lp (float x, float a) { s += a * (x - s); return s; }        // a = 1 - exp(-2 pi fc / sr)
    static float coef (float fc, double sr) { return 1.0f - std::exp (-kTwoPi * juce::jmin (fc, (float) sr * 0.49f) / (float) sr); }
};

struct DcBlock
{
    float x1 = 0, y1 = 0;
    inline float process (float x) { const float y = x - x1 + 0.995f * y1; x1 = x; y1 = y; return y; }
};

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] {}, z2[2] {};
    inline float process (float x, int ch)
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
    void lowShelf (double sr, double f, double db, double q = 0.707)
    {
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * juce::jmin (f, sr * 0.45) / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * q), sa = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * cs + sa), 2 * A * ((A - 1) - (A + 1) * cs), A * ((A + 1) - (A - 1) * cs - sa),
             (A + 1) + (A - 1) * cs + sa, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - sa);
    }
    void highShelf (double sr, double f, double db, double q = 0.707)
    {
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * juce::jmin (f, sr * 0.45) / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * q), sa = 2.0 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * cs + sa), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - sa),
             (A + 1) - (A - 1) * cs + sa, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - sa);
    }
    void peak (double sr, double f, double db, double q)
    {
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * juce::jmin (f, sr * 0.45) / sr;
        const double cs = std::cos (w), al = std::sin (w) / (2.0 * q);
        set (1 + al * A, -2 * cs, 1 - al * A, 1 + al / A, -2 * cs, 1 - al / A);
    }
};

// Simper zero-delay state variable filter (one channel)
struct Svf
{
    float ic1 = 0, ic2 = 0;
    float lp = 0, bp = 0, hp = 0;
    inline void tick (float v0, float g, float k)
    {
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = v0 - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
    void reset() { ic1 = ic2 = lp = bp = hp = 0; }
};

struct MsegData
{
    int n = 4;
    float x[kMsegMaxPoints] { 0.0f, 0.15f, 1.0f, 2.0f }, y[kMsegMaxPoints] { 0.0f, 1.0f, 0.5f, 0.0f }, c[kMsegMaxPoints] { 0.0f, 0.4f, 0.0f, 0.4f };
    int loopStart = 1, loopEnd = 2;
    float valueAt (float t) const;   // t in grid units
    float length() const { return x[n - 1]; }
};

struct MapData
{
    float v[kMapSteps] {};
};

// Shared per-block state that every voice reads.
struct Shared
{
    double sr = 48000.0;
    double bpm = 120.0;
    std::vector<float> val, norm;                          // parameter values (real units) and normalised values
    std::vector<juce::NormalisableRange<float>> rng;
    std::vector<int> destOf;                               // catalogue index -> matrix dest (0 none)
    int nDest = 0;
    int grid[kNumLanes][kNumSlots] {};                      // grid entry index (duplicates removed)
    int fxGrid[kNumBuses][kFxSlots] {};                     // fx entry index (duplicates removed)
    MsegData mseg[4];
    MapData map[4];
    float modWheel = 0, aftertouch = 0, bend = 0, expression = 1, macro[4] {};
    float globalLfo[4] {};
    bool lfoGlobal[4] {};
    struct MtxSlot { int src, via, dst; float amt; };
    MtxSlot mtx[kNumMatrix];
    int nMtx = 0;
    float bendUp = 2, bendDown = 2, transpose = 0;
    double beatsPerSample = 120.0 / 60.0 / 48000.0;

    inline float get (int p, const float* mod) const
    {
        const int d = destOf[(size_t) p];
        if (d == 0 || mod == nullptr || mod[d] == 0.0f) return val[(size_t) p];
        return rng[(size_t) p].convertFrom0to1 (clampf (norm[(size_t) p] + mod[d], 0.0f, 1.0f));
    }
    inline int choice (int p) const { return (int) (val[(size_t) p] + 0.5f); }
    inline bool on (int p) const { return val[(size_t) p] > 0.5f; }
};

// Accessor bound to one module instance and one voice's modulation offsets
struct P
{
    const Shared& sh;
    int base;
    const float* mod;
    inline float operator() (int local) const { return sh.get (base + local, mod); }
    inline int ch (int local) const { return sh.choice (base + local); }
    inline bool on (int local) const { return sh.on (base + local); }
};
}
