#include "Effects.h"

namespace mc::dsp
{
// ======================================================================= Mod FX
void ModFxUnit::process (const P& p, double sr, float* L, float* R, int n)
{
    const int mode = p.ch (mfx::mode);
    const float rate = p (mfx::rate), depth = clampf (p (mfx::depth), 0, 1), fb = clampf (p (mfx::feedback), -0.95f, 0.95f);
    const float centre = clampf (p (mfx::centre), 0, 1), width = p (mfx::width), mix = clampf (p (mfx::mix), 0, 1);
    const bool analog = p.ch (mfx::character) == 1;
    const int voices = juce::roundToInt (p (mfx::voices));
    const float lpA = OnePole::coef (6500.0f, sr);
    const double inc = rate / sr;
    for (int i = 0; i < n; ++i)
    {
        lfo += inc;
        if (lfo >= 1.0) lfo -= 1.0;
        const float xl = L[i], xr = R[i];
        float wl = 0, wr = 0;
        if (mode == 0)
        {
            const int nv = juce::jlimit (2, 8, voices);
            const float base = (5.0f + centre * 20.0f) * 0.001f * (float) sr;
            const float dep = depth * juce::jmin (6.0f, 4.0f + centre * 8.0f) * 0.001f * (float) sr;
            d[0].push (0.5f * (xl + xr));
            for (int k = 0; k < nv; ++k)
            {
                const float ph = (float) lfo + (float) k / (float) nv;
                const float v = d[0].read (base + dep * (0.5f + 0.5f * fastSin (ph)));
                const float pan = ((float) k / (float) (nv - 1) * 2.0f - 1.0f) * width;
                wl += v * (1.0f - pan); wr += v * (1.0f + pan);
            }
            const float g = 1.2f / std::sqrt ((float) nv);
            wl *= g; wr *= g;
        }
        else if (mode == 1)
        {
            const float base = (0.3f + centre * 7.0f) * 0.001f * (float) sr;
            const float dep = depth * base * 0.9f;
            for (int c = 0; c < 2; ++c)
            {
                const float ph = (float) lfo + (c ? width * 0.25f : 0.0f);
                const float v = d[c].read (1.0f + base + dep * fastSin (ph));
                d[c].push ((c ? xr : xl) + fb * v);
                (c ? wr : wl) = v;
            }
        }
        else
        {
            const int st = juce::jlimit (2, 24, voices & ~1);
            for (int c = 0; c < 2; ++c)
            {
                const float ph = (float) lfo + (c ? width * 0.25f : 0.0f);
                const float f = clampf (100.0f * std::exp2 (centre * 6.0f + depth * fastSin (ph) * 2.5f), 30.0f, (float) sr * 0.45f);
                const float t = std::tan (kPi * f / (float) sr);
                const float a = (t - 1.0f) / (t + 1.0f);
                float in = (c ? xr : xl) + fbs[c] * fb;
                for (int k = 0; k < st; ++k)
                {
                    const float y = a * in + ap[c][k];
                    ap[c][k] = in - a * y;
                    in = y;
                }
                fbs[c] = in;
                (c ? wr : wl) = in;
            }
        }
        if (analog)
        {
            lp[0] += lpA * (fastTanh (wl * 0.9f) / 0.9f - lp[0]);
            lp[1] += lpA * (fastTanh (wr * 0.9f) / 0.9f - lp[1]);
            wl = lp[0]; wr = lp[1];
        }
        L[i] = xl + mix * (wl - xl);
        R[i] = xr + mix * (wr - xr);
    }
}

// ======================================================================= Delay
void DelayUnit::process (const P& p, double sr, double bpm, float* L, float* R, int n)
{
    const int mode = p.ch (dly::mode);
    float tL, tR;
    if (p.on (dly::sync))
    {
        tL = (float) (divisionBeats (p.ch (dly::divL)) * 60.0 / bpm);
        tR = (float) (divisionBeats (p.ch (dly::divR)) * 60.0 / bpm);
    }
    else { tL = p (dly::timeMs) * 0.001f; tR = tL * 1.5f; }
    if (mode != 1) tR = tL;
    const float maxT = (float) (d[0].b.size() - 8) / (float) sr;
    tL = clampf (tL, 0.001f, maxT); tR = clampf (tR, 0.001f, maxT);
    const float targ[2] { tL * (float) sr, tR * (float) sr };
    if (t[0] < 0) { t[0] = targ[0]; t[1] = targ[1]; }
    const float fb = clampf (p (dly::feedback), 0, 1.05f);
    const float hpA = OnePole::coef (p (dly::lowCut), sr), lpA = OnePole::coef (p (dly::highCut), sr);
    const float modD = p (dly::modDepth) * 0.004f * (float) sr;
    const float diff = p (dly::diffusion) * 0.65f;
    const bool analog = p.ch (dly::style) == 1;
    const float mix = clampf (p (dly::mix), 0, 1);
    const double inc = 0.55 / sr;
    for (int i = 0; i < n; ++i)
    {
        lfo += inc;
        if (lfo >= 1.0) lfo -= 1.0;
        float fbsig[2];
        for (int c = 0; c < 2; ++c)
        {
            t[c] += 0.0004f * (targ[c] - t[c]);
            const float m = modD * (0.5f + 0.5f * fastSin ((float) lfo + c * 0.25f)) + modD * 0.25f * fastSin ((float) lfo * 7.3f);
            float r = d[c].read (juce::jmax (1.0f, t[c] + m));
            hp[c] += hpA * (r - hp[c]);
            r -= hp[c];
            lp[c] += lpA * (r - lp[c]);
            r = lp[c];
            if (diff > 0) r = ap[c][1].process (ap[c][0].process (r, diff), diff);
            if (analog || fb > 0.95f) r = fastTanh (r * 1.1f) / 1.1f;
            fbsig[c] = r;
        }
        const float xl = L[i], xr = R[i];
        if (mode == 0)
        {
            const float in = 0.5f * (xl + xr);
            d[0].push (in + fb * fbsig[0]);
            d[1].push (in + fb * fbsig[0]);
            fbsig[1] = fbsig[0];
        }
        else if (mode == 1)
        {
            d[0].push (xl + fb * fbsig[0]);
            d[1].push (xr + fb * fbsig[1]);
        }
        else
        {
            d[0].push (0.5f * (xl + xr) + fb * fbsig[1]);
            d[1].push (fb * fbsig[0]);
        }
        L[i] = xl + mix * (fbsig[0] - xl);
        R[i] = xr + mix * (fbsig[1] - xr);
    }
}

// ======================================================================= Reverb (8-line FDN)
static const float kLineMs[8] { 29.7f, 37.1f, 41.1f, 43.7f, 53.3f, 59.9f, 67.7f, 73.1f };

void ReverbUnit::prepare (double sr)
{
    for (auto& x : pre) x.setSize ((int) (sr * 0.3));
    for (int i = 0; i < N; ++i) line[i].setSize ((int) (sr * (kLineMs[i] * 0.001 * 2.2 + 0.01)));
    static const float apMs[6] { 4.7f, 3.6f, 12.7f, 9.3f, 6.1f, 15.3f };
    for (int c = 0; c < 2; ++c)
        for (int k = 0; k < 6; ++k) diff[c][k].setSize ((int) (sr * apMs[k] * 0.001 * (c ? 1.07 : 1.0)));
    for (int i = 0; i < N; ++i) lfo[i] = (double) i / N;
}

void ReverbUnit::reset()
{
    for (auto& x : pre) x.clear();
    for (auto& x : line) x.clear();
    for (auto& d : diff) for (auto& a : d) a.clear();
    for (auto& v : lp) v = 0;
    hpS[0] = hpS[1] = 0;
}

void ReverbUnit::process (const P& p, double sr, float* L, float* R, int n)
{
    const bool lush = p.ch (rvb::model) == 1;
    const float size = clampf (p (rvb::size), 0, 1);
    const float decay = juce::jmax (0.1f, p (rvb::decay));
    const float preS = juce::jmax (1.0f, p (rvb::predelay) * 0.001f * (float) sr);
    const float damp = clampf (p (rvb::damping), 0, 1);
    const float dif = clampf (p (rvb::diffusion), 0, 1) * 0.72f;
    const float modD = clampf (p (rvb::modDepth), 0, 1) * (lush ? 0.0025f : 0.0006f) * (float) sr;
    const float width = clampf (p (rvb::width), 0, 1);
    const float hpA = OnePole::coef (p (rvb::lowCut), sr);
    const float mix = clampf (p (rvb::mix), 0, 1);
    const float sizeMul = (0.35f + size * 1.25f) * (lush ? 1.3f : 1.0f);
    float len[N], g[N];
    for (int i = 0; i < N; ++i)
    {
        len[i] = juce::jmin ((float) line[i].mask - modD - 4.0f, kLineMs[i] * 0.001f * (float) sr * sizeMul);
        g[i] = std::pow (10.0f, -3.0f * len[i] / (decay * (float) sr));
    }
    const float dampA = 1.0f - damp * 0.85f;
    const int nDiff = lush ? 6 : 4;
    const double lfoInc = (lush ? 0.31 : 0.17) / sr;
    for (int s = 0; s < n; ++s)
    {
        const float xl = L[s], xr = R[s];
        pre[0].push (xl); pre[1].push (xr);
        float il = pre[0].read (preS), ir = pre[1].read (preS);
        for (int k = 0; k < nDiff; ++k) { il = diff[0][k].process (il, dif); ir = diff[1][k].process (ir, dif); }
        float v[N];
        for (int i = 0; i < N; ++i)
        {
            lfo[i] += lfoInc * (1.0 + i * 0.13);
            if (lfo[i] >= 1.0) lfo[i] -= 1.0;
            const float d = len[i] + (modD > 0 ? modD * (0.5f + 0.5f * fastSin ((float) lfo[i])) : 0.0f);
            const float r = line[i].read (juce::jmax (2.0f, d));
            lp[i] += dampA * (r - lp[i]);
            v[i] = lp[i] * g[i];
        }
        // fast Walsh-Hadamard (8)
        for (int h = 1; h < N; h <<= 1)
            for (int i = 0; i < N; i += h << 1)
                for (int j = i; j < i + h; ++j)
                {
                    const float a = v[j], b = v[j + h];
                    v[j] = a + b; v[j + h] = a - b;
                }
        const float norm = 0.35355339f;
        for (int i = 0; i < N; ++i) line[i].push (v[i] * norm + ((i & 1) ? ir : il) * 0.5f);
        float wl = 0, wr = 0;
        for (int i = 0; i < N; ++i) { if (i & 1) wr += lp[i]; else wl += lp[i]; }
        wl *= 0.5f; wr *= 0.5f;
        const float mid = 0.5f * (wl + wr), side = 0.5f * (wl - wr) * width;
        wl = mid + side; wr = mid - side;
        hpS[0] += hpA * (wl - hpS[0]); hpS[1] += hpA * (wr - hpS[1]);
        wl -= hpS[0]; wr -= hpS[1];
        L[s] = xl + mix * (wl - xl);
        R[s] = xr + mix * (wr - xr);
    }
}

// ======================================================================= TextureVerb (granular)
void TextureUnit::process (const P& p, double sr, float* L, float* R, int n)
{
    const float grainMs = clampf (p (txp::grain), 10, 800);
    const float dens = clampf (p (txp::density), 0, 1);
    const float pitch = p (txp::pitch), spread = clampf (p (txp::spread), 0, 1);
    const float rev = clampf (p (txp::reverse), 0, 1), fb = clampf (p (txp::feedback), 0, 0.98f);
    const float jit = clampf (p (txp::jitter), 0, 1), damp = clampf (p (txp::damping), 0, 1);
    const bool freeze = p.on (txp::freeze);
    const float mix = clampf (p (txp::mix), 0, 1);
    const float rate = 1.0f + dens * dens * 80.0f;
    const int lenS = juce::jmax (16, (int) (grainMs * 0.001f * (float) sr));
    const float overlap = juce::jmax (1.0f, rate * grainMs * 0.001f);
    const float norm = 1.0f / std::sqrt (overlap);
    const float lpA = OnePole::coef (18000.0f * std::exp2 (-damp * 5.0f), sr);
    const int bufLen = buf[0].mask + 1;
    for (int s = 0; s < n; ++s)
    {
        spawn += rate / sr;
        if (spawn >= 1.0)
        {
            spawn -= 1.0;
            for (auto& gr : g)
                if (! gr.on)
                {
                    const float ratio = std::exp2 ((pitch + spread * rng.bi() * 7.0f) / 12.0f);
                    const bool back = rng.uni() < rev;
                    const double jitS = jit * rng.uni() * 1.5 * sr;
                    const double reach = (double) lenS * juce::jmax (1.0f, ratio);
                    const double d0 = back ? jitS + 64.0 + lenS : reach + jitS + 64.0;
                    if (d0 + (double) lenS * (1.0 + ratio) >= bufLen - 8) break;
                    gr.on = true; gr.age = 0; gr.len = lenS;
                    gr.inc = back ? -ratio : ratio;
                    gr.pos = d0;
                    const float pan = rng.bi() * 0.7f;
                    gr.gl = 1.0f - pan; gr.gr = 1.0f + pan;
                    break;
                }
        }
        float wl = 0, wr = 0;
        for (auto& gr : g)
        {
            if (! gr.on) continue;
            const float t = (float) gr.age / (float) gr.len;
            const float win = 0.5f - 0.5f * fastSin (t + 0.25f);
            const float dd = (float) juce::jlimit (1.0, (double) bufLen - 4.0, gr.pos);
            wl += buf[0].read (dd) * win * gr.gl;
            wr += buf[1].read (dd) * win * gr.gr;
            gr.pos += (freeze ? 0.0 : 1.0) - gr.inc;      // pos = distance behind the write head
            if (++gr.age >= gr.len) gr.on = false;
        }
        wl *= norm; wr *= norm;
        lp[0] += lpA * (wl - lp[0]); lp[1] += lpA * (wr - lp[1]);
        wl = ap[0].process (lp[0], 0.5f); wr = ap[1].process (lp[1], 0.5f);
        const float xl = L[s], xr = R[s];
        if (! freeze)
        {
            buf[0].push (xl + fb * fastTanh (wl));
            buf[1].push (xr + fb * fastTanh (wr));
        }
        L[s] = xl + mix * (wl - xl);
        R[s] = xr + mix * (wr - xr);
    }
}

// ======================================================================= Compressor
void CompUnit::process (const P& p, double sr, float* L, float* R, int n)
{
    const float thr = p (cmp::threshold), ratio = juce::jmax (1.0f, p (cmp::ratio)), knee = juce::jmax (0.0f, p (cmp::knee));
    const float at = std::exp (-1.0f / (juce::jmax (0.1f, p (cmp::attack)) * 0.001f * (float) sr));
    const float rl = std::exp (-1.0f / (juce::jmax (1.0f, p (cmp::release)) * 0.001f * (float) sr));
    const float mk = dbToGain (p (cmp::makeup), -100.0f), mix = clampf (p (cmp::mix), 0, 1);
    const float slope = 1.0f / ratio - 1.0f;
    for (int i = 0; i < n; ++i)
    {
        const float pk = juce::jmax (std::abs (L[i]), std::abs (R[i]), 1e-7f);
        const float lvl = 20.0f * std::log10 (pk);
        const float over = lvl - thr;
        float target;
        if (2.0f * over < -knee) target = 0.0f;
        else if (knee > 0 && 2.0f * std::abs (over) <= knee) target = slope * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);
        else target = slope * over;
        gr = target < gr ? at * gr + (1.0f - at) * target : rl * gr + (1.0f - rl) * target;
        const float g = dbToGain (gr, -200.0f) * mk;
        L[i] = L[i] + mix * (L[i] * g - L[i]);
        R[i] = R[i] + mix * (R[i] * g - R[i]);
    }
}

