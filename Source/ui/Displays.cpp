#include "Displays.h"
#include "../dsp/VoiceModules.h"

namespace mc::ui
{
using namespace mc::dsp;

// ======================================================================= LiveParams
LiveParams::LiveParams()
{
    const auto& defs = params();
    for (size_t i = 0; i < defs.size(); ++i)
    {
        sh.rng.push_back (defs[i].range());
        sh.val.push_back (defs[i].def);
        sh.norm.push_back (0.0f);
        sh.destOf.push_back (0);
    }
    sh.sr = 48000.0;
}

bool LiveParams::refresh (APVTS& s)
{
    bool changed = false;
    const auto& defs = params();
    for (size_t i = 0; i < defs.size(); ++i)
    {
        const float x = s.getRawParameterValue (defs[i].id)->load();
        if (x != sh.val[i]) { sh.val[i] = x; changed = true; }
    }
    if (changed) ++version;
    return changed;
}

float LiveParams::v (const juce::String& id) const
{
    const int i = paramIndex (id);
    return i < 0 ? 0.0f : sh.val[(size_t) i];
}

static void frame (juce::Graphics& g, juce::Rectangle<float> r, float s)
{
    g.setColour (col::bg);
    g.fillRoundedRectangle (r, 5.0f * s);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f * s, 1.0f);
}

static void grid (juce::Graphics& g, juce::Rectangle<float> r, int nx, int ny)
{
    g.setColour (col::edge.withAlpha (0.45f));
    for (int i = 1; i < nx; ++i) g.drawVerticalLine (juce::roundToInt (r.getX() + r.getWidth() * (float) i / (float) nx), r.getY(), r.getBottom());
    for (int i = 1; i < ny; ++i) g.drawHorizontalLine (juce::roundToInt (r.getY() + r.getHeight() * (float) i / (float) ny), r.getX(), r.getRight());
}

// ======================================================================= ModuleDisplay
void ModuleDisplay::tick()
{
    bool lfoMoving = false;
    if (kind == Kind::Lfo && live.c (Kind::Lfo, inst, lfop::mode) == 2)
    {
        lfoDot = proc.engine.lfoView[inst].load();
        lfoMoving = true;
    }
    if (live.version != lastVersion || lfoMoving || kind == Kind::Mseg)
    {
        lastVersion = live.version;
        repaint();
    }
}

void ModuleDisplay::drawWave (juce::Graphics& g, juce::Rectangle<float> r, const std::vector<float>& y, juce::Colour c, float th)
{
    if (y.empty()) return;
    juce::Path p;
    for (size_t i = 0; i < y.size(); ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (float) (y.size() - 1);
        const float yy = r.getCentreY() - juce::jlimit (-1.1f, 1.1f, y[i]) * r.getHeight() * 0.45f;
        if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
    }
    g.setColour (c.withAlpha (0.18f));
    g.strokePath (p, juce::PathStrokeType (th * 3.0f, juce::PathStrokeType::curved));
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (th, juce::PathStrokeType::curved));
}

void ModuleDisplay::drawResponse (juce::Graphics& g, juce::Rectangle<float> r, const std::function<float (float)>& magDb, float lo, float hi, juce::Colour c)
{
    // log frequency axis 20 Hz .. 20 kHz
    g.setColour (col::edge.withAlpha (0.5f));
    for (float f : { 100.0f, 1000.0f, 10000.0f })
    {
        const float x = r.getX() + r.getWidth() * std::log10 (f / 20.0f) / 3.0f;
        g.drawVerticalLine (juce::roundToInt (x), r.getY(), r.getBottom());
    }
    const float y0 = r.getY() + r.getHeight() * (hi / (hi - lo));
    g.drawHorizontalLine (juce::roundToInt (y0), r.getX(), r.getRight());
    juce::Path p;
    const int N = juce::jmax (64, (int) r.getWidth());
    for (int i = 0; i <= N; ++i)
    {
        const float f = 20.0f * std::pow (1000.0f, (float) i / (float) N);
        const float db = juce::jlimit (lo, hi, magDb (f));
        const float x = r.getX() + r.getWidth() * (float) i / (float) N;
        const float y = r.getY() + r.getHeight() * (hi - db) / (hi - lo);
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    juce::Path fill (p);
    fill.lineTo (r.getRight(), r.getBottom());
    fill.lineTo (r.getX(), r.getBottom());
    fill.closeSubPath();
    g.setColour (c.withAlpha (0.12f));
    g.fillPath (fill);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (2.0f * scaleOf (*this), juce::PathStrokeType::curved));
}

void ModuleDisplay::drawTransfer (juce::Graphics& g, juce::Rectangle<float> r, const std::function<float (float)>& f, juce::Colour c)
{
    g.setColour (col::edge.withAlpha (0.6f));
    g.drawHorizontalLine (juce::roundToInt (r.getCentreY()), r.getX(), r.getRight());
    g.drawVerticalLine (juce::roundToInt (r.getCentreX()), r.getY(), r.getBottom());
    g.drawLine (r.getX(), r.getBottom(), r.getRight(), r.getY(), 1.0f);
    std::vector<float> y (256);
    juce::Path p;
    for (int i = 0; i < 256; ++i)
    {
        const float x = -1.0f + 2.0f * (float) i / 255.0f;
        const float v = juce::jlimit (-1.2f, 1.2f, f (x));
        const float px = r.getX() + r.getWidth() * (float) i / 255.0f;
        const float py = r.getCentreY() - v * r.getHeight() * 0.45f;
        if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
    }
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (2.0f * scaleOf (*this), juce::PathStrokeType::curved));
}

static float lp2 (float x, float q) { return 1.0f / std::sqrt ((1 - x * x) * (1 - x * x) + (x / q) * (x / q)); }
static float db (float m) { return 20.0f * std::log10 (juce::jmax (1e-6f, m)); }

