#include "Params.h"

namespace mc
{
static const KindInfo kInfo[] = {
    { "osc", "OSC", "Oscillator", 4 },       { "fm", "FM", "FM Oscillator", 4 },     { "noise", "NOISE", "Noise", 2 },
    { "exc", "EXC", "Exciter", 2 },          { "res", "RES", "Resonator", 2 },       { "comb", "COMB", "Comb Filter", 2 },
    { "vcf", "VCF", "Filter", 4 },           { "shape", "SHAPE", "Shaper", 4 },      { "ring", "RING", "Ring Modulator", 2 },
    { "eq", "EQ", "Voice EQ", 2 },
    { "env", "ENV", "Envelope", 4 },         { "lfo", "LFO", "LFO", 4 },             { "mseg", "MSEG", "MSEG", 4 },
    { "map", "MAP", "Mapper", 4 },           { "math", "MATH", "Mod Math", 4 },
    { "modfx", "MODFX", "Mod FX", 3 },       { "delay", "DELAY", "Delay", 2 },       { "reverb", "REVERB", "Reverb", 2 },
    { "texture", "TEXTURE", "TextureVerb", 2 }, { "comp", "COMP", "Compressor", 2 }, { "fxeq", "FX EQ", "Master EQ", 2 },
    { "dist", "DIST", "Distortion", 2 },     { "fxfilter", "FILTER", "FX Filter", 2 },
};

const KindInfo& kindInfo (Kind k) { return kInfo[(int) k]; }

juce::String moduleName (Kind k, int inst) { return juce::String (kindInfo (k).label) + " " + juce::String (inst + 1); }

juce::Colour kindColour (Kind k)
{
    switch (k)
    {
        case Kind::Osc:   return juce::Colour (0xff1fd0bf);
        case Kind::FM:    return juce::Colour (0xff52b6ff);
        case Kind::Noise: return juce::Colour (0xffa9b8bd);
        case Kind::Exc:   return juce::Colour (0xffeed46b);
        case Kind::Res:   return juce::Colour (0xffd9a6ff);
        case Kind::Comb:  return juce::Colour (0xffb98cff);
        case Kind::Vcf:   return juce::Colour (0xffffb33f);
        case Kind::Shape: return juce::Colour (0xffff7b47);
        case Kind::Ring:  return juce::Colour (0xffff5c8a);
        case Kind::Eq:    return juce::Colour (0xff86e07c);
        case Kind::Env:   return juce::Colour (0xffffb33f);
        case Kind::Lfo:   return juce::Colour (0xff1fd0bf);
        case Kind::Mseg:  return juce::Colour (0xffd9a6ff);
        case Kind::Map:   return juce::Colour (0xff86e07c);
        case Kind::Math:  return juce::Colour (0xff52b6ff);
        default:          return juce::Colour (0xff1fd0bf);
    }
}

int localCount (Kind k)
{
    switch (k)
    {
        case Kind::Osc: return op::count;     case Kind::FM: return fmp::count;     case Kind::Noise: return np::count;
        case Kind::Exc: return xp::count;     case Kind::Res: return rp::count;     case Kind::Comb: return cp::count;
        case Kind::Vcf: return vp::count;     case Kind::Shape: return sp::count;   case Kind::Ring: return gp::count;
        case Kind::Eq: return ep::count;      case Kind::Env: return envp::count;   case Kind::Lfo: return lfop::count;
        case Kind::Mseg: return msp::count;   case Kind::Map: return mpp::count;    case Kind::Math: return mathp::count;
        case Kind::ModFx: return mfx::count;  case Kind::Delay: return dly::count;  case Kind::Reverb: return rvb::count;
        case Kind::Texture: return txp::count; case Kind::Comp: return cmp::count;  case Kind::FxEq: return feq::count;
        case Kind::Dist: return dsp_::count;  case Kind::FxFilter: return ffp::count;
        default: return 0;
    }
}

// ------------------------------------------------------------------ choice lists
juce::StringArray tableNames()
{
    return { "Classic", "Pulse Width", "Formant Vox", "Hollow Odd", "Organ Bars", "Bell Partials", "Sync Sweep", "Fold Sweep",
             "PD Resonant", "Brass Swell", "Bowed String", "Glass", "Growl", "Digital Bits", "Choir", "Dark Pad",
             "Spline Shapes", "Metallic", "Reed", "Sine Stack", "Phase Warp", "Triangle Fold", "Clav Pluck", "Ghost" };
}

juce::StringArray divisionNames()
{
    return { "4 bars", "2 bars", "1/1", "1/2.", "1/2", "1/2T", "1/4.", "1/4", "1/4T", "1/8.", "1/8", "1/8T", "1/16.", "1/16", "1/16T", "1/32" };
}

double divisionBeats (int i)
{
    static const double b[] { 16.0, 8.0, 4.0, 3.0, 2.0, 4.0 / 3.0, 1.5, 1.0, 2.0 / 3.0, 0.75, 0.5, 1.0 / 3.0, 0.375, 0.25, 1.0 / 6.0, 0.125 };
    return b[juce::jlimit (0, 15, i)];
}

static juce::StringArray oscFxNames()
{
    return { "Off", "Sync", "Phase Dist", "Bend +", "Bend -", "Mirror", "Pulse", "Formant", "Quantize", "Fold", "Squash", "Crush", "Ring" };
}

juce::StringArray sourceNames()
{
    juce::StringArray s { "None" };
    for (int i = 1; i <= 4; ++i) s.add ("Env " + juce::String (i));
    for (int i = 1; i <= 4; ++i) s.add ("LFO " + juce::String (i));
    for (int i = 1; i <= 4; ++i) s.add ("MSEG " + juce::String (i));
    for (int i = 1; i <= 4; ++i) s.add ("Map " + juce::String (i));
    for (int i = 1; i <= 4; ++i) s.add ("Math " + juce::String (i));
    s.addArray ({ "Velocity", "Key Follow", "Mod Wheel", "Aftertouch", "Pitch Bend", "Macro A", "Macro B", "Macro C", "Macro D",
                  "Random", "Gate", "Expression", "Alternate" });
    for (int i = 1; i <= kNumXY; ++i) { s.add ("XY " + juce::String (i) + " X"); s.add ("XY " + juce::String (i) + " Y"); }
    jassert (s.size() == sNumSources);
    return s;
}

bool sourceIsGlobal (int s)
{
    return (s >= sLfo1 && s < sLfo1 + 4) || s == sModWheel || s == sAftertouch || s == sPitchBend
        || (s >= sMacroA && s <= sMacroD) || s == sExpression || (s >= sXY1X && s < sXY1X + 2 * kNumXY);
}

// ------------------------------------------------------------------ ParamDef helpers
juce::NormalisableRange<float> ParamDef::range() const
{
    if (type == Choice) return { 0.0f, (float) juce::jmax (1, choices.size() - 1), 1.0f };
    if (type == Bool) return { 0.0f, 1.0f, 1.0f };
    juce::NormalisableRange<float> r (min, max, step);
    if (centre > min && centre < max) r.setSkewForCentre (centre);
    return r;
}

static juce::String num (float v, int dp)
{
    return juce::String (v, dp);
}

juce::String ParamDef::format (float v) const
{
    if (type == Choice) return choices[juce::jlimit (0, choices.size() - 1, juce::roundToInt (v))];
    if (type == Bool) return v > 0.5f ? "On" : "Off";
    if (unit == "Hz")
    {
        if (v >= 1000.0f) return num (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz";
        return num (v, v < 10.0f ? 2 : (v < 100.0f ? 1 : 0)) + " Hz";
    }
    if (unit == "dB") return (v <= min + 0.01f && min <= -59.0f) ? juce::String ("-inf dB") : (v > 0.05f ? "+" : "") + num (v, 1) + " dB";
    if (unit == "%") return num (v * 100.0f, 0) + "%";
    if (unit == "%b") return (v > 0.005f ? "+" : "") + num (v * 100.0f, 0) + "%";
    if (unit == "st") return (v > 0.005f ? "+" : "") + num (v, step >= 1.0f ? 0 : 2) + " st";
    if (unit == "ct") return (v > 0.05f ? "+" : "") + num (v, 0) + " ct";
    if (unit == "ms") return v >= 1000.0f ? num (v / 1000.0f, 2) + " s" : num (v, v < 10.0f ? 1 : 0) + " ms";
    if (unit == "s") return v < 1.0f ? num (v * 1000.0f, v < 0.01f ? 1 : 0) + " ms" : num (v, 2) + " s";
    if (unit == "x") return num (v, v < 10.0f ? 2 : 1) + "x";
    if (unit == "pan")
    {
        if (std::abs (v) < 0.005f) return "C";
        return num (std::abs (v) * 100.0f, 0) + (v < 0 ? " L" : " R");
    }
    if (unit == "int") return juce::String (juce::roundToInt (v));
    if (unit == "bp") return (v > 0.005f ? "+" : "") + num (v, 2);
    if (unit == "ratio") return num (v, 1) + ":1";
    return num (v, 2) + (unit.isNotEmpty() ? " " + unit : juce::String());
}

// ------------------------------------------------------------------ catalogue
namespace
{
struct Catalogue
{
    std::vector<ParamDef> defs;
    std::unordered_map<juce::String, int> byId;
    int base[kNumKinds][8] {};
    std::vector<int> dests;
    std::vector<int> destOf;
};

struct Builder
{
    std::vector<ParamDef>& v;
    Kind kind = Kind::Count;
    int inst = -1;
    juce::String pre, mname;

    void begin (Kind k, int i)
    {
        kind = k; inst = i;
        pre = juce::String (kindInfo (k).prefix) + juce::String (i + 1) + "_";
        mname = moduleName (k, i) + " ";
    }

    ParamDef& add (int local, const juce::String& key, const juce::String& caption, const juce::String& tip)
    {
        ParamDef d;
        d.id = pre + key;
        d.name = mname + caption;
        d.caption = caption;
        d.tip = tip;
        d.kind = kind; d.inst = inst; d.local = local;
        v.push_back (d);
        return v.back();
    }
    ParamDef& f (int local, const juce::String& key, const juce::String& caption, float mn, float mx, float def,
                 const juce::String& unit, const juce::String& tip, float centre = -1e9f, bool mod = true, float step = 0.0f)
    {
        auto& d = add (local, key, caption, tip);
        d.type = ParamDef::Float; d.min = mn; d.max = mx; d.def = def; d.unit = unit; d.centre = centre; d.mod = mod; d.step = step;
        return d;
    }
    ParamDef& c (int local, const juce::String& key, const juce::String& caption, const juce::StringArray& ch, int def, const juce::String& tip)
    {
        auto& d = add (local, key, caption, tip);
        d.type = ParamDef::Choice; d.choices = ch; d.def = (float) def;
        return d;
    }
    ParamDef& b (int local, const juce::String& key, const juce::String& caption, bool def, const juce::String& tip)
    {
        auto& d = add (local, key, caption, tip);
        d.type = ParamDef::Bool; d.def = def ? 1.0f : 0.0f;
        return d;
    }
    // global (non-module) parameter
    ParamDef& gf (const juce::String& pid, const juce::String& name, const juce::String& caption, float mn, float mx, float def,
                  const juce::String& unit, const juce::String& tip, float centre = -1e9f, bool mod = false, float step = 0.0f)
    {
        kind = Kind::Count; inst = -1; pre = {}; mname = {};
        auto& d = f (-1, pid, caption, mn, mx, def, unit, tip, centre, mod, step);
        d.name = name;
        return d;
    }
    ParamDef& gc (const juce::String& pid, const juce::String& name, const juce::String& caption, const juce::StringArray& ch, int def, const juce::String& tip)
    {
        kind = Kind::Count; inst = -1; pre = {}; mname = {};
        auto& d = c (-1, pid, caption, ch, def, tip);
        d.name = name;
        return d;
    }
    ParamDef& gb (const juce::String& pid, const juce::String& name, const juce::String& caption, bool def, const juce::String& tip)
    {
        kind = Kind::Count; inst = -1; pre = {}; mname = {};
        auto& d = b (-1, pid, caption, def, tip);
        d.name = name;
        return d;
    }
};

const juce::String kTuneTip = "Coarse tuning in semitones.";
const juce::String kFineTip = "Fine tuning in cents (hundredths of a semitone).";

void voiceModules (Builder& B, Catalogue& cat)
{
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::Osc][i] = (int) B.v.size();
        B.begin (Kind::Osc, i);
        B.c (op::engine, "engine", "Engine", { "Wavetable", "Additive" }, 0,
             "Wavetable reads the selected table and morphs through its frames. Additive rebuilds the same spectrum from up to 64 "
             "sine partials, which lets you stretch them into bells and decay the upper partials over time.");
        B.c (op::table, "table", "Table", tableNames(), i == 0 ? 0 : 0, "Which of the 24 built-in wavetables this oscillator plays.");
        B.f (op::wave, "wave", "Wave", 0, 1, 0, "%", "Morph position through the table's eight frames. Sweep it with an envelope or LFO for evolving timbres.");
        B.f (op::tune, "tune", "Tune", -48, 48, 0, "st", kTuneTip, -1e9f, true, 0.0f);
        B.f (op::fine, "fine", "Fine", -100, 100, 0, "ct", kFineTip);
        B.f (op::vol, "vol", "Volume", 0, 1, 0.7f, "%", "Output level of this oscillator into its lane.");
        B.f (op::pan, "pan", "Pan", -1, 1, 0, "pan", "Stereo position.");
        B.f (op::phase, "phase", "Phase", 0, 1, 0, "%", "Start phase at each new note (ignored when Free Run is on).", -1e9f, false);
        B.b (op::freeRun, "free", "Free Run", false, "On: the oscillator keeps running between notes instead of restarting, for a looser analogue feel.");
        B.f (op::keytrack, "keytrack", "Keytrack", 0, 1, 1, "%", "How much the pitch follows the keyboard. 0% gives a fixed pitch for drones and effects.", -1e9f, false);
        B.f (op::uniVoices, "univoices", "Unison", 1, 16, 1, "int", "Number of stacked, detuned copies (1 to 16). More voices sound thicker but cost more CPU.", -1e9f, false, 1.0f);
        B.f (op::uniDetune, "unidetune", "Detune", 0, 1, 0.2f, "%", "Spread of the unison voices. Low values shimmer, high values roar.");
        B.f (op::uniWidth, "uniwidth", "Width", 0, 1, 0.7f, "%", "Stereo spread of the unison voices (additive mode spreads odd and even partials instead).");
        auto fxTip = juce::String ("Wave FX reshape the oscillator: Sync and Formant add sharp resonant harmonics, Phase Dist and Bend skew the cycle, "
                                   "Mirror doubles it, Pulse squeezes it, Quantize steps it, Fold/Squash/Crush/Ring distort it.");
        B.c (op::fx1Type, "fx1", "FX 1", oscFxNames(), 0, fxTip);
        B.f (op::fx1Amt, "fx1amt", "FX 1 Amt", 0, 1, 0, "%", "Amount of Wave FX 1.");
        B.c (op::fx2Type, "fx2", "FX 2", oscFxNames(), 0, fxTip + " FX 2 is applied after FX 1.");
        B.f (op::fx2Amt, "fx2amt", "FX 2 Amt", 0, 1, 0, "%", "Amount of Wave FX 2.");
        B.f (op::partials, "partials", "Partials", 1, 64, 48, "int", "Additive engine: how many sine partials are used.", -1e9f, false, 1.0f);
        B.f (op::bright, "bright", "Bright", -1, 1, 0, "bp", "Additive engine: tilts the spectrum darker (left) or brighter (right).");
        B.f (op::oddEven, "oddeven", "Odd/Even", -1, 1, 0, "bp", "Additive engine: left keeps only odd partials (hollow, clarinet), right favours even partials.");
        B.f (op::stretch, "stretch", "Stretch", -1, 1, 0, "bp", "Additive engine: pulls the partials off the harmonic series for bells, gongs and alien metals.");
        B.f (op::pDecay, "pdecay", "P.Decay", 0, 1, 0, "%", "Additive engine: upper partials die away faster than the fundamental, like a struck string or bell.");
    }
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::FM][i] = (int) B.v.size();
        B.begin (Kind::FM, i);
        B.f (fmp::tune, "tune", "Tune", -48, 48, 0, "st", kTuneTip);
        B.f (fmp::fine, "fine", "Fine", -100, 100, 0, "ct", kFineTip);
        B.f (fmp::ratio, "ratio", "Ratio", 0.25f, 16, 1, "x", "Modulator frequency as a multiple of the carrier. Whole numbers are harmonic; in-between values clang.", 2.0f);
        B.f (fmp::depth, "depth", "Depth", 0, 1, 0.3f, "%", "FM index: how hard the modulator bends the carrier. Map an envelope here for classic FM plucks and bells.");
        B.f (fmp::feedback, "feedback", "Feedback", 0, 1, 0, "%", "Modulator self-feedback, from pure sine towards saw and then noise.");
        B.c (fmp::modShape, "modshape", "Carrier", { "Sine", "Half Sine", "Abs Sine", "Quarter" }, 0, "Carrier waveform shape (DX-style alternative waves).");
        B.f (fmp::inputFm, "inputfm", "Input FM", 0, 1, 0, "%", "Uses whatever is already in the lane (above this slot) as a third operator modulating the carrier.");
        B.f (fmp::vol, "vol", "Volume", 0, 1, 0.6f, "%", "Output level into the lane.");
        B.f (fmp::pan, "pan", "Pan", -1, 1, 0, "pan", "Stereo position.");
        B.f (fmp::keytrack, "keytrack", "Keytrack", 0, 1, 1, "%", "How much the pitch follows the keyboard.", -1e9f, false);
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Noise][i] = (int) B.v.size();
        B.begin (Kind::Noise, i);
        B.c (np::type, "type", "Type", { "White", "Pink", "Brown", "Blue", "Crackle", "Digital" }, 0,
             "Noise flavour. Crackle gives vinyl-like dust, Digital gives stepped sample-and-hold noise.");
        B.f (np::color, "color", "Color", -1, 1, 0, "bp", "Tilts the noise darker or brighter.");
        B.f (np::density, "density", "Density", 0, 1, 0.5f, "%", "Crackle: how often the dust pops. Digital: the step rate.");
        B.f (np::width, "width", "Width", 0, 1, 1, "%", "Stereo width (0% is mono).", -1e9f, false);
        B.f (np::vol, "vol", "Volume", 0, 1, 0.3f, "%", "Output level into the lane.");
        B.f (np::pan, "pan", "Pan", -1, 1, 0, "pan", "Stereo position.");
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Exc][i] = (int) B.v.size();
        B.begin (Kind::Exc, i);
        B.c (xp::mode, "mode", "Mode", { "Impulse", "Burst", "Mallet", "Bow", "Pattern" }, 2,
             "How the exciter strikes: a click, a noise burst, a soft mallet, a continuous bow or a repeating pattern. Place a Resonator or Comb after it.");
        B.f (xp::decay, "decay", "Decay", 1, 2000, 40, "ms", "Length of the strike (Bow mode: how slowly the bow fades in).", 100.0f);
        B.f (xp::color, "color", "Color", -1, 1, 0, "bp", "Darker (felt) to brighter (metal) strike.");
        B.f (xp::rate, "rate", "Rate", 0.5f, 40, 6, "Hz", "Pattern mode: strikes per second.", 6.0f);
        B.f (xp::level, "level", "Level", 0, 1, 0.8f, "%", "Strike strength.");
        B.f (xp::width, "width", "Width", 0, 1, 0.3f, "%", "Stereo randomness between the left and right strike.", -1e9f, false);
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Res][i] = (int) B.v.size();
        B.begin (Kind::Res, i);
        B.c (rp::profile, "profile", "Profile", { "Marimba", "Vibes", "Bell", "Glass", "Gong", "Plate", "String", "Tube" }, 0,
             "The physical object being modelled: which overtones ring and how they relate.");
        B.f (rp::tune, "tune", "Tune", -48, 48, 0, "st", kTuneTip);
        B.f (rp::fine, "fine", "Fine", -100, 100, 0, "ct", kFineTip);
        B.f (rp::decay, "decay", "Decay", 0.02f, 12, 1.2f, "s", "Ring time of the body.", 1.0f);
        B.f (rp::damping, "damping", "Damping", 0, 1, 0.4f, "%", "Makes the upper modes die faster than the low ones (wood versus metal).");
        B.f (rp::bright, "bright", "Bright", -1, 1, 0, "bp", "Level of the upper modes.");
        B.f (rp::spread, "spread", "Spread", 0, 1, 0.1f, "%", "Detunes the modes and the stereo channels for a wider, more unstable body.");
        B.f (rp::input, "input", "Input", -48, 12, 0, "dB", "Drive into the resonator. Lower it when feeding sustained oscillators rather than short exciter strikes.", -12.0f);
        B.f (rp::mix, "mix", "Mix", 0, 1, 1, "%", "Dry lane signal versus resonating body.");
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Comb][i] = (int) B.v.size();
        B.begin (Kind::Comb, i);
        B.c (cp::mode, "mode", "Mode", { "Comb", "Diffuse" }, 0,
             "Comb is a tuned delay that rings at the played pitch (plucked strings, flanged tones). Diffuse spreads it into a small metallic room.");
        B.f (cp::tune, "tune", "Tune", -48, 48, 0, "st", kTuneTip);
        B.f (cp::fine, "fine", "Fine", -100, 100, 0, "ct", kFineTip);
        B.f (cp::feedback, "feedback", "Feedback", -1, 1, 0.85f, "bp", "Resonance. Negative values ring an octave lower with hollow, odd overtones.");
        B.f (cp::damping, "damping", "Damping", 0, 1, 0.3f, "%", "High-frequency loss inside the loop: brighter strings to the left, duller to the right.");
        B.f (cp::input, "input", "Input", 0, 1, 1, "%", "Signal sent into the comb.");
        B.f (cp::mix, "mix", "Mix", 0, 1, 1, "%", "Dry versus combed signal.");
        B.f (cp::vol, "vol", "Volume", 0, 1, 0.7f, "%", "Output level.");
    }
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::Vcf][i] = (int) B.v.size();
        B.begin (Kind::Vcf, i);
        B.c (vp::model, "model", "Model", { "Ladder", "Cascade", "SVF", "Diode", "Sallen-Key", "OTA", "Steiner", "Bite", "Smooth",
                                             "Formant", "Phaser", "Vintage LPG", "Twin Peak" }, 0,
             "Filter circuit. Ladder is fat and creamy, Diode squelchy, Sallen-Key and Bite aggressive, SVF and Smooth clean, Formant "
             "speaks vowels, Phaser notches, Vintage LPG closes like a low-pass gate, Twin Peak sweeps two resonances.");
        B.c (vp::response, "response", "Response", { "LP 24", "LP 18", "LP 12", "LP 6", "BP 12", "BP 6", "HP 24", "HP 12", "HP 6", "Notch", "Peak", "All Pass" }, 0,
             "Filter type and slope. Some models only approximate some responses.");
        B.f (vp::cutoff, "cutoff", "Cutoff", 20, 20000, 2000, "Hz", "Cutoff frequency.", 1000.0f);
        B.f (vp::reso, "reso", "Reso", 0, 1, 0.2f, "%", "Resonance. High values whistle and the Ladder, Diode and Bite models self-oscillate.");
        B.f (vp::drive, "drive", "Drive", 0, 1, 0, "%", "Input drive into the filter's saturation.");
        B.f (vp::keytrack, "keytrack", "Keytrack", 0, 1, 0.5f, "%", "How much the cutoff follows the keyboard (100% tracks one octave per octave).");
        B.f (vp::spread, "spread", "Spread", -1, 1, 0, "bp", "Offsets left and right cutoffs in opposite directions for stereo movement.");
        B.f (vp::mix, "mix", "Mix", 0, 1, 1, "%", "Dry versus filtered signal.");
    }
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::Shape][i] = (int) B.v.size();
        B.begin (Kind::Shape, i);
        B.c (sp::mode, "mode", "Mode", { "Tube", "Hard Clip", "Wavefold", "Asym Fold", "Sine Shaper", "Rectify", "Bitcrush", "Decimate" }, 0,
             "Distortion character. Tube is warm, Hard Clip is fuzz, the folders add metallic overtones, Bitcrush and Decimate add lo-fi grit.");
        B.f (sp::drive, "drive", "Drive", 0, 1, 0.3f, "%", "How hard the signal is pushed into the shaper.");
        B.f (sp::bias, "bias", "Bias", -1, 1, 0, "bp", "Asymmetry: adds even harmonics and sputter.");
        B.f (sp::tone, "tone", "Tone", 0, 1, 0.8f, "%", "Post-shaper low-pass to tame fizz.");
        B.f (sp::mix, "mix", "Mix", 0, 1, 1, "%", "Dry versus shaped signal.");
        B.f (sp::out, "out", "Output", -24, 12, 0, "dB", "Output level after shaping.", -1e9f, true);
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Ring][i] = (int) B.v.size();
        B.begin (Kind::Ring, i);
        B.c (gp::source, "source", "Source", { "Sine", "Triangle", "Square", "Lane 1", "Lane 2", "Lane 3" }, 0,
             "What the lane is multiplied with: an internal oscillator, or the output of an earlier lane.");
        B.f (gp::tune, "tune", "Tune", -48, 48, 7, "st", "Internal oscillator pitch relative to the played note.");
        B.f (gp::fine, "fine", "Fine", -100, 100, 0, "ct", kFineTip);
        B.f (gp::keytrack, "keytrack", "Keytrack", 0, 1, 1, "%", "How much the internal oscillator follows the keyboard.", -1e9f, false);
        B.f (gp::mix, "mix", "Mix", 0, 1, 1, "%", "Dry versus ring-modulated signal.");
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Eq][i] = (int) B.v.size();
        B.begin (Kind::Eq, i);
        B.f (ep::lowFreq, "lowfreq", "Low Freq", 30, 1000, 200, "Hz", "Low shelf frequency.", 150.0f, false);
        B.f (ep::lowGain, "lowgain", "Low", -18, 18, 0, "dB", "Low shelf gain.");
        B.f (ep::midFreq, "midfreq", "Mid Freq", 100, 10000, 1000, "Hz", "Mid peak frequency.", 1000.0f);
        B.f (ep::midGain, "midgain", "Mid", -18, 18, 0, "dB", "Mid peak gain.");
        B.f (ep::midQ, "midq", "Mid Q", 0.3f, 8, 1, "", "Mid peak width (higher is narrower).", 1.2f, false);
        B.f (ep::highFreq, "highfreq", "High Freq", 1000, 16000, 5000, "Hz", "High shelf frequency.", 5000.0f, false);
        B.f (ep::highGain, "highgain", "High", -18, 18, 0, "dB", "High shelf gain.");
    }
}

