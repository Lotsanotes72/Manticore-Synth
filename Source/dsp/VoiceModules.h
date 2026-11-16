#pragma once
#include "Common.h"

namespace mc::dsp
{
// Everything a grid module needs to know about the voice it is running in.
struct VoiceCtx
{
    const Shared& sh;
    const float* mod;          // per-voice matrix offsets (index = dest)
    float note;                // current pitch in MIDI notes (glide, bend and transpose applied)
    float velocity;
    float noteTime;            // seconds since note-on
    bool gate;
    bool trigger;              // true for the first block after a (re)trigger
    const float (*laneOut)[2][kBlock];
    int lane;
    uint32_t seed;
};

inline float keyedHz (float note, float keytrack, float semis)
{
    return noteHz (60.0f + (note - 60.0f) * keytrack + semis);
}

// ------------------------------------------------------------ states
struct OscState
{
    double ph[16] {};
    float c[64] {}, s[64] {};
    float volPrev = 0;
    bool started = false;
    void reset() { started = false; }
};

struct FmState { double pc = 0, pm = 0; float m1 = 0, m2 = 0; DcBlock dc[2]; float volPrev = 0; };

struct NoiseState
{
    Rng rng { 7 };
    float pink[2][7] {}, brown[2] {}, blue[2] {}, tilt[2] {}, hold[2] {}, crackle[2] {};
    float holdCnt = 0;
};

struct ExcState
{
    Rng rng { 3 };
    float env = 0, t = 0, patT = 0, lp[2] {}, low[2] {}, fade = 0;
    bool fired = false;
};

struct ResState
{
    static constexpr int M = 12;
    float y1[2][M] {}, y2[2][M] {};
    float a[2][M] {}, b1[2][M] {}, b2[2][M] {};
    float dry = 1;
};

struct CombState
{
    static constexpr int L = 8192;
    std::vector<float> buf;   // 4 lines * L
    int w = 0;
    float lp[4] {};
    float dLen[4] {};
    void allocate() { buf.assign ((size_t) 4 * L, 0.0f); }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); for (auto& v : lp) v = 0; }
};

struct VcfState
{
    Svf s1[2], s2[2], f[2][3];
    float lad[2][4] {}, ladOut[2] {};
    float one[2] {}, one2[2] {};
    float ap[2][12] {}, apFb[2] {};
    float gPrev[2] { -1, -1 };
    void reset() { *this = VcfState(); }
};

struct ShapeState { float lp[2] {}, hold[2] {}; float holdCnt = 0; DcBlock dc[2]; };
struct RingState { double ph = 0; };
struct EqState { Biquad lo, mid, hi; float cache[7] { -1, -1, -1, -1, -1, -1, -1 }; };

// ------------------------------------------------------------ processors (operate in place on the lane buffer)
void processOsc (OscState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processFm (FmState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processNoise (NoiseState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processExc (ExcState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processRes (ResState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processComb (CombState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processVcf (VcfState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processShape (ShapeState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processRing (RingState&, const P&, const VoiceCtx&, float* L, float* R, int n);
void processEq (EqState&, const P&, const VoiceCtx&, float* L, float* R, int n);

// Helpers shared with the UI previews
float oscPhaseFx (int type, float a, float p, float& win);
float oscOutFx (int type, float a, float y, float p);
float oscFxMipMul (int type, float a);
void renderOscCycle (const Shared&, int oscInst, float* out, int n);     // one cycle at the current settings (for the display)
const float* resonatorRatios (int profile);
const float* resonatorGains (int profile);
}