static float biquadDb (const Biquad& b, float f, double sr)
{
    const double w = juce::MathConstants<double>::twoPi * f / sr;
    const std::complex<double> z1 = std::polar (1.0, -w), z2 = z1 * z1;
    const auto num = (double) b.b0 + (double) b.b1 * z1 + (double) b.b2 * z2;
    const auto den = 1.0 + (double) b.a1 * z1 + (double) b.a2 * z2;
    return (float) (20.0 * std::log10 (juce::jmax (1e-9, std::abs (num / den))));
}

void ModuleDisplay::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto full = getLocalBounds().toFloat();
    frame (g, full, s);
    auto r = full.reduced (10.0f * s, 10.0f * s);
    const auto colour = kindColour (kind);
    const auto& sh = live.sh;
    auto V = [&] (int local) { return live.v (kind, inst, local); };
    auto C = [&] (int local) { return live.c (kind, inst, local); };

    switch (kind)
    {
        case Kind::Osc:
        {
            grid (g, r, 4, 4);
            const auto& T = WavetableBank::get().table (C (op::table));
            // ghost frames (the table's morph stack)
            for (int f = 0; f < Wavetable::F; ++f)
            {
                std::vector<float> y (128);
                for (int i = 0; i < 128; ++i) y[(size_t) i] = readTable (T, (float) f, 2, (float) i / 128.0f);
                const float off = ((float) f - 3.5f) * r.getHeight() * 0.035f;
                auto rr = r.translated (((float) f - 3.5f) * r.getWidth() * 0.012f, -off).reduced (r.getWidth() * 0.06f, r.getHeight() * 0.18f);
                juce::Path p;
                for (int i = 0; i < 128; ++i)
                {
                    const float x = rr.getX() + rr.getWidth() * (float) i / 127.0f;
                    const float yy = rr.getCentreY() - y[(size_t) i] * rr.getHeight() * 0.45f;
                    if (i == 0) p.startNewSubPath (x, yy); else p.lineTo (x, yy);
                }
                g.setColour (col::faint.withAlpha (0.35f));
                g.strokePath (p, juce::PathStrokeType (1.0f));
            }
            std::vector<float> y (512);
            renderOscCycle (sh, inst, y.data(), 512);
            drawWave (g, r, y, colour, 2.2f * s);
            g.setFont (Fonts::label (13.0f * s));
            g.setColour (col::dim);
            g.drawText (T.name + "  " + dot() + (C (op::engine) == 1 ? "  ADDITIVE" : "  WAVETABLE"), r.removeFromBottom (16 * s), juce::Justification::bottomLeft);
            break;
        }
        case Kind::FM:
        {
            grid (g, r, 4, 4);
            const float ratio = V (fmp::ratio), depth = V (fmp::depth), fb = V (fmp::feedback);
            std::vector<float> y (512);
            float m1 = 0;
            for (int i = 0; i < 512; ++i)
            {
                const float t = (float) i / 512.0f;
                const float m = fastSin (t * std::round (juce::jmax (1.0f, ratio)) + fb * fb * 0.32f * m1);
                m1 = m;
                y[(size_t) i] = fastSin (t + depth * depth * 3.0f * m);
            }
            drawWave (g, r, y, colour, 2.0f * s);
            break;
        }
        case Kind::Noise:
        {
            Rng rn (17);
            const float color = V (np::color);
            std::vector<float> y (400);
            float st = 0;
            const float a = color < 0 ? std::exp2 (color * 5.0f) : 1.0f;
            for (auto& v : y) { const float w = rn.bi(); st += a * (w - st); v = (color > 0 ? w - st * (1.0f - color) : st) * 0.8f; }
            drawWave (g, r, y, colour, 1.2f * s);
            break;
        }
        case Kind::Exc:
        {
            grid (g, r, 4, 4);
            Rng rn (3);
            const int mode = C (xp::mode);
            const float dec = V (xp::decay);
            std::vector<float> y (400);
            for (int i = 0; i < 400; ++i)
            {
                const float t = (float) i / 400.0f * juce::jmax (0.05f, dec * 0.0025f);
                const float env = std::exp (-t * 4.6f / juce::jmax (0.001f, dec * 0.001f));
                float v = 0;
                if (mode == 0) v = i < 3 ? 1.0f : 0.0f;
                else if (mode == 2) v = (t < 0.004f ? std::sin (kPi * t / 0.004f) : 0.0f) + rn.bi() * env * 0.1f;
                else if (mode == 3) v = rn.bi() * (1.0f - std::exp (-t * 30.0f)) * 0.5f;
                else if (mode == 4) v = rn.bi() * std::exp (-std::fmod (t, 1.0f / juce::jmax (0.5f, V (xp::rate))) * 4.6f / juce::jmax (0.001f, dec * 0.001f));
                else v = rn.bi() * env;
                y[(size_t) i] = v * 0.9f;
            }
            drawWave (g, r, y, colour, 1.2f * s);
            break;
        }
        case Kind::Res: case Kind::Comb:
        {
            const float f0 = 130.81f * std::exp2 ((kind == Kind::Res ? V (rp::tune) + V (rp::fine) * 0.01f : V (cp::tune) + V (cp::fine) * 0.01f) / 12.0f);
            if (kind == Kind::Res)
            {
                const int prof = C (rp::profile);
                const float* ratio = resonatorRatios (prof);
                const float* gain = resonatorGains (prof);
                const float bright = V (rp::bright);
                drawResponse (g, r, [] (float) { return -60.0f; }, -60, 6, col::edge);
                for (int m = 0; m < 12; ++m)
                {
                    if (ratio[m] <= 0) continue;
                    const float f = f0 * ratio[m];
                    if (f > 20000) continue;
                    const float x = r.getX() + r.getWidth() * std::log10 (f / 20.0f) / 3.0f;
                    const float h = juce::jlimit (0.05f, 1.0f, gain[m] * std::pow (juce::jmax (0.5f, ratio[m]), bright * 0.7f));
                    g.setColour (colour.withAlpha (0.25f));
                    g.fillRect (x - 3.0f * s, r.getBottom() - r.getHeight() * h, 6.0f * s, r.getHeight() * h);
                    g.setColour (colour);
                    g.fillRect (x - 1.0f * s, r.getBottom() - r.getHeight() * h, 2.0f * s, r.getHeight() * h);
                }
            }
            else
            {
                const float fb = V (cp::feedback);
                const float D = 48000.0f / f0;
                drawResponse (g, r, [&] (float f)
                {
                    const std::complex<float> e = std::polar (1.0f, -kTwoPi * f / 48000.0f * D);
                    return db (1.0f / std::abs (1.0f - fb * 0.98f * e) * (1.0f - std::abs (fb) * 0.5f));
                }, -30, 24, colour);
            }
            break;
        }
        case Kind::Vcf: case Kind::FxFilter:
        {
            const bool fxf = kind == Kind::FxFilter;
            const float fc = fxf ? V (ffp::cutoff) : V (vp::cutoff);
            const float res = fxf ? V (ffp::reso) : V (vp::reso);
            const int model = fxf ? 2 : C (vp::model);
            int resp = fxf ? (C (ffp::type) == 0 ? 2 : C (ffp::type) == 1 ? 7 : C (ffp::type) == 2 ? 4 : 9) : C (vp::response);
            const float q = 0.55f + res * res * 14.0f;
            drawResponse (g, r, [&] (float f)
            {
                const float x = f / fc;
                if (model == 9)
                {
                    static const float V5[5][3] { { 800, 1150, 2900 }, { 400, 1700, 2600 }, { 300, 2200, 3000 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
                    const float pos = juce::jlimit (0.0f, 1.0f, std::log2 (fc / 20.0f) / 9.966f) * 4.0f;
                    const int a = juce::jmin (3, (int) pos);
                    const float fr = pos - (float) a;
                    float m = 0;
                    const float amps[3] { 1.0f, 0.6f, 0.35f };
                    for (int j = 0; j < 3; ++j)
                    {
                        const float F = V5[a][j] + fr * (V5[a + 1][j] - V5[a][j]);
                        const float xx = f / F;
                        m += amps[j] * (xx / q) * lp2 (xx, q) / (1.0f / q) * 0.5f;
                    }
                    return db (m);
                }
                if (model == 10)
                {
                    const float ph = std::atan (x) * 8.0f;
                    return db (std::abs (std::cos (ph * 0.5f)) * (1.0f + res * 0.3f));
                }
                if (model == 12)
                    return db ((x / q) * lp2 (x, q) * (1.0f / q) * 1.5f + (f / (fc * 2.5f) / q) * lp2 (f / (fc * 2.5f), q) * (1.0f / q) * 1.2f + 1e-4f);
                const float l2 = lp2 (x, q), lb = lp2 (x, 0.707f), l1 = 1.0f / std::sqrt (1.0f + x * x);
                const float x2 = x * x;
                switch (resp)
                {
                    case 0: return db (l2 * lb);
                    case 1: return db (l2 * l1);
                    case 2: return db (l2);
                    case 3: return db (l1);
                    case 4: return db ((x / q) * l2);
                    case 5: return db (x * l1 * l1 * 2.0f);
                    case 6: return db (x2 * l2 * x2 * lb);
                    case 7: return db (x2 * l2);
                    case 8: return db (x * l1);
                    case 9: return db (std::abs (1.0f - x2) * l2);
                    case 10: return db (std::sqrt ((1 - x2) * (1 - x2) + 0.0f) * l2 + (x / q) * l2 * 0.5f);
                    default: return 0.0f;
                }
            }, -48, 24, colour);
            break;
        }
        case Kind::Shape: case Kind::Dist:
        {
            const bool d = kind == Kind::Dist;
            const int mode = C (0);
            const float drive = d ? V (dsp_::drive) : V (sp::drive), bias = d ? V (dsp_::bias) : V (sp::bias);
            const float dg = 1.0f + drive * drive * (d ? 40.0f : 24.0f);
            drawTransfer (g, r, [&] (float x)
            {
                if (d)
                    switch (mode)
                    {
                        case 0: return fastTanh (x * dg + bias * 0.4f) - fastTanh (bias * 0.4f);
                        case 1: return juce::jlimit (-1.0f, 1.0f, x * dg + bias * 0.3f);
                        case 2: { const float z = x * dg * 2.0f + bias * 0.5f; return z > 0 ? 1.0f - std::exp (-z) : -1.0f + std::exp (z * 1.3f); }
                        case 3: return fastSin ((x * dg * 0.3f + bias * 0.2f) * 0.25f);
                        case 4: return fastTanh (std::abs (x * dg * 0.5f) + bias * 0.2f);
                        case 5: { const float lv = std::exp2 (1.0f + (1.0f - drive) * 11.0f); return std::round (x * lv) / lv; }
                        default: return x;
                    }
                switch (mode)
                {
                    case 0: return fastTanh (x * dg + bias * 0.5f) - fastTanh (bias * 0.5f);
                    case 1: return juce::jlimit (-1.0f, 1.0f, x * dg + bias * 0.3f);
                    case 2: return fastSin ((x * dg * 0.5f + bias * 0.25f) * 0.25f);
                    case 3: { const float u = x * dg * 0.5f + bias; return 4.0f * std::abs (frac (u * 0.25f + 0.25f) - 0.5f) - 1.0f; }
                    case 4: return fastSin ((x * (0.6f + drive * 5.0f) + bias * 0.4f) * 0.25f);
                    case 5: { const float z = x * dg * 0.4f; return fastTanh (bias < 0 ? juce::jmax (z, z * (1.0f + bias)) : std::abs (z)) * 1.3f - 0.4f; }
                    case 6: { const float lv = std::exp2 (1.0f + (1.0f - drive) * 11.0f); return std::round (x * lv) / lv; }
                    default: return std::round (x * 8.0f) / 8.0f;
                }
            }, colour);
            break;
        }
        case Kind::Ring:
        {
            grid (g, r, 4, 4);
            const int src = C (gp::source);
            const float ratio = std::exp2 (V (gp::tune) / 12.0f);
            std::vector<float> y (512);
            for (int i = 0; i < 512; ++i)
            {
                const float t = (float) i / 512.0f * 2.0f;
                const float x = 2.0f * frac (t) - 1.0f;
                float m = src == 0 ? fastSin (t * ratio) : src == 1 ? 1.0f - 4.0f * std::abs (frac (t * ratio + 0.25f) - 0.5f)
                                                         : src == 2 ? (frac (t * ratio) < 0.5f ? 1.0f : -1.0f) : fastSin (t * 1.5f);
                y[(size_t) i] = x + V (gp::mix) * (x * m - x);
            }
            drawWave (g, r, y, colour, 1.6f * s);
            break;
        }
        case Kind::Eq: case Kind::FxEq:
        {
            Biquad b[4];
            int nb = 0;
            if (kind == Kind::Eq)
            {
                b[0].lowShelf (48000, V (ep::lowFreq), V (ep::lowGain));
                b[1].peak (48000, V (ep::midFreq), V (ep::midGain), V (ep::midQ));
                b[2].highShelf (48000, V (ep::highFreq), V (ep::highGain));
                nb = 3;
            }
            else
            {
                b[0].lowShelf (48000, V (feq::lowFreq), V (feq::lowGain));
                b[1].peak (48000, V (feq::m1Freq), V (feq::m1Gain), V (feq::m1Q));
                b[2].peak (48000, V (feq::m2Freq), V (feq::m2Gain), V (feq::m2Q));
                b[3].highShelf (48000, V (feq::highFreq), V (feq::highGain));
                nb = 4;
            }
            drawResponse (g, r, [&] (float f) { float d = 0; for (int k = 0; k < nb; ++k) d += biquadDb (b[k], f, 48000); return d; }, -20, 20, colour);
            break;
        }
        case Kind::Env:
        {
            grid (g, r, 4, 4);
            const float del = V (envp::delay), a = V (envp::attack), d = V (envp::decay), su = V (envp::sustain), rl = V (envp::release), c = V (envp::slope);
            const float hold = juce::jmax (0.25f, (del + a + d) * 0.35f);
            const float total = del + a + d + hold + rl;
            auto X = [&] (float t) { return r.getX() + r.getWidth() * t / total; };
            auto Y = [&] (float v) { return r.getBottom() - r.getHeight() * 0.92f * v; };
            juce::Path p;
            p.startNewSubPath (X (0), Y (0));
            p.lineTo (X (del), Y (0));
            auto curveA = [&] (float pp) { return c >= 0 ? 1.0f - std::pow (1.0f - pp, 1.0f + c * 3.0f) : std::pow (pp, 1.0f - c * 2.0f); };
            auto curveD = [&] (float pp) { return c >= 0 ? 1.0f - std::pow (1.0f - pp, 1.0f + c * 4.0f) : std::pow (pp, 1.0f - c * 2.0f); };
            for (int i = 1; i <= 40; ++i) { const float pp = (float) i / 40.0f; p.lineTo (X (del + a * pp), Y (curveA (pp))); }
            for (int i = 1; i <= 40; ++i) { const float pp = (float) i / 40.0f; p.lineTo (X (del + a + d * pp), Y (1.0f + (su - 1.0f) * curveD (pp))); }
            p.lineTo (X (del + a + d + hold), Y (su));
            for (int i = 1; i <= 40; ++i) { const float pp = (float) i / 40.0f; p.lineTo (X (del + a + d + hold + rl * pp), Y (su * (1.0f - curveD (pp)))); }
            juce::Path f (p);
            f.lineTo (X (total), Y (0));
            f.closeSubPath();
            g.setColour (colour.withAlpha (0.14f));
            g.fillPath (f);
            g.setColour (colour);
            g.strokePath (p, juce::PathStrokeType (2.2f * s, juce::PathStrokeType::curved));
            g.setFont (Fonts::label (12.5f * s));
            g.setColour (col::dim);
            g.drawText (juce::String (total, 2) + " s shown", r.removeFromTop (14 * s), juce::Justification::topRight);
            break;
        }
        case Kind::Lfo:
        {
            grid (g, r, 4, 2);
            const int w = C (lfop::wave);
            Rng rn (5);
            LfoState st;
            std::vector<float> y (400);
            for (int i = 0; i < 400; ++i)
            {
                const double ph = std::fmod ((double) i / 200.0 + V (lfop::phase), 1.0);
                const bool wrapped = i > 0 && std::fmod ((double) (i - 1) / 200.0 + V (lfop::phase), 1.0) > ph;
                y[(size_t) i] = lfoShape (w, ph, st, wrapped, rn) * V (lfop::amount);
                if (live.c (Kind::Lfo, inst, lfop::unipolar) > 0) y[(size_t) i] = y[(size_t) i] * 0.5f + 0.5f * V (lfop::amount);
            }
            drawWave (g, r, y, colour, 2.0f * s);
            if (C (lfop::mode) == 2)
            {
                const float yy = r.getCentreY() - lfoDot * r.getHeight() * 0.45f;
                g.setColour (col::amber);
                g.fillEllipse (r.getRight() - 12 * s, yy - 5 * s, 10 * s, 10 * s);
            }
            g.setFont (Fonts::label (12.5f * s));
            g.setColour (col::dim);
            const auto rate = live.c (Kind::Lfo, inst, lfop::sync) > 0 ? divisionNames()[C (lfop::division)] : juce::String (V (lfop::rate), 2) + " Hz";
            g.drawText ("2 cycles  " + dot() + "  " + rate, r.removeFromBottom (14 * s), juce::Justification::bottomLeft);
            break;
        }
        case Kind::Math:
        {
            auto sources = sourceNames();
            juce::StringArray mathSrc;
            for (int k = 0; k < sNumSources; ++k) if (! (k >= sMath1 && k < sMath1 + 4)) mathSrc.add (sources[k]);
            const auto ops = params()[(size_t) pidx (Kind::Math, inst, mathp::opr)].choices;
            g.setFont (Fonts::title (20.0f * s));
            g.setColour (colour);
            g.drawText ("MATH " + juce::String (inst + 1), r.removeFromTop (34 * s), juce::Justification::centredLeft);
            g.setFont (Fonts::value (17.0f * s));
            g.setColour (col::text);
            auto line = [&] (const juce::String& t) { g.drawText (t, r.removeFromTop (26 * s), juce::Justification::centredLeft); };
            line ("A = " + mathSrc[C (mathp::srcA)]);
            line ("B = " + mathSrc[C (mathp::srcB)]);
            line ("Out = (" + ops[C (mathp::opr)] + ") x " + juce::String (juce::roundToInt (V (mathp::amount) * 100)) + "%");
            g.setFont (Fonts::value (14.0f * s));
            g.setColour (col::dim);
            g.drawFittedText ("Use Math sources in the matrix like any other modulator. Lag smooths a stepped source, Quantize turns "
                              "a smooth one into steps, Max and Min combine two envelopes.", r.toNearestInt(), juce::Justification::topLeft, 4);
            break;
        }
        case Kind::ModFx:
        {
            grid (g, r, 4, 2);
            std::vector<float> y (400);
            const float depth = V (mfx::depth);
            for (int i = 0; i < 400; ++i) y[(size_t) i] = depth * fastSin ((float) i / 400.0f * 2.0f);
            drawWave (g, r, y, colour, 2.0f * s);
            g.setFont (Fonts::label (13.0f * s));
            g.setColour (col::dim);
            g.drawText (juce::String (V (mfx::rate), 2) + " Hz sweep", r.removeFromBottom (16 * s), juce::Justification::bottomLeft);
            break;
        }
        case Kind::Delay:
        {
            grid (g, r, 8, 2);
            const float fb = V (dly::feedback);
            float lvl = 1.0f;
            for (int k = 0; k < 10; ++k)
            {
                const float x = r.getX() + r.getWidth() * (0.06f + 0.09f * (float) k);
                const float h = r.getHeight() * 0.9f * lvl;
                const bool right = C (dly::mode) == 2 ? (k & 1) : false;
                g.setColour ((right ? col::amber : colour).withAlpha (0.85f));
                g.fillRoundedRectangle (x, r.getBottom() - h, 7.0f * s, h, 2.0f * s);
                lvl *= juce::jmin (1.0f, fb);
            }
            break;
        }
        case Kind::Reverb: case Kind::Texture:
        {
            grid (g, r, 4, 2);
            Rng rn (kind == Kind::Reverb ? 9 : 21);
            if (kind == Kind::Reverb)
            {
                const float dec = V (rvb::decay);
                juce::Path p;
                p.startNewSubPath (r.getX(), r.getBottom());
                for (int i = 0; i <= 200; ++i)
                {
                    const float t = (float) i / 200.0f * juce::jmax (1.0f, dec * 1.2f);
                    const float env = std::pow (10.0f, -3.0f * t / juce::jmax (0.1f, dec));
                    p.lineTo (r.getX() + r.getWidth() * (float) i / 200.0f, r.getBottom() - r.getHeight() * 0.9f * env * (0.75f + 0.25f * rn.uni()));
                }
                p.lineTo (r.getRight(), r.getBottom());
                p.closeSubPath();
                g.setColour (colour.withAlpha (0.3f));
                g.fillPath (p);
                g.setColour (colour);
                g.strokePath (p, juce::PathStrokeType (1.2f * s));
            }
            else
            {
                const int n = 20 + (int) (V (txp::density) * 120);
                const float len = V (txp::grain) / 800.0f;
                for (int k = 0; k < n; ++k)
                {
                    const float x = r.getX() + r.getWidth() * rn.uni();
                    const float y = r.getCentreY() + r.getHeight() * 0.4f * (V (txp::pitch) / -24.0f + rn.bi() * (0.1f + V (txp::spread)));
                    const float w = juce::jmax (3.0f, r.getWidth() * len * 0.3f);
                    g.setColour ((rn.uni() < V (txp::reverse) ? col::amber : colour).withAlpha (0.45f));
                    g.fillRoundedRectangle (x - w * 0.5f, y - 3 * s, w, 6 * s, 3 * s);
                }
            }
            break;
        }
        case Kind::Comp:
        {
            const float thr = V (cmp::threshold), ratio = V (cmp::ratio), knee = V (cmp::knee);
            drawTransfer (g, r, [&] (float x)
            {
                const float in = (x + 1.0f) * 30.0f - 60.0f;
                const float over = in - thr;
                float out = in;
                if (2 * over > knee) out = thr + over / ratio;
                else if (knee > 0 && 2 * std::abs (over) <= knee) out = in + (1.0f / ratio - 1.0f) * (over + knee * 0.5f) * (over + knee * 0.5f) / (2 * knee);
                return (out + 60.0f) / 30.0f - 1.0f;
            }, colour);
            break;
        }
        default: break;
    }
}

// ======================================================================= MsegEditor
MsegEditor::MsegEditor (ManticoreProcessor& p, LiveParams& lp) : proc (p), live (lp)
{
    setTooltip ("Drag points to shape the curve. Double-click empty space to add a point, double-click a point to delete it. "
                "Drag the small diamonds up and down to bend a segment. Right-click for loop markers and ready-made shapes. Hold Alt to move freely off the grid.");
}

juce::Rectangle<float> MsegEditor::plot() const
{
    const float s = scaleOf (*this);
    return getLocalBounds().toFloat().reduced (12 * s, 12 * s).withTrimmedBottom (8 * s);
}

float MsegEditor::viewLen() const
{
    if (drag >= 0) return frozenLen;
    const auto& d = proc.mseg[index];
    return juce::jmax (1.0f, std::ceil (d.length() * 1.15f));
}

juce::Point<float> MsegEditor::toScreen (float x, float y) const
{
    const auto r = plot();
    return { r.getX() + r.getWidth() * x / viewLen(), r.getBottom() - r.getHeight() * y };
}

juce::Point<float> MsegEditor::fromScreen (juce::Point<float> p) const
{
    const auto r = plot();
    return { (p.x - r.getX()) / r.getWidth() * viewLen(), (r.getBottom() - p.y) / r.getHeight() };
}

int MsegEditor::hitPoint (juce::Point<float> p) const
{
    const float s = scaleOf (*this);
    const auto& d = proc.mseg[index];
    for (int i = 0; i < d.n; ++i)
        if (toScreen (d.x[i], d.y[i]).getDistanceFrom (p) < 8.0f * s) return i;
    return -1;
}

int MsegEditor::hitHandle (juce::Point<float> p) const
{
    const float s = scaleOf (*this);
    const auto& d = proc.mseg[index];
    for (int i = 1; i < d.n; ++i)
    {
        const float xm = 0.5f * (d.x[i - 1] + d.x[i]);
        const float ym = d.valueAt (xm);
        if (toScreen (xm, ym).getDistanceFrom (p) < 7.0f * s) return i;
    }
    return -1;
}

void MsegEditor::changed()
{
    proc.shapesChanged();
    repaint();
}

void MsegEditor::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    frame (g, getLocalBounds().toFloat(), s);
    const auto r = plot();
    const auto& d = proc.mseg[index];
    const float len = viewLen();
    // grid in units (quarter divisions)
    for (int i = 0; i <= (int) (len * 4); ++i)
    {
        const float x = r.getX() + r.getWidth() * (float) i / (len * 4.0f);
        g.setColour (col::edge.withAlpha (i % 4 == 0 ? 0.8f : 0.3f));
        g.drawVerticalLine (juce::roundToInt (x), r.getY(), r.getBottom());
    }
    g.setColour (col::edge.withAlpha (0.5f));
    for (int i = 1; i < 4; ++i) g.drawHorizontalLine (juce::roundToInt (r.getY() + r.getHeight() * (float) i / 4.0f), r.getX(), r.getRight());
    // loop region
    const int loop = live.c (Kind::Mseg, index, msp::loop);
    if (loop > 0)
    {
        const float a = loop == 2 ? 0.0f : d.x[d.loopStart], b = loop == 2 ? d.length() : d.x[d.loopEnd];
        const auto pa = toScreen (a, 0), pb = toScreen (b, 0);
        g.setColour (col::amber.withAlpha (0.08f));
        g.fillRect (juce::Rectangle<float> (pa.x, r.getY(), pb.x - pa.x, r.getHeight()));
        g.setColour (col::amber.withAlpha (0.7f));
        g.drawVerticalLine (juce::roundToInt (pa.x), r.getY(), r.getBottom());
        g.drawVerticalLine (juce::roundToInt (pb.x), r.getY(), r.getBottom());
        g.setFont (Fonts::label (12.0f * s));
        g.drawText (loop == 2 ? "REPEAT" : "SUSTAIN LOOP", juce::Rectangle<float> (pa.x + 4 * s, r.getY() + 2 * s, 120 * s, 14 * s), juce::Justification::left);
    }
    // curve
    juce::Path p;
    const int N = juce::jmax (100, (int) r.getWidth());
    for (int i = 0; i <= N; ++i)
    {
        const float x = d.length() * (float) i / (float) N;
        const auto pt = toScreen (x, d.valueAt (x));
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    juce::Path f (p);
    f.lineTo (toScreen (d.length(), 0));
    f.lineTo (toScreen (0, 0));
    f.closeSubPath();
    const auto c = kindColour (Kind::Mseg);
    g.setColour (c.withAlpha (0.12f));
    g.fillPath (f);
    g.setColour (c);
    g.strokePath (p, juce::PathStrokeType (2.2f * s, juce::PathStrokeType::curved));
    // handles and points
    for (int i = 1; i < d.n; ++i)
    {
        const float xm = 0.5f * (d.x[i - 1] + d.x[i]);
        const auto pt = toScreen (xm, d.valueAt (xm));
        juce::Path dia;
        const float k = 4.0f * s;
        dia.addQuadrilateral (pt.x, pt.y - k, pt.x + k, pt.y, pt.x, pt.y + k, pt.x - k, pt.y);
        g.setColour (col::amber.withAlpha (dragHandle == i ? 1.0f : 0.6f));
        g.fillPath (dia);
    }
    for (int i = 0; i < d.n; ++i)
    {
        const auto pt = toScreen (d.x[i], d.y[i]);
        const float rr = (i == hover || i == drag ? 6.5f : 5.0f) * s;
        g.setColour (col::bg);
        g.fillEllipse (pt.x - rr, pt.y - rr, rr * 2, rr * 2);
        g.setColour (i == drag ? col::amber : c);
        g.drawEllipse (pt.x - rr, pt.y - rr, rr * 2, rr * 2, 2.0f * s);
    }
    g.setFont (Fonts::label (12.0f * s));
    g.setColour (col::dim);
    g.drawText (live.c (Kind::Mseg, index, msp::sync) > 0 ? "GRID = 1 BEAT" : "GRID = 1 SECOND",
                getLocalBounds().toFloat().reduced (12 * s, 4 * s).removeFromBottom (14 * s), juce::Justification::bottomRight);
}

void MsegEditor::mouseMove (const juce::MouseEvent& e)
{
    const int h = hitPoint (e.position);
    if (h != hover) { hover = h; repaint(); }
}

void MsegEditor::mouseDown (const juce::MouseEvent& e)
{
    auto& d = proc.mseg[index];
    frozenLen = viewLen();
    if (e.mods.isPopupMenu())
    {
        const int h = hitPoint (e.position);
        if (h >= 0)
        {
            juce::PopupMenu m;
            m.addItem (1, "Set loop start here", h < d.n - 1);
            m.addItem (2, "Set loop end here", h > 0);
            m.addItem (3, "Delete point", h > 0 && d.n > 2);
            juce::Component::SafePointer<MsegEditor> safe (this);
            m.showMenuAsync ({}, [safe, h] (int r)
            {
                if (safe == nullptr) return;
                auto& dd = safe->proc.mseg[safe->index];
                if (r == 1) { dd.loopStart = h; if (dd.loopEnd <= h) dd.loopEnd = juce::jmin (dd.n - 1, h + 1); }
                else if (r == 2) { dd.loopEnd = h; if (dd.loopStart >= h) dd.loopStart = juce::jmax (0, h - 1); }
                else if (r == 3)
                {
                    for (int k = h; k < dd.n - 1; ++k) { dd.x[k] = dd.x[k + 1]; dd.y[k] = dd.y[k + 1]; dd.c[k] = dd.c[k + 1]; }
                    --dd.n;
                    dd.loopStart = juce::jmin (dd.loopStart, dd.n - 2); dd.loopEnd = juce::jlimit (dd.loopStart + 1, dd.n - 1, dd.loopEnd);
                }
                else return;
                safe->changed();
            });
        }
        else templates();
        return;
    }
    drag = hitPoint (e.position);
    dragHandle = drag < 0 ? hitHandle (e.position) : -1;
    if (dragHandle >= 0) { dragStartC = d.c[dragHandle]; dragStartY = e.position.y; }
}

void MsegEditor::mouseDrag (const juce::MouseEvent& e)
{
    auto& d = proc.mseg[index];
    if (dragHandle >= 0)
    {
        const float dy = (dragStartY - e.position.y) / juce::jmax (40.0f, plot().getHeight());
        const bool rising = d.y[dragHandle] >= d.y[dragHandle - 1];
        d.c[dragHandle] = juce::jlimit (-1.0f, 1.0f, dragStartC + (rising ? -dy : dy) * 2.0f);
        changed();
        return;
    }
    if (drag < 0) return;
    auto p = fromScreen (e.position);
    const bool snap = ! e.mods.isAltDown();
    if (snap) { p.x = std::round (p.x * 16.0f) / 16.0f; p.y = std::round (p.y * 48.0f) / 48.0f; }
    d.y[drag] = juce::jlimit (0.0f, 1.0f, p.y);
    if (drag == 0) d.x[0] = 0;
    else
    {
        const float lo = d.x[drag - 1];
        const float hi = drag < d.n - 1 ? d.x[drag + 1] : 16.0f;
        d.x[drag] = juce::jlimit (lo, hi, p.x);
    }
    changed();
}

void MsegEditor::mouseUp (const juce::MouseEvent&)
{
    drag = -1;
    dragHandle = -1;
    repaint();
}

void MsegEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    auto& d = proc.mseg[index];
    const int h = hitPoint (e.position);
    if (h > 0 && d.n > 2)
    {
        for (int k = h; k < d.n - 1; ++k) { d.x[k] = d.x[k + 1]; d.y[k] = d.y[k + 1]; d.c[k] = d.c[k + 1]; }
        --d.n;
        d.loopStart = juce::jmin (d.loopStart, d.n - 2);
        d.loopEnd = juce::jlimit (d.loopStart + 1, d.n - 1, d.loopEnd);
        changed();
        return;
    }
    if (h >= 0 || d.n >= kMsegMaxPoints) return;
    auto p = fromScreen (e.position);
    p.x = juce::jlimit (0.0f, 16.0f, std::round (p.x * 16.0f) / 16.0f);
    p.y = juce::jlimit (0.0f, 1.0f, p.y);
    int at = d.n;
    for (int k = 0; k < d.n; ++k) if (d.x[k] > p.x) { at = k; break; }
    if (at == 0) return;
    for (int k = d.n; k > at; --k) { d.x[k] = d.x[k - 1]; d.y[k] = d.y[k - 1]; d.c[k] = d.c[k - 1]; }
    d.x[at] = p.x; d.y[at] = p.y; d.c[at] = 0;
    ++d.n;
    if (d.loopStart >= at) ++d.loopStart;
    if (d.loopEnd >= at) ++d.loopEnd;
    changed();
}