void modModules (Builder& B, Catalogue& cat)
{
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::Env][i] = (int) B.v.size();
        B.begin (Kind::Env, i);
        const auto pre = i == 0 ? juce::String ("ENV 1 is the amp envelope: it shapes every voice's volume. ") : juce::String();
        B.f (envp::delay, "delay", "Delay", 0, 4, 0, "s", pre + "Wait before the attack starts.", 0.5f, false);
        B.f (envp::attack, "attack", "Attack", 0.0005f, 20, i == 0 ? 0.003f : 0.01f, "s", pre + "Rise time.", 1.0f);
        B.f (envp::decay, "decay", "Decay", 0.001f, 20, 0.6f, "s", pre + "Fall time to the sustain level.", 1.0f);
        B.f (envp::sustain, "sustain", "Sustain", 0, 1, i == 0 ? 0.8f : 0.5f, "%", pre + "Level held while the key is down.");
        B.f (envp::release, "release", "Release", 0.001f, 30, i == 0 ? 0.3f : 0.5f, "s", pre + "Fade time after the key is released.", 1.5f);
        B.f (envp::slope, "slope", "Slope", -1, 1, 0.3f, "bp", "Curvature: right is snappy and exponential, left is slow and linear-to-logarithmic.", -1e9f, false);
        B.f (envp::velocity, "velocity", "Velocity", 0, 1, 0.3f, "%", "How much key velocity scales the envelope.", -1e9f, false);
        B.f (envp::keyscale, "keyscale", "Key Scale", -1, 1, 0, "bp", "Positive values make higher notes shorter, like real strings and bells.", -1e9f, false);
        B.b (envp::oneShot, "oneshot", "One Shot", false, "The envelope runs through its whole shape regardless of how long the key is held.");
    }
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::Lfo][i] = (int) B.v.size();
        B.begin (Kind::Lfo, i);
        B.c (lfop::wave, "wave", "Wave", { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Smooth Rnd" }, 0, "LFO waveform.");
        B.f (lfop::rate, "rate", "Rate", 0.01f, 40, 2, "Hz", "Speed in Hz (when Sync is off).", 2.0f);
        B.b (lfop::sync, "sync", "Sync", false, "Lock the LFO to the host tempo using the Division setting.");
        B.c (lfop::division, "div", "Division", divisionNames(), 7, "Tempo-synced cycle length.");
        B.f (lfop::phase, "phase", "Phase", 0, 1, 0, "%", "Start phase at each note (or offset of the global LFO).", -1e9f, false);
        B.f (lfop::delay, "delay", "Delay", 0, 5, 0, "s", "Wait before the LFO starts.", 1.0f, false);
        B.f (lfop::fade, "fade", "Fade In", 0, 10, 0, "s", "Time for the LFO to fade up after its delay, for vibrato that blooms.", 1.5f, false);
        B.c (lfop::mode, "mode", "Mode", { "Retrig", "Free", "Global" }, 0,
             "Retrig restarts per note, Free runs per voice without restarting, Global is one shared LFO (and can modulate effects).");
        B.b (lfop::unipolar, "unipolar", "Unipolar", false, "On: swings 0 to +1 instead of -1 to +1.");
        B.f (lfop::amount, "amount", "Amount", 0, 1, 1, "%", "Output level of the LFO.");
    }
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::Mseg][i] = (int) B.v.size();
        B.begin (Kind::Mseg, i);
        B.f (msp::time, "time", "Time", 0.05f, 20, 1, "x", "Scales the whole curve's timing (Sync off: one grid unit = one second x Time).", 1.0f);
        B.b (msp::sync, "sync", "Sync", false, "Grid units become beats at the host tempo (Time still scales them).");
        B.c (msp::loop, "loop", "Loop", { "One Shot", "Sustain Loop", "Repeat" }, 0,
             "One Shot plays once. Sustain Loop cycles between the loop markers while the key is held, then plays on. Repeat loops the whole curve forever.");
        B.b (msp::bipolar, "bipolar", "Bipolar", false, "On: the curve's bottom is -1 instead of 0, for pitch or pan sweeps around the centre.");
        B.f (msp::velocity, "velocity", "Velocity", 0, 1, 0, "%", "How much key velocity scales the curve.", -1e9f, false);
        B.f (msp::amount, "amount", "Amount", 0, 1, 1, "%", "Output level of the MSEG.");
    }
    juce::StringArray mapIn { "Key Follow", "Velocity", "Random", "Mod Wheel", "Aftertouch", "Env 1", "Env 2", "Env 3", "Env 4",
                              "LFO 1", "LFO 2", "LFO 3", "LFO 4", "MSEG 1", "MSEG 2", "MSEG 3", "MSEG 4", "Alternate" };
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::Map][i] = (int) B.v.size();
        B.begin (Kind::Map, i);
        B.c (mpp::input, "input", "Input", mapIn, 0, "Map mode: the signal looked up in the drawn curve (Key Follow gives custom keyboard scaling).");
        B.c (mpp::mode, "mode", "Mode", { "Map", "Step Seq" }, 0, "Map: transfer curve. Step Seq: the bars play in time as a step sequencer.");
        B.f (mpp::steps, "steps", "Steps", 2, 32, 16, "int", "How many bars are used.", -1e9f, false, 1.0f);
        B.c (mpp::division, "div", "Division", divisionNames(), 13, "Step Seq: length of each step.");
        B.f (mpp::smooth, "smooth", "Smooth", 0, 1, 0, "%", "Glides between bars instead of stepping.", -1e9f, false);
        B.f (mpp::amount, "amount", "Amount", 0, 1, 1, "%", "Output level of the mapper.");
    }
    juce::StringArray mathSrc;
    {
        auto all = sourceNames();
        for (int s = 0; s < sNumSources; ++s)
            if (! (s >= sMath1 && s < sMath1 + 4)) mathSrc.add (all[s]);
    }
    for (int i = 0; i < 4; ++i)
    {
        cat.base[(int) Kind::Math][i] = (int) B.v.size();
        B.begin (Kind::Math, i);
        B.c (mathp::srcA, "srca", "Source A", mathSrc, 0, "First input.");
        B.c (mathp::srcB, "srcb", "Source B", mathSrc, 0, "Second input (or the control amount for Lag and Quantize).");
        B.c (mathp::opr, "op", "Operation", { "A + B", "A - B", "A x B", "Max", "Min", "Average", "Lag A by B", "Quantize A by B", "Abs A", "Invert A" }, 0,
             "How A and B are combined.");
        B.f (mathp::amount, "amount", "Amount", -1, 1, 1, "bp", "Output scaling (negative inverts).");
    }
}

