#pragma once
#include "VoiceModules.h"

namespace mc::dsp
{
struct EnvState { int stage = 0; float pos = 0, level = 0, from = 0; };
struct LfoState { double phase = 0; float prev = 0, next = 0, held = 0, value = 0; bool started = false; };
struct MsegState { float t = 0; };
struct MapState { double stepPos = 0; float value = 0; };

// One LFO cycle value for a given phase (shared by voice and global LFOs)
float lfoShape (int wave, double phase, LfoState& st, bool wrapped, Rng& rng);

class Voice
{
public:
    void prepare (double sampleRate);
    void start (int note, float velocity, float fromNote, bool retrigger, uint32_t seed, float alternate);
    void glideTo (int note, float velocity, bool retriggerEnvs);
    void release();
    void kill();
    bool isActive() const { return active; }
    bool isGateOn() const { return gate; }

    // bus: 8 channel pointers (main L/R, aux1 L/R, aux2 L/R, aux3 L/R), already offset to the render start
    void render (const Shared&, float* const* bus, int n);

    int note = -1;
    float velocity = 0, polyAT = 0;
    uint64_t age = 0;
    bool sustained = false;     // held by the sustain pedal after note-off
    float ampLevel() const { return env[0].level; }

private:
    void renderBlock (const Shared&, float* const* bus, int offset, int n);
    void computeModulators (const Shared&, int n, float* env1Buf);

    double sr = 48000.0;
    bool active = false, gate = false, trigger = false;
    float curNote = 60, targetNote = 60, noteTime = 0;
    float randomVal = 0, alternate = 1;
    uint32_t seed = 1;
    Rng rng { 5 };

    EnvState env[4];
    LfoState lfo[4];
    MsegState mseg[4];
    MapState map[4];
    float mathLag[4] {};
    float src[sNumSources] {};
    std::vector<float> mod;

    OscState osc[4];
    FmState fm[4];
    NoiseState noise[2];
    ExcState exc[2];
    ResState res[2];
    CombState comb[2];
    VcfState vcf[4];
    ShapeState shape[4];
    RingState ring[2];
    EqState eq[2];

    float laneOut[kNumLanes][2][kBlock] {};
    float laneGain[kNumLanes][5] {};     // previous block: L, R, aux1, aux2, aux3
    bool laneGainInit = false;
};
}