// ======================================================================= Master EQ
void FxEqUnit::process (const P& p, double sr, float* L, float* R, int n)
{
    float c[10];
    bool changed = false;
    for (int k = 0; k < 10; ++k) { c[k] = p (k); if (c[k] != cache[k]) { cache[k] = c[k]; changed = true; } }
    if (changed)
    {
        b[0].lowShelf (sr, c[feq::lowFreq], c[feq::lowGain]);
        b[1].peak (sr, c[feq::m1Freq], c[feq::m1Gain], c[feq::m1Q]);
        b[2].peak (sr, c[feq::m2Freq], c[feq::m2Gain], c[feq::m2Q]);
        b[3].highShelf (sr, c[feq::highFreq], c[feq::highGain]);
    }
    for (int i = 0; i < n; ++i)
    {
        float l = L[i], r = R[i];
        for (auto& q : b) { l = q.process (l, 0); r = q.process (r, 1); }
        L[i] = l; R[i] = r;
    }
}

// ======================================================================= Distortion
void DistUnit::process (const P& p, double sr, float* L, float* R, int n)
{
    const int mode = p.ch (dsp_::mode);
    const float drive = clampf (p (dsp_::drive), 0, 1), bias = p (dsp_::bias), mix = clampf (p (dsp_::mix), 0, 1);
    const float outG = dbToGain (p (dsp_::out), -100.0f);
    const float dg = 1.0f + drive * drive * 40.0f;
    const float lpA = OnePole::coef (800.0f * std::exp2 (clampf (p (dsp_::tone), 0, 1) * 4.6f), sr);
    const float levels = std::exp2 (1.0f + (1.0f - drive) * 11.0f);
    const float holdInc = 1.0f / (1.0f + drive * drive * 40.0f);
    const float tb = fastTanh (bias * 0.4f);
    for (int i = 0; i < n; ++i)
    {
        bool tick = false;
        if (mode == 6) { cnt += holdInc; if (cnt >= 1.0f) { cnt -= 1.0f; tick = true; } }
        for (int c = 0; c < 2; ++c)
        {
            float* io = c ? R : L;
            const float x = io[i];
            float y;
            switch (mode)
            {
                case 0: y = fastTanh (x * dg + bias * 0.4f) - tb; break;
                case 1: y = clampf (x * dg + bias * 0.3f, -1.0f, 1.0f); break;
                case 2: { const float z = x * dg * 2.0f + bias * 0.5f; y = (z > 0 ? 1.0f - std::exp (-z) : -1.0f + std::exp (z * 1.3f)) ; break; }
                case 3: y = fastSin ((x * dg * 0.3f + bias * 0.2f) * 0.25f); break;
                case 4: y = fastTanh (std::abs (x * dg * 0.5f) + bias * 0.2f); break;
                case 5: y = std::round (x * levels) / levels; break;
                default: if (tick) hold[c] = x; y = hold[c]; break;
            }
            y = dc[c].process (y);
            lp[c] += lpA * (y - lp[c]);
            io[i] = (x + mix * (lp[c] - x)) * outG;
        }
    }
}