void MsegEditor::templates()
{
    juce::PopupMenu m;
    m.addSectionHeader ("Ready-made shapes");
    m.addItem (1, "Pluck (fast attack, decay)");
    m.addItem (2, "Slow swell");
    m.addItem (3, "Rise and fall");
    m.addItem (4, "Ramp down (saw)");
    m.addItem (5, "Rhythmic gate (8 steps, repeat)");
    m.addItem (6, "Stair steps up");
    m.addItem (7, "Reset to default");
    juce::Component::SafePointer<MsegEditor> safe (this);
    m.showMenuAsync ({}, [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& d = safe->proc.mseg[safe->index];
        auto set = [&] (std::initializer_list<std::array<float, 3>> pts, int ls, int le)
        {
            d.n = 0;
            for (auto& p : pts) { d.x[d.n] = p[0]; d.y[d.n] = p[1]; d.c[d.n] = p[2]; ++d.n; }
            d.loopStart = ls; d.loopEnd = le;
        };
        switch (r)
        {
            case 1: set ({ { 0, 0, 0 }, { 0.02f, 1, 0 }, { 0.6f, 0, 0.6f } }, 0, 1); break;
            case 2: set ({ { 0, 0, 0 }, { 2.5f, 1, -0.4f }, { 4, 0.8f, 0 } }, 1, 2); break;
            case 3: set ({ { 0, 0, 0 }, { 1, 1, -0.3f }, { 2, 0, 0.3f } }, 0, 2); break;
            case 4: set ({ { 0, 1, 0 }, { 1, 0, 0 } }, 0, 1); break;
            case 5:
            {
                d.n = 0;
                const float pat[8] { 1, 0.2f, 0.8f, 0, 1, 0.5f, 0, 0.7f };
                for (int k = 0; k < 8; ++k)
                {
                    d.x[d.n] = k * 0.25f; d.y[d.n] = pat[k]; d.c[d.n] = 0; ++d.n;
                    d.x[d.n] = k * 0.25f + 0.2f; d.y[d.n] = pat[k] * 0.1f; d.c[d.n] = 0.5f; ++d.n;
                }
                d.loopStart = 0; d.loopEnd = d.n - 1;
                break;
            }
            case 6: set ({ { 0, 0, 0 }, { 0.5f, 0, 0 }, { 0.5f, 0.33f, 0 }, { 1, 0.33f, 0 }, { 1, 0.66f, 0 }, { 1.5f, 0.66f, 0 }, { 1.5f, 1, 0 }, { 2, 1, 0 } }, 0, 7); break;
            default: d = MsegData(); break;
        }
        safe->changed();
    });
}

