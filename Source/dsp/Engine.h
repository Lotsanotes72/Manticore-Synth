#pragma once
#include "Voice.h"
#include "Effects.h"
#include "Arp.h"

namespace mc::dsp
{
class Engine
{
public:
    Engine();
    void bind (juce::AudioProcessorValueTreeState&);
    void prepare (double sampleRate, int maxBlock);
    void reset();
    void process (juce::AudioBuffer<float>&, juce::MidiBuffer&, juce::AudioPlayHead*);

    // Shape data from the editor (message thread). Picked up by the audio thread without blocking.
    void setMseg (int i, const MsegData& d);
    void setMap (int i, const MapData& d);

    std::atomic<float> peakL { 0 }, peakR { 0 };
    std::atomic<int> voicesPlaying { 0 };
    std::atomic<float> lfoView[4] {};

    // for the offline tests
    const Shared& shared() const { return sh; }

private:
    struct Chunk { int pos, len; float lfo[4]; };
    void updateShared();
    void computeGlobalMod (const float* lfo);
    void advanceGlobalLfos (int n);
    void renderVoices (int from, int to);
    void noteOn (int note, float vel);
    void noteOff (int note);
    void allNotesOff();
    int findVoice (int note);

    Shared sh;
    std::vector<std::atomic<float>*> raw;
    std::array<Voice, kMaxVoices> voices;
    FxRack fx;
    Arp arp;
    std::vector<Arp::Event> arpEvents;
    struct Ev { int pos; int type; int a; float b; };   // 0 on, 1 off, 3 bend, 4 mod wheel, 5 expression, 6 sustain, 7 all off, 8 AT, 9 poly AT
    std::vector<Ev> evs;
    std::vector<float> gmod;
    std::vector<int> gDest;
    struct GSlot { int src, via, dst; float amt; };
    std::vector<GSlot> gSlots;
    juce::AudioBuffer<float> bus;
    float* busPtr[8] {};
    std::vector<Chunk> chunks;

    juce::SpinLock dataLock;
    MsegData pendingMseg[4];
    MapData pendingMap[4];
    std::atomic<bool> dataDirty { true };

    LfoState gLfo[4];
    Rng gRng { 4242 };
    uint64_t ageCounter = 0;
    uint32_t seedCounter = 1;
    float lastNote = -1;
    std::vector<int> monoStack;
    bool sustainPedal = false, lastArpOn = false;
    float altFlip = 1.0f;
    float masterPrev = -1;
    int maxVoices = 16;
    struct Idx { int master, glide, voices, vmode, glideMode, pbUp, pbDown, transpose, arpOn, arpHold, macro[4], slot[4][4], fxSlot[4][4],
                 mtxSrc[kNumMatrix], mtxVia[kNumMatrix], mtxDst[kNumMatrix], mtxAmt[kNumMatrix], lfoBase[4]; } ix {};
};
}
