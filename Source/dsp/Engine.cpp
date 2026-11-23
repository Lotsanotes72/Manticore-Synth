#include "Engine.h"
#include "../Evolve.h"

namespace mc::dsp
{
Engine::Engine()
{
    const auto& defs = params();
    const int np = (int) defs.size();
    sh.val.assign ((size_t) np, 0.0f);
    sh.norm.assign ((size_t) np, 0.0f);
    sh.destOf.assign ((size_t) np, 0);
    for (int i = 0; i < np; ++i)
    {
        sh.rng.push_back (defs[(size_t) i].range());
        sh.val[(size_t) i] = defs[(size_t) i].def;
        sh.norm[(size_t) i] = sh.rng.back().convertTo0to1 (defs[(size_t) i].def);
        sh.destOf[(size_t) i] = destOfParam (i);
    }
    sh.nDest = (int) destParams().size();
    gmod.assign ((size_t) sh.nDest + 2, 0.0f);
    rawVal = sh.val;
    hOff.assign ((size_t) np, 0.0f);
    hauntView = std::vector<std::atomic<float>> ((size_t) np);
    hSpeed.resize ((size_t) np);
    for (int i = 0; i < np; ++i)
    {
        uint32_t h = (uint32_t) i * 2654435761u ^ 0x9e3779b9u;
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
        hSpeed[(size_t) i] = 0.55f + 0.9f * (float) (h & 0xffff) / 65535.0f;
    }

    ix.master = paramIndex (id::master); ix.voices = paramIndex (id::voices); ix.vmode = paramIndex (id::vmode);
    ix.glideMode = paramIndex (id::glideMode); ix.pbUp = paramIndex (id::pbUp); ix.pbDown = paramIndex (id::pbDown);
    ix.transpose = paramIndex (id::transpose); ix.glide = paramIndex (id::glide); ix.arpOn = paramIndex (id::arpOn); ix.arpHold = paramIndex (id::arpHold);
    for (int m = 0; m < 4; ++m) ix.macro[m] = paramIndex (id::macro (m));
    for (int l = 0; l < 4; ++l) for (int s = 0; s < 4; ++s) { ix.slot[l][s] = paramIndex (id::slot (l, s)); ix.fxSlot[l][s] = paramIndex (id::fxSlot (l, s)); }
    for (int i = 0; i < kNumMatrix; ++i)
    {
        ix.mtxSrc[i] = paramIndex (id::mtx (i, "src")); ix.mtxVia[i] = paramIndex (id::mtx (i, "via"));
        ix.mtxDst[i] = paramIndex (id::mtx (i, "dst")); ix.mtxAmt[i] = paramIndex (id::mtx (i, "amt"));
    }
    for (int i = 0; i < 4; ++i) ix.lfoBase[i] = moduleBase (Kind::Lfo, i);
    for (int n = 0; n < kNumXY; ++n)
        for (int a = 0; a < 2; ++a)
        {
            ix.xyPos[n * 2 + a] = paramIndex (id::xyPos (n, a));
            for (int k = 0; k < kXYTargets; ++k)
            {
                const int row = (n * 2 + a) * kXYTargets + k;
                ix.xyDst[row] = paramIndex (id::xyTarget (n, a, k, "dst"));
                ix.xyAmt[row] = paramIndex (id::xyTarget (n, a, k, "amt"));
            }
        }
    ix.hauntOn = paramIndex (id::hauntOn); ix.hauntDepth = paramIndex (id::hauntDepth);
    ix.hauntSpeed = paramIndex (id::hauntSpeed); ix.hauntFollow = paramIndex (id::hauntFollow);
    // default MSEG / mapper shapes
    for (int i = 0; i < 4; ++i)
    {
        pendingMseg[i] = MsegData();
        for (int k = 0; k < kMapSteps; ++k) pendingMap[i].v[k] = juce::jmin (1.0f, (float) k / 15.0f * 2.0f - 1.0f);
    }
    WavetableBank::get();
}

void Engine::bind (juce::AudioProcessorValueTreeState& s)
{
    raw.clear();
    for (const auto& d : params()) raw.push_back (s.getRawParameterValue (d.id));
}

void Engine::bindValues (std::vector<std::atomic<float>>& v)
{
    raw.clear();
    for (auto& a : v) raw.push_back (&a);
}

void Engine::prepare (double sampleRate, int maxBlock)
{
    sh.sr = sampleRate;
    for (auto& v : voices) v.prepare (sampleRate);
    fx.prepare (sampleRate);
    bus.setSize (8, juce::jmax (64, maxBlock), false, true, false);
    chunks.reserve ((size_t) maxBlock / 4 + 64);
    arpEvents.reserve (512);
    evs.reserve (1024);
    reset();
}

void Engine::reset()
{
    for (auto& v : voices) v.kill();
    fx.reset();
    arp.reset();
    monoStack.clear();
    lastNote = -1;
    sustainPedal = false;
    for (auto& l : gLfo) l = LfoState();
    // deterministic restart (the offline level measurements rely on it)
    gRng = Rng (4242);
    ageCounter = 0;
    seedCounter = 1;
    altFlip = 1.0f;
    masterPrev = -1;
    lastArpOn = false;
}

void Engine::setMseg (int i, const MsegData& d)
{
    const juce::SpinLock::ScopedLockType sl (dataLock);
    pendingMseg[i] = d;
    dataDirty = true;
}

void Engine::setMap (int i, const MapData& d)
{
    const juce::SpinLock::ScopedLockType sl (dataLock);
    pendingMap[i] = d;
    dataDirty = true;
}

void Engine::updateShared()
{
    const int np = (int) raw.size();
    for (int i = 0; i < np; ++i) rawVal[(size_t) i] = raw[(size_t) i]->load (std::memory_order_relaxed);
    // stored values, plus Haunt's drift where it is active
    for (int i = 0; i < np; ++i)
    {
        const float v = rawVal[(size_t) i];
        const float off = hOff[(size_t) i];
        if (off == 0.0f)
        {
            if (v != sh.val[(size_t) i])
            {
                sh.val[(size_t) i] = v;
                sh.norm[(size_t) i] = sh.rng[(size_t) i].convertTo0to1 (v);
            }
        }
        else if (hauntTick || v != sh.val[(size_t) i])
        {
            const float n = clampf (sh.rng[(size_t) i].convertTo0to1 (v) + off, 0.0f, 1.0f);
            sh.norm[(size_t) i] = n;
            sh.val[(size_t) i] = sh.rng[(size_t) i].convertFrom0to1 (n);
        }
    }
    hauntTick = false;
    if (dataDirty.load())
    {
        const juce::SpinLock::ScopedTryLockType tl (dataLock);
        if (tl.isLocked())
        {
            for (int i = 0; i < 4; ++i) { sh.mseg[i] = pendingMseg[i]; sh.map[i] = pendingMap[i]; }
            dataDirty = false;
        }
    }
    // grids without duplicates
    {
        bool used[64] {};
        for (int l = 0; l < kNumLanes; ++l)
            for (int s = 0; s < kNumSlots; ++s)
            {
                int e = sh.choice (ix.slot[l][s]);
                if (e < 0 || e >= (int) gridEntries().size() || used[e]) e = 0;
                if (e > 0) used[e] = true;
                sh.grid[l][s] = e;
            }
    }
    {
        bool used[64] {};
        for (int b = 0; b < kNumBuses; ++b)
            for (int s = 0; s < kFxSlots; ++s)
            {
                int e = sh.choice (ix.fxSlot[b][s]);
                if (e < 0 || e >= (int) fxEntries().size() || used[e]) e = 0;
                if (e > 0) used[e] = true;
                sh.fxGrid[b][s] = e;
            }
    }
    for (int i = 0; i < 4; ++i) sh.lfoGlobal[i] = sh.choice (ix.lfoBase[i] + lfop::mode) == 2;
    // matrix
    sh.nMtx = 0;
    gSlots.clear();
    const auto& dp = destParams();
    const auto& defs = params();
    for (int i = 0; i < kNumMatrix; ++i)
    {
        const int s = sh.choice (ix.mtxSrc[i]), d = sh.choice (ix.mtxDst[i]), v = sh.choice (ix.mtxVia[i]);
        const float a = sh.val[(size_t) ix.mtxAmt[i]];
        if (s <= 0 || d <= 0 || d > (int) dp.size() || a == 0.0f || s >= sNumSources) continue;
        const bool globalDest = defs[(size_t) dp[(size_t) d - 1]].global;
        if (globalDest)
        {
            if (sourceIsGlobal (s)) gSlots.push_back ({ s, v, d, a });
        }
        else sh.mtx[sh.nMtx++] = { s, v, d, a };
    }
    // XY pads: positions (bipolar) and their target rows
    for (int k = 0; k < 2 * kNumXY; ++k) sh.xy[k] = sh.val[(size_t) ix.xyPos[k]] * 2.0f - 1.0f;
    for (int row = 0; row < kNumXYRows; ++row)
    {
        const int d = sh.choice (ix.xyDst[row]);
        const float a = sh.val[(size_t) ix.xyAmt[row]];
        if (d <= 0 || d > (int) dp.size() || a == 0.0f) continue;
        const int src = sXY1X + row / kXYTargets;
        if (defs[(size_t) dp[(size_t) d - 1]].global) gSlots.push_back ({ src, 0, d, a });
        else sh.mtx[sh.nMtx++] = { src, 0, d, a };
    }
    for (int m = 0; m < 4; ++m) sh.macro[m] = sh.val[(size_t) ix.macro[m]];
    sh.bendUp = sh.val[(size_t) ix.pbUp];
    sh.bendDown = sh.val[(size_t) ix.pbDown];
    sh.transpose = sh.val[(size_t) ix.transpose];
    maxVoices = juce::jlimit (1, kMaxVoices, juce::roundToInt (sh.val[(size_t) ix.voices]));
}

void Engine::advanceGlobalLfos (int n)
{
    const float dt = (float) n / (float) sh.sr;
    for (int i = 0; i < 4; ++i)
    {
        if (! sh.lfoGlobal[i]) { sh.globalLfo[i] = 0; continue; }
        P p { sh, ix.lfoBase[i], gmod.data() };
        auto& L = gLfo[i];
        const double rate = p.on (lfop::sync) ? sh.bpm / 60.0 / divisionBeats (p.ch (lfop::division)) : (double) p (lfop::rate);
        L.phase += rate * dt;
        bool wrapped = false;
        if (L.phase >= 1.0) { L.phase -= std::floor (L.phase); wrapped = true; }
        double ph = L.phase + p (lfop::phase);
        ph -= std::floor (ph);
        float v = lfoShape (p.ch (lfop::wave), ph, L, wrapped, gRng);
        if (p.on (lfop::unipolar)) v = 0.5f * (v + 1.0f);
        sh.globalLfo[i] = v;
    }
}

void Engine::computeGlobalMod (const float* lfo)
{
    for (int d : gDest) gmod[(size_t) d] = 0.0f;
    gDest.clear();
    for (const auto& s : gSlots)
    {
        auto val = [&] (int src) -> float
        {
            if (src >= sLfo1 && src < sLfo1 + 4)
            {
                const int i = src - sLfo1;
                return sh.lfoGlobal[i] ? lfo[i] * sh.val[(size_t) (ix.lfoBase[i] + lfop::amount)] : 0.0f;
            }
            switch (src)
            {
                case sModWheel: return sh.modWheel;
                case sAftertouch: return sh.aftertouch;
                case sPitchBend: return sh.bend;
                case sExpression: return sh.expression;
                default: break;
            }
            if (src >= sMacroA && src <= sMacroD) return sh.macro[src - sMacroA];
            if (src >= sXY1X && src < sXY1X + 2 * kNumXY) return sh.xy[src - sXY1X];
            return 0.0f;
        };
        float v = val (s.src) * s.amt;
        if (s.via > 0) v *= val (s.via);
        if (gmod[(size_t) s.dst] == 0.0f) gDest.push_back (s.dst);
        gmod[(size_t) s.dst] += v;
    }
}

int Engine::findVoice (int note)
{
    for (int i = 0; i < maxVoices; ++i)
        if (voices[(size_t) i].isActive() && voices[(size_t) i].note == note && voices[(size_t) i].isGateOn()) return i;
    for (int i = 0; i < maxVoices; ++i)
        if (! voices[(size_t) i].isActive()) return i;
    // steal: quietest released voice, else the oldest
    int best = -1;
    float bestLvl = 1e9f;
    for (int i = 0; i < maxVoices; ++i)
        if (! voices[(size_t) i].isGateOn() && voices[(size_t) i].ampLevel() < bestLvl) { bestLvl = voices[(size_t) i].ampLevel(); best = i; }
    if (best >= 0) return best;
    uint64_t oldest = UINT64_MAX;
    for (int i = 0; i < maxVoices; ++i)
        if (voices[(size_t) i].age < oldest) { oldest = voices[(size_t) i].age; best = i; }
    return juce::jmax (0, best);
}

void Engine::noteOn (int note, float vel)
{
    lastVel = vel;
    const int mode = sh.choice (ix.vmode);
    const float glide = sh.val[(size_t) ix.glide];
    const bool legatoGlideOnly = sh.choice (ix.glideMode) == 1;
    altFlip = -altFlip;
    if (mode == 0)
    {
        const int v = findVoice (note);
        auto& vc = voices[(size_t) v];
        bool anyHeld = false;
        for (int i = 0; i < maxVoices; ++i) if (voices[(size_t) i].isActive() && voices[(size_t) i].isGateOn()) { anyHeld = true; break; }
        const bool doGlide = glide > 0.0005f && lastNote >= 0 && (! legatoGlideOnly || anyHeld);
        vc.start (note, vel, doGlide ? lastNote : (float) note, true, seedCounter++, altFlip);
        vc.age = ++ageCounter;
    }
    else
    {
        auto& vc = voices[0];
        const bool held = ! monoStack.empty() && vc.isActive() && vc.isGateOn();
        monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
        monoStack.push_back (note);
        if (held && mode == 2)
            vc.glideTo (note, vel, false);
        else if (held)
        {
            vc.glideTo (note, vel, true);
        }
        else
        {
            const bool doGlide = glide > 0.0005f && lastNote >= 0 && ! legatoGlideOnly;
            vc.start (note, vel, doGlide ? lastNote : (float) note, true, seedCounter++, altFlip);
        }
        if (glide <= 0.0005f) { /* glideTo jumps immediately inside the voice */ }
        vc.age = ++ageCounter;
    }
    lastNote = (float) note;
}

void Engine::noteOff (int note)
{
    const int mode = sh.choice (ix.vmode);
    if (mode == 0)
    {
        for (int i = 0; i < kMaxVoices; ++i)
        {
            auto& v = voices[(size_t) i];
            if (v.isActive() && v.note == note && v.isGateOn())
            {
                if (sustainPedal) v.sustained = true;
                else v.release();
            }
        }
        return;
    }
    monoStack.erase (std::remove (monoStack.begin(), monoStack.end(), note), monoStack.end());
    auto& vc = voices[0];
    if (vc.note != note) return;
    if (! monoStack.empty())
    {
        vc.glideTo (monoStack.back(), vc.velocity, mode == 1);
        lastNote = (float) monoStack.back();
    }
    else if (sustainPedal) vc.sustained = true;
    else vc.release();
}

void Engine::allNotesOff()
{
    for (auto& v : voices) if (v.isActive()) v.release();
    monoStack.clear();
}

void Engine::renderVoices (int from, int to)
{
    int pos = from;
    while (pos < to)
    {
        const int len = juce::jmin (kBlock, to - pos);
        advanceGlobalLfos (len);
        Chunk c { pos, len, {} };
        for (int i = 0; i < 4; ++i) c.lfo[i] = sh.globalLfo[i];
        chunks.push_back (c);
        float* p[8];
        for (int k = 0; k < 8; ++k) p[k] = busPtr[k] + pos;
        for (auto& v : voices)
            if (v.isActive()) v.render (sh, p, len);
        pos += len;
    }
}

// ------------------------------------------------------------------ Haunt
namespace
{
inline float hashNoise (uint32_t i, int64_t k)
{
    uint32_t h = i * 0x9E3779B1u ^ (uint32_t) k * 0x85EBCA77u ^ (uint32_t) (k >> 32) * 0xC2B2AE3Du;
    h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15; h *= 0x846ca68bu; h ^= h >> 16;
    return (float) (h & 0xffffff) / 8388607.5f - 1.0f;
}
inline float valueNoise (uint32_t i, double t)
{
    const double fl = std::floor (t);
    const auto k = (int64_t) fl;
    const float f = (float) (t - fl);
    const float a = hashNoise (i, k), b = hashNoise (i, k + 1);
    return a + (b - a) * f * f * (3.0f - 2.0f * f);
}
}

void Engine::updateHaunt (int n)
{
    // runs before the parameters are read; uses the stored values in rawVal from the previous block for the gate
    const bool on = rawVal[(size_t) ix.hauntOn] > 0.5f;
    const float dt = (float) n / (float) sh.sr;
    if (hauntSnap.exchange (false)) hauntAmt = 0.0f;
    else hauntAmt += ((on ? 1.0f : 0.0f) - hauntAmt) * (1.0f - std::exp (-dt / 1.2f));
    if (! on && hauntAmt < 1.0e-3f) hauntAmt = 0.0f;

    // Follow: how hard you are playing (velocity of held notes, or the mod wheel)
    bool held = false;
    for (auto& v : voices) if (v.isActive() && v.isGateOn()) { held = true; break; }
    const float drive = juce::jmax (held ? lastVel : 0.0f, sh.modWheel);
    followLvl += (drive - followLvl) * (1.0f - std::exp (-dt / (drive > followLvl ? 0.35f : 2.5f)));

    hauntCountdown -= n;
    hauntElapsed += n;
    if (hauntCountdown > 0) return;
    const int elapsed = hauntElapsed;
    hauntElapsed = 0;
    hauntCountdown = juce::jmax (64, (int) (sh.sr / 200.0));        // ~5 ms control rate
    hauntLevel.store (hauntAmt, std::memory_order_relaxed);

    const int np = (int) hOff.size();
    if (hauntAmt <= 0.0f)
    {
        for (int i = 0; i < np; ++i)
            if (hOff[(size_t) i] != 0.0f) { hOff[(size_t) i] = 0.0f; hauntView[(size_t) i].store (0.0f, std::memory_order_relaxed); }
        return;
    }
    hauntTick = true;
    const float speed = rawVal[(size_t) ix.hauntSpeed];
    const double rate = 0.03 * std::pow (40.0, (double) speed);           // noise knots per second
    hauntTime += rate * (double) elapsed / sh.sr;
    const bool follow = rawVal[(size_t) ix.hauntFollow] > 0.5f;
    const float depth = rawVal[(size_t) ix.hauntDepth] * 0.3f * hauntAmt * (follow ? 0.1f + 0.9f * followLvl : 1.0f);
    const uint32_t lk = locks.load (std::memory_order_relaxed);

    GridMap gm;
    gm.build (rawVal.data());
    const auto& pcs = paramClasses();
    for (int i = 0; i < np; ++i)
    {
        float off = 0.0f;
        const auto& pc = pcs[(size_t) i];
        if (pc.haunt)
        {
            const int bit = lockBitOf (i, gm);
            bool ok = bit >= 0 && ! mc::locked (lk, bit);
            // drift only routings that exist
            if (ok && pc.group == PGroup::MtxAmt) ok = rawVal[(size_t) i] != 0.0f;
            if (ok && pc.group == PGroup::XYAmt) ok = rawVal[(size_t) i] != 0.0f;
            if (ok)
            {
                const double tt = hauntTime * hSpeed[(size_t) i] + (double) (i % 97) * 0.37;
                const float nz = 0.75f * valueNoise ((uint32_t) i, tt) + 0.25f * valueNoise ((uint32_t) i + 7919u, tt * 2.3 + 11.0);
                off = nz * depth * pc.hauntScale;
            }
        }
        if (off != hOff[(size_t) i])
        {
            hOff[(size_t) i] = off;
            hauntView[(size_t) i].store (off, std::memory_order_relaxed);
        }
    }
}

void Engine::process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi, juce::AudioPlayHead* ph)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    if (raw.empty()) { buffer.clear(); return; }
    if (bus.getNumSamples() < n) bus.setSize (8, n, false, true, true);
    updateHaunt (n);
    updateShared();

    double beat0 = -1.0;
    sh.bpm = 120.0;
    if (ph != nullptr)
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) sh.bpm = juce::jlimit (20.0, 400.0, *b);
            if (pos->getIsPlaying()) if (auto q = pos->getPpqPosition()) beat0 = *q;
        }
    sh.beatsPerSample = sh.bpm / 60.0 / sh.sr;

    // voice count changes
    for (int i = maxVoices; i < kMaxVoices; ++i) if (voices[(size_t) i].isActive()) voices[(size_t) i].kill();

    bus.clear();
    for (int k = 0; k < 8; ++k) busPtr[k] = bus.getWritePointer (k);
    chunks.clear();

    const bool arpOn = sh.on (ix.arpOn);
    if (arpOn != lastArpOn) { allNotesOff(); arp.reset(); lastArpOn = arpOn; }

    // collect events (player MIDI, possibly through the arpeggiator)
    evs.clear();
    arpEvents.clear();
    int last = 0;
    const bool hold = sh.on (ix.arpHold);
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        const int pos = juce::jlimit (0, n - 1, meta.samplePosition);
        if (arpOn && (m.isNoteOn() || m.isNoteOff()))
        {
            if (pos > last) { arp.process (sh, pos - last, beat0 < 0 ? -1.0 : beat0 + last * sh.beatsPerSample, arpEvents); for (auto& e : arpEvents) evs.push_back ({ e.pos + last, e.on ? 0 : 1, e.note, e.vel }); arpEvents.clear(); last = pos; }
            if (m.isNoteOn()) arp.noteOn (m.getNoteNumber(), m.getFloatVelocity(), hold);
            else arp.noteOff (m.getNoteNumber());
            continue;
        }
        if (m.isNoteOn()) evs.push_back ({ pos, 0, m.getNoteNumber(), m.getFloatVelocity() });
        else if (m.isNoteOff()) evs.push_back ({ pos, 1, m.getNoteNumber(), 0 });
        else if (m.isPitchWheel()) evs.push_back ({ pos, 3, 0, (float) (m.getPitchWheelValue() - 8192) / 8192.0f });
        else if (m.isController())
        {
            const int cc = m.getControllerNumber();
            const float v = (float) m.getControllerValue() / 127.0f;
            if (cc == 1) evs.push_back ({ pos, 4, 0, v });
            else if (cc == 11) evs.push_back ({ pos, 5, 0, v });
            else if (cc == 64) evs.push_back ({ pos, 6, 0, v });
            else if (cc == 123 || cc == 120) evs.push_back ({ pos, 7, 0, 0 });
        }
        else if (m.isChannelPressure()) evs.push_back ({ pos, 8, 0, (float) m.getChannelPressureValue() / 127.0f });
        else if (m.isAftertouch()) evs.push_back ({ pos, 9, m.getNoteNumber(), (float) m.getAfterTouchValue() / 127.0f });
        else if (m.isAllNotesOff() || m.isAllSoundOff()) evs.push_back ({ pos, 7, 0, 0 });
    }
    if (arpOn)
    {
        arp.process (sh, n - last, beat0 < 0 ? -1.0 : beat0 + last * sh.beatsPerSample, arpEvents);
        for (auto& e : arpEvents) evs.push_back ({ e.pos + last, e.on ? 0 : 1, e.note, e.vel });
        std::stable_sort (evs.begin(), evs.end(), [] (const Ev& a, const Ev& b) { return a.pos < b.pos; });
    }

    int cursor = 0;
    for (const auto& e : evs)
    {
        if (e.pos > cursor) { renderVoices (cursor, e.pos); cursor = e.pos; }
        switch (e.type)
        {
            case 0: if (e.b > 0) noteOn (e.a, e.b); else noteOff (e.a); break;
            case 1: noteOff (e.a); break;
            case 3: sh.bend = e.b; break;
            case 4: sh.modWheel = e.b; break;
            case 5: sh.expression = e.b; break;
            case 6:
                sustainPedal = e.b >= 0.5f;
                if (! sustainPedal) for (auto& v : voices) if (v.sustained) v.release();
                break;
            case 7: allNotesOff(); break;
            case 8: sh.aftertouch = e.b; break;
            case 9: for (auto& v : voices) if (v.isActive() && v.note == e.a) v.polyAT = e.b; break;
            default: break;
        }
    }
    if (cursor < n) renderVoices (cursor, n);

    // effects, in the same control-rate chunks so global LFOs stay smooth
    for (const auto& c : chunks)
    {
        computeGlobalMod (c.lfo);
        float* p[8];
        for (int k = 0; k < 8; ++k) p[k] = busPtr[k] + c.pos;
        fx.process (sh, gmod.data(), p, c.len);
    }

    // master
    const float master = dbToGain (sh.val[(size_t) ix.master]);
    if (masterPrev < 0) masterPrev = master;
    const float step = (master - masterPrev) / (float) n;
    float g = masterPrev;
    masterPrev = master;
    float pl = 0, pr = 0;
    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    for (int i = 0; i < n; ++i)
    {
        g += step;
        float l = busPtr[0][i] * g, r = busPtr[1][i] * g;
        if (! std::isfinite (l) || ! std::isfinite (r)) { l = r = 0; }
        outL[i] = l;
        if (outR) outR[i] = r; else outL[i] = 0.5f * (l + r);
        pl = juce::jmax (pl, std::abs (l));
        pr = juce::jmax (pr, std::abs (r));
    }
    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
    peakL = juce::jmax (pl, peakL.load());
    peakR = juce::jmax (pr, peakR.load());
    int playing = 0;
    for (auto& v : voices) if (v.isActive()) ++playing;
    voicesPlaying = playing;
    for (int i = 0; i < 4; ++i) lfoView[i] = sh.globalLfo[i];
}
}