// ======================================================================= FX filter
void FilterUnit::process (const P& p, double sr, float* L, float* R, int n)
{
    const int type = p.ch (ffp::type);
    const float target = std::tan (kPi * clampf (p (ffp::cutoff), 20.0f, (float) sr * 0.45f) / (float) sr);
    if (g < 0) g = target;
    const float k = 2.0f - 1.96f * clampf (p (ffp::reso), 0, 1);
    const float drive = p (ffp::drive), dg = 1.0f + drive * drive * 10.0f, mix = clampf (p (ffp::mix), 0, 1);
    const float step = (target - g) / (float) n;
    for (int i = 0; i < n; ++i)
    {
        g += step;
        for (int c = 0; c < 2; ++c)
        {
            float* io = c ? R : L;
            const float x = io[i];
            s[c].tick (drive > 0 ? fastTanh (x * dg) : x, g, k);
            float y;
            switch (type)
            {
                case 1: y = s[c].hp; break;
                case 2: y = s[c].bp * k; break;
                case 3: y = s[c].lp + s[c].hp; break;
                default: y = s[c].lp; break;
            }
            io[i] = x + mix * (y - x);
        }
    }
    g = target;
}

// ======================================================================= rack
void FxRack::prepare (double s)
{
    sr = s;
    for (auto& u : modfx) u.prepare (sr);
    for (auto& u : delay) u.prepare (sr);
    for (auto& u : reverb) u.prepare (sr);
    for (auto& u : texture) u.prepare (sr);
    reset();
}