void fxModules (Builder& B, Catalogue& cat)
{
    auto glob = [] (ParamDef& d) -> ParamDef& { d.global = true; return d; };
    for (int i = 0; i < 3; ++i)
    {
        cat.base[(int) Kind::ModFx][i] = (int) B.v.size();
        B.begin (Kind::ModFx, i);
        B.c (mfx::mode, "mode", "Mode", { "Chorus", "Flanger", "Phaser" }, 0, "Chorus thickens, Flanger sweeps a jet, Phaser swirls notches.");
        glob (B.f (mfx::rate, "rate", "Rate", 0.01f, 10, 0.4f, "Hz", "Modulation speed.", 0.8f));
        glob (B.f (mfx::depth, "depth", "Depth", 0, 1, 0.5f, "%", "Modulation depth."));
        glob (B.f (mfx::feedback, "feedback", "Feedback", -0.95f, 0.95f, 0, "bp", "Feedback (Flanger and Phaser intensity)."));
        B.f (mfx::voices, "voices", "Voices", 2, 24, 4, "int", "Chorus voices (2-8) or phaser stages (2-24).", -1e9f, false, 1.0f);
        glob (B.f (mfx::centre, "centre", "Centre", 0, 1, 0.5f, "%", "Base delay (chorus, flanger) or centre frequency (phaser)."));
        B.f (mfx::width, "width", "Width", 0, 1, 0.8f, "%", "Stereo spread.", -1e9f, false);
        B.c (mfx::character, "character", "Character", { "Digital", "Analog" }, 1, "Analog adds bucket-brigade darkness and subtle saturation.");
        glob (B.f (mfx::mix, "mix", "Mix", 0, 1, 0.5f, "%", "Dry/wet balance."));
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Delay][i] = (int) B.v.size();
        B.begin (Kind::Delay, i);
        B.c (dly::mode, "mode", "Mode", { "Mono", "Stereo", "Ping Pong" }, 1, "Echo routing.");
        B.b (dly::sync, "sync", "Sync", true, "Lock delay times to the host tempo.");
        B.c (dly::divL, "divl", "Time L", divisionNames(), 9, "Left (or mono) delay time when synced.");
        B.c (dly::divR, "divr", "Time R", divisionNames(), 7, "Right delay time when synced.");
        B.f (dly::timeMs, "timems", "Time", 1, 2000, 375, "ms", "Delay time when Sync is off (right side is 1.5x for Stereo mode).", 300.0f);
        glob (B.f (dly::feedback, "feedback", "Feedback", 0, 1.05f, 0.4f, "%", "Number of repeats. Above 100% it slowly builds into a self-oscillating wall."));
        B.f (dly::lowCut, "lowcut", "Low Cut", 20, 2000, 120, "Hz", "Removes low end from the repeats.", 200.0f, false);
        B.f (dly::highCut, "highcut", "High Cut", 500, 20000, 7000, "Hz", "Darkens each repeat.", 5000.0f, false);
        glob (B.f (dly::modDepth, "mod", "Wobble", 0, 1, 0.15f, "%", "Tape-style wow and flutter."));
        B.f (dly::diffusion, "diffusion", "Diffusion", 0, 1, 0, "%", "Smears repeats into a reverb-like wash.", -1e9f, false);
        B.c (dly::style, "style", "Style", { "Digital", "Analog" }, 1, "Analog saturates and darkens the repeats.");
        glob (B.f (dly::mix, "mix", "Mix", 0, 1, 0.3f, "%", "Dry/wet balance."));
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Reverb][i] = (int) B.v.size();
        B.begin (Kind::Reverb, i);
        B.c (rvb::model, "model", "Model", { "Classic", "Lush" }, 0, "Classic is a focused hall, Lush a dense, modulated cathedral.");
        glob (B.f (rvb::size, "size", "Size", 0, 1, 0.6f, "%", "Room size."));
        glob (B.f (rvb::decay, "decay", "Decay", 0.2f, 30, 3, "s", "Reverb time.", 3.0f));
        B.f (rvb::predelay, "predelay", "Pre-Delay", 0, 250, 20, "ms", "Gap before the reverb begins.", 60.0f, false);
        B.f (rvb::damping, "damping", "Damping", 0, 1, 0.4f, "%", "High-frequency absorption.");
        B.f (rvb::diffusion, "diffusion", "Diffusion", 0, 1, 0.75f, "%", "Density of the early reflections.", -1e9f, false);
        glob (B.f (rvb::modDepth, "mod", "Mod", 0, 1, 0.3f, "%", "Chorusing inside the tail."));
        B.f (rvb::width, "width", "Width", 0, 1, 1, "%", "Stereo width.", -1e9f, false);
        B.f (rvb::lowCut, "lowcut", "Low Cut", 20, 1000, 100, "Hz", "Keeps the tail out of the low end.", 150.0f, false);
        glob (B.f (rvb::mix, "mix", "Mix", 0, 1, 0.3f, "%", "Dry/wet balance."));
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Texture][i] = (int) B.v.size();
        B.begin (Kind::Texture, i);
        glob (B.f (txp::grain, "grain", "Grain", 10, 800, 120, "ms", "Length of each grain.", 120.0f));
        glob (B.f (txp::density, "density", "Density", 0, 1, 0.5f, "%", "How many grains overlap."));
        glob (B.f (txp::pitch, "pitch", "Pitch", -24, 24, 0, "st", "Grain transposition. +12 gives a shimmer, -12 a dark undertow."));
        glob (B.f (txp::spread, "spread", "Spread", 0, 1, 0.1f, "%", "Random pitch scatter between grains."));
        B.f (txp::reverse, "reverse", "Reverse", 0, 1, 0.2f, "%", "Chance that a grain plays backwards.", -1e9f, false);
        glob (B.f (txp::feedback, "feedback", "Feedback", 0, 0.98f, 0.5f, "%", "Feeds the cloud back into itself for endless swells."));
        glob (B.f (txp::jitter, "jitter", "Jitter", 0, 1, 0.4f, "%", "How far back in time grains are picked."));
        B.f (txp::damping, "damping", "Damping", 0, 1, 0.3f, "%", "Darkens the cloud.", -1e9f, false);
        B.b (txp::freeze, "freeze", "Freeze", false, "Stops recording: the current texture hangs in the air.");
        glob (B.f (txp::mix, "mix", "Mix", 0, 1, 0.35f, "%", "Dry/wet balance."));
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Comp][i] = (int) B.v.size();
        B.begin (Kind::Comp, i);
        B.f (cmp::threshold, "threshold", "Threshold", -60, 0, -18, "dB", "Level where compression starts.", -1e9f, false);
        B.f (cmp::ratio, "ratio", "Ratio", 1, 20, 4, "ratio", "Compression strength.", 4.0f, false);
        B.f (cmp::attack, "attack", "Attack", 0.1f, 200, 10, "ms", "How fast it clamps down.", 10.0f, false);
        B.f (cmp::release, "release", "Release", 10, 2000, 150, "ms", "How fast it lets go.", 150.0f, false);
        B.f (cmp::knee, "knee", "Knee", 0, 24, 6, "dB", "Softness of the onset.", -1e9f, false);
        B.f (cmp::makeup, "makeup", "Makeup", -12, 24, 0, "dB", "Gain after compression.", -1e9f, false);
        glob (B.f (cmp::mix, "mix", "Mix", 0, 1, 1, "%", "Parallel compression blend."));
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::FxEq][i] = (int) B.v.size();
        B.begin (Kind::FxEq, i);
        B.f (feq::lowFreq, "lowfreq", "Low Freq", 20, 1000, 100, "Hz", "Low shelf frequency.", 120.0f, false);
        B.f (feq::lowGain, "lowgain", "Low", -18, 18, 0, "dB", "Low shelf gain.", -1e9f, false);
        B.f (feq::m1Freq, "m1freq", "Mid 1 Freq", 50, 8000, 400, "Hz", "First mid band frequency.", 600.0f, false);
        B.f (feq::m1Gain, "m1gain", "Mid 1", -18, 18, 0, "dB", "First mid band gain.", -1e9f, false);
        B.f (feq::m1Q, "m1q", "Mid 1 Q", 0.3f, 8, 1, "", "First mid band width.", 1.2f, false);
        B.f (feq::m2Freq, "m2freq", "Mid 2 Freq", 300, 16000, 2500, "Hz", "Second mid band frequency.", 2500.0f, false);
        B.f (feq::m2Gain, "m2gain", "Mid 2", -18, 18, 0, "dB", "Second mid band gain.", -1e9f, false);
        B.f (feq::m2Q, "m2q", "Mid 2 Q", 0.3f, 8, 1, "", "Second mid band width.", 1.2f, false);
        B.f (feq::highFreq, "highfreq", "High Freq", 1000, 18000, 8000, "Hz", "High shelf frequency.", 6000.0f, false);
        B.f (feq::highGain, "highgain", "High", -18, 18, 0, "dB", "High shelf gain.", -1e9f, false);
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::Dist][i] = (int) B.v.size();
        B.begin (Kind::Dist, i);
        B.c (dsp_::mode, "mode", "Mode", { "Tube", "Hard Clip", "Fuzz", "Fold", "Rectify", "Bitcrush", "Downsample" }, 0, "Distortion type.");
        glob (B.f (dsp_::drive, "drive", "Drive", 0, 1, 0.3f, "%", "Distortion amount."));
        glob (B.f (dsp_::tone, "tone", "Tone", 0, 1, 0.6f, "%", "Post-distortion tone: dark to bright."));
        B.f (dsp_::bias, "bias", "Bias", -1, 1, 0, "bp", "Asymmetry.", -1e9f, false);
        glob (B.f (dsp_::mix, "mix", "Mix", 0, 1, 1, "%", "Dry/wet balance."));
        B.f (dsp_::out, "out", "Output", -24, 12, 0, "dB", "Output level.", -1e9f, false);
    }
    for (int i = 0; i < 2; ++i)
    {
        cat.base[(int) Kind::FxFilter][i] = (int) B.v.size();
        B.begin (Kind::FxFilter, i);
        B.c (ffp::type, "type", "Type", { "Low Pass", "High Pass", "Band Pass", "Notch" }, 0, "Filter type for the whole bus.");
        glob (B.f (ffp::cutoff, "cutoff", "Cutoff", 20, 20000, 20000, "Hz", "Cutoff frequency. Map a macro here for whole-mix sweeps.", 1000.0f));
        glob (B.f (ffp::reso, "reso", "Reso", 0, 1, 0.1f, "%", "Resonance."));
        B.f (ffp::drive, "drive", "Drive", 0, 1, 0, "%", "Input drive.", -1e9f, false);
        glob (B.f (ffp::mix, "mix", "Mix", 0, 1, 1, "%", "Dry/wet balance."));
    }
}