// ======================================================================= MapEditor
MapEditor::MapEditor (ManticoreProcessor& p, LiveParams& lp) : proc (p), live (lp)
{
    setTooltip ("Draw the bars with the mouse. In Map mode the input picks a bar (left = low); in Step Seq mode the bars play in time. "
                "Right-click for ready-made patterns.");
}

juce::Rectangle<float> MapEditor::plot() const
{
    const float s = scaleOf (*this);
    return getLocalBounds().toFloat().reduced (12 * s, 12 * s);
}

void MapEditor::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    frame (g, getLocalBounds().toFloat(), s);
    const auto r = plot();
    const auto& d = proc.map[index];
    const int steps = juce::jlimit (2, kMapSteps, juce::roundToInt (live.v (Kind::Map, index, mpp::steps)));
    const float w = r.getWidth() / (float) steps;
    g.setColour (col::edge);
    g.drawHorizontalLine (juce::roundToInt (r.getCentreY()), r.getX(), r.getRight());
    const auto c = kindColour (Kind::Map);
    for (int k = 0; k < steps; ++k)
    {
        const float v = d.v[k];
        const float x = r.getX() + w * (float) k;
        const float h = r.getHeight() * 0.5f * std::abs (v);
        const float y = v >= 0 ? r.getCentreY() - h : r.getCentreY();
        g.setColour (c.withAlpha (0.25f));
        g.fillRect (x + 1.0f * s, y, w - 2.0f * s, h);
        g.setColour (c);
        g.fillRect (x + 1.0f * s, v >= 0 ? y : y + h - 2.0f * s, w - 2.0f * s, 2.0f * s);
        if (k % 4 == 0)
        {
            g.setColour (col::edge.withAlpha (0.6f));
            g.drawVerticalLine (juce::roundToInt (x), r.getY(), r.getBottom());
        }
    }
    g.setFont (Fonts::label (12.0f * s));
    g.setColour (col::dim);
    g.drawText (live.c (Kind::Map, index, mpp::mode) == 1 ? "STEP SEQUENCER" : "TRANSFER MAP",
                getLocalBounds().toFloat().reduced (12 * s, 2 * s).removeFromBottom (12 * s), juce::Justification::bottomRight);
}

