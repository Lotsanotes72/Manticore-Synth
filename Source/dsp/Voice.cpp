#include "Voice.h"

namespace mc::dsp
{
namespace
{
struct Idx
{
    int env[4], lfo[4], mseg[4], map[4], math[4];
    int laneVol[4], lanePan[4], laneSend[4][3], laneIn[4], laneInLvl[4];
    int gridBase[64];
    int glide;
    Idx()
    {
        for (int i = 0; i < 4; ++i)
        {
            env[i] = moduleBase (Kind::Env, i); lfo[i] = moduleBase (Kind::Lfo, i); mseg[i] = moduleBase (Kind::Mseg, i);
            map[i] = moduleBase (Kind::Map, i); math[i] = moduleBase (Kind::Math, i);
            laneVol[i] = paramIndex (id::lane (i, "vol")); lanePan[i] = paramIndex (id::lane (i, "pan"));
            for (int a = 0; a < 3; ++a) laneSend[i][a] = paramIndex (id::lane (i, ("send" + juce::String (a + 1)).toRawUTF8()));
            laneIn[i] = paramIndex (id::lane (i, "in")); laneInLvl[i] = paramIndex (id::lane (i, "inlvl"));
        }
        glide = paramIndex (id::glide);
        const auto& g = gridEntries();
        for (size_t e = 0; e < g.size() && e < 64; ++e) gridBase[e] = g[e].inst < 0 ? -1 : moduleBase (g[e].kind, g[e].inst);
    }
};
const Idx& ix() { static Idx i; return i; }

// mapper input list (see Params.cpp) -> source ids
const int kMapInput[] { sKey, sVelocity, sRandom, sModWheel, sAftertouch, sEnv1, sEnv1 + 1, sEnv1 + 2, sEnv1 + 3,
                        sLfo1, sLfo1 + 1, sLfo1 + 2, sLfo1 + 3, sMseg1, sMseg1 + 1, sMseg1 + 2, sMseg1 + 3, sAlternate };

int mathSourceToSrc (int choice)   // math source lists skip the four Math entries
{
    return choice >= sMath1 ? choice + 4 : choice;
}

inline float shapeA (float p, float c) { return c >= 0 ? 1.0f - std::pow (1.0f - p, 1.0f + c * 3.0f) : std::pow (p, 1.0f - c * 2.0f); }
inline float shapeD (float p, float c) { return c >= 0 ? 1.0f - std::pow (1.0f - p, 1.0f + c * 4.0f) : std::pow (p, 1.0f - c * 2.0f); }

// Advance an envelope by dt seconds and return the level.
float envAdvance (EnvState& e, const P& p, float dt, float keyMul, bool oneShot)
{
    const float c = p (envp::slope);
    float budget = dt;
    for (int guard = 0; guard < 4 && budget > 0; ++guard)
    {
        switch (e.stage)
        {
            case 0: e.level = 0; return 0;
            case 1:
            {
                const float T = juce::jmax (1e-5f, p (envp::delay));
                const float need = (1.0f - e.pos) * T;
                if (budget < need) { e.pos += budget / T; budget = 0; }
                else { budget -= need; e.stage = 2; e.pos = 0; e.from = e.level; }
                break;
            }
            case 2:
            {
                const float T = juce::jmax (0.0002f, p (envp::attack) * keyMul);
                const float need = (1.0f - e.pos) * T;
                if (budget < need) { e.pos += budget / T; budget = 0; e.level = e.from + (1.0f - e.from) * shapeA (e.pos, c); }
                else { budget -= need; e.stage = 3; e.pos = 0; e.level = 1.0f; }
                break;
            }
            case 3:
            {
                const float S = clampf (p (envp::sustain), 0, 1);
                const float T = juce::jmax (0.0005f, p (envp::decay) * keyMul);
                const float need = (1.0f - e.pos) * T;
                if (budget < need) { e.pos += budget / T; budget = 0; e.level = 1.0f + (S - 1.0f) * shapeD (e.pos, c); }
                else
                {
                    budget -= need; e.level = S; e.pos = 0;
                    if (oneShot) { e.stage = 5; e.from = S; } else e.stage = 4;
                }
                break;
            }
            case 4:
                e.level = clampf (p (envp::sustain), 0, 1);
                budget = 0;
                break;
            default:
            {
                const float T = juce::jmax (0.0005f, p (envp::release) * keyMul);
                const float need = (1.0f - e.pos) * T;
                if (budget < need) { e.pos += budget / T; budget = 0; e.level = e.from * (1.0f - shapeD (e.pos, c)); }
                else { e.stage = 0; e.level = 0; e.pos = 0; return 0; }
                break;
            }
        }
    }
    return e.level;
}
}

float lfoShape (int wave, double phase, LfoState& st, bool wrapped, Rng& rng)
{
    const float p = (float) phase;
    switch (wave)
    {
        case 0: return fastSin (p);
        case 1: return 1.0f - 4.0f * std::abs (frac (p + 0.25f) - 0.5f);
        case 2: return 2.0f * p - 1.0f;
        case 3: return 1.0f - 2.0f * p;
        case 4: return p < 0.5f ? 1.0f : -1.0f;
        case 5:
            if (wrapped || ! st.started) st.held = rng.bi();
            st.started = true;
            return st.held;
        default:
        {
            if (wrapped || ! st.started) { st.prev = st.started ? st.next : rng.bi(); st.next = rng.bi(); }
            st.started = true;
            const float s = 0.5f - 0.5f * std::cos (kPi * p);
            return st.prev + (st.next - st.prev) * s;
        }
    }
}

void Voice::prepare (double sampleRate)
{
    sr = sampleRate;
    for (auto& c : comb) c.allocate();
    mod.assign ((size_t) destParams().size() + 2, 0.0f);
    active = false;
}

void Voice::start (int n, float vel, float fromNote, bool retrigger, uint32_t sd, float alt)
{
    const bool wasActive = active;
    note = n;
    velocity = vel;
    targetNote = (float) n;
    curNote = fromNote;
    gate = true;
    sustained = false;
    active = true;
    trigger = true;
    noteTime = 0;
    seed = sd;
    rng = Rng (sd);
    randomVal = rng.bi();
    alternate = alt;
    polyAT = 0;
    for (auto& e : env)
    {
        if (! wasActive) e.level = 0;
        e.stage = 1; e.pos = 0; e.from = e.level;
    }
    for (auto& m : mseg) m.t = 0;
    for (auto& m : map) m.stepPos = 0;
    for (auto& m : mathLag) m = 0;
    if (! wasActive || retrigger)
    {
        for (auto& v : vcf) if (! wasActive) v.reset();
        for (auto& c : comb) if (! wasActive) { c.clear(); c.dLen[0] = 0; }
        for (auto& r : res) for (int c = 0; c < 2; ++c) for (int m = 0; m < ResState::M; ++m) if (! wasActive) r.y1[c][m] = r.y2[c][m] = 0;
        if (! wasActive) laneGainInit = false;
    }
}

void Voice::glideTo (int n, float vel, bool retriggerEnvs)
{
    note = n;
    targetNote = (float) n;
    gate = true;
    sustained = false;
    if (retriggerEnvs)
    {
        velocity = vel;
        for (auto& e : env) { e.stage = 1; e.pos = 0; e.from = e.level; }
        for (auto& m : mseg) m.t = 0;
        noteTime = 0;
        trigger = true;
    }
}

void Voice::release()
{
    gate = false;
    sustained = false;
    for (int i = 0; i < 4; ++i)
    {
        auto& e = env[i];
        if (e.stage >= 1 && e.stage <= 4) { e.stage = 5; e.pos = 0; e.from = e.level; }
    }
}

void Voice::kill()
{
    active = false;
    gate = false;
    for (auto& e : env) { e.stage = 0; e.level = 0; }
}

void Voice::render (const Shared& sh, float* const* bus, int n)
{
    int done = 0;
    while (done < n && active)
    {
        const int len = juce::jmin (kBlock, n - done);
        renderBlock (sh, bus, done, len);
        done += len;
    }
}

void Voice::computeModulators (const Shared& sh, int n, float* env1Buf)
{
    const auto& I = ix();
    const float dt = (float) n / (float) sr;
    const float* m = mod.data();
    // envelopes (ENV 1 per sample)
    for (int i = 0; i < 4; ++i)
    {
        P p { sh, I.env[i], m };
        const float keyMul = std::exp2 (-p (envp::keyscale) * (curNote - 60.0f) / 24.0f);
        const bool oneShot = p.on (envp::oneShot);
        float lvl;
        if (i == 0)
        {
            const float sdt = 1.0f / (float) sr;
            for (int k = 0; k < n; ++k) env1Buf[k] = envAdvance (env[0], p, sdt, keyMul, oneShot);
            lvl = env1Buf[n - 1];
        }
        else lvl = envAdvance (env[i], p, dt, keyMul, oneShot);
        const float vs = p (envp::velocity);
        const float vScale = 1.0f - vs + vs * velocity;
        if (i == 0) for (int k = 0; k < n; ++k) env1Buf[k] *= vScale;
        src[sEnv1 + i] = lvl * vScale;
    }
    // LFOs
    for (int i = 0; i < 4; ++i)
    {
        P p { sh, I.lfo[i], m };
        const int mode = p.ch (lfop::mode);
        float v;
        if (mode == 2) v = sh.globalLfo[i];
        else
        {
            auto& L = lfo[i];
            if (trigger && (mode == 0 || ! L.started)) { L.phase = p (lfop::phase); L.started = false; }
            const double rate = p.on (lfop::sync) ? sh.bpm / 60.0 / divisionBeats (p.ch (lfop::division)) : (double) p (lfop::rate);
            L.phase += rate * dt;
            bool wrapped = false;
            if (L.phase >= 1.0) { L.phase -= std::floor (L.phase); wrapped = true; }
            v = lfoShape (p.ch (lfop::wave), L.phase, L, wrapped, rng);
            if (p.on (lfop::unipolar)) v = 0.5f * (v + 1.0f);
            const float del = p (lfop::delay), fade = p (lfop::fade);
            float amp = 1.0f;
            if (noteTime < del) amp = 0.0f;
            else if (fade > 0.0f) amp = juce::jmin (1.0f, (noteTime - del) / fade);
            v *= amp;
        }
        src[sLfo1 + i] = v * p (lfop::amount);
    }
    // MSEGs
    for (int i = 0; i < 4; ++i)
    {
        P p { sh, I.mseg[i], m };
        const auto& d = sh.mseg[i];
        auto& s = mseg[i];
        const float tscale = juce::jmax (0.01f, p (msp::time));
        const double unitsPerSec = (p.on (msp::sync) ? sh.bpm / 60.0 : 1.0) / tscale;
        s.t += (float) (unitsPerSec * dt);
        const int loop = p.ch (msp::loop);
        const float len = d.length();
        if (loop == 2 && len > 0) { if (s.t >= len) s.t = std::fmod (s.t, len); }
        else if (loop == 1 && gate && d.loopEnd > d.loopStart && d.loopEnd < d.n)
        {
            const float a = d.x[d.loopStart], b = d.x[d.loopEnd];
            if (s.t >= b && b > a) s.t = a + std::fmod (s.t - a, b - a);
        }
        if (s.t > len) s.t = len;
        float v = d.valueAt (s.t);
        if (p.on (msp::bipolar)) v = v * 2.0f - 1.0f;
        const float vs = p (msp::velocity);
        v *= 1.0f - vs + vs * velocity;
        src[sMseg1 + i] = v * p (msp::amount);
    }
    // performance sources
    src[sVelocity] = velocity;
    src[sKey] = (curNote - 60.0f) / 60.0f;
    src[sModWheel] = sh.modWheel;
    src[sAftertouch] = juce::jmax (sh.aftertouch, polyAT);
    src[sPitchBend] = sh.bend;
    for (int k = 0; k < 4; ++k) src[sMacroA + k] = sh.macro[k];
    src[sRandom] = randomVal;
    src[sGate] = gate ? 1.0f : 0.0f;
    src[sExpression] = sh.expression;
    src[sAlternate] = alternate;
    for (int k = 0; k < 2 * kNumXY; ++k) src[sXY1X + k] = sh.xy[k];
    // mappers
    for (int i = 0; i < 4; ++i)
    {
        P p { sh, I.map[i], m };
        const auto& d = sh.map[i];
        auto& s = map[i];
        const int steps = juce::jlimit (2, kMapSteps, juce::roundToInt (p (mpp::steps)));
        const float smooth = clampf (p (mpp::smooth), 0, 1);
        float v;
        if (p.ch (mpp::mode) == 1)
        {
            s.stepPos += sh.bpm / 60.0 / divisionBeats (p.ch (mpp::division)) * dt;
            const int idx = (int) std::floor (s.stepPos) % steps;
            const float target = d.v[idx];
            const float a = smooth <= 0.001f ? 1.0f : 1.0f - std::exp (-dt / (smooth * 0.25f));
            s.value = trigger ? target : s.value + a * (target - s.value);
            v = s.value;
        }
        else
        {
            const int inSel = juce::jlimit (0, 17, p.ch (mpp::input));
            float u = src[kMapInput[inSel]];
            const bool bip = kMapInput[inSel] == sKey || (kMapInput[inSel] >= sLfo1 && kMapInput[inSel] < sLfo1 + 4)
                             || kMapInput[inSel] == sRandom || kMapInput[inSel] == sAlternate;
            if (bip) u = 0.5f * (u + 1.0f);
            u = clampf (u, 0, 1) * (float) (steps - 1);
            const int a = juce::jmin (steps - 1, (int) u);
            const int b = juce::jmin (steps - 1, a + 1);
            const float f = (u - (float) a) * smooth;
            v = d.v[a] + f * (d.v[b] - d.v[a]);
        }
        src[sMap1 + i] = v * p (mpp::amount);
    }
    // mod math
    for (int i = 0; i < 4; ++i)
    {
        P p { sh, I.math[i], m };
        const float a = src[mathSourceToSrc (p.ch (mathp::srcA))], b = src[mathSourceToSrc (p.ch (mathp::srcB))];
        float v;
        switch (p.ch (mathp::opr))
        {
            case 0: v = a + b; break;
            case 1: v = a - b; break;
            case 2: v = a * b; break;
            case 3: v = juce::jmax (a, b); break;
            case 4: v = juce::jmin (a, b); break;
            case 5: v = 0.5f * (a + b); break;
            case 6: { const float T = 0.002f + std::abs (b) * 2.0f; mathLag[i] += (1.0f - std::exp (-dt / T)) * (a - mathLag[i]); v = mathLag[i]; break; }
            case 7: { const float st = 2.0f + std::abs (b) * 14.0f; v = std::round (a * st) / st; break; }
            case 8: v = std::abs (a); break;
            default: v = -a; break;
        }
        src[sMath1 + i] = clampf (v, -1.0f, 1.0f) * p (mathp::amount);
    }
    src[sNone] = 0.0f;
}

void Voice::renderBlock (const Shared& sh, float* const* bus, int offset, int n)
{
    const auto& I = ix();
    const auto& entries = gridEntries();
    const float dt = (float) n / (float) sr;

    // glide
    if (curNote != targetNote)
    {
        const float gt = sh.val[(size_t) I.glide];
        if (gt <= 0.0005f) curNote = targetNote;
        else
        {
            curNote += (targetNote - curNote) * (1.0f - std::exp (-dt / (gt * 0.3f)));
            if (std::abs (targetNote - curNote) < 0.001f) curNote = targetNote;
        }
    }

    float env1[kBlock];
    computeModulators (sh, n, env1);

    // modulation matrix
    std::fill (mod.begin(), mod.end(), 0.0f);
    for (int k = 0; k < sh.nMtx; ++k)
    {
        const auto& s = sh.mtx[k];
        float v = src[s.src] * s.amt;
        if (s.via > 0) v *= src[s.via];
        mod[(size_t) s.dst] += v;
    }

    const float bendSemis = sh.bend >= 0 ? sh.bend * sh.bendUp : sh.bend * sh.bendDown;
    VoiceCtx ctx { sh, mod.data(), curNote + sh.transpose + bendSemis, velocity, noteTime, gate, trigger,
                   laneOut, 0, seed };

    for (int l = 0; l < kNumLanes; ++l)
    {
        float* L = laneOut[l][0];
        float* R = laneOut[l][1];
        std::fill (L, L + n, 0.0f);
        std::fill (R, R + n, 0.0f);
        bool used = false;
        const int in = sh.choice (I.laneIn[l]);
        if (in >= 1 && in - 1 < l)
        {
            const float g = sh.get (I.laneInLvl[l], mod.data());
            for (int i = 0; i < n; ++i) { L[i] = laneOut[in - 1][0][i] * g; R[i] = laneOut[in - 1][1][i] * g; }
            used = true;
        }
        ctx.lane = l;
        for (int s = 0; s < kNumSlots; ++s)
        {
            const int e = sh.grid[l][s];
            if (e <= 0) continue;
            used = true;
            const auto& ge = entries[(size_t) e];
            P p { sh, I.gridBase[e], mod.data() };
            const int k = ge.inst;
            switch (ge.kind)
            {
                case Kind::Osc:   processOsc (osc[k], p, ctx, L, R, n); break;
                case Kind::FM:    processFm (fm[k], p, ctx, L, R, n); break;
                case Kind::Noise: processNoise (noise[k], p, ctx, L, R, n); break;
                case Kind::Exc:   processExc (exc[k], p, ctx, L, R, n); break;
                case Kind::Res:   processRes (res[k], p, ctx, L, R, n); break;
                case Kind::Comb:  processComb (comb[k], p, ctx, L, R, n); break;
                case Kind::Vcf:   processVcf (vcf[k], p, ctx, L, R, n); break;
                case Kind::Shape: processShape (shape[k], p, ctx, L, R, n); break;
                case Kind::Ring:  processRing (ring[k], p, ctx, L, R, n); break;
                case Kind::Eq:    processEq (eq[k], p, ctx, L, R, n); break;
                default: break;
            }
        }
        if (! used) continue;

        // lane output: VCA (ENV 1), volume, pan, sends
        const float vol = dbToGain (sh.get (I.laneVol[l], mod.data()));
        float pl, pr;
        panGains (sh.get (I.lanePan[l], mod.data()), pl, pr);
        float tgt[5] { vol * pl, vol * pr, 0, 0, 0 };
        for (int a = 0; a < 3; ++a) tgt[2 + a] = vol * clampf (sh.get (I.laneSend[l][a], mod.data()), 0, 1);
        if (! laneGainInit) for (int k = 0; k < 5; ++k) laneGain[l][k] = tgt[k];
        float step[5], gcur[5];
        for (int k = 0; k < 5; ++k) { step[k] = (tgt[k] - laneGain[l][k]) / (float) n; gcur[k] = laneGain[l][k]; laneGain[l][k] = tgt[k]; }
        float* mL = bus[0] + offset; float* mR = bus[1] + offset;
        for (int i = 0; i < n; ++i)
        {
            for (int k = 0; k < 5; ++k) gcur[k] += step[k];
            const float e = env1[i];
            const float xl = L[i] * e, xr = R[i] * e;
            mL[i] += xl * gcur[0];
            mR[i] += xr * gcur[1];
            for (int a = 0; a < 3; ++a)
                if (gcur[2 + a] > 0.0f || step[2 + a] != 0.0f)
                {
                    bus[2 + a * 2][offset + i] += xl * gcur[2 + a];
                    bus[3 + a * 2][offset + i] += xr * gcur[2 + a];
                }
        }
    }
    laneGainInit = true;
    noteTime += dt;
    trigger = false;
    if (env[0].stage == 0) active = false;
}
}