void globalParams (Builder& B)
{
    // voice grid
    const auto gridNames = gridChoiceNames();
    for (int l = 0; l < kNumLanes; ++l)
        for (int s = 0; s < kNumSlots; ++s)
        {
            const int def = (l == 0 && s == 0) ? 1 : (l == 0 && s == 1) ? 1 + 4 + 4 + 2 + 2 + 2 + 2 : 0;   // OSC 1 -> VCF 1
            B.gc (id::slot (l, s), "Grid L" + juce::String (l + 1) + " S" + juce::String (s + 1), "Slot", gridNames, def,
                  "Module placed in this grid slot. Signal flows top to bottom through each lane.");
        }
    for (int l = 0; l < kNumLanes; ++l)
    {
        const auto L = "Lane " + juce::String (l + 1) + " ";
        B.gf (id::lane (l, "vol"), L + "Volume", "Volume", -60, 6, l == 0 ? 0.0f : 0.0f, "dB", "Lane output level into the main bus.", -12.0f, true);
        B.gf (id::lane (l, "pan"), L + "Pan", "Pan", -1, 1, 0, "pan", "Lane stereo position.", -1e9f, true);
        for (int a = 0; a < 3; ++a)
            B.gf (id::lane (l, ("send" + juce::String (a + 1)).toRawUTF8()), L + "Send Aux " + juce::String (a + 1), "Aux " + juce::String (a + 1),
                  0, 1, 0, "%", "Amount sent to effect bus Aux " + juce::String (a + 1) + ".", -1e9f, true);
        B.gc (id::lane (l, "in"), L + "Input", "Input", { "Off", "Lane 1", "Lane 2", "Lane 3" }, 0,
              "Feeds the output of an earlier lane into the top of this lane, so you can stack filters or process one lane with another. "
              "Only lanes to the left can feed a lane.");
        B.gf (id::lane (l, "inlvl"), L + "Input Level", "In Level", 0, 1, 1, "%", "Level of the incoming lane signal.", -1e9f, true);
    }
    // FX grid
    const auto fxNames = fxChoiceNames();
    const char* busNames[] { "Main", "Aux 1", "Aux 2", "Aux 3" };
    for (int b = 0; b < kNumBuses; ++b)
        for (int s = 0; s < kFxSlots; ++s)
        {
            int def = 0;
            if (b == 1 && s == 0) def = 1 + 3 + 2;         // REVERB 1
            if (b == 2 && s == 0) def = 1 + 3;             // DELAY 1
            B.gc (id::fxSlot (b, s), juce::String ("FX ") + busNames[b] + " S" + juce::String (s + 1), "FX Slot", fxNames, def,
                  juce::String ("Effect placed in this slot of the ") + busNames[b] + " bus.");
        }
    for (int a = 0; a < 3; ++a)
        B.gf (id::auxRet (a), "Aux " + juce::String (a + 1) + " Return", "Return", -60, 6, 0, "dB",
              "Level of Aux " + juce::String (a + 1) + " returning to the main bus (before the Main bus effects).", -12.0f, true).global = true;

    // modulation matrix
    const auto srcs = sourceNames();
    juce::StringArray dests { "None" };
    for (int i = 0; i < kNumMatrix; ++i)
    {
        const auto M = "Mod " + juce::String (i + 1) + " ";
        B.gc (id::mtx (i, "src"), M + "Source", "Source", srcs, 0, "Modulation source.");
        B.gc (id::mtx (i, "via"), M + "Via", "Via", srcs, 0, "Optional second source that scales this connection (for example Mod Wheel).");
        B.gc (id::mtx (i, "dst"), M + "Target", "Target", { "None" }, 0, "Parameter being modulated.");   // filled after the catalogue is complete
        B.gf (id::mtx (i, "amt"), M + "Amount", "Amount", -1, 1, 0, "bp", "Modulation depth (negative inverts). 100% sweeps the full range of the target.");
    }

    // macros and voice
    for (int m = 0; m < 4; ++m)
        B.gf (id::macro (m), juce::String ("Macro ") + (char) ('A' + m), juce::String ("Ctrl ") + (char) ('A' + m), 0, 1, 0, "%",
              "Performance macro. Route it to anything in the Mod Matrix, or right-click any knob and choose Modulate.", -1e9f, false);
    B.gf (id::master, "Master Volume", "Master", -60, 6, -6, "dB", "Final output level.", -12.0f, false);
    B.gf (id::voices, "Polyphony", "Voices", 1, 32, 16, "int", "Maximum simultaneous notes.", -1e9f, false, 1.0f);
    B.gc (id::vmode, "Voice Mode", "Mode", { "Poly", "Mono", "Legato" }, 0, "Poly plays chords. Mono plays one note; Legato only retriggers envelopes when you lift between notes.");
    B.gf (id::glide, "Glide", "Glide", 0, 2, 0, "s", "Portamento time.", 0.3f, false);
    B.gc (id::glideMode, "Glide Mode", "Glide Mode", { "Always", "Legato Only" }, 0, "When glide happens.");
    B.gf (id::pbUp, "Bend Up", "Bend +", 0, 24, 2, "st", "Pitch bend range up.", -1e9f, false, 1.0f);
    B.gf (id::pbDown, "Bend Down", "Bend -", 0, 24, 2, "st", "Pitch bend range down.", -1e9f, false, 1.0f);
    B.gf (id::transpose, "Transpose", "Transpose", -24, 24, 0, "st", "Global transpose.", -1e9f, false, 1.0f);

    // arpeggiator
    B.gb (id::arpOn, "Arp On", "Arp", false, "Turns the arpeggiator on. Held notes are played one at a time in the chosen order.");
    B.gc (id::arpMode, "Arp Mode", "Order", { "Up", "Down", "Up Down", "Down Up", "As Played", "Random", "Chord" }, 0, "Note order.");
    B.gf (id::arpOct, "Arp Octaves", "Octaves", 1, 4, 1, "int", "Octave range.", -1e9f, false, 1.0f);
    B.gc (id::arpDiv, "Arp Rate", "Rate", divisionNames(), 13, "Step length.");
    B.gf (id::arpSteps, "Arp Steps", "Steps", 1, kArpSteps, 8, "int", "Pattern length in steps.", -1e9f, false, 1.0f);
    B.gf (id::arpSwing, "Arp Swing", "Swing", 0, 0.75f, 0, "%", "Delays every second step.", -1e9f, false);
    B.gf (id::arpGate, "Arp Gate", "Gate", 0.05f, 1.5f, 0.8f, "%", "Global note length (multiplies each step's gate).", -1e9f, false);
    B.gb (id::arpHold, "Arp Hold", "Hold", false, "Latch: keeps arpeggiating after you lift your hands. Play a new chord to replace it.");
    for (int s = 0; s < kArpSteps; ++s)
    {
        const auto S = "Arp Step " + juce::String (s + 1) + " ";
        B.gb (id::arpStep (s, "on"), S + "On", "On", true, "Step on or rest.");
        B.gf (id::arpStep (s, "tr"), S + "Transpose", "Trans", -24, 24, 0, "st", "Step transposition in semitones.", -1e9f, false, 1.0f);
        B.gf (id::arpStep (s, "vel"), S + "Velocity", "Vel", 0, 1, 0.8f, "%", "Step velocity.", -1e9f, false);
        B.gf (id::arpStep (s, "gate"), S + "Gate", "Gate", 0, 1, 0.8f, "%", "Step length. 100% ties into the next step (legato).", -1e9f, false);
    }

    // XY vector mixer (appended last so older sessions keep their parameter order)
    const char* axisName[] { "X", "Y" };
    for (int n = 0; n < kNumXY; ++n)
    {
        const auto P = "XY " + juce::String (n + 1) + " ";
        for (int a = 0; a < 2; ++a)
            B.gf (id::xyPos (n, a), P + axisName[a], axisName[a], 0, 1, 0.5f, "%",
                  juce::String ("Pad position on the ") + (a == 0 ? "horizontal" : "vertical") + " axis. The centre leaves the sound as stored; "
                  "moving away pushes every target on this axis by up to its amount.", -1e9f, false).global = true;
        for (int a = 0; a < 2; ++a)
            for (int k = 0; k < kXYTargets; ++k)
            {
                const auto T = P + axisName[a] + " Target " + juce::String (k + 1) + " ";
                B.gc (id::xyTarget (n, a, k, "dst"), T + "Param", "Target", { "None" }, 0, "Parameter moved by this axis of the pad.");
                B.gf (id::xyTarget (n, a, k, "amt"), T + "Amount", "Amount", -1, 1, 0, "bp",
                      "How far the target moves when the pad is at the edge (negative moves it the other way).");
            }
    }
    // Haunt (performance settings: kept when presets change)
    B.gb (id::hauntOn, "Haunt", "Haunt", false, "Haunt: every unlocked control drifts slowly around its setting, each at its own speed, so the sound "
          "never quite repeats. Stored settings don't move; pale blue ghost pointers show where each control has wandered.");
    B.gf (id::hauntDepth, "Haunt Depth", "Depth", 0, 1, 0.35f, "%", "How far Haunt lets each control wander.", -1e9f, false);
    B.gf (id::hauntSpeed, "Haunt Speed", "Speed", 0, 1, 0.35f, "%", "How fast the controls wander.", -1e9f, false);
    B.gb (id::hauntFollow, "Haunt Follow", "Follow", false, "Haunt Follow: links the drift to your playing. Dig in (or push the mod wheel) and it "
          "wanders further; play softly and it settles home.");
}

