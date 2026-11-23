#pragma once
#include <JuceHeader.h>

// Manticore parameter catalogue.
// Every parameter is described once here (id, range, default, caption, tooltip). The DSP, the UI panels,
// the preset loader and the modulation matrix are all driven from this table.
namespace mc
{
constexpr int kNumLanes = 4, kNumSlots = 4;          // voice grid: 4 lanes x 4 slots
constexpr int kNumBuses = 4, kFxSlots = 4;           // FX grid: Main + 3 aux buses, 4 slots each
constexpr int kMaxVoices = 32;
constexpr int kNumMatrix = 24;
constexpr int kArpSteps = 16;
constexpr int kMsegMaxPoints = 16;
constexpr int kMapSteps = 32;
constexpr int kBlock = 32;                            // control-rate block (samples)
constexpr int kNumXY = 4, kXYTargets = 4;             // XY vector mixer: 4 pads, 4 targets per axis
constexpr int kNumXYRows = kNumXY * 2 * kXYTargets;   // 32 XY target rows

enum class Kind
{
    Osc, FM, Noise, Exc, Res, Comb, Vcf, Shape, Ring, Eq,          // voice generator rack (grid modules)
    Env, Lfo, Mseg, Map, Math,                                      // modulation rack
    ModFx, Delay, Reverb, Texture, Comp, FxEq, Dist, FxFilter,      // effects rack
    Count
};
constexpr int kNumKinds = (int) Kind::Count;

struct KindInfo
{
    const char* prefix;   // id prefix: "osc"
    const char* label;    // display: "OSC"
    const char* longName; // "Oscillator"
    int count;            // instances in the pool
};
const KindInfo& kindInfo (Kind);
inline int kindCount (Kind k) { return kindInfo (k).count; }
juce::String moduleName (Kind, int inst);          // "OSC 1"
juce::Colour kindColour (Kind);

// ---- local parameter indices per module kind (order == order in the catalogue)
namespace op  { enum { engine, table, wave, tune, fine, vol, pan, phase, freeRun, keytrack, uniVoices, uniDetune, uniWidth,
                       fx1Type, fx1Amt, fx2Type, fx2Amt, partials, bright, oddEven, stretch, pDecay, count }; }
namespace fmp { enum { tune, fine, ratio, depth, feedback, modShape, inputFm, vol, pan, keytrack, count }; }
namespace np  { enum { type, color, density, width, vol, pan, count }; }
namespace xp  { enum { mode, decay, color, rate, level, width, count }; }
namespace rp  { enum { profile, tune, fine, decay, damping, bright, spread, input, mix, count }; }
namespace cp  { enum { mode, tune, fine, feedback, damping, input, mix, vol, count }; }
namespace vp  { enum { model, response, cutoff, reso, drive, keytrack, spread, mix, count }; }
namespace sp  { enum { mode, drive, bias, tone, mix, out, count }; }
namespace gp  { enum { source, tune, fine, keytrack, mix, count }; }
namespace ep  { enum { lowFreq, lowGain, midFreq, midGain, midQ, highFreq, highGain, count }; }
namespace envp { enum { delay, attack, decay, sustain, release, slope, velocity, keyscale, oneShot, count }; }
namespace lfop { enum { wave, rate, sync, division, phase, delay, fade, mode, unipolar, amount, count }; }
namespace msp { enum { time, sync, loop, bipolar, velocity, amount, count }; }
namespace mpp { enum { input, mode, steps, division, smooth, amount, count }; }
namespace mathp { enum { srcA, srcB, opr, amount, count }; }
namespace mfx { enum { mode, rate, depth, feedback, voices, centre, width, character, mix, count }; }
namespace dly { enum { mode, sync, divL, divR, timeMs, feedback, lowCut, highCut, modDepth, diffusion, style, mix, count }; }
namespace rvb { enum { model, size, decay, predelay, damping, diffusion, modDepth, width, lowCut, mix, count }; }
namespace txp { enum { grain, density, pitch, spread, reverse, feedback, jitter, damping, freeze, mix, count }; }
namespace cmp { enum { threshold, ratio, attack, release, knee, makeup, mix, count }; }
namespace feq { enum { lowFreq, lowGain, m1Freq, m1Gain, m1Q, m2Freq, m2Gain, m2Q, highFreq, highGain, count }; }
namespace dsp_ { enum { mode, drive, tone, bias, mix, out, count }; }
namespace ffp { enum { type, cutoff, reso, drive, mix, count }; }

int localCount (Kind);

struct ParamDef
{
    enum Type { Float, Choice, Bool };
    juce::String id, name, caption, unit, tip;
    Type type = Float;
    float min = 0, max = 1, def = 0, centre = -1e9f, step = 0;   // centre: value placed at the knob's mid-point (skew)
    juce::StringArray choices;
    bool mod = false;               // usable as a modulation destination
    bool global = false;            // effect/global param: only global sources modulate it
    Kind kind = Kind::Count;
    int inst = -1, local = -1;
    juce::NormalisableRange<float> range() const;
    juce::String format (float v) const;
};

const std::vector<ParamDef>& params();
int numParams();
int paramIndex (const juce::String& id);            // -1 when unknown
int moduleBase (Kind, int inst);                    // catalogue index of local param 0
inline int pidx (Kind k, int inst, int local) { return moduleBase (k, inst) + local; }
juce::String pid (Kind k, int inst, int local);
juce::String tooltipFor (const juce::String& id);

// ---- well known global ids
namespace id
{
    juce::String slot (int lane, int slot);         // "slot_l1s1"
    juce::String fxSlot (int bus, int slot);        // "fxslot_b1s1"
    juce::String lane (int lane, const char* what); // "lane1_vol"
    juce::String mtx (int n, const char* what);     // "mtx1_src"
    juce::String arpStep (int n, const char* what); // "arp_s1_tr"
    juce::String auxRet (int aux);                  // "aux1_ret"
    juce::String macro (int m);                     // "macro_a"
    juce::String xyPos (int pad, int axis);         // "xy1_x"
    juce::String xyTarget (int pad, int axis, int k, const char* what);   // "xy1_x1_dst"
    inline const char* hauntOn = "haunt_on";
    inline const char* hauntDepth = "haunt_depth";
    inline const char* hauntSpeed = "haunt_speed";
    inline const char* hauntFollow = "haunt_follow";
    inline const char* master = "master";
    inline const char* voices = "voices";
    inline const char* vmode = "vmode";
    inline const char* glide = "glide";
    inline const char* glideMode = "glidemode";
    inline const char* pbUp = "pbup";
    inline const char* pbDown = "pbdn";
    inline const char* transpose = "transpose";
    inline const char* arpOn = "arp_on";
    inline const char* arpMode = "arp_mode";
    inline const char* arpOct = "arp_oct";
    inline const char* arpDiv = "arp_div";
    inline const char* arpSteps = "arp_steps";
    inline const char* arpSwing = "arp_swing";
    inline const char* arpGate = "arp_gate";
    inline const char* arpHold = "arp_hold";
}

// ---- grid choices: entry 0 = empty, then each voice-module instance in pool order
struct GridEntry { Kind kind; int inst; };
const std::vector<GridEntry>& gridEntries();       // index 0 => {Count,-1}
const std::vector<GridEntry>& fxEntries();
juce::StringArray gridChoiceNames();
juce::StringArray fxChoiceNames();

// ---- modulation sources
enum Src
{
    sNone = 0,
    sEnv1 = 1, sLfo1 = 5, sMseg1 = 9, sMap1 = 13, sMath1 = 17,
    sVelocity = 21, sKey, sModWheel, sAftertouch, sPitchBend, sMacroA, sMacroB, sMacroC, sMacroD,
    sRandom, sGate, sExpression, sAlternate,
    sXY1X, sXY1Y, sXY2X, sXY2Y, sXY3X, sXY3Y, sXY4X, sXY4Y,
    sNumSources
};
juce::StringArray sourceNames();
bool sourceIsGlobal (int src);                      // usable on effects
const std::vector<int>& destParams();               // matrix dest d (1-based) -> catalogue index destParams()[d-1]
int destOfParam (int paramIndex);                   // matrix dest index (1-based) or 0
juce::StringArray destNames();

// ---- choice lists used outside the catalogue
juce::StringArray divisionNames();
double divisionBeats (int index);                   // length in quarter notes
juce::StringArray tableNames();
}