void MapEditor::setAt (juce::Point<float> p)
{
    const auto r = plot();
    const int steps = juce::jlimit (2, kMapSteps, juce::roundToInt (live.v (Kind::Map, index, mpp::steps)));
    const int cell = juce::jlimit (0, steps - 1, (int) ((p.x - r.getX()) / r.getWidth() * (float) steps));
    const float v = juce::jlimit (-1.0f, 1.0f, (r.getCentreY() - p.y) / (r.getHeight() * 0.5f));
    auto& d = proc.map[index];
    if (lastCell >= 0 && lastCell != cell)
    {
        const int a = juce::jmin (lastCell, cell), b = juce::jmax (lastCell, cell);
        for (int k = a; k <= b; ++k)
        {
            const float t = b == a ? 1.0f : (float) (k - lastCell) / (float) (cell - lastCell);
            d.v[k] = lastVal + (v - lastVal) * t;
        }
    }
    d.v[cell] = v;
    lastCell = cell;
    lastVal = v;
    proc.shapesChanged();
    repaint();
}

void MapEditor::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu()) { templates(); return; }
    lastCell = -1;
    setAt (e.position);
}

void MapEditor::mouseDrag (const juce::MouseEvent& e) { setAt (e.position); }
void MapEditor::mouseUp (const juce::MouseEvent&) { lastCell = -1; }

