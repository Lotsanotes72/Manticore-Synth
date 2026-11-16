#include "VoiceModules.h"

namespace mc::dsp
{
// ======================================================================= wave FX
float oscPhaseFx (int type, float a, float p, float& win)
{
    switch (type)
    {
        case 1: return frac (p * (1.0f + a * 15.0f));                                       // Sync
        case 2: { const float d = 0.5f - 0.49f * a;                                           // Phase distortion
                  return p < d ? p * 0.5f / d : 0.5f + (p - d) * 0.5f / (1.0f - d); }
        case 3: return p - a * 0.9f * p * (1.0f - p);                                         // Bend +
        case 4: return p + a * 0.9f * p * (1.0f - p);                                         // Bend -
        case 5: { const float m = p < 0.5f ? 2.0f * p : 2.0f - 2.0f * p; return juce::jmin (0.99999f, p + a * (m - p)); }   // Mirror
        case 6: { const float w = 1.0f - a * 0.9f; return p < w ? p / w : 0.0f; }            // Pulse
        case 7: win *= (1.0f - p); return frac (p * (1.0f + a * 15.0f));                    // Formant
        case 8: { const float steps = std::exp2 (1.0f + (1.0f - a) * 9.0f); return std::floor (p * steps) / steps; }   // Quantize
        default: return p;
    }
}

float oscOutFx (int type, float a, float y, float p)
{
    switch (type)
    {
        case 9:  return fastSin (y * (1.0f + a * 5.0f) * 0.25f);                             // Fold
        case 10: { const float k = 1.0f + a * 9.0f; return fastTanh (y * k) / fastTanh (k); } // Squash
        case 11: { const float lv = std::exp2 (1.0f + (1.0f - a) * 10.0f); return std::round (y * lv) / lv; }   // Crush
        case 12: return y * fastSin (p * (1.0f + std::floor (a * 15.99f)));                 // Ring
        default: return y;
    }
}

float oscFxMipMul (int type, float a)
{
    switch (type)
    {
        case 1: case 7: return 1.0f + a * 15.0f;
        case 2: return 1.0f / (1.0f - 0.98f * a);
        case 3: case 4: return 1.0f + a * 2.0f;
        case 5: return 1.0f + a;
        case 6: return 1.0f / (1.0f - a * 0.9f);
        default: return 1.0f;
    }
}

// ======================================================================= OSC
static void additiveAmps (const Wavetable& T, const P& p, float fpos, float noteTime, int np, float* amp)
{
    const int f0 = juce::jlimit (0, Wavetable::F - 1, (int) fpos);
    const int f1 = juce::jmin (Wavetable::F - 1, f0 + 1);
    const float ff = fpos - (float) f0;
    const float b = p (op::bright), e = p (op::oddEven), pd = p (op::pDecay);
    const float decayK = noteTime * pd * pd * 14.0f / 12.0f;
    for (int h = 1; h <= np; ++h)
    {
        float a = T.spec[f0][h - 1] + ff * (T.spec[f1][h - 1] - T.spec[f0][h - 1]);
        a *= std::pow ((float) h, b);
        if (h > 1)
        {
            if ((h & 1) == 0 && e < 0) a *= 1.0f + e;
            if ((h & 1) == 1 && e > 0) a *= 1.0f - e;
        }
        if (decayK > 0) a *= std::exp (-decayK * (float) (h - 1));
        amp[h - 1] = a;
    }
}

void processOsc (OscState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    const double sr = v.sh.sr;
    const auto& T = WavetableBank::get().table (p.ch (op::table));
    const float fpos = clampf (p (op::wave), 0.0f, 1.0f) * (float) (Wavetable::F - 1);
    const float hz = keyedHz (v.note, p (op::keytrack), p (op::tune) + p (op::fine) * 0.01f);
    const float vol = juce::jmax (0.0f, p (op::vol));
    const float pan = p (op::pan);
    const int fx1 = p.ch (op::fx1Type), fx2 = p.ch (op::fx2Type);
    const float a1 = clampf (p (op::fx1Amt), 0, 1), a2 = clampf (p (op::fx2Amt), 0, 1);
    const bool additive = p.ch (op::engine) == 1;
    const int uni = additive ? 1 : juce::jlimit (1, 16, juce::roundToInt (p (op::uniVoices)));

    if (v.trigger || ! st.started)
    {
        if (! p.on (op::freeRun) || ! st.started)
        {
            const float ph0 = p (op::phase);
            Rng r (v.seed);
            for (int k = 0; k < 16; ++k) st.ph[k] = frac (ph0 + (k == 0 ? 0.0f : (uni > 1 ? r.uni() : 0.0f)));
            for (int h = 0; h < 64; ++h) { st.c[h] = 1.0f; st.s[h] = 0.0f; }
        }
        if (! st.started) st.volPrev = vol;
        st.started = true;
    }

    const float gStep = (vol - st.volPrev) / (float) n;
    float g = st.volPrev;
    st.volPrev = vol;

    if (additive)
    {
        const int np = juce::jlimit (1, 64, juce::roundToInt (p (op::partials)));
        float amp[64], cw[64], sw[64], gl[64], gr[64];
        additiveAmps (T, p, fpos, v.noteTime, np, amp);
        const float stretch = p (op::stretch);
        const float expo = 1.0f + stretch * 0.35f;
        const float width = p (op::uniWidth);
        float sum2 = 0;
        int top = 0;
        for (int h = 1; h <= np; ++h)
        {
            const float fh = hz * (stretch == 0.0f ? (float) h : std::pow ((float) h, expo));
            if (fh >= (float) sr * 0.46f) { amp[h - 1] = 0; continue; }
            const float w = kTwoPi * fh / (float) sr;
            cw[h - 1] = std::cos (w); sw[h - 1] = std::sin (w);
            const float side = h == 1 ? 0.0f : ((h & 1) ? -0.7f : 0.7f) * width;
            gl[h - 1] = 1.0f - side; gr[h - 1] = 1.0f + side;
            sum2 += amp[h - 1] * amp[h - 1];
            top = h;
        }
        const float norm = sum2 > 1e-9f ? 0.6f / std::sqrt (sum2) : 0.0f;
        float pl, pr;
        panGains (pan, pl, pr);
        const bool outFx = fx1 >= 9 || fx2 >= 9;
        for (int i = 0; i < n; ++i)
        {
            g += gStep;
            float sl = 0, sr_ = 0;
            for (int h = 0; h < top; ++h)
            {
                const float c = st.c[h], s = st.s[h];
                const float c2 = c * cw[h] - s * sw[h];
                const float s2 = c * sw[h] + s * cw[h];
                st.c[h] = c2; st.s[h] = s2;
                const float y = amp[h] * s2;
                sl += y * gl[h]; sr_ += y * gr[h];
            }
            sl *= norm; sr_ *= norm;
            if (outFx)
            {
                const float ph = frac ((float) st.ph[0]);
                if (fx1 >= 9) { sl = oscOutFx (fx1, a1, sl, ph); sr_ = oscOutFx (fx1, a1, sr_, ph); }
                if (fx2 >= 9) { sl = oscOutFx (fx2, a2, sl, ph); sr_ = oscOutFx (fx2, a2, sr_, ph); }
                st.ph[0] += hz / sr;
                if (st.ph[0] >= 1.0) st.ph[0] -= 1.0;
            }
            L[i] += sl * g * pl;
            R[i] += sr_ * g * pr;
        }
        for (int h = 0; h < top; ++h)
        {
            const float m = st.c[h] * st.c[h] + st.s[h] * st.s[h];
            const float k = 1.5f - 0.5f * m;
            st.c[h] *= k; st.s[h] *= k;
        }
        return;
    }

    const float det = p (op::uniDetune), width = p (op::uniWidth);
    const float spreadCents = 70.0f * det * std::sqrt (juce::jmax (0.0f, det));
    double inc[16];
    float gl[16], gr[16];
    for (int k = 0; k < uni; ++k)
    {
        const float off = uni > 1 ? (2.0f * (float) k / (float) (uni - 1) - 1.0f) : 0.0f;
        inc[k] = hz * std::exp2 (off * spreadCents / 1200.0f) / sr;
        const float pk = clampf (pan + ((k & 1) ? -1.0f : 1.0f) * std::abs (off) * width, -1.0f, 1.0f);
        panGains (pk, gl[k], gr[k]);
    }
    const float norm = 1.0f / std::sqrt ((float) uni);
    const float fxMul = oscFxMipMul (fx1, a1) * oscFxMipMul (fx2, a2);
    const float maxHz = hz * std::exp2 (spreadCents / 1200.0f) * fxMul;
    const int mip = WavetableBank::mipFor (0.5f * (float) sr / juce::jmax (1.0f, maxHz));
    const bool phaseFx = (fx1 >= 1 && fx1 <= 8) || (fx2 >= 1 && fx2 <= 8);
    const bool outFx = fx1 >= 9 || fx2 >= 9;

    for (int i = 0; i < n; ++i)
    {
        g += gStep;
        float sl = 0, sr_ = 0;
        for (int k = 0; k < uni; ++k)
        {
            const float pp = (float) st.ph[k];
            float q = pp, win = 1.0f;
            if (phaseFx)
            {
                if (fx1 >= 1 && fx1 <= 8) q = oscPhaseFx (fx1, a1, q, win);
                if (fx2 >= 1 && fx2 <= 8) q = oscPhaseFx (fx2, a2, q, win);
            }
            float y = readTable (T, fpos, mip, q) * win;
            if (outFx)
            {
                if (fx1 >= 9) y = oscOutFx (fx1, a1, y, pp);
                if (fx2 >= 9) y = oscOutFx (fx2, a2, y, pp);
            }
            sl += y * gl[k];
            sr_ += y * gr[k];
            st.ph[k] += inc[k];
            if (st.ph[k] >= 1.0) st.ph[k] -= 1.0;
        }
        L[i] += sl * g * norm;
        R[i] += sr_ * g * norm;
    }
}

void renderOscCycle (const Shared& sh, int inst, float* out, int n)
{
    P p { sh, moduleBase (Kind::Osc, inst), nullptr };
    const auto& T = WavetableBank::get().table (p.ch (op::table));
    const float fpos = clampf (p (op::wave), 0, 1) * (float) (Wavetable::F - 1);
    const int fx1 = p.ch (op::fx1Type), fx2 = p.ch (op::fx2Type);
    const float a1 = p (op::fx1Amt), a2 = p (op::fx2Amt);
    if (p.ch (op::engine) == 1)
    {
        float amp[64];
        const int np = juce::jlimit (1, 64, juce::roundToInt (p (op::partials)));
        additiveAmps (T, p, fpos, 0.0f, np, amp);
        float peak = 1e-6f;
        for (int i = 0; i < n; ++i)
        {
            const float ph = (float) i / (float) n;
            float y = 0;
            for (int h = 1; h <= np; ++h) y += amp[h - 1] * fastSin (ph * (float) h);
            if (fx1 >= 9) y = oscOutFx (fx1, a1, y, ph);
            out[i] = y;
            peak = juce::jmax (peak, std::abs (y));
        }
        for (int i = 0; i < n; ++i) out[i] /= peak;
        return;
    }
    for (int i = 0; i < n; ++i)
    {
        const float pp = (float) i / (float) n;
        float q = pp, win = 1.0f;
        if (fx1 >= 1 && fx1 <= 8) q = oscPhaseFx (fx1, a1, q, win);
        if (fx2 >= 1 && fx2 <= 8) q = oscPhaseFx (fx2, a2, q, win);
        float y = readTable (T, fpos, 0, q) * win;
        if (fx1 >= 9) y = oscOutFx (fx1, a1, y, pp);
        if (fx2 >= 9) y = oscOutFx (fx2, a2, y, pp);
        out[i] = y;
    }
}

// ======================================================================= FM
void processFm (FmState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    const double sr = v.sh.sr;
    const float hz = keyedHz (v.note, p (fmp::keytrack), p (fmp::tune) + p (fmp::fine) * 0.01f);
    const float ratio = p (fmp::ratio), depth = clampf (p (fmp::depth), 0, 1), fb = clampf (p (fmp::feedback), 0, 1);
    const float inFm = p (fmp::inputFm);
    const int shape = p.ch (fmp::modShape);
    const float vol = juce::jmax (0.0f, p (fmp::vol));
    float pl, pr;
    panGains (p (fmp::pan), pl, pr);
    if (v.trigger) { st.pc = 0; st.pm = 0; st.m1 = st.m2 = 0; }
    const double ic = hz / sr, im = hz * ratio / sr;
    const float idx = depth * depth * 3.0f, fbk = fb * fb * 0.32f;
    const float gStep = (vol - st.volPrev) / (float) n;
    float g = st.volPrev;
    st.volPrev = vol;
    for (int i = 0; i < n; ++i)
    {
        g += gStep;
        const float m = fastSin ((float) st.pm + fbk * (st.m1 + st.m2));
        st.m2 = st.m1; st.m1 = m;
        const float in = 0.5f * (L[i] + R[i]);
        const float arg = (float) st.pc + idx * m + inFm * in * 0.5f;
        float c = fastSin (arg);
        switch (shape)
        {
            case 1: c = st.dc[0].process (juce::jmax (0.0f, c) * 2.0f); break;
            case 2: c = st.dc[0].process (std::abs (c) * 1.6f); break;
            case 3: c = st.dc[0].process (frac (2.0f * arg) < 0.5f ? c * 1.4f : 0.0f); break;
            default: break;
        }
        st.pc += ic; if (st.pc >= 1.0) st.pc -= 1.0;
        st.pm += im; if (st.pm >= 1.0) st.pm -= 1.0;
        L[i] += c * g * pl;
        R[i] += c * g * pr;
    }
}

// ======================================================================= NOISE
void processNoise (NoiseState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    const int type = p.ch (np::type);
    const float color = p (np::color), dens = clampf (p (np::density), 0, 1), width = p (np::width);
    const float vol = juce::jmax (0.0f, p (np::vol));
    float pl, pr;
    panGains (p (np::pan), pl, pr);
    const float tiltA = color < 0 ? OnePole::coef (18000.0f * std::exp2 (color * 7.0f), v.sh.sr)
                                  : OnePole::coef (30.0f * std::exp2 (color * 8.0f), v.sh.sr);
    const float crackP = 0.0001f + dens * dens * 0.01f;
    const float holdInc = 50.0f * std::exp2 (dens * 8.5f) / (float) v.sh.sr;
    for (int i = 0; i < n; ++i)
    {
        float x[2];
        if (type == 5)
        {
            st.holdCnt -= holdInc;
            if (st.holdCnt <= 0) { st.holdCnt += 1.0f; st.hold[0] = st.rng.bi(); st.hold[1] = st.rng.bi(); }
        }
        for (int c = 0; c < 2; ++c)
        {
            const float w = st.rng.bi();
            float y;
            switch (type)
            {
                case 1:
                {
                    auto* b = st.pink[c];
                    b[0] = 0.99886f * b[0] + w * 0.0555179f; b[1] = 0.99332f * b[1] + w * 0.0750759f;
                    b[2] = 0.96900f * b[2] + w * 0.1538520f; b[3] = 0.86650f * b[3] + w * 0.3104856f;
                    b[4] = 0.55000f * b[4] + w * 0.5329522f; b[5] = -0.7616f * b[5] - w * 0.0168980f;
                    y = (b[0] + b[1] + b[2] + b[3] + b[4] + b[5] + b[6] + w * 0.5362f) * 0.2f;
                    b[6] = w * 0.115926f;
                    break;
                }
                case 2: st.brown[c] = (st.brown[c] + 0.02f * w) / 1.02f; y = st.brown[c] * 3.5f; break;
                case 3: y = (w - st.blue[c]) * 0.6f; st.blue[c] = w; break;
                case 4:
                    if (st.rng.uni() < crackP) st.crackle[c] = st.rng.bi() * (0.6f + st.rng.uni());
                    y = st.crackle[c]; st.crackle[c] *= 0.55f;
                    break;
                case 5: y = st.hold[c] * 0.7f; break;
                default: y = w * 0.7f; break;
            }
            st.tilt[c] += tiltA * (y - st.tilt[c]);
            x[c] = color < 0 ? st.tilt[c] : (color > 0 ? y - st.tilt[c] : y);
        }
        x[1] = width * x[1] + (1.0f - width) * x[0];
        L[i] += x[0] * vol * pl;
        R[i] += x[1] * vol * pr;
    }
}

// ======================================================================= EXCITER
void processExc (ExcState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    const double sr = v.sh.sr;
    const int mode = p.ch (xp::mode);
    const float decayMs = juce::jmax (1.0f, p (xp::decay)), color = p (xp::color), rate = p (xp::rate);
    const float level = juce::jmax (0.0f, p (xp::level)), width = p (xp::width);
    if (v.trigger) { st.env = 1.0f; st.t = 0; st.patT = 0; st.fired = false; st.fade = 0; st.rng = Rng (v.seed + 11); }
    const float dt = 1.0f / (float) sr;
    const float k = std::exp (-dt * 4.6f / (decayMs * 0.001f));
    const float aLp = OnePole::coef (juce::jmin (300.0f * std::exp2 ((1.0f + color) * 4.0f), (float) sr * 0.45f), sr);
    const float hpAmt = color > 0 ? color * 0.9f : 0.0f;
    const float lowA = OnePole::coef (600.0f, sr);
    const float malletT = juce::jlimit (0.0003f, 0.02f, decayMs * 0.001f * 0.15f);
    const float bowA = 1.0f - std::exp (-dt / (decayMs * 0.001f));
    for (int i = 0; i < n; ++i)
    {
        float x = 0, xr = 0;
        switch (mode)
        {
            case 0: x = st.fired ? 0.0f : 6.0f; xr = x; st.fired = true; break;
            case 1: x = st.rng.bi() * st.env; xr = (1.0f - width) * x + width * st.rng.bi() * st.env; st.env *= k; break;
            case 2:
            {
                const float body = st.t < malletT ? std::sin (kPi * st.t / malletT) : 0.0f;
                const float nz = st.rng.bi() * st.env * 0.12f;
                x = body * 1.5f + nz; xr = body * 1.5f + (1.0f - width) * nz + width * st.rng.bi() * st.env * 0.12f;
                st.env *= k;
                break;
            }
            case 3:
                st.fade += bowA * ((v.gate ? 1.0f : 0.0f) - st.fade);
                x = st.rng.bi() * st.fade * 0.5f; xr = (1.0f - width) * x + width * st.rng.bi() * st.fade * 0.5f;
                break;
            default:
                st.patT += dt * rate;
                if (st.patT >= 1.0f) { st.patT -= 1.0f; st.env = 1.0f; }
                x = st.rng.bi() * st.env; xr = (1.0f - width) * x + width * st.rng.bi() * st.env;
                st.env *= k;
                break;
        }
        st.t += dt;
        st.lp[0] += aLp * (x - st.lp[0]);
        st.lp[1] += aLp * (xr - st.lp[1]);
        float yl = st.lp[0], yr = st.lp[1];
        if (hpAmt > 0)
        {
            st.low[0] += lowA * (yl - st.low[0]);
            st.low[1] += lowA * (yr - st.low[1]);
            yl -= hpAmt * st.low[0];
            yr -= hpAmt * st.low[1];
        }
        L[i] += yl * level;
        R[i] += yr * level;
    }
}

// ======================================================================= RESONATOR
static const float kResRatio[8][12] = {
    { 1.0f, 3.93f, 9.2f, 16.0f, 24.6f, 34.8f, 0, 0, 0, 0, 0, 0 },
    { 1.0f, 4.0f, 10.0f, 17.6f, 27.0f, 38.0f, 0, 0, 0, 0, 0, 0 },
    { 0.5f, 1.0f, 1.19f, 1.5f, 2.0f, 2.5f, 2.66f, 3.0f, 4.0f, 5.2f, 6.1f, 8.0f },
    { 1.0f, 2.32f, 4.25f, 6.63f, 9.38f, 12.5f, 16.0f, 19.9f, 0, 0, 0, 0 },
    { 1.0f, 1.47f, 1.9f, 2.4f, 2.85f, 3.3f, 3.8f, 4.3f, 4.9f, 5.5f, 6.2f, 7.0f },
    { 1.0f, 1.59f, 2.14f, 2.3f, 2.65f, 2.92f, 3.16f, 3.5f, 3.6f, 4.15f, 4.6f, 5.1f },
    { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 },
    { 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23 },
};
static const float kResGain[8][12] = {
    { 1.0f, 0.45f, 0.2f, 0.08f, 0.04f, 0.02f, 0, 0, 0, 0, 0, 0 },
    { 1.0f, 0.6f, 0.25f, 0.1f, 0.05f, 0.02f, 0, 0, 0, 0, 0, 0 },
    { 0.6f, 1.0f, 0.5f, 0.4f, 0.6f, 0.3f, 0.25f, 0.3f, 0.2f, 0.15f, 0.1f, 0.08f },
    { 1.0f, 0.7f, 0.5f, 0.35f, 0.25f, 0.18f, 0.12f, 0.08f, 0, 0, 0, 0 },
    { 1.0f, 0.8f, 0.7f, 0.6f, 0.55f, 0.5f, 0.45f, 0.4f, 0.35f, 0.3f, 0.25f, 0.2f },
    { 1.0f, 0.8f, 0.6f, 0.6f, 0.5f, 0.45f, 0.4f, 0.35f, 0.3f, 0.25f, 0.2f, 0.18f },
    { 1.0f, 0.5f, 0.333f, 0.25f, 0.2f, 0.167f, 0.143f, 0.125f, 0.111f, 0.1f, 0.091f, 0.083f },
    { 1.0f, 0.42f, 0.28f, 0.21f, 0.17f, 0.15f, 0.13f, 0.11f, 0.1f, 0.09f, 0.085f, 0.08f },
};
const float* resonatorRatios (int profile) { return kResRatio[juce::jlimit (0, 7, profile)]; }
const float* resonatorGains (int profile) { return kResGain[juce::jlimit (0, 7, profile)]; }

void processRes (ResState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    constexpr int M = ResState::M;
    const double sr = v.sh.sr;
    const int prof = juce::jlimit (0, 7, p.ch (rp::profile));
    const float f = keyedHz (v.note, 1.0f, p (rp::tune) + p (rp::fine) * 0.01f);
    const float decay = juce::jmax (0.02f, p (rp::decay)), damp = clampf (p (rp::damping), 0, 1);
    const float bright = p (rp::bright), spread = clampf (p (rp::spread), 0, 1);
    const float inG = dbToGain (p (rp::input), -47.9f), mix = clampf (p (rp::mix), 0, 1);
    if (v.trigger)
        for (int c = 0; c < 2; ++c) for (int m = 0; m < M; ++m) st.y1[c][m] = st.y2[c][m] = 0;
    float gsum = 0;
    for (int m = 0; m < M; ++m) gsum += kResGain[prof][m];
    const float gNorm = 1.0f / juce::jmax (1.0f, gsum * 0.6f);
    for (int c = 0; c < 2; ++c)
        for (int m = 0; m < M; ++m)
        {
            const float ratio = kResRatio[prof][m];
            const float det = 1.0f + spread * 0.012f * (float) ((m * 7) % 5 - 2) * (c ? -1.0f : 1.0f) + (c ? spread * 0.004f : 0.0f);
            const float fm = f * ratio * det;
            if (ratio <= 0 || fm >= (float) sr * 0.45f || fm < 10.0f) { st.a[c][m] = 0; st.b1[c][m] = 0; st.b2[c][m] = 0; continue; }
            const float T = decay * std::pow (juce::jmax (0.5f, ratio), -damp * 1.3f);
            const float r = std::exp (-6.9078f / (T * (float) sr));
            const float w = kTwoPi * fm / (float) sr;
            st.a[c][m] = 0.2f * std::sin (w) * kResGain[prof][m] * gNorm * std::pow (juce::jmax (0.5f, ratio), bright * 0.7f);
            st.b1[c][m] = 2.0f * r * std::cos (w);
            st.b2[c][m] = -r * r;
        }
    for (int i = 0; i < n; ++i)
    {
        for (int c = 0; c < 2; ++c)
        {
            float* io = c ? R : L;
            const float x = io[i] * inG;
            float y = 0;
            for (int m = 0; m < M; ++m)
            {
                const float ym = st.a[c][m] * x + st.b1[c][m] * st.y1[c][m] + st.b2[c][m] * st.y2[c][m];
                st.y2[c][m] = st.y1[c][m];
                st.y1[c][m] = ym;
                y += ym;
            }
            const float wet = 2.0f * fastTanh (0.5f * y);
            io[i] = io[i] + mix * (wet - io[i]);
        }
    }
}

// ======================================================================= COMB
void processComb (CombState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    if (st.buf.empty()) return;
    constexpr int Lm = CombState::L - 1;
    const double sr = v.sh.sr;
    const int mode = p.ch (cp::mode);
    const float f = keyedHz (v.note, 1.0f, p (cp::tune) + p (cp::fine) * 0.01f);
    const float fb = clampf (p (cp::feedback), -0.999f, 0.999f), damp = clampf (p (cp::damping), 0, 1) * 0.92f;
    const float inG = p (cp::input), mix = clampf (p (cp::mix), 0, 1), vol = p (cp::vol);
    const float D = clampf ((float) sr / f, 2.0f, (float) CombState::L - 4.0f);
    static const float diff[4] { 1.0f, 1.137f, 1.283f, 1.414f };
    float d[4];
    if (mode == 0) { d[0] = D; d[1] = clampf (D * 1.0015f, 2.0f, (float) CombState::L - 4.0f); d[2] = d[3] = D; }
    else for (int k = 0; k < 4; ++k) d[k] = clampf (D * diff[k] * 2.0f, 2.0f, (float) CombState::L - 4.0f);
    if (v.trigger && st.dLen[0] == 0) for (int k = 0; k < 4; ++k) st.dLen[k] = d[k];
    float* buf = st.buf.data();
    auto read = [&] (int line, float delay)
    {
        const float pos = (float) st.w - delay;
        const float fl = std::floor (pos);
        const int i0 = (int) fl;
        const float fr = pos - fl;
        const float* b = buf + line * CombState::L;
        const float a = b[i0 & Lm], c = b[(i0 + 1) & Lm];
        return a + fr * (c - a);
    };
    const float lines = mode == 0 ? 2 : 4;
    for (int i = 0; i < n; ++i)
    {
        for (int k = 0; k < (int) lines; ++k) st.dLen[k] += 0.02f * (d[k] - st.dLen[k]);
        const float xl = L[i], xr = R[i];
        if (mode == 0)
        {
            for (int c = 0; c < 2; ++c)
            {
                const float rd = read (c, st.dLen[c]);
                st.lp[c] += (1.0f - damp) * (rd - st.lp[c]);
                float y = (c ? xr : xl) * inG + fb * st.lp[c];
                y = 2.0f * fastTanh (0.5f * y);
                buf[c * CombState::L + st.w] = y;
                const float dry = c ? xr : xl;
                (c ? R : L)[i] = (dry + mix * (y - dry)) * vol;
            }
        }
        else
        {
            float rd[4], s = 0;
            for (int k = 0; k < 4; ++k) { rd[k] = read (k, st.dLen[k]); st.lp[k] += (1.0f - damp) * (rd[k] - st.lp[k]); s += st.lp[k]; }
            s *= 0.5f;
            for (int k = 0; k < 4; ++k)
            {
                float y = ((k & 1) ? xr : xl) * inG * 0.5f + fb * (st.lp[k] - s);
                buf[k * CombState::L + st.w] = 2.0f * fastTanh (0.5f * y);
            }
            const float wl = (rd[0] + rd[2]) * 0.7f, wr = (rd[1] + rd[3]) * 0.7f;
            L[i] = (xl + mix * (wl - xl)) * vol;
            R[i] = (xr + mix * (wr - xr)) * vol;
        }
        st.w = (st.w + 1) & Lm;
    }
}

// ======================================================================= VCF
namespace
{
enum Model { mLadder, mCascade, mSvf, mDiode, mSallenKey, mOta, mSteiner, mBite, mSmooth, mFormant, mPhaser, mLpg, mTwinPeak };
enum Resp { rLP24, rLP18, rLP12, rLP6, rBP12, rBP6, rHP24, rHP12, rHP6, rNotch, rPeak, rAP };

inline float tptOnePole (float& s, float x, float G) { const float v = (x - s) * G; const float y = v + s; s = y + v; return y; }

inline float soft (float x, float k) { return fastTanh (x * k) / k; }
}

void processVcf (VcfState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    const double sr = v.sh.sr;
    const int model = p.ch (vp::model), resp = p.ch (vp::response);
    const float res = clampf (p (vp::reso), 0, 1), drive = clampf (p (vp::drive), 0, 1);
    const float mix = clampf (p (vp::mix), 0, 1);
    const float fc = p (vp::cutoff) * std::exp2 (p (vp::keytrack) * (v.note - 60.0f) / 12.0f);
    const float spread = p (vp::spread);
    const float nyq = (float) sr * 0.45f;
    float gT[2];
    for (int c = 0; c < 2; ++c)
    {
        const float f = clampf (fc * std::exp2 (c ? spread : -spread), 15.0f, nyq);
        gT[c] = std::tan (kPi * f / (float) sr);
        if (st.gPrev[c] < 0 || v.trigger) st.gPrev[c] = gT[c];
    }
    const float dg = 1.0f + drive * drive * 12.0f;
    // per-block extras
    float k = 2.0f - 2.0f * res * 0.985f;
    if (model == mSmooth) k = 2.0f - res * 1.8f;
    if (model == mBite) k = 2.0f - 2.08f * res;
    const float kB = (model == mSvf || model == mSteiner || model == mBite) ? k : 1.41421f;
    float ladK = res * 4.1f;
    if (model == mCascade) ladK = res * 3.7f;
    if (model == mDiode) ladK = res * 5.2f;
    const float ladComp = 1.0f + res * (model == mDiode ? 2.0f : 1.4f);
    // formant / phaser / twin peak coefficients (per block)
    float formG[2][3] {}, formK = juce::jmax (0.05f, 0.4f - res * 0.34f), formAmp[3] { 1.0f, 0.6f, 0.35f };
    float apA[2][8] {};
    float g2[2] {};
    if (model == mFormant)
    {
        static const float V[5][3] { { 800, 1150, 2900 }, { 400, 1700, 2600 }, { 300, 2200, 3000 }, { 450, 800, 2830 }, { 325, 700, 2530 } };
        for (int c = 0; c < 2; ++c)
        {
            const float f = clampf (fc * std::exp2 (c ? spread : -spread), 20.0f, 20000.0f);
            const float pos = clampf (std::log2 (f / 20.0f) / 9.966f, 0.0f, 1.0f) * 4.0f;
            const int a = juce::jmin (3, (int) pos);
            const float fr = pos - (float) a;
            for (int j = 0; j < 3; ++j)
            {
                const float F = V[a][j] + fr * (V[a + 1][j] - V[a][j]);
                formG[c][j] = std::tan (kPi * juce::jmin (F, nyq) / (float) sr);
            }
        }
    }
    else if (model == mPhaser)
    {
        for (int c = 0; c < 2; ++c)
            for (int j = 0; j < 8; ++j)
            {
                const float f = clampf (fc * std::exp2 ((c ? spread : -spread) + ((float) j - 3.5f) * 0.3f), 20.0f, nyq);
                const float t = std::tan (kPi * f / (float) sr);
                apA[c][j] = (t - 1.0f) / (t + 1.0f);
            }
    }
    else if (model == mTwinPeak)
    {
        for (int c = 0; c < 2; ++c)
            g2[c] = std::tan (kPi * clampf (fc * 2.5f * std::exp2 (c ? spread : -spread), 15.0f, nyq) / (float) sr);
    }
    float lpgGain = 1.0f;
    if (model == mLpg) lpgGain = std::pow (clampf (std::log2 (juce::jmax (30.0f, fc) / 30.0f) / 9.4f, 0.0f, 1.0f), 0.6f);

    for (int c = 0; c < 2; ++c)
    {
        float* io = c ? R : L;
        const float gStep = (gT[c] - st.gPrev[c]) / (float) n;
        float g = st.gPrev[c];
        auto& s1 = st.s1[c];
        auto& s2 = st.s2[c];
        for (int i = 0; i < n; ++i)
        {
            g += gStep;
            const float x = io[i];
            float y = 0;
            const float G = g / (1.0f + g);
            switch (model)
            {
                case mLadder: case mCascade: case mDiode:
                {
                    float u;
                    if (model == mLadder) u = fastTanh (x * dg - ladK * st.ladOut[c]);
                    else if (model == mCascade) u = x * dg - ladK * fastTanh (st.ladOut[c]);
                    else u = fastTanh (x * dg * 1.3f - ladK * st.ladOut[c] + 0.1f * st.ladOut[c] * st.ladOut[c]);
                    float in = u, ys[4];
                    for (int s = 0; s < 4; ++s)
                    {
                        ys[s] = tptOnePole (st.lad[c][s], in, G);
                        if (model == mDiode && s == 0) ys[s] = soft (ys[s], 1.2f);
                        in = ys[s];
                    }
                    st.ladOut[c] = ys[3];
                    switch (resp)
                    {
                        case rLP24: y = ys[3]; break;
                        case rLP18: y = ys[2]; break;
                        case rLP12: y = ys[1]; break;
                        case rLP6: y = ys[0]; break;
                        case rBP12: y = (ys[1] - ys[3]) * 2.0f; break;
                        case rBP6: y = (ys[0] - ys[1]) * 2.0f; break;
                        case rHP24: y = u - 4 * ys[0] + 6 * ys[1] - 4 * ys[2] + ys[3]; break;
                        case rHP12: y = u - 2 * ys[0] + ys[1]; break;
                        case rHP6: y = u - ys[0]; break;
                        case rNotch: y = u - 4 * ys[0] + 6 * ys[1] - 4 * ys[2] + 2 * ys[3]; break;
                        case rPeak: y = ys[3] + 2.0f * (ys[1] - ys[3]); break;
                        default: y = 2.0f * ys[0] - u; break;
                    }
                    y *= ladComp;
                    break;
                }
                case mFormant:
                {
                    const float xin = drive > 0 ? fastTanh (x * dg) : x;
                    for (int j = 0; j < 3; ++j)
                    {
                        st.f[c][j].tick (xin, formG[c][j], formK);
                        y += st.f[c][j].bp * formK * formAmp[j] * 2.0f;
                    }
                    break;
                }
                case mPhaser:
                {
                    float in = (drive > 0 ? fastTanh (x * dg) : x) + st.apFb[c] * res * 0.85f;
                    for (int j = 0; j < 8; ++j)
                    {
                        const float a = apA[c][j];
                        const float yy = a * in + st.ap[c][j];
                        st.ap[c][j] = in - a * yy;
                        in = yy;
                    }
                    st.apFb[c] = in;
                    y = resp == rPeak ? 0.5f * (x - in) : (resp == rAP ? in : 0.5f * (x + in));
                    break;
                }
                case mLpg:
                {
                    s1.tick (drive > 0 ? fastTanh (x * dg) : x, g, 2.0f - res * 1.2f);
                    y = tptOnePole (st.one[c], s1.lp, G) * lpgGain;
                    break;
                }
                case mTwinPeak:
                {
                    const float xin = drive > 0 ? fastTanh (x * dg) : x;
                    const float kk = juce::jmax (0.08f, 2.0f - res * 1.92f);
                    st.f[c][0].tick (xin, g, kk);
                    st.f[c][1].tick (xin, g2[c], kk);
                    y = (st.f[c][0].bp + st.f[c][1].bp * 0.8f) * kk * 1.5f;
                    break;
                }
                default:   // SVF family
                {
                    float xin = x;
                    switch (model)
                    {
                        case mSallenKey: xin = fastTanh (x * dg * 1.5f); break;
                        case mOta: xin = fastTanh (x * dg); break;
                        case mBite: xin = fastTanh (x * dg * 3.0f); break;
                        case mSteiner: xin = fastTanh (x * dg * 1.2f); break;
                        default: if (drive > 0) xin = fastTanh (x * dg); break;
                    }
                    s1.tick (xin, g, k);
                    if (model == mSallenKey) s1.ic1 = soft (s1.ic1, 0.7f);
                    if (model == mSteiner) { s1.ic1 = soft (s1.ic1, 0.5f); s1.ic2 = soft (s1.ic2, 0.4f); }
                    if (model == mOta) s1.ic1 = soft (s1.ic1, 0.35f);
                    if (model == mBite) s1.ic1 = soft (s1.ic1, 0.8f);
                    switch (resp)
                    {
                        case rLP24: s2.tick (s1.lp, g, kB); y = s2.lp; break;
                        case rLP18: y = tptOnePole (st.one[c], s1.lp, G); break;
                        case rLP12: y = s1.lp; break;
                        case rLP6: y = tptOnePole (st.one[c], xin, G); break;
                        case rBP12: y = s1.bp * (0.5f + 0.5f * k); break;
                        case rBP6: { const float lp = tptOnePole (st.one[c], xin, G); y = tptOnePole (st.one2[c], xin - lp, G) * 2.0f; break; }
                        case rHP24: s2.tick (s1.hp, g, kB); y = s2.hp; break;
                        case rHP12: y = s1.hp; break;
                        case rHP6: y = xin - tptOnePole (st.one[c], xin, G); break;
                        case rNotch: y = s1.lp + s1.hp; break;
                        case rPeak: y = s1.lp - s1.hp; break;
                        default: y = s1.lp + s1.hp - k * s1.bp; break;
                    }
                    if (model == mBite) y = fastTanh (y * 1.5f);
                    break;
                }
            }
            if (! std::isfinite (y)) { st.reset(); y = 0; }
            io[i] = x + mix * (y - x);
        }
        st.gPrev[c] = gT[c];
    }
}

// ======================================================================= SHAPER
void processShape (ShapeState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    const int mode = p.ch (sp::mode);
    const float drive = clampf (p (sp::drive), 0, 1), bias = p (sp::bias), mix = clampf (p (sp::mix), 0, 1);
    const float outG = dbToGain (p (sp::out), -100.0f);
    const float dg = 1.0f + drive * drive * 24.0f;
    const float toneA = OnePole::coef (400.0f * std::exp2 (clampf (p (sp::tone), 0, 1) * 5.6f), v.sh.sr);
    const float levels = std::exp2 (1.0f + (1.0f - drive) * 11.0f);
    const float holdInc = 1.0f / (1.0f + drive * drive * 48.0f);
    const bool dc = mode == 0 || mode == 3 || mode == 5 || bias != 0.0f;
    const float tb = fastTanh (bias * 0.5f);
    for (int i = 0; i < n; ++i)
    {
        bool tick = false;
        if (mode == 7) { st.holdCnt += holdInc; if (st.holdCnt >= 1.0f) { st.holdCnt -= 1.0f; tick = true; } }
        for (int c = 0; c < 2; ++c)
        {
            float* io = c ? R : L;
            const float x = io[i];
            float y;
            switch (mode)
            {
                case 0: y = fastTanh (x * dg + bias * 0.5f) - tb; break;
                case 1: y = clampf (x * dg + bias * 0.3f, -1.0f, 1.0f); break;
                case 2: y = fastSin ((x * dg * 0.5f + bias * 0.25f) * 0.25f); break;
                case 3: { const float u = x * dg * 0.5f + bias; y = 4.0f * std::abs (frac (u * 0.25f + 0.25f) - 0.5f) - 1.0f; break; }
                case 4: y = fastSin ((x * (0.6f + drive * 5.0f) + bias * 0.4f) * 0.25f); break;
                case 5: { const float z = x * dg * 0.4f; y = fastTanh (bias < 0 ? juce::jmax (z, z * (1.0f + bias)) : std::abs (z)) * 1.3f; break; }
                case 6: y = std::round (x * levels) / levels; break;
                default: if (tick) st.hold[c] = x; y = st.hold[c]; break;
            }
            if (dc) y = st.dc[c].process (y);
            st.lp[c] += toneA * (y - st.lp[c]);
            y = st.lp[c];
            io[i] = (x + mix * (y - x)) * outG;
        }
    }
}

// ======================================================================= RING
void processRing (RingState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    const int src = p.ch (gp::source);
    const float mix = clampf (p (gp::mix), 0, 1);
    if (src >= 3)
    {
        const int j = src - 3;
        if (j >= v.lane) return;
        for (int i = 0; i < n; ++i)
        {
            L[i] += mix * (L[i] * v.laneOut[j][0][i] * 2.0f - L[i]);
            R[i] += mix * (R[i] * v.laneOut[j][1][i] * 2.0f - R[i]);
        }
        return;
    }
    if (v.trigger) st.ph = 0;
    const double inc = keyedHz (v.note, p (gp::keytrack), p (gp::tune) + p (gp::fine) * 0.01f) / v.sh.sr;
    for (int i = 0; i < n; ++i)
    {
        const float ph = (float) st.ph;
        float m;
        if (src == 0) m = fastSin (ph);
        else if (src == 1) m = 1.0f - 4.0f * std::abs (frac (ph + 0.25f) - 0.5f);
        else m = fastTanh (fastSin (ph) * 6.0f);
        st.ph += inc;
        if (st.ph >= 1.0) st.ph -= 1.0;
        L[i] += mix * (L[i] * m - L[i]);
        R[i] += mix * (R[i] * m - R[i]);
    }
}

// ======================================================================= EQ
void processEq (EqState& st, const P& p, const VoiceCtx& v, float* L, float* R, int n)
{
    float c[7];
    for (int k = 0; k < 7; ++k) c[k] = p (k);
    bool changed = false;
    for (int k = 0; k < 7; ++k) if (c[k] != st.cache[k]) { changed = true; st.cache[k] = c[k]; }
    if (changed)
    {
        st.lo.lowShelf (v.sh.sr, c[ep::lowFreq], c[ep::lowGain]);
        st.mid.peak (v.sh.sr, c[ep::midFreq], c[ep::midGain], c[ep::midQ]);
        st.hi.highShelf (v.sh.sr, c[ep::highFreq], c[ep::highGain]);
    }
    for (int i = 0; i < n; ++i)
    {
        L[i] = st.hi.process (st.mid.process (st.lo.process (L[i], 0), 0), 0);
        R[i] = st.hi.process (st.mid.process (st.lo.process (R[i], 1), 1), 1);
    }
}

// ======================================================================= MSEG curve
float MsegData::valueAt (float t) const
{
    if (n <= 0) return 0.0f;
    if (t <= x[0]) return y[0];
    for (int i = 0; i < n - 1; ++i)
    {
        if (t < x[i + 1])
        {
            const float span = juce::jmax (1e-6f, x[i + 1] - x[i]);
            const float f = (t - x[i]) / span;
            const float cv = c[i + 1];
            const float s = cv >= 0 ? std::pow (f, 1.0f + cv * 4.0f) : 1.0f - std::pow (1.0f - f, 1.0f - cv * 4.0f);
            return y[i] + (y[i + 1] - y[i]) * s;
        }
    }
    return y[n - 1];
}
}
