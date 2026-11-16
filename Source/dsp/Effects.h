#pragma once
#include "Common.h"

namespace mc::dsp
{
struct Allpass
{
    std::vector<float> b;
    int w = 0;
    void setSize (int n) { b.assign ((size_t) juce::jmax (2, n), 0.0f); w = 0; }
    void clear() { std::fill (b.begin(), b.end(), 0.0f); }
    inline float process (float x, float g)
    {
        const float d = b[(size_t) w];
        const float y = d - g * x;
        b[(size_t) w] = x + g * y;
        if (++w >= (int) b.size()) w = 0;
        return y;
    }
};

struct DelayLine
{
    std::vector<float> b;
    int mask = 0, w = 0;
    void setSize (int minLen)
    {
        int n = 1;
        while (n < minLen + 4) n <<= 1;
        b.assign ((size_t) n, 0.0f);
        mask = n - 1; w = 0;
    }
    void clear() { std::fill (b.begin(), b.end(), 0.0f); }
    inline void push (float x) { b[(size_t) w] = x; w = (w + 1) & mask; }
    inline float read (float delay) const          // delay in samples (>= 1)
    {
        const float pos = (float) w - delay;
        const float fl = std::floor (pos);
        const int i0 = (int) fl;
        const float fr = pos - fl;
        const float a = b[(size_t) (i0 & mask)], c = b[(size_t) ((i0 + 1) & mask)];
        return a + fr * (c - a);
    }
    inline float readInt (int delay) const { return b[(size_t) ((w - delay) & mask)]; }
};

struct ModFxUnit
{
    DelayLine d[2];
    double lfo = 0;
    float ap[2][24] {}, fbs[2] {}, lp[2] {};
    void prepare (double sr) { for (auto& x : d) x.setSize ((int) (sr * 0.06)); }
    void reset() { for (auto& x : d) x.clear(); for (auto& a : ap) for (auto& v : a) v = 0; fbs[0] = fbs[1] = lp[0] = lp[1] = 0; }
    void process (const P&, double sr, float* L, float* R, int n);
};

struct DelayUnit
{
    DelayLine d[2];
    Allpass ap[2][2];
    float t[2] { -1, -1 }, hp[2] {}, lp[2] {};
    double lfo = 0;
    void prepare (double sr)
    {
        for (auto& x : d) x.setSize ((int) (sr * 4.2));
        for (int c = 0; c < 2; ++c) { ap[c][0].setSize ((int) (sr * (0.0071 + c * 0.0007))); ap[c][1].setSize ((int) (sr * (0.0113 + c * 0.0011))); }
    }
    void reset() { for (auto& x : d) x.clear(); for (auto& a : ap) { a[0].clear(); a[1].clear(); } hp[0] = hp[1] = lp[0] = lp[1] = 0; t[0] = t[1] = -1; }
    void process (const P&, double sr, double bpm, float* L, float* R, int n);
};

struct ReverbUnit
{
    static constexpr int N = 8;
    DelayLine pre[2], line[N];
    Allpass diff[2][6];
    float lp[N] {}, hpS[2] {};
    double lfo[N] {};
    void prepare (double sr);
    void reset();
    void process (const P&, double sr, float* L, float* R, int n);
};

struct TextureUnit
{
    struct Grain { bool on = false; double pos = 0, inc = 1; int len = 0, age = 0; float gl = 1, gr = 1; };
    DelayLine buf[2];
    Grain g[32];
    double spawn = 0;
    float lp[2] {};
    Allpass ap[2];
    Rng rng { 99 };
    void prepare (double sr) { for (auto& b : buf) b.setSize ((int) (sr * 4.5)); ap[0].setSize ((int) (sr * 0.0131)); ap[1].setSize ((int) (sr * 0.0173)); }
    void reset() { for (auto& b : buf) b.clear(); for (auto& x : g) x.on = false; lp[0] = lp[1] = 0; ap[0].clear(); ap[1].clear(); }
    void process (const P&, double sr, float* L, float* R, int n);
};

struct CompUnit
{
    float gr = 0;   // current gain reduction in dB (<= 0)
    void reset() { gr = 0; }
    void process (const P&, double sr, float* L, float* R, int n);
};

struct FxEqUnit
{
    Biquad b[4];
    float cache[10] { -999, -999, -999, -999, -999, -999, -999, -999, -999, -999 };
    void reset() { for (auto& x : b) x.reset(); }
    void process (const P&, double sr, float* L, float* R, int n);
};

struct DistUnit
{
    float lp[2] {}, hold[2] {}, cnt = 0;
    DcBlock dc[2];
    void reset() { lp[0] = lp[1] = hold[0] = hold[1] = 0; cnt = 0; }
    void process (const P&, double sr, float* L, float* R, int n);
};

struct FilterUnit
{
    Svf s[2];
    float g = -1;
    void reset() { s[0].reset(); s[1].reset(); g = -1; }
    void process (const P&, double sr, float* L, float* R, int n);
};

class FxRack
{
public:
    void prepare (double sr);
    void reset();
    // bus: main L/R then aux 1..3 L/R. Result ends up in bus[0], bus[1].
    void process (const Shared&, const float* gmod, float* const* bus, int n);
private:
    void runUnit (int entry, const Shared&, const float* gmod, float* L, float* R, int n);
    double sr = 48000;
    ModFxUnit modfx[3];
    DelayUnit delay[2];
    ReverbUnit reverb[2];
    TextureUnit texture[2];
    CompUnit comp[2];
    FxEqUnit eq[2];
    DistUnit dist[2];
    FilterUnit filt[2];
    float retPrev[3] { -1, -1, -1 };
};
}