void MapEditor::templates()
{
    juce::PopupMenu m;
    m.addSectionHeader ("Patterns");
    m.addItem (1, "Ramp up");
    m.addItem (2, "Ramp down");
    m.addItem (3, "Triangle");
    m.addItem (4, "Sine");
    m.addItem (5, "Random");
    m.addItem (6, "Octave steps (for pitch)");
    m.addItem (7, "Flat zero");
    juce::Component::SafePointer<MapEditor> safe (this);
    m.showMenuAsync ({}, [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& d = safe->proc.map[safe->index];
        Rng rn ((uint32_t) juce::Time::getMillisecondCounter());
        for (int k = 0; k < kMapSteps; ++k)
        {
            const float t = (float) k / (float) (kMapSteps - 1);
            switch (r)
            {
                case 1: d.v[k] = t * 2 - 1; break;
                case 2: d.v[k] = 1 - t * 2; break;
                case 3: d.v[k] = 1.0f - 4.0f * std::abs (t - 0.5f); break;
                case 4: d.v[k] = std::sin (t * kTwoPi); break;
                case 5: d.v[k] = rn.bi(); break;
                case 6: d.v[k] = (float) ((k % 4) - 0) / 4.0f; break;
                default: d.v[k] = 0; break;
            }
        }
        safe->proc.shapesChanged();
        safe->repaint();
    });
}
}
