#pragma once
#include <JuceHeader.h>

namespace mc::dsp
{
// 24 built-in band-limited wavetables. Each has 8 morph frames and 10 mip levels
// (mip m holds harmonics 1 .. 512 >> m), plus a 64-partial magnitude spectrum per frame for the additive engine.
struct Wavetable
{
    static constexpr int N = 1024, F = 8, M = 10, H = 64;
    juce::String name;
    std::vector<float> data;          // F * M * (N + 1) (one guard sample per cycle)
    float spec[F][H] {};
    const float* cycle (int frame, int mip) const { return data.data() + ((size_t) frame * M + (size_t) mip) * (N + 1); }
};

class WavetableBank
{
public:
    static const WavetableBank& get();
    const Wavetable& table (int i) const { return tables[(size_t) juce::jlimit (0, (int) tables.size() - 1, i)]; }
    int size() const { return (int) tables.size(); }
    // pick the mip whose highest harmonic stays below Nyquist for the given number of allowed harmonics
    static int mipFor (float allowedHarmonics)
    {
        int m = 0;
        while (m < Wavetable::M - 1 && (float) (512 >> m) > allowedHarmonics) ++m;
        return m;
    }
private:
    WavetableBank();
    std::vector<Wavetable> tables;
};

// Read one sample: frame position fpos in [0, F-1], phase in [0,1)
inline float readTable (const Wavetable& t, float fpos, int mip, float phase)
{
    const int f0 = juce::jlimit (0, Wavetable::F - 1, (int) fpos);
    const int f1 = juce::jmin (Wavetable::F - 1, f0 + 1);
    const float ff = fpos - (float) f0;
    const float x = phase * (float) Wavetable::N;
    int i = (int) x;
    const float fr = x - (float) i;
    i &= (Wavetable::N - 1);
    const float* a = t.cycle (f0, mip);
    const float va = a[i] + fr * (a[i + 1] - a[i]);
    if (ff <= 0.0001f || f0 == f1) return va;
    const float* b = t.cycle (f1, mip);
    const float vb = b[i] + fr * (b[i + 1] - b[i]);
    return va + ff * (vb - va);
}

// Fast sine for phase in turns (any real value)
struct SineTable
{
    static constexpr int N = 4096;
    float t[N + 1];
    SineTable() { for (int i = 0; i <= N; ++i) t[i] = (float) std::sin (juce::MathConstants<double>::twoPi * i / N); }
    static const SineTable& get() { static SineTable s; return s; }
};
inline float fastSin (float turns)
{
    const auto& s = SineTable::get();
    float p = turns - std::floor (turns);
    const float x = p * (float) SineTable::N;
    const int i = (int) x;
    const float fr = x - (float) i;
    return s.t[i] + fr * (s.t[i + 1] - s.t[i]);
}

// Small fast PRNG
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    explicit Rng (uint32_t seed = 1) : s (seed * 2654435761u + 12345u) {}
    inline uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    inline float uni() { return (float) (next() >> 8) * (1.0f / 16777216.0f); }       // [0,1)
    inline float bi() { return uni() * 2.0f - 1.0f; }                                 // [-1,1)
};
}