void FxRack::reset()
{
    for (auto& u : modfx) u.reset();
    for (auto& u : delay) u.reset();
    for (auto& u : reverb) u.reset();
    for (auto& u : texture) u.reset();
    for (auto& u : comp) u.reset();
    for (auto& u : eq) u.reset();
    for (auto& u : dist) u.reset();
    for (auto& u : filt) u.reset();
}

void FxRack::runUnit (int e, const Shared& sh, const float* gmod, float* L, float* R, int n)
{
    const auto& ge = fxEntries()[(size_t) e];
    P p { sh, moduleBase (ge.kind, ge.inst), gmod };
    const int k = ge.inst;
    switch (ge.kind)
    {
        case Kind::ModFx:    modfx[k].process (p, sr, L, R, n); break;
        case Kind::Delay:    delay[k].process (p, sr, sh.bpm, L, R, n); break;
        case Kind::Reverb:   reverb[k].process (p, sr, L, R, n); break;
        case Kind::Texture:  texture[k].process (p, sr, L, R, n); break;
        case Kind::Comp:     comp[k].process (p, sr, L, R, n); break;
        case Kind::FxEq:     eq[k].process (p, sr, L, R, n); break;
        case Kind::Dist:     dist[k].process (p, sr, L, R, n); break;
        case Kind::FxFilter: filt[k].process (p, sr, L, R, n); break;
        default: break;
    }
}

void FxRack::process (const Shared& sh, const float* gmod, float* const* bus, int n)
{
    static const int retIdx[3] { paramIndex (id::auxRet (0)), paramIndex (id::auxRet (1)), paramIndex (id::auxRet (2)) };
    for (int a = 0; a < 3; ++a)
    {
        float* L = bus[2 + a * 2];
        float* R = bus[3 + a * 2];
        bool any = false;
        for (int s = 0; s < kFxSlots; ++s)
        {
            const int e = sh.fxGrid[a + 1][s];
            if (e > 0) { runUnit (e, sh, gmod, L, R, n); any = true; }
        }
        juce::ignoreUnused (any);
        const float ret = dbToGain (sh.get (retIdx[a], gmod));
        if (retPrev[a] < 0) retPrev[a] = ret;
        const float step = (ret - retPrev[a]) / (float) n;
        float g = retPrev[a];
        for (int i = 0; i < n; ++i) { g += step; bus[0][i] += L[i] * g; bus[1][i] += R[i] * g; }
        retPrev[a] = ret;
    }
    for (int s = 0; s < kFxSlots; ++s)
    {
        const int e = sh.fxGrid[0][s];
        if (e > 0) runUnit (e, sh, gmod, bus[0], bus[1], n);
    }
}
}
