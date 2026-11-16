#include "Wavetables.h"
#include "../Params.h"

namespace mc::dsp
{
namespace
{
constexpr double twoPi = juce::MathConstants<double>::twoPi;
constexpr int A = 2048;   // analysis length

inline double frac (double x) { return x - std::floor (x); }
inline double tri (double x) { return 2.0 * std::abs (2.0 * frac (x + 0.25) - 1.0) - 1.0; }
inline double saw (double x) { return 2.0 * frac (x) - 1.0; }
inline double sq (double x) { return frac (x) < 0.5 ? 1.0 : -1.0; }

using Gen = std::function<void (double t, int frame, std::vector<double>& out)>;   // fills A samples of one cycle

void additive (std::vector<double>& out, const std::function<double (int)>& amp, int maxH = 400)
{
    // build the cycle from its spectrum with one inverse FFT
    static juce::dsp::FFT f (11);
    std::vector<float> b ((size_t) A * 2, 0.0f);
    for (int n = 1; n <= juce::jmin (maxH, A / 2 - 1); ++n)
        b[(size_t) 2 * n + 1] = (float) -amp (n);
    f.performRealOnlyInverseTransform (b.data());
    for (int i = 0; i < A; ++i) out[(size_t) i] = b[(size_t) i];
}

void timeDomain (std::vector<double>& out, const std::function<double (double)>& f)
{
    for (int i = 0; i < A; ++i) out[(size_t) i] = f ((double) i / A);
}

double formantAmp (double hz, const double* F, const double* G, int n)
{
    double a = 0;
    for (int k = 0; k < n; ++k)
    {
        const double bw = 60.0 + F[k] * 0.06;
        const double d = (hz - F[k]) / bw;
        a += G[k] / (1.0 + d * d);
    }
    return a;
}

std::vector<std::pair<juce::String, Gen>> definitions()
{
    std::vector<std::pair<juce::String, Gen>> d;
    auto names = mc::tableNames();
    int k = 0;
    auto add = [&] (Gen g) { d.push_back ({ names[k++], std::move (g) }); };

    add ([] (double t, int, std::vector<double>& o)       // Classic: sine > tri > saw > square
    {
        const double p = t * 3.0;
        const int seg = juce::jmin (2, (int) p);
        const double f = p - seg;
        timeDomain (o, [&] (double x)
        {
            const double s[] { std::sin (twoPi * x), tri (x), saw (x + 0.5), sq (x) };
            return s[seg] * (1.0 - f) + s[seg + 1] * f;
        });
    });
    add ([] (double t, int, std::vector<double>& o)       // Pulse width
    {
        const double duty = 0.5 - t * 0.45;
        timeDomain (o, [&] (double x) { return x < duty ? 1.0 : -1.0; });
    });
    add ([] (double t, int, std::vector<double>& o)       // Formant vox A E I O U
    {
        static const double V[5][3] { { 800, 1150, 2900 }, { 400, 1700, 2600 }, { 300, 2200, 3000 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
        static const double G[3] { 1.0, 0.5, 0.25 };
        const double p = t * 4.0;
        const int a = juce::jmin (3, (int) p);
        const double f = p - a;
        double F[3];
        for (int i = 0; i < 3; ++i) F[i] = V[a][i] * (1 - f) + V[a + 1][i] * f;
        additive (o, [&] (int n) { return formantAmp (n * 110.0, F, G, 3) / std::sqrt ((double) n); }, 120);
    });
    add ([] (double t, int, std::vector<double>& o)       // Hollow odd
    {
        const double p = 2.0 - 1.25 * t;
        additive (o, [&] (int n) { return (n % 2 == 1) ? 1.0 / std::pow ((double) n, p) : 0.0; }, 300);
    });
    add ([] (double, int fr, std::vector<double>& o)      // Organ drawbars
    {
        static const double R[8][6] { { 1, 0, 0, 0, 0, 0 }, { 1, 0.8, 0, 0, 0, 0 }, { 1, 0.8, 0.6, 0, 0, 0 }, { 1, 0.6, 0.6, 0.5, 0, 0 },
                                      { 1, 0.8, 0.6, 0.6, 0.4, 0 }, { 1, 0.4, 0.8, 0.3, 0.6, 0.4 }, { 1, 1, 1, 0.8, 0.8, 0.7 }, { 0.8, 1, 0.6, 1, 0.8, 1 } };
        static const int H[6] { 1, 2, 3, 4, 6, 8 };
        additive (o, [&] (int n) { for (int i = 0; i < 6; ++i) if (H[i] == n) return R[fr][i]; return 0.0; }, 8);
    });
    add ([] (double t, int, std::vector<double>& o)       // Bell partials (harmonic approximation)
    {
        additive (o, [&] (int n)
        {
            static const int P[] { 2, 3, 5, 7, 9, 11, 13, 17 };
            if (n == 1) return 1.0;
            for (int i = 0; i < 8; ++i) if (P[i] == n) return (0.15 + 0.85 * t) * std::exp (-i * (0.5 - 0.35 * t));
            return 0.0;
        }, 20);
    });
    add ([] (double t, int, std::vector<double>& o)       // Sync sweep
    {
        const double r = 1.0 + t * 7.0;
        timeDomain (o, [&] (double x) { return saw (x * r) * (1.0 - 0.3 * x); });
    });
    add ([] (double t, int, std::vector<double>& o)       // Fold sweep
    {
        const double g = 1.0 + t * 6.0;
        timeDomain (o, [&] (double x) { return std::sin (juce::MathConstants<double>::halfPi * g * std::sin (twoPi * x)); });
    });
    add ([] (double t, int, std::vector<double>& o)       // PD resonant (CZ style)
    {
        const double r = 1.0 + t * 11.0;
        timeDomain (o, [&] (double x) { return (1.0 - x) * std::cos (twoPi * x * r) - (1.0 - x) * 0.0; });
    });
    add ([] (double t, int, std::vector<double>& o)       // Brass swell
    {
        const double kk = 0.6 * (1 - t) * (1 - t) + 0.015;
        additive (o, [&] (int n) { return std::exp (-(n - 1) * kk) / n; }, 300);
    });
    add ([] (double t, int, std::vector<double>& o)       // Bowed string
    {
        const double pos = 0.08 + 0.3 * t;
        additive (o, [&] (int n) { return (0.15 + std::abs (std::sin (juce::MathConstants<double>::pi * n * pos))) / n; }, 300);
    });
    add ([] (double t, int, std::vector<double>& o)       // Glass
    {
        additive (o, [&] (int n)
        {
            if (n == 1) return 1.0;
            static const int P[] { 4, 7, 11, 17, 23, 31 };
            for (int i = 0; i < 6; ++i) if (P[i] == n) return (0.1 + 0.9 * t) * 0.8 / std::sqrt ((double) (i + 1));
            return 0.0;
        }, 40);
    });
    add ([] (double t, int, std::vector<double>& o)       // Growl
    {
        timeDomain (o, [&] (double x) { return std::tanh (1.6 * std::sin (twoPi * x + (0.5 + 4.0 * t) * std::sin (2.0 * twoPi * x))); });
    });
    add ([] (double t, int, std::vector<double>& o)       // Digital bits
    {
        const double lv = std::pow (2.0, 6.0 - 5.0 * t);
        timeDomain (o, [&] (double x) { const double v = 0.7 * std::sin (twoPi * x) + 0.3 * saw (x); return std::round (v * lv) / lv; });
    });
    add ([] (double t, int, std::vector<double>& o)       // Choir (Oo > Ah, breathy)
    {
        static const double V[2][3] { { 350, 650, 2700 }, { 750, 1200, 2800 } };
        static const double G[3] { 1.0, 0.45, 0.18 };
        double F[3];
        for (int i = 0; i < 3; ++i) F[i] = V[0][i] * (1 - t) + V[1][i] * t;
        additive (o, [&] (int n) { return formantAmp (n * 220.0, F, G, 3) / n; }, 60);
    });
    add ([] (double t, int, std::vector<double>& o)       // Dark pad
    {
        additive (o, [&] (int n) { return n > 24 ? 0.0 : (1.0 + t * n / 6.0) / ((double) n * n); }, 24);
    });
    add ([] (double, int fr, std::vector<double>& o)      // Spline shapes (seeded random control points)
    {
        Rng r ((uint32_t) (fr * 977 + 31));
        double pts[9];
        for (int i = 0; i < 8; ++i) pts[i] = r.bi();
        pts[8] = pts[0];
        timeDomain (o, [&] (double x)
        {
            const double p = x * 8.0;
            const int i = juce::jmin (7, (int) p);
            const double f = p - i;
            const double s = 0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * f);
            return pts[i] * (1 - s) + pts[i + 1] * s;
        });
    });
    add ([] (double t, int, std::vector<double>& o)       // Metallic
    {
        static const int P[] { 1, 5, 7, 11, 13, 17, 19, 23, 29, 31 };
        additive (o, [&] (int n)
        {
            for (int i = 0; i < 10; ++i)
                if (P[i] == n) return i == 0 ? 1.0 : (0.3 + 0.7 * t) * std::exp (-std::abs (i - 1 - t * 8.0) * 0.4) * 0.8;
            return 0.0;
        }, 32);
    });
    add ([] (double t, int, std::vector<double>& o)       // Reed
    {
        additive (o, [&] (int n) { return (n % 2 == 1 ? 1.0 : t * 0.6) / n * std::exp (-n * 0.03); }, 200);
    });
    add ([] (double t, int, std::vector<double>& o)       // Sine stack
    {
        const double top = 1.0 + 11.0 * t;
        additive (o, [&] (int n) { const double w = juce::jlimit (0.0, 1.0, top - n + 1.0); return w / n; }, 13);
    });
    add ([] (double t, int, std::vector<double>& o)       // Phase warp
    {
        const double dd = 0.5 - 0.46 * t;
        timeDomain (o, [&] (double x) { const double p = x < dd ? x * 0.5 / dd : 0.5 + (x - dd) * 0.5 / (1.0 - dd); return std::sin (twoPi * p); });
    });
    add ([] (double t, int, std::vector<double>& o)       // Triangle fold
    {
        const double g = 1.0 + 5.0 * t;
        timeDomain (o, [&] (double x) { return std::sin (juce::MathConstants<double>::halfPi * g * tri (x)); });
    });
    add ([] (double t, int, std::vector<double>& o)       // Clav pluck
    {
        const double duty = 0.45 - 0.37 * t;
        timeDomain (o, [&] (double x) { return (x < duty ? 1.0 : -1.0) * 0.8 + 0.2 * saw (x); });
    });
    add ([] (double t, int, std::vector<double>& o)       // Ghost
    {
        Rng ra (77), rb (91);
        std::vector<double> a (64), b (64);
        for (int i = 0; i < 64; ++i) { a[(size_t) i] = std::pow (ra.uni(), 2.0); b[(size_t) i] = std::pow (rb.uni(), 2.0); }
        additive (o, [&] (int n) { if (n > 64) return 0.0; const double v = a[(size_t) n - 1] * (1 - t) + b[(size_t) n - 1] * t; return (n == 1 ? 1.0 : v) / std::pow ((double) n, 0.8); }, 64);
    });
    return d;
}
}

WavetableBank::WavetableBank()
{
    juce::dsp::FFT fwd (11), inv (10);
    const int N = Wavetable::N;
    std::vector<double> cycle ((size_t) A);
    std::vector<float> spec ((size_t) A * 2), syn ((size_t) N * 2);

    for (auto& [name, gen] : definitions())
    {
        Wavetable w;
        w.name = name;
        w.data.assign ((size_t) Wavetable::F * Wavetable::M * (N + 1), 0.0f);
        for (int f = 0; f < Wavetable::F; ++f)
        {
            gen ((double) f / (Wavetable::F - 1), f, cycle);
            std::fill (spec.begin(), spec.end(), 0.0f);
            for (int i = 0; i < A; ++i) spec[(size_t) i] = (float) cycle[(size_t) i];
            fwd.performRealOnlyForwardTransform (spec.data());
            // spec now holds complex bins re,im interleaved: bin k at [2k],[2k+1]
            float peak0 = 0;
            for (int m = 0; m < Wavetable::M; ++m)
            {
                const int limit = 512 >> m;
                std::fill (syn.begin(), syn.end(), 0.0f);
                for (int k = 1; k <= limit && k < N / 2; ++k)
                {
                    syn[(size_t) 2 * k] = spec[(size_t) 2 * k];
                    syn[(size_t) 2 * k + 1] = spec[(size_t) 2 * k + 1];
                }
                inv.performRealOnlyInverseTransform (syn.data());
                float* dst = w.data.data() + ((size_t) f * Wavetable::M + (size_t) m) * (N + 1);
                for (int i = 0; i < N; ++i) dst[i] = syn[(size_t) i];
                dst[N] = dst[0];
                if (m == 0)
                    for (int i = 0; i < N; ++i) peak0 = juce::jmax (peak0, std::abs (dst[i]));
            }
            const float g = peak0 > 1e-9f ? 1.0f / peak0 : 0.0f;
            for (int m = 0; m < Wavetable::M; ++m)
            {
                float* dst = w.data.data() + ((size_t) f * Wavetable::M + (size_t) m) * (N + 1);
                for (int i = 0; i <= N; ++i) dst[i] *= g;
            }
            float mx = 0;
            for (int h = 1; h <= Wavetable::H; ++h)
            {
                const float re = spec[(size_t) 2 * h], im = spec[(size_t) 2 * h + 1];
                w.spec[f][h - 1] = std::sqrt (re * re + im * im);
                mx = juce::jmax (mx, w.spec[f][h - 1]);
            }
            for (int h = 0; h < Wavetable::H; ++h) w.spec[f][h] = mx > 0 ? w.spec[f][h] / mx : 0.0f;
        }
        tables.push_back (std::move (w));
    }
}

const WavetableBank& WavetableBank::get()
{
    static WavetableBank b;
    return b;
}
}
