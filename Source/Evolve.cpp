#include "Evolve.h"
#include <cstring>

namespace mc
{
juce::String lockName (int bit)
{
    switch (bit)
    {
        case kLockGrid: return "Grid layout";
        case kLockMod: return "Modulators";
        case kLockMatrix: return "Mod Matrix";
        case kLockFx: return "Effects";
        case kLockArp: return "Arpeggiator";
        case kLockXY: return "XY pads";
        default: break;
    }
    if (bit >= kLockLane1 && bit < kLockLane1 + kNumLanes) return "Lane " + juce::String (bit - kLockLane1 + 1);
    return {};
}

// ======================================================================= classification
namespace
{
bool isGenKind (Kind k) { return (int) k <= (int) Kind::Eq; }
bool isModKindE (Kind k) { return k == Kind::Env || k == Kind::Lfo || k == Kind::Mseg || k == Kind::Map || k == Kind::Math; }
bool isSourceKind (Kind k) { return k == Kind::Osc || k == Kind::FM || k == Kind::Noise || k == Kind::Exc; }

struct Tables
{
    int slot[kNumLanes][kNumSlots], fxSlot[kNumBuses][kFxSlots];
    int laneVol[kNumLanes], lanePan[kNumLanes], laneSend[kNumLanes][3], laneIn[kNumLanes], laneInLvl[kNumLanes];
    int mtxSrc[kNumMatrix], mtxVia[kNumMatrix], mtxDst[kNumMatrix], mtxAmt[kNumMatrix];
    int xyPos[kNumXY][2], xyDst[kNumXY][2][kXYTargets], xyAmt[kNumXY][2][kXYTargets];
    int auxRet[3], macro[4];
    int arpOn, master, vmode, glide, glideMode;
    int genEntry[kNumKinds][8], fxEntry[kNumKinds][8];
    Tables()
    {
        for (int l = 0; l < kNumLanes; ++l)
        {
            for (int s = 0; s < kNumSlots; ++s) slot[l][s] = paramIndex (id::slot (l, s));
            laneVol[l] = paramIndex (id::lane (l, "vol")); lanePan[l] = paramIndex (id::lane (l, "pan"));
            for (int a = 0; a < 3; ++a) laneSend[l][a] = paramIndex (id::lane (l, ("send" + juce::String (a + 1)).toRawUTF8()));
            laneIn[l] = paramIndex (id::lane (l, "in")); laneInLvl[l] = paramIndex (id::lane (l, "inlvl"));
        }
        for (int b = 0; b < kNumBuses; ++b) for (int s = 0; s < kFxSlots; ++s) fxSlot[b][s] = paramIndex (id::fxSlot (b, s));
        for (int i = 0; i < kNumMatrix; ++i)
        {
            mtxSrc[i] = paramIndex (id::mtx (i, "src")); mtxVia[i] = paramIndex (id::mtx (i, "via"));
            mtxDst[i] = paramIndex (id::mtx (i, "dst")); mtxAmt[i] = paramIndex (id::mtx (i, "amt"));
        }
        for (int n = 0; n < kNumXY; ++n)
            for (int a = 0; a < 2; ++a)
            {
                xyPos[n][a] = paramIndex (id::xyPos (n, a));
                for (int k = 0; k < kXYTargets; ++k)
                {
                    xyDst[n][a][k] = paramIndex (id::xyTarget (n, a, k, "dst"));
                    xyAmt[n][a][k] = paramIndex (id::xyTarget (n, a, k, "amt"));
                }
            }
        for (int a = 0; a < 3; ++a) auxRet[a] = paramIndex (id::auxRet (a));
        for (int m = 0; m < 4; ++m) macro[m] = paramIndex (id::macro (m));
        arpOn = paramIndex (id::arpOn); master = paramIndex (id::master); vmode = paramIndex (id::vmode);
        glide = paramIndex (id::glide); glideMode = paramIndex (id::glideMode);
        for (auto& row : genEntry) for (auto& e : row) e = 0;
        for (auto& row : fxEntry) for (auto& e : row) e = 0;
        const auto& g = gridEntries();
        for (int e = 1; e < (int) g.size(); ++e) genEntry[(int) g[(size_t) e].kind][g[(size_t) e].inst] = e;
        const auto& f = fxEntries();
        for (int e = 1; e < (int) f.size(); ++e) fxEntry[(int) f[(size_t) e].kind][f[(size_t) e].inst] = e;
    }
};
const Tables& T() { static Tables t; return t; }

int laneOfId (const juce::String& s) { return s.substring (4, 5).getIntValue() - 1; }   // "lane2_vol" -> 1
}

int gridEntryOf (Kind k, int i) { return (int) k < kNumKinds && i >= 0 && i < 8 ? T().genEntry[(int) k][i] : 0; }
int fxEntryOf (Kind k, int i) { return (int) k < kNumKinds && i >= 0 && i < 8 ? T().fxEntry[(int) k][i] : 0; }

const std::vector<ParamClass>& paramClasses()
{
    static const std::vector<ParamClass> c = []
    {
        const auto& defs = params();
        std::vector<ParamClass> v (defs.size());
        for (size_t i = 0; i < defs.size(); ++i)
        {
            const auto& d = defs[i];
            auto& pc = v[i];
            const auto& id = d.id;
            if (d.kind != Kind::Count)
            {
                pc.group = isGenKind (d.kind) ? PGroup::GenModule : isModKindE (d.kind) ? PGroup::ModModule : PGroup::FxModule;
                const bool pitchy = d.caption == "Tune" || d.caption == "Fine";
                const bool keyT = d.caption == "Keytrack" && (d.kind == Kind::Osc || d.kind == Kind::FM || d.kind == Kind::Ring);
                pc.protect = pitchy || keyT
                          || (d.kind == Kind::Texture && d.local == txp::freeze)
                          || (d.kind == Kind::Env && d.inst == 0 && d.local == envp::oneShot)
                          || (d.kind == Kind::Lfo && d.local == lfop::mode)
                          || (d.kind == Kind::Osc && d.local == op::phase);
                pc.haunt = d.type == ParamDef::Float && d.unit != "int" && ! pitchy && ! keyT && ! (d.kind == Kind::Osc && d.local == op::phase)
                        && ! (d.kind == Kind::Env && d.local == envp::delay);
                if (d.kind == Kind::Env && (d.local == envp::attack || d.local == envp::decay || d.local == envp::release))
                    pc.hauntScale = 0.5f;
                if (d.kind == Kind::FM && d.local == fmp::ratio) pc.hauntScale = 0.25f;
                if (d.unit == "pan") pc.hauntScale = 0.8f;
            }
            else if (id.startsWith ("slot_")) pc.group = PGroup::Slot;
            else if (id.startsWith ("fxslot_")) pc.group = PGroup::FxSlot;
            else if (id.startsWith ("lane"))
            {
                pc.lane = laneOfId (id);
                pc.group = id.endsWith ("_in") ? PGroup::LaneIn : PGroup::LaneMix;
                pc.haunt = pc.group == PGroup::LaneMix;
                pc.hauntScale = id.endsWith ("_vol") ? 0.3f : 0.8f;
            }
            else if (id.startsWith ("aux") && id.endsWith ("_ret")) { pc.group = PGroup::AuxRet; pc.haunt = true; pc.hauntScale = 0.3f; }
            else if (id.startsWith ("mtx"))
            {
                pc.group = id.endsWith ("_src") ? PGroup::MtxSrc : id.endsWith ("_via") ? PGroup::MtxVia : id.endsWith ("_dst") ? PGroup::MtxDst : PGroup::MtxAmt;
                if (pc.group == PGroup::MtxAmt) { pc.haunt = true; pc.hauntScale = 0.5f; }
            }
            else if (id.startsWith ("macro_")) { pc.group = PGroup::Macro; pc.protect = true; }
            else if (id.startsWith ("arp_s"))
            {
                pc.group = PGroup::ArpStep;
                if (id.endsWith ("_vel")) { pc.haunt = true; pc.hauntScale = 0.6f; }
            }
            else if (id.startsWith ("arp_"))
            {
                pc.group = PGroup::ArpGlobal;
                pc.protect = id == id::arpOn || id == id::arpHold;
            }
            else if (id.startsWith ("xy"))
            {
                pc.group = id.endsWith ("_dst") ? PGroup::XYDst : id.endsWith ("_amt") ? PGroup::XYAmt : PGroup::XYPos;
                if (pc.group == PGroup::XYPos) { pc.haunt = true; pc.hauntScale = 1.6f; pc.protect = true; }
                if (pc.group == PGroup::XYAmt) { pc.haunt = true; pc.hauntScale = 0.5f; }
            }
            else if (id.startsWith ("haunt_")) { pc.group = PGroup::Perform; pc.protect = true; }
            else { pc.group = PGroup::Global; pc.protect = true; }
        }
        return v;
    }();
    return c;
}

void GridMap::build (const float* vals)
{
    const auto& t = T();
    for (int e = 0; e < 64; ++e) { entryLane[e] = -1; entrySlot[e] = -1; fxBus[e] = -1; }
    const int nG = (int) gridEntries().size(), nF = (int) fxEntries().size();
    for (int l = 0; l < kNumLanes; ++l)
    {
        laneActive[l] = false;
        for (int s = 0; s < kNumSlots; ++s)
        {
            const int e = juce::roundToInt (vals[t.slot[l][s]]);
            if (e <= 0 || e >= nG || entryLane[e] >= 0) continue;
            entryLane[e] = l; entrySlot[e] = s;
            laneActive[l] = true;
        }
        const int in = juce::roundToInt (vals[t.laneIn[l]]);
        if (in >= 1 && in - 1 < l) laneActive[l] = true;
    }
    for (int b = 0; b < kNumBuses; ++b)
        for (int s = 0; s < kFxSlots; ++s)
        {
            const int e = juce::roundToInt (vals[t.fxSlot[b][s]]);
            if (e > 0 && e < nF && fxBus[e] < 0) fxBus[e] = b;
        }
}

int lockBitOf (int i, const GridMap& gm)
{
    const auto& pc = paramClasses()[(size_t) i];
    const auto& d = params()[(size_t) i];
    switch (pc.group)
    {
        case PGroup::GenModule:
        {
            const int lane = gm.entryLane[gridEntryOf (d.kind, d.inst)];
            return lane < 0 ? -1 : kLockLane1 + lane;
        }
        case PGroup::LaneMix: return gm.laneActive[pc.lane] ? kLockLane1 + pc.lane : -1;
        case PGroup::LaneIn: case PGroup::Slot: return kLockGrid;
        case PGroup::ModModule: return kLockMod;
        case PGroup::MtxSrc: case PGroup::MtxVia: case PGroup::MtxDst: case PGroup::MtxAmt: return kLockMatrix;
        case PGroup::FxModule: return gm.fxBus[fxEntryOf (d.kind, d.inst)] >= 0 ? kLockFx : -1;
        case PGroup::FxSlot: case PGroup::AuxRet: return kLockFx;
        case PGroup::ArpGlobal: case PGroup::ArpStep: return kLockArp;
        case PGroup::XYPos: case PGroup::XYDst: case PGroup::XYAmt: return kLockXY;
        default: return -1;
    }
}

// ======================================================================= Patch
float Patch::norm (int i) const { return params()[(size_t) i].range().convertTo0to1 (v[(size_t) i]); }

void Patch::setNorm (int i, float n)
{
    const auto r = params()[(size_t) i].range();
    v[(size_t) i] = r.snapToLegalValue (r.convertFrom0to1 (juce::jlimit (0.0f, 1.0f, n)));
}

bool Patch::sameAs (const Patch& o) const
{
    if (v.size() != o.v.size()) return false;
    for (size_t i = 0; i < v.size(); ++i)
        if (std::abs (v[i] - o.v[i]) > 1e-5f * (1.0f + std::abs (v[i]))) return false;
    for (int k = 0; k < 4; ++k)
    {
        if (mseg[k].n != o.mseg[k].n) return false;
        // shapes are stored with 3-4 decimals in the state, so compare them with a tolerance
        auto near = [] (float a, float b) { return std::abs (a - b) < 2.0e-3f; };
        for (int p = 0; p < mseg[k].n; ++p)
            if (! near (mseg[k].x[p], o.mseg[k].x[p]) || ! near (mseg[k].y[p], o.mseg[k].y[p]) || ! near (mseg[k].c[p], o.mseg[k].c[p])) return false;
        for (int p = 0; p < kMapSteps; ++p) if (! near (map[k].v[p], o.map[k].v[p])) return false;
    }
    return true;
}

uint64_t Patch::hash() const
{
    uint64_t h = 1469598103934665603ull;
    auto mix = [&h] (float f) { uint32_t b; std::memcpy (&b, &f, 4); h = (h ^ b) * 1099511628211ull; };
    for (float f : v) mix (f);
    for (int k = 0; k < 4; ++k)
    {
        for (int p = 0; p < mseg[k].n; ++p) { mix (mseg[k].x[p]); mix (mseg[k].y[p]); mix (mseg[k].c[p]); }
        for (float f : map[k].v) mix (f);
    }
    return h;
}

Patch defaultPatch()
{
    Patch p;
    for (const auto& d : params()) p.v.push_back (d.type == ParamDef::Float ? d.def : (float) juce::roundToInt (d.def));
    for (int i = 0; i < 4; ++i)
    {
        p.mseg[i] = dsp::MsegData();
        for (int k = 0; k < kMapSteps; ++k) p.map[i].v[k] = juce::jmin (1.0f, (float) k / 15.0f * 2.0f - 1.0f);
    }
    return p;
}

// ======================================================================= helpers
namespace
{
float gauss (juce::Random& r)
{
    const float u1 = juce::jmax (1e-6f, r.nextFloat()), u2 = r.nextFloat();
    return std::sqrt (-2.0f * std::log (u1)) * std::cos (juce::MathConstants<float>::twoPi * u2);
}

template <typename T> const T& pick (const std::vector<T>& v, juce::Random& r) { return v[(size_t) r.nextInt ((int) v.size())]; }

int I (Kind k, int inst, int local) { return pidx (k, inst, local); }
int ch (const Patch& p, int i) { return juce::roundToInt (p.v[(size_t) i]); }

bool laneLocked (uint32_t locks, int l) { return locked (locks, kLockLane1 + l); }

// free instance of a kind (not placed in the grid)
int freeInstance (const Patch& p, Kind k, juce::Random& r)
{
    GridMap gm; gm.build (p.v.data());
    std::vector<int> free;
    for (int i = 0; i < kindCount (k); ++i) if (gm.entryLane[gridEntryOf (k, i)] < 0) free.push_back (i);
    return free.empty() ? -1 : pick (free, r);
}

void resetModule (Patch& p, Kind k, int inst)
{
    const auto& defs = params();
    for (int l = 0; l < localCount (k); ++l)
    {
        const int i = I (k, inst, l);
        p.v[(size_t) i] = defs[(size_t) i].def;
    }
}

float logRand (juce::Random& r, float lo, float hi) { return lo * std::pow (hi / lo, r.nextFloat()); }

// voice-module processors that Mutate may insert
void initInserted (Patch& p, Kind k, int inst, juce::Random& r)
{
    resetModule (p, k, inst);
    switch (k)
    {
        case Kind::Vcf:
        {
            const int models[] { 0, 1, 2, 4, 5, 8 };
            p.v[(size_t) I (k, inst, vp::model)] = (float) models[r.nextInt (6)];
            p.v[(size_t) I (k, inst, vp::response)] = r.nextBool() ? 0.0f : 2.0f;
            p.v[(size_t) I (k, inst, vp::cutoff)] = logRand (r, 500.0f, 7000.0f);
            p.v[(size_t) I (k, inst, vp::reso)] = 0.1f + 0.3f * r.nextFloat();
            break;
        }
        case Kind::Shape:
        {
            const int modes[] { 0, 1, 2, 4 };
            p.v[(size_t) I (k, inst, sp::mode)] = (float) modes[r.nextInt (4)];
            p.v[(size_t) I (k, inst, sp::drive)] = 0.15f + 0.35f * r.nextFloat();
            p.v[(size_t) I (k, inst, sp::mix)] = 0.4f + 0.5f * r.nextFloat();
            p.v[(size_t) I (k, inst, sp::out)] = -3.0f;
            break;
        }
        case Kind::Comb:
            p.v[(size_t) I (k, inst, cp::feedback)] = 0.6f + 0.3f * r.nextFloat();
            p.v[(size_t) I (k, inst, cp::mix)] = 0.25f + 0.25f * r.nextFloat();
            p.v[(size_t) I (k, inst, cp::damping)] = 0.3f + 0.3f * r.nextFloat();
            break;
        case Kind::Ring:
        {
            const float iv[] { 7.0f, 12.0f, 5.0f, 19.0f };
            p.v[(size_t) I (k, inst, gp::tune)] = iv[r.nextInt (4)];
            p.v[(size_t) I (k, inst, gp::mix)] = 0.2f + 0.3f * r.nextFloat();
            break;
        }
        case Kind::Eq:
            p.v[(size_t) I (k, inst, ep::lowGain)] = (r.nextFloat() * 2 - 1) * 5.0f;
            p.v[(size_t) I (k, inst, ep::midGain)] = (r.nextFloat() * 2 - 1) * 6.0f;
            p.v[(size_t) I (k, inst, ep::midFreq)] = logRand (r, 300.0f, 4000.0f);
            p.v[(size_t) I (k, inst, ep::highGain)] = (r.nextFloat() * 2 - 1) * 5.0f;
            break;
        default: break;
    }
}

// ---- structural grid edits
bool gridOp (Patch& p, uint32_t locks, float st, juce::Random& r)
{
    const auto& t = T();
    const auto& ge = gridEntries();
    GridMap gm; gm.build (p.v.data());
    auto slotE = [&] (int l, int s) { const int e = ch (p, t.slot[l][s]); return (e > 0 && gm.entryLane[e] == l && gm.entrySlot[e] == s) ? e : 0; };

    struct Op { int type, lane, a, b; };
    std::vector<Op> ops;
    for (int l = 0; l < kNumLanes; ++l)
    {
        if (laneLocked (locks, l) || ! gm.laneActive[l]) continue;
        std::vector<int> filled;
        int lastSrc = -1, nProc = 0;
        for (int s = 0; s < kNumSlots; ++s)
            if (const int e = slotE (l, s))
            {
                filled.push_back (s);
                if (isSourceKind (ge[(size_t) e].kind)) lastSrc = s; else ++nProc;
            }
        // 0: swap two neighbouring processors
        for (size_t k = 0; k + 1 < filled.size(); ++k)
            if (! isSourceKind (ge[(size_t) slotE (l, filled[k])].kind) && ! isSourceKind (ge[(size_t) slotE (l, filled[k + 1])].kind))
                for (int w = 0; w < 3; ++w) ops.push_back ({ 0, l, filled[k], filled[k + 1] });
        // 1: insert a processor below the last source
        const bool hasSource = lastSrc >= 0 || ch (p, t.laneIn[l]) > 0;
        if (hasSource && nProc < 3)
            for (int s = lastSrc + 1; s < kNumSlots; ++s)
                if (ch (p, t.slot[l][s]) == 0) { for (int w = 0; w < 3; ++w) ops.push_back ({ 1, l, s, 0 }); break; }
        // 2: remove a processor (only when there are two or more)
        if (nProc >= 2 && st > 0.35f)
            for (int s : filled)
                if (! isSourceKind (ge[(size_t) slotE (l, s)].kind)) ops.push_back ({ 2, l, s, 0 });
        // 3: lane input on / off
        if (l > 0 && nProc > 0 && st > 0.45f && ! locked (locks, kLockGrid)) ops.push_back ({ 3, l, 0, 0 });
    }
    if (ops.empty()) return false;
    const auto op = pick (ops, r);
    switch (op.type)
    {
        case 0:
        {
            const float a = p.v[(size_t) t.slot[op.lane][op.a]];
            p.v[(size_t) t.slot[op.lane][op.a]] = p.v[(size_t) t.slot[op.lane][op.b]];
            p.v[(size_t) t.slot[op.lane][op.b]] = a;
            return true;
        }
        case 1:
        {
            std::vector<std::pair<Kind, int>> kinds { { Kind::Vcf, 3 }, { Kind::Shape, 3 }, { Kind::Comb, 1 }, { Kind::Ring, 1 }, { Kind::Eq, 1 } };
            std::vector<Kind> bag;
            for (auto& [k, w] : kinds) for (int n = 0; n < w; ++n) bag.push_back (k);
            for (int attempt = 0; attempt < 6; ++attempt)
            {
                const Kind k = pick (bag, r);
                const int inst = freeInstance (p, k, r);
                if (inst < 0) continue;
                initInserted (p, k, inst, r);
                p.v[(size_t) t.slot[op.lane][op.a]] = (float) gridEntryOf (k, inst);
                return true;
            }
            return false;
        }
        case 2: p.v[(size_t) t.slot[op.lane][op.a]] = 0.0f; return true;
        case 3:
        {
            const int in = ch (p, t.laneIn[op.lane]);
            if (in > 0) { p.v[(size_t) t.laneIn[op.lane]] = 0.0f; return true; }
            std::vector<int> src;
            for (int j = 0; j < op.lane; ++j) if (gm.laneActive[j]) src.push_back (j);
            if (src.empty()) return false;
            p.v[(size_t) t.laneIn[op.lane]] = (float) (pick (src, r) + 1);
            p.v[(size_t) t.laneInLvl[op.lane]] = 0.5f + 0.5f * r.nextFloat();
            return true;
        }
        default: return false;
    }
}

// ---- matrix helpers
bool isMacroSrc (int s) { return s >= sMacroA && s <= sMacroD; }

std::vector<int> destCandidates (const Patch& p, bool allowGlobal)
{
    const auto& defs = params();
    const auto& dp = destParams();
    GridMap gm; gm.build (p.v.data());
    std::vector<int> out;
    for (int d = 1; d <= (int) dp.size(); ++d)
    {
        const int i = dp[(size_t) d - 1];
        const auto& def = defs[(size_t) i];
        if (def.caption == "Tune" || def.caption == "Fine") continue;
        if (def.global && ! allowGlobal) continue;
        const auto& pc = paramClasses()[(size_t) i];
        if (pc.group == PGroup::GenModule && gm.entryLane[gridEntryOf (def.kind, def.inst)] < 0) continue;
        if (pc.group == PGroup::FxModule && gm.fxBus[fxEntryOf (def.kind, def.inst)] < 0) continue;
        if (pc.group == PGroup::LaneMix && ! gm.laneActive[pc.lane]) continue;
        if (pc.group == PGroup::AuxRet) continue;
        out.push_back (d);
    }
    return out;
}

bool srcIsGlobalFor (const Patch& p, int s)
{
    if (s >= sLfo1 && s < sLfo1 + 4) return ch (p, I (Kind::Lfo, s - sLfo1, lfop::mode)) == 2;
    return sourceIsGlobal (s);
}

int randomSource (const Patch& p, juce::Random& r)
{
    std::vector<int> bag;
    for (int i = 0; i < 4; ++i) if (p.v[(size_t) I (Kind::Lfo, i, lfop::amount)] > 0.05f) for (int w = 0; w < 2; ++w) bag.push_back (sLfo1 + i);
    for (int i = 1; i < 4; ++i) bag.push_back (sEnv1 + i);
    for (int s : { (int) sMseg1, (int) sVelocity, (int) sVelocity, (int) sModWheel, (int) sRandom, (int) sKey, (int) sAlternate, (int) sAftertouch })
        bag.push_back (s);
    return pick (bag, r);
}

bool matrixOps (Patch& p, float st, juce::Random& r)
{
    const auto& t = T();
    bool changed = false;
    std::vector<int> active, empty;
    for (int i = 0; i < kNumMatrix; ++i)
    {
        if (ch (p, t.mtxSrc[i]) > 0 && ch (p, t.mtxDst[i]) > 0) active.push_back (i);
        else empty.push_back (i);
    }
    std::vector<int> free;      // active rows Mutate may rewire (macro routings are kept)
    for (int i : active) if (! isMacroSrc (ch (p, t.mtxSrc[i]))) free.push_back (i);

    if (! empty.empty() && r.nextFloat() < 0.45f * st)
    {
        const int row = empty.front();
        const int src = randomSource (p, r);
        const auto cands = destCandidates (p, srcIsGlobalFor (p, src));
        if (! cands.empty())
        {
            p.v[(size_t) t.mtxSrc[row]] = (float) src;
            p.v[(size_t) t.mtxVia[row]] = 0.0f;
            p.v[(size_t) t.mtxDst[row]] = (float) pick (cands, r);
            p.v[(size_t) t.mtxAmt[row]] = (r.nextBool() ? 1.0f : -1.0f) * (0.05f + 0.35f * st * r.nextFloat());
            changed = true;
        }
    }
    if (! free.empty() && r.nextFloat() < 0.25f * st)
    {
        const int row = pick (free, r);
        const auto cands = destCandidates (p, srcIsGlobalFor (p, ch (p, t.mtxSrc[row])));
        // prefer another parameter of the same module
        const auto& defs = params();
        const auto& cur = defs[(size_t) destParams()[(size_t) ch (p, t.mtxDst[row]) - 1]];
        std::vector<int> same;
        for (int d : cands)
        {
            const auto& def = defs[(size_t) destParams()[(size_t) d - 1]];
            if (def.kind == cur.kind && def.inst == cur.inst && def.kind != Kind::Count) same.push_back (d);
        }
        const auto& from = same.size() > 1 ? same : cands;
        if (! from.empty()) { p.v[(size_t) t.mtxDst[row]] = (float) pick (from, r); changed = true; }
    }
    if (! free.empty() && r.nextFloat() < 0.2f * st)
    {
        const int row = pick (free, r);
        const int d = ch (p, t.mtxDst[row]);
        const bool needGlobal = params()[(size_t) destParams()[(size_t) d - 1]].global;
        for (int attempt = 0; attempt < 6; ++attempt)
        {
            const int s = randomSource (p, r);
            if (needGlobal && ! srcIsGlobalFor (p, s)) continue;
            p.v[(size_t) t.mtxSrc[row]] = (float) s;
            changed = true;
            break;
        }
    }
    if (! free.empty() && r.nextFloat() < 0.08f * st)
    {
        const int row = pick (free, r);
        const int vias[] { 0, 0, (int) sModWheel, (int) sVelocity };
        p.v[(size_t) t.mtxVia[row]] = (float) vias[r.nextInt (4)];
        changed = true;
    }
    if (free.size() > 1 && r.nextFloat() < 0.1f * st)
    {
        const int row = pick (free, r);
        p.v[(size_t) t.mtxSrc[row]] = p.v[(size_t) t.mtxVia[row]] = p.v[(size_t) t.mtxDst[row]] = p.v[(size_t) t.mtxAmt[row]] = 0.0f;
        changed = true;
    }
    return changed;
}

void fxOps (Patch& p, float st, juce::Random& r)
{
    if (r.nextFloat() >= 0.15f * st) return;
    const auto& t = T();
    std::vector<std::pair<int, int>> pairs;
    for (int b = 0; b < kNumBuses; ++b)
    {
        std::vector<int> filled;
        for (int s = 0; s < kFxSlots; ++s) if (ch (p, t.fxSlot[b][s]) > 0) filled.push_back (s);
        for (size_t k = 0; k + 1 < filled.size(); ++k) pairs.push_back ({ t.fxSlot[b][filled[k]], t.fxSlot[b][filled[k + 1]] });
    }
    if (pairs.empty()) return;
    const auto pr = pick (pairs, r);
    std::swap (p.v[(size_t) pr.first], p.v[(size_t) pr.second]);
}

// ---- one parameter
int choiceGroup (Kind k, int local, int value)
{
    if (k == Kind::Vcf && local == vp::response) return value <= 3 ? 0 : value <= 5 ? 1 : value <= 8 ? 2 : 3;
    return 0;
}

bool isDivision (const ParamDef& d) { return d.choices.size() == 16 && d.choices[0] == "4 bars"; }

void mutateOne (Patch& p, int i, float st, juce::Random& r, bool arpOn)
{
    const auto& d = params()[(size_t) i];
    const auto& pc = paramClasses()[(size_t) i];
    float& v = p.v[(size_t) i];

    // musical-only parameters handled first (they are otherwise protected)
    if (d.kind == Kind::FM && d.local == fmp::ratio)
    {
        if (r.nextFloat() < 0.3f * st) { const float rs[] { 0.5f, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8 }; v = rs[r.nextInt (11)]; }
        return;
    }
    if (d.kind == Kind::Ring && d.local == gp::tune)
    {
        if (r.nextFloat() < 0.3f * st) { const float iv[] { -12, -5, 0, 3, 4, 5, 7, 7, 12, 12, 19, 24 }; v = iv[r.nextInt (12)]; }
        return;
    }
    if (pc.group == PGroup::ArpStep && d.id.endsWith ("_tr"))
    {
        if (arpOn && r.nextFloat() < 0.2f * st) { const float iv[] { -12, -5, 0, 0, 0, 7, 12, 12 }; v = iv[r.nextInt (8)]; }
        return;
    }
    if (pc.protect) return;
    if ((pc.group == PGroup::ArpStep || pc.group == PGroup::ArpGlobal) && ! arpOn) return;

    if (d.type == ParamDef::Bool)
    {
        if (pc.group == PGroup::ArpStep)
        {
            if (d.id.startsWith ("arp_s1_")) return;            // the first step always plays
            if (r.nextFloat() < 0.12f * st) v = v > 0.5f ? 0.0f : 1.0f;
            return;
        }
        if (r.nextFloat() < 0.05f * st) v = v > 0.5f ? 0.0f : 1.0f;
        return;
    }
    if (d.type == ParamDef::Choice)
    {
        float prob = 0.10f * st;
        if (d.kind == Kind::Osc && d.local == op::engine) prob *= 0.4f;
        if (d.kind == Kind::Osc && d.local == op::table) prob *= 1.6f;
        if (d.kind == Kind::Vcf && d.local == vp::response) prob *= 0.5f;
        if (d.kind == Kind::Exc && d.local == xp::mode) prob *= 0.3f;
        if (d.kind == Kind::Map && d.local == mpp::mode) prob *= 0.3f;
        if (d.kind == Kind::Math) prob *= 0.5f;
        if (d.kind == Kind::FxFilter && d.local == ffp::type) prob *= 0.15f;
        if (d.kind == Kind::Ring && d.local == gp::source) prob *= 0.4f;
        if (r.nextFloat() >= prob) return;
        const int n = d.choices.size(), cur = juce::roundToInt (v);
        if (n < 2) return;
        if (isDivision (d)) { v = (float) juce::jlimit (0, n - 1, cur + (r.nextBool() ? 1 : -1) * (1 + r.nextInt (2))); return; }
        if (d.kind == Kind::Vcf && d.local == vp::response)
        {
            const int g = choiceGroup (d.kind, d.local, cur);
            const int nv = juce::jlimit (0, n - 1, cur + (r.nextBool() ? 1 : -1));
            if (choiceGroup (d.kind, d.local, nv) == g) v = (float) nv;
            return;
        }
        int nv = r.nextInt (n - 1);
        if (nv >= cur) ++nv;
        v = (float) nv;
        return;
    }
    // ---- floats
    if (d.unit == "int")
    {
        if (r.nextFloat() >= 0.2f * st) return;
        float nv = v + (float) ((r.nextBool() ? 1 : -1) * (1 + r.nextInt (2)));
        float hi = d.max;
        if (d.kind == Kind::Osc && d.local == op::uniVoices) hi = juce::jmax (v, 8.0f);
        if (d.id == id::arpSteps) nv = juce::jlimit (3.0f, 16.0f, nv);
        v = juce::jlimit (d.min, hi, nv);
        return;
    }
    if (r.nextFloat() >= 0.35f + 0.6f * st) return;
    float scale = 1.0f;
    if (pc.group == PGroup::LaneMix && d.id.endsWith ("_vol")) scale = 0.4f;
    if (pc.group == PGroup::MtxAmt || pc.group == PGroup::XYAmt) scale = 0.6f;
    if (pc.group == PGroup::AuxRet) scale = 0.4f;
    if (d.kind == Kind::Env && d.inst == 0) scale = 0.6f;
    const float sigma = 0.03f + 0.32f * std::pow (st, 1.2f);
    p.setNorm (i, p.norm (i) + gauss (r) * sigma * scale);
}

// keep the patch playable after a mutation (orig = before)
void safety (Patch& p, const Patch& o)
{
    auto clampMax = [&] (int i, float hi) { p.v[(size_t) i] = juce::jmin (p.v[(size_t) i], juce::jmax (hi, o.v[(size_t) i])); };
    auto keepMin = [&] (int i, float lo) { if (o.v[(size_t) i] >= lo) p.v[(size_t) i] = juce::jmax (p.v[(size_t) i], lo); };
    for (int k = 0; k < 4; ++k) { keepMin (I (Kind::Osc, k, op::vol), 0.2f); keepMin (I (Kind::FM, k, fmp::vol), 0.2f); }
    for (int k = 0; k < 2; ++k)
    {
        keepMin (I (Kind::Noise, k, np::vol), 0.1f);
        keepMin (I (Kind::Comb, k, cp::vol), 0.2f);
        keepMin (I (Kind::Exc, k, xp::level), 0.3f);
        clampMax (I (Kind::Res, k, rp::decay), 6.0f);
        const int cf = I (Kind::Comb, k, cp::feedback);
        p.v[(size_t) cf] = juce::jlimit (-juce::jmax (0.97f, std::abs (o.v[(size_t) cf])), juce::jmax (0.97f, std::abs (o.v[(size_t) cf])), p.v[(size_t) cf]);
        clampMax (I (Kind::Delay, k, dly::feedback), 0.85f);
        clampMax (I (Kind::Texture, k, txp::feedback), 0.85f);
        clampMax (I (Kind::Reverb, k, rvb::decay), juce::jmax (8.0f, o.v[(size_t) I (Kind::Reverb, k, rvb::decay)] * 1.5f));
        const int fc = I (Kind::FxFilter, k, ffp::cutoff);
        if (juce::roundToInt (p.v[(size_t) I (Kind::FxFilter, k, ffp::type)]) == 0) keepMin (fc, juce::jmin (1500.0f, o.v[(size_t) fc]));
    }
    for (int k = 0; k < 4; ++k)
    {
        const int cut = I (Kind::Vcf, k, vp::cutoff);
        if (juce::roundToInt (p.v[(size_t) I (Kind::Vcf, k, vp::response)]) <= 3) keepMin (cut, 110.0f);
        clampMax (I (Kind::Shape, k, sp::out), 6.0f);
    }
    // amp envelope
    const int att = I (Kind::Env, 0, envp::attack), dec = I (Kind::Env, 0, envp::decay), sus = I (Kind::Env, 0, envp::sustain),
              rel = I (Kind::Env, 0, envp::release), del = I (Kind::Env, 0, envp::delay);
    clampMax (att, 0.6f);
    clampMax (rel, 5.0f);
    clampMax (del, 0.05f);
    if (p.v[(size_t) sus] < 0.05f && p.v[(size_t) dec] < 0.12f) p.v[(size_t) dec] = juce::jmax (0.12f, o.v[(size_t) dec]);
    const auto& t = T();
    for (int l = 0; l < kNumLanes; ++l) keepMin (t.laneVol[l], -30.0f);
}
} // namespace

// ======================================================================= Mutate
void mutatePatch (Patch& p, const MutateOptions& o, juce::Random& r)
{
    const float st = juce::jlimit (0.02f, 1.0f, o.strength);
    const Patch orig = p;
    const auto& defs = params();
    const auto& pcs = paramClasses();
    const auto& t = T();

    if (! o.moduleOnly)
    {
        if (! locked (o.locks, kLockGrid))
        {
            int nOps = r.nextFloat() < st * 0.8f ? 1 : 0;
            if (st > 0.6f && r.nextFloat() < st - 0.5f) ++nOps;
            for (int k = 0; k < nOps; ++k) gridOp (p, o.locks, st, r);
        }
        if (! locked (o.locks, kLockMatrix)) matrixOps (p, st, r);
        if (! locked (o.locks, kLockFx)) fxOps (p, st, r);
    }

    GridMap gm; gm.build (p.v.data());
    const bool arpOn = p.v[(size_t) t.arpOn] > 0.5f;
    for (int i = 0; i < (int) defs.size(); ++i)
    {
        const auto& d = defs[(size_t) i];
        const auto g = pcs[(size_t) i].group;
        if (o.moduleOnly)
        {
            if (d.kind != o.kind || d.inst != o.inst) continue;
        }
        else
        {
            const int bit = lockBitOf (i, gm);
            if (bit < 0 || locked (o.locks, bit)) continue;
        }
        if (g == PGroup::Slot || g == PGroup::LaneIn || g == PGroup::FxSlot || g == PGroup::MtxSrc || g == PGroup::MtxVia || g == PGroup::MtxDst
            || g == PGroup::XYDst || g == PGroup::XYPos || g == PGroup::Global || g == PGroup::Macro || g == PGroup::Perform)
            continue;
        if (g == PGroup::MtxAmt)
        {
            int row = -1;
            for (int k = 0; k < kNumMatrix; ++k) if (t.mtxAmt[k] == i) { row = k; break; }
            if (row < 0 || juce::roundToInt (p.v[(size_t) t.mtxSrc[row]]) <= 0 || juce::roundToInt (p.v[(size_t) t.mtxDst[row]]) <= 0) continue;
        }
        if (g == PGroup::XYAmt && juce::roundToInt (p.v[(size_t) (i - 1)]) <= 0) continue;   // the target sits just before its amount
        mutateOne (p, i, st, r, arpOn);
    }
    safety (p, orig);
}

// ======================================================================= Breed
namespace
{
struct BreedCtx
{
    const Patch* par[2];
    GridMap gm[2];
    int remap[2][64];       // per parent: grid entry -> child entry (0 = not placed)
    float t;                // blend towards the other parent
    juce::Random* r;
};

// blend a float param of the child towards value y (normalised)
void blendTo (Patch& c, int i, float y, float t)
{
    const auto& d = params()[(size_t) i];
    if (d.type != ParamDef::Float || isPerformParam (i)) return;
    const auto rg = d.range();
    const float a = rg.convertTo0to1 (c.v[(size_t) i]), b = rg.convertTo0to1 (y);
    c.setNorm (i, a + t * (b - a));
}

void copyModule (Patch& c, Kind k, int dstInst, const Patch& src, int srcInst)
{
    for (int l = 0; l < localCount (k); ++l) c.v[(size_t) pidx (k, dstInst, l)] = src.v[(size_t) pidx (k, srcInst, l)];
}

void blendModule (Patch& c, Kind k, int dstInst, const Patch& other, int otherInst, float t)
{
    for (int l = 0; l < localCount (k); ++l) blendTo (c, pidx (k, dstInst, l), other.v[(size_t) pidx (k, otherInst, l)], t);
}

// remap a matrix / XY destination taken from parent x
int remapDest (int d, const BreedCtx& b, int x)
{
    if (d <= 0) return d;
    const auto& dp = destParams();
    if (d > (int) dp.size()) return 0;
    const auto& def = params()[(size_t) dp[(size_t) d - 1]];
    if (def.kind == Kind::Count || ! ((int) def.kind <= (int) Kind::Eq)) return d;
    const int e = gridEntryOf (def.kind, def.inst);
    const int ne = b.remap[x][e];
    if (ne <= 0 || ne == e) return d;
    const auto& g = gridEntries()[(size_t) ne];
    const int np = destOfParam (pidx (g.kind, g.inst, def.local));
    return np > 0 ? np : d;
}
}

Patch breedPatches (const Patch& cur, const Patch& oth, uint32_t locks, juce::Random& r)
{
    Patch c = cur;
    const auto& t = T();
    const auto& ge = gridEntries();
    BreedCtx b;
    b.par[0] = &cur; b.par[1] = &oth;
    b.gm[0].build (cur.v.data());
    b.gm[1].build (oth.v.data());
    b.t = 0.1f + 0.35f * r.nextFloat();
    b.r = &r;
    for (int x = 0; x < 2; ++x) for (int e = 0; e < 64; ++e) b.remap[x][e] = e;
    auto other = [] (int x) { return 1 - x; };
    int lane1Parent = 0;

    // ---- grid and lanes
    if (locked (locks, kLockGrid))
    {
        for (int e = 1; e < (int) ge.size(); ++e)
        {
            const int lane = b.gm[0].entryLane[e];
            if (lane < 0 || laneLocked (locks, lane) || b.gm[1].entryLane[e] < 0) continue;
            const int x = r.nextInt (2);
            const auto& g = ge[(size_t) e];
            copyModule (c, g.kind, g.inst, *b.par[x], g.inst);
            blendModule (c, g.kind, g.inst, *b.par[other (x)], g.inst, b.t);
        }
        for (int l = 0; l < kNumLanes; ++l)
        {
            if (laneLocked (locks, l) || ! b.gm[1].laneActive[l] || ! b.gm[0].laneActive[l]) continue;
            const int x = r.nextInt (2);
            if (l == 0) lane1Parent = x;
            for (int i : { t.laneVol[l], t.lanePan[l], t.laneSend[l][0], t.laneSend[l][1], t.laneSend[l][2], t.laneInLvl[l] })
            {
                c.v[(size_t) i] = b.par[x]->v[(size_t) i];
                blendTo (c, i, b.par[other (x)]->v[(size_t) i], b.t);
            }
        }
    }
    else
    {
        bool used[64] {};
        for (int x = 0; x < 2; ++x) for (int e = 0; e < 64; ++e) b.remap[x][e] = 0;
        for (int l = 0; l < kNumLanes; ++l)
            for (int s = 0; s < kNumSlots; ++s) c.v[(size_t) t.slot[l][s]] = 0.0f;
        // locked lanes keep the current modules
        for (int l = 0; l < kNumLanes; ++l)
        {
            if (! laneLocked (locks, l)) continue;
            for (int s = 0; s < kNumSlots; ++s)
            {
                const int e = juce::roundToInt (cur.v[(size_t) t.slot[l][s]]);
                c.v[(size_t) t.slot[l][s]] = (float) e;
                if (e > 0 && b.gm[0].entryLane[e] == l) { used[e] = true; b.remap[0][e] = e; }
            }
        }
        for (int l = 0; l < kNumLanes; ++l)
        {
            if (laneLocked (locks, l)) continue;
            int x = r.nextInt (2);
            if (! b.gm[x].laneActive[l] && b.gm[other (x)].laneActive[l] && r.nextFloat() < 0.7f) x = other (x);
            if (l == 0) lane1Parent = x;
            const auto& P = *b.par[x];
            const auto& Y = *b.par[other (x)];
            for (int s = 0; s < kNumSlots; ++s)
            {
                const int e = juce::roundToInt (P.v[(size_t) t.slot[l][s]]);
                if (e <= 0 || b.gm[x].entryLane[e] != l || b.gm[x].entrySlot[e] != s) continue;
                const auto& g = ge[(size_t) e];
                int ne = 0;
                if (! used[e]) ne = e;
                else
                    for (int i = 0; i < kindCount (g.kind); ++i)
                    {
                        const int cand = gridEntryOf (g.kind, i);
                        if (! used[cand]) { ne = cand; break; }
                    }
                if (ne == 0) continue;
                used[ne] = true;
                b.remap[x][e] = ne;
                const int ni = ge[(size_t) ne].inst;
                c.v[(size_t) t.slot[l][s]] = (float) ne;
                copyModule (c, g.kind, ni, P, g.inst);
                if (b.gm[other (x)].entryLane[e] >= 0) blendModule (c, g.kind, ni, Y, g.inst, b.t);
            }
            for (int i : { t.laneVol[l], t.lanePan[l], t.laneSend[l][0], t.laneSend[l][1], t.laneSend[l][2], t.laneIn[l], t.laneInLvl[l] })
            {
                c.v[(size_t) i] = P.v[(size_t) i];
                if (b.gm[other (x)].laneActive[l]) blendTo (c, i, Y.v[(size_t) i], b.t);
            }
        }
        GridMap gc; gc.build (c.v.data());
        bool any = false;
        for (int l = 0; l < kNumLanes; ++l) any = any || gc.laneActive[l];
        if (! any) return cur;          // nothing survived: keep the current sound
        // modules of the current sound that were not placed keep their identity mapping for the matrix
        for (int e = 0; e < 64; ++e) for (int x = 0; x < 2; ++x) if (b.remap[x][e] == 0) b.remap[x][e] = e;
    }

    // ---- modulators (one parent per modulator, blended)
    if (! locked (locks, kLockMod))
        for (auto k : { Kind::Env, Kind::Lfo, Kind::Mseg, Kind::Map, Kind::Math })
            for (int i = 0; i < 4; ++i)
            {
                const int x = r.nextInt (2);
                copyModule (c, k, i, *b.par[x], i);
                blendModule (c, k, i, *b.par[other (x)], i, b.t);
                if (k == Kind::Mseg) c.mseg[i] = b.par[x]->mseg[i];
                if (k == Kind::Map) c.map[i] = b.par[x]->map[i];
            }

    // ---- matrix (and the macro knobs that go with it)
    if (! locked (locks, kLockMatrix))
    {
        const int x = r.nextInt (2);
        const auto& P = *b.par[x];
        const auto& Y = *b.par[other (x)];
        for (int i = 0; i < kNumMatrix; ++i)
        {
            c.v[(size_t) t.mtxSrc[i]] = P.v[(size_t) t.mtxSrc[i]];
            c.v[(size_t) t.mtxVia[i]] = P.v[(size_t) t.mtxVia[i]];
            c.v[(size_t) t.mtxDst[i]] = (float) remapDest (juce::roundToInt (P.v[(size_t) t.mtxDst[i]]), b, x);
            c.v[(size_t) t.mtxAmt[i]] = P.v[(size_t) t.mtxAmt[i]];
            // same routing in the other parent: blend its depth
            for (int j = 0; j < kNumMatrix; ++j)
                if (Y.v[(size_t) t.mtxSrc[j]] == P.v[(size_t) t.mtxSrc[i]] && P.v[(size_t) t.mtxSrc[i]] > 0.5f
                    && remapDest (juce::roundToInt (Y.v[(size_t) t.mtxDst[j]]), b, other (x)) == juce::roundToInt (c.v[(size_t) t.mtxDst[i]]))
                {
                    blendTo (c, t.mtxAmt[i], Y.v[(size_t) t.mtxAmt[j]], b.t);
                    break;
                }
        }
        // inherit a few routings from the other parent
        for (int j = 0; j < kNumMatrix; ++j)
        {
            const int s = juce::roundToInt (Y.v[(size_t) t.mtxSrc[j]]), d = juce::roundToInt (Y.v[(size_t) t.mtxDst[j]]);
            if (s <= 0 || d <= 0 || isMacroSrc (s) || r.nextFloat() > 0.35f) continue;
            for (int i = 0; i < kNumMatrix; ++i)
                if (juce::roundToInt (c.v[(size_t) t.mtxSrc[i]]) <= 0 || juce::roundToInt (c.v[(size_t) t.mtxDst[i]]) <= 0)
                {
                    c.v[(size_t) t.mtxSrc[i]] = (float) s;
                    c.v[(size_t) t.mtxVia[i]] = Y.v[(size_t) t.mtxVia[j]];
                    c.v[(size_t) t.mtxDst[i]] = (float) remapDest (d, b, other (x));
                    c.v[(size_t) t.mtxAmt[i]] = Y.v[(size_t) t.mtxAmt[j]] * (0.5f + 0.5f * r.nextFloat());
                    break;
                }
        }
        for (int m = 0; m < 4; ++m) c.v[(size_t) t.macro[m]] = P.v[(size_t) t.macro[m]];
    }

    // ---- effects
    if (! locked (locks, kLockFx))
    {
        const int x = r.nextInt (2);
        const auto& P = *b.par[x];
        const auto& Y = *b.par[other (x)];
        const auto& fe = fxEntries();
        for (int bus = 0; bus < kNumBuses; ++bus)
            for (int s = 0; s < kFxSlots; ++s) c.v[(size_t) t.fxSlot[bus][s]] = P.v[(size_t) t.fxSlot[bus][s]];
        for (int e = 1; e < (int) fe.size(); ++e)
        {
            if (b.gm[x].fxBus[e] < 0) continue;
            const auto& g = fe[(size_t) e];
            copyModule (c, g.kind, g.inst, P, g.inst);
            if (b.gm[other (x)].fxBus[e] >= 0) blendModule (c, g.kind, g.inst, Y, g.inst, b.t);
        }
        for (int a = 0; a < 3; ++a) { c.v[(size_t) t.auxRet[a]] = P.v[(size_t) t.auxRet[a]]; blendTo (c, t.auxRet[a], Y.v[(size_t) t.auxRet[a]], b.t); }
    }

    // ---- arpeggiator
    if (! locked (locks, kLockArp))
    {
        const int x = r.nextInt (2);
        const auto& defs = params();
        for (int i = 0; i < (int) defs.size(); ++i)
        {
            const auto g = paramClasses()[(size_t) i].group;
            if (g == PGroup::ArpGlobal || g == PGroup::ArpStep) c.v[(size_t) i] = b.par[x]->v[(size_t) i];
        }
    }

    // ---- XY pads
    if (! locked (locks, kLockXY))
    {
        const int x = r.nextInt (2);
        const auto& P = *b.par[x];
        for (int n = 0; n < kNumXY; ++n)
            for (int a = 0; a < 2; ++a)
            {
                c.v[(size_t) t.xyPos[n][a]] = P.v[(size_t) t.xyPos[n][a]];
                for (int k = 0; k < kXYTargets; ++k)
                {
                    c.v[(size_t) t.xyDst[n][a][k]] = (float) remapDest (juce::roundToInt (P.v[(size_t) t.xyDst[n][a][k]]), b, x);
                    c.v[(size_t) t.xyAmt[n][a][k]] = P.v[(size_t) t.xyAmt[n][a][k]];
                }
            }
    }

    // ---- voice character follows lane 1
    if (! locked (locks, kLockLane1))
    {
        const auto& P = *b.par[lane1Parent];
        for (int i : { t.vmode, t.glide, t.glideMode }) c.v[(size_t) i] = P.v[(size_t) i];
    }
    return c;
}

// ======================================================================= names
juce::String stripMarker (const juce::String& n)
{
    auto s = n.trim();
    const int k = s.lastIndexOf (" ~");
    if (k > 0 && s.substring (k + 2).containsOnly ("0123456789")) s = s.substring (0, k).trim();
    return s;
}

juce::String mutatedName (const juce::String& n)
{
    auto s = n.trim();
    const int k = s.lastIndexOf (" ~");
    int count = 0;
    if (k > 0 && s.substring (k + 2).containsOnly ("0123456789") && s.substring (k + 2).isNotEmpty())
    {
        count = s.substring (k + 2).getIntValue();
        s = s.substring (0, k).trim();
    }
    if (s.isEmpty()) s = "Init";
    return s + " ~" + juce::String (count + 1);
}

juce::String bredName (const juce::String& a, const juce::String& b)
{
    const auto A = stripMarker (a), B = stripMarker (b);
    juce::StringArray wa, wb;
    wa.addTokens (A, " ", ""); wa.removeEmptyStrings();
    wb.addTokens (B, " ", ""); wb.removeEmptyStrings();
    if (wa.isEmpty()) return B;
    if (wb.isEmpty()) return A;
    // skip small linking words at the end of B ("Reverie of the Void" -> "Void")
    juce::String last = wb[wb.size() - 1];
    juce::String first = wa[0];
    if (first.equalsIgnoreCase ("The") && wa.size() > 1) first = wa[1];
    auto out = first + " " + last;
    if (out.equalsIgnoreCase (A) || out.equalsIgnoreCase (B) || first.equalsIgnoreCase (last))
        out = (wb.size() > 1 ? wb[0] : B) + " " + wa[wa.size() - 1];
    if (out.equalsIgnoreCase (A) || out.equalsIgnoreCase (B)) out = A + " II";
    return out;
}
}
