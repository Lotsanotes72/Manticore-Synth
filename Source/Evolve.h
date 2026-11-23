#pragma once
#include <JuceHeader.h>
#include "Params.h"
#include "dsp/Common.h"

// Mutate, Breed and Haunt support: parameter classification, padlocks and patch-level operations.
// Everything here works on a plain value snapshot of the synth (a Patch), never on the live parameters,
// so the processor can try a candidate, measure it offline and only then apply it.
namespace mc
{
// ---- padlocks (bit numbers)
enum LockBit
{
    kLockGrid = 0,          // module placement and lane inputs
    kLockLane1 = 1,         // lanes 1-4: the modules in the lane and its mixer settings (bits 1..4)
    kLockMod = 5,           // ENV, LFO, MSEG, MAP, MATH
    kLockMatrix = 6,        // the 24 matrix slots
    kLockFx = 7,            // effect placement, effect settings and aux returns
    kLockArp = 8,
    kLockXY = 9,            // XY pad assignments
    kNumLocks = 10
};
juce::String lockName (int bit);
inline bool locked (uint32_t locks, int bit) { return ((locks >> bit) & 1u) != 0; }

enum class PGroup : uint8_t
{
    None, GenModule, LaneMix, LaneIn, Slot, ModModule, MtxSrc, MtxVia, MtxDst, MtxAmt, FxModule, FxSlot, AuxRet,
    ArpGlobal, ArpStep, XYPos, XYDst, XYAmt, Global, Macro, Perform
};

struct ParamClass
{
    PGroup group = PGroup::None;
    int lane = -1;              // lane params
    bool protect = false;       // Mutate never changes it (pitch, key tracking, freeze, ...)
    bool haunt = false;         // Haunt may drift it
    float hauntScale = 1.0f;    // relative drift depth
};
const std::vector<ParamClass>& paramClasses();
inline bool isPerformParam (int i) { return paramClasses()[(size_t) i].group == PGroup::Perform; }

// Which modules are active (first use in the grid wins, as in the engine).
struct GridMap
{
    int entryLane[64];          // grid entry -> lane, -1 when not placed
    int entrySlot[64];
    int fxBus[64];              // fx entry -> bus, -1 when not placed
    bool laneActive[kNumLanes];
    void build (const float* values);   // values indexed by catalogue index (real units)
};
int gridEntryOf (Kind, int inst);       // index in gridEntries(), 0 if none
int fxEntryOf (Kind, int inst);         // index in fxEntries(), 0 if none

// Lock bit that governs parameter i, or -1 when it belongs to no active, lockable section.
int lockBitOf (int paramIndex, const GridMap&);

// ---- patch snapshot
struct Patch
{
    std::vector<float> v;       // real values, catalogue order
    dsp::MsegData mseg[4];
    dsp::MapData map[4];
    float norm (int i) const;
    void setNorm (int i, float n);
    bool sameAs (const Patch& o) const;
    uint64_t hash() const;
};
Patch defaultPatch();

struct MutateOptions
{
    float strength = 0.35f;     // 0..1
    uint32_t locks = 0;
    bool moduleOnly = false;    // only the module below, ignoring the padlocks
    Kind kind = Kind::Count;
    int inst = -1;
};
void mutatePatch (Patch&, const MutateOptions&, juce::Random&);

// Child of the current sound and the other A/B slot. Locked sections are kept from 'current'.
Patch breedPatches (const Patch& current, const Patch& other, uint32_t locks, juce::Random&);

juce::String stripMarker (const juce::String& presetName);
juce::String mutatedName (const juce::String& presetName);        // "Name ~3"
juce::String bredName (const juce::String& a, const juce::String& b);
}