Catalogue& cat()
{
    static Catalogue c = []
    {
        Catalogue k;
        Builder B { k.defs, Kind::Count, -1, {}, {} };
        voiceModules (B, k);
        modModules (B, k);
        fxModules (B, k);
        globalParams (B);
        // every effect parameter is evaluated once per block, not per voice
        for (auto& d : k.defs)
            if (d.kind != Kind::Count && (int) d.kind >= (int) Kind::ModFx) d.global = true;
        // matrix destinations
        k.destOf.assign (k.defs.size(), 0);
        juce::StringArray dn { "None" };
        for (int i = 0; i < (int) k.defs.size(); ++i)
            if (k.defs[(size_t) i].mod && k.defs[(size_t) i].type == ParamDef::Float)
            {
                k.dests.push_back (i);
                k.destOf[(size_t) i] = (int) k.dests.size();
                dn.add (k.defs[(size_t) i].name);
            }
        for (auto& d : k.defs)
            if ((d.id.startsWith ("mtx") || d.id.startsWith ("xy")) && d.id.endsWith ("_dst")) d.choices = dn;
        for (int i = 0; i < (int) k.defs.size(); ++i) k.byId[k.defs[(size_t) i].id] = i;
        return k;
    }();
    return c;
}
} // namespace

const std::vector<ParamDef>& params() { return cat().defs; }
int numParams() { return (int) cat().defs.size(); }
int paramIndex (const juce::String& s)
{
    auto& m = cat().byId;
    auto it = m.find (s);
    return it == m.end() ? -1 : it->second;
}
int moduleBase (Kind k, int inst) { return cat().base[(int) k][inst]; }
juce::String pid (Kind k, int inst, int local) { return cat().defs[(size_t) moduleBase (k, inst) + (size_t) local].id; }
juce::String tooltipFor (const juce::String& s)
{
    const int i = paramIndex (s);
    return i < 0 ? juce::String() : cat().defs[(size_t) i].tip;
}
const std::vector<int>& destParams() { return cat().dests; }
int destOfParam (int i) { return (i >= 0 && i < (int) cat().destOf.size()) ? cat().destOf[(size_t) i] : 0; }
juce::StringArray destNames()
{
    juce::StringArray dn { "None" };
    for (int i : cat().dests) dn.add (cat().defs[(size_t) i].name);
    return dn;
}

namespace id
{
juce::String slot (int l, int s) { return "slot_l" + juce::String (l + 1) + "s" + juce::String (s + 1); }
juce::String fxSlot (int b, int s) { return "fxslot_b" + juce::String (b + 1) + "s" + juce::String (s + 1); }
juce::String lane (int l, const char* w) { return "lane" + juce::String (l + 1) + "_" + w; }
juce::String mtx (int n, const char* w) { return "mtx" + juce::String (n + 1) + "_" + w; }
juce::String arpStep (int n, const char* w) { return "arp_s" + juce::String (n + 1) + "_" + w; }
juce::String auxRet (int a) { return "aux" + juce::String (a + 1) + "_ret"; }
juce::String macro (int m) { return juce::String ("macro_") + (char) ('a' + m); }
juce::String xyPos (int n, int a) { return "xy" + juce::String (n + 1) + (a == 0 ? "_x" : "_y"); }
juce::String xyTarget (int n, int a, int k, const char* w)
{
    return "xy" + juce::String (n + 1) + (a == 0 ? "_x" : "_y") + juce::String (k + 1) + "_" + w;
}
}

const std::vector<GridEntry>& gridEntries()
{
    static std::vector<GridEntry> e = []
    {
        std::vector<GridEntry> v { { Kind::Count, -1 } };
        for (auto k : { Kind::Osc, Kind::FM, Kind::Noise, Kind::Exc, Kind::Res, Kind::Comb, Kind::Vcf, Kind::Shape, Kind::Ring, Kind::Eq })
            for (int i = 0; i < kindCount (k); ++i) v.push_back ({ k, i });
        return v;
    }();
    return e;
}

const std::vector<GridEntry>& fxEntries()
{
    static std::vector<GridEntry> e = []
    {
        std::vector<GridEntry> v { { Kind::Count, -1 } };
        for (auto k : { Kind::ModFx, Kind::Delay, Kind::Reverb, Kind::Texture, Kind::Comp, Kind::FxEq, Kind::Dist, Kind::FxFilter })
            for (int i = 0; i < kindCount (k); ++i) v.push_back ({ k, i });
        return v;
    }();
    return e;
}

juce::StringArray gridChoiceNames()
{
    juce::StringArray s;
    for (auto& e : gridEntries()) s.add (e.inst < 0 ? juce::String ("Empty") : moduleName (e.kind, e.inst));
    return s;
}

juce::StringArray fxChoiceNames()
{
    juce::StringArray s;
    for (auto& e : fxEntries()) s.add (e.inst < 0 ? juce::String ("Empty") : moduleName (e.kind, e.inst));
    return s;
}
}
