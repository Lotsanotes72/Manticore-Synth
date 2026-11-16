#pragma once
#include "Common.h"

namespace mc::dsp
{
// Step arpeggiator. Fed with the player's notes, it emits its own note-ons and note-offs.
class Arp
{
public:
    struct Event { int pos; int note; float vel; bool on; };

    void reset()
    {
        held.clear(); latched.clear(); keysDown = 0;
        nSounding = 0; stepK = 0; running = false; idx = 0; dir = 1;
    }

    void noteOn (int note, float vel, bool hold)
    {
        if (hold && keysDown == 0) latched.clear();
        ++keysDown;
        lastVel = vel;
        remove (held, note);
        held.push_back (note);
        if (hold) { remove (latched, note); latched.push_back (note); }
    }

    void noteOff (int note)
    {
        keysDown = juce::jmax (0, keysDown - 1);
        remove (held, note);
    }

    // Generate events for n samples. beat0 = host beat at block start (or < 0 when the host is stopped).
    void process (const Shared& sh, int n, double beat0, std::vector<Event>& out)
    {
        const bool hold = sh.on (paramIndex (id::arpHold));
        const auto& pool = hold ? latched : held;
        const double bps = sh.beatsPerSample;
        const double SL = divisionBeats (sh.choice (paramIndex (id::arpDiv)));
        const double swing = sh.val[(size_t) paramIndex (id::arpSwing)];
        if (pool.empty())
        {
            if (nSounding > 0) releaseAll (0, out);
            running = false;
            return;
        }
        double beat;
        if (beat0 >= 0) beat = beat0;
        else
        {
            if (! running) internalBeat = 0;
            beat = internalBeat;
        }
        if (! running)
        {
            running = true;
            idx = 0; dir = 1;
            stepK = beat0 >= 0 ? (int64_t) std::ceil (beat / SL - 1e-9) : 0;
            offBeat = -1;
        }
        auto stepStart = [&] (int64_t k) { return (double) k * SL + ((k & 1) ? swing * SL : 0.0); };
        // re-sync after a host jump
        if (beat0 >= 0 && std::abs (stepStart (stepK) - beat) > SL * 2.0) stepK = (int64_t) std::ceil (beat / SL);
        for (int i = 0; i < n; ++i)
        {
            const double b = beat + (double) i * bps;
            if (nSounding > 0 && offBeat >= 0 && b >= offBeat) { releaseAll (i, out); offBeat = -1; }
            if (b >= stepStart (stepK))
            {
                triggerStep (sh, pool, i, stepStart (stepK), SL, out);
                ++stepK;
            }
        }
        if (beat0 < 0) internalBeat = beat + (double) n * bps;
    }

private:
    static void remove (std::vector<int>& v, int note) { v.erase (std::remove (v.begin(), v.end(), note), v.end()); }

    void releaseAll (int pos, std::vector<Event>& out)
    {
        for (int k = 0; k < nSounding; ++k) out.push_back ({ pos, sounding[k], 0.0f, false });
        nSounding = 0;
    }

    void triggerStep (const Shared& sh, const std::vector<int>& pool, int pos, double start, double SL, std::vector<Event>& out)
    {
        const int steps = juce::jlimit (1, kArpSteps, juce::roundToInt (sh.val[(size_t) paramIndex (id::arpSteps)]));
        const int s = (int) (stepK % steps);
        const bool on = sh.on (paramIndex (id::arpStep (s, "on")));
        const int tr = juce::roundToInt (sh.val[(size_t) paramIndex (id::arpStep (s, "tr"))]);
        const float vel = sh.val[(size_t) paramIndex (id::arpStep (s, "vel"))];
        const float gate = sh.val[(size_t) paramIndex (id::arpStep (s, "gate"))] * sh.val[(size_t) paramIndex (id::arpGate)];
        const int mode = sh.choice (paramIndex (id::arpMode));
        const int oct = juce::jlimit (1, 4, juce::roundToInt (sh.val[(size_t) paramIndex (id::arpOct)]));

        if (nSounding > 0) releaseAll (pos, out);
        if (! on) return;

        std::vector<int> notes (pool.begin(), pool.end());
        if (mode != 4) std::sort (notes.begin(), notes.end());
        const int N = (int) notes.size();
        if (mode == 6)
        {
            const int o = (int) (stepK % oct);
            for (int k = 0; k < N && k < 16; ++k)
            {
                const int nn = juce::jlimit (0, 127, notes[(size_t) k] + 12 * o + tr);
                out.push_back ({ pos, nn, vel, true });
                sounding[nSounding++] = nn;
            }
        }
        else
        {
            const int total = N * oct;
            int pick = 0;
            switch (mode)
            {
                case 0: case 4: pick = idx % total; idx = (idx + 1) % total; break;
                case 1: pick = total - 1 - idx % total; idx = (idx + 1) % total; break;
                case 2: case 3:
                {
                    if (total == 1) { pick = 0; break; }
                    if (idx >= total) idx = total - 1;
                    pick = mode == 2 ? idx : total - 1 - idx;
                    idx += dir;
                    if (idx >= total - 1) { idx = total - 1; dir = -1; }
                    else if (idx <= 0) { idx = 0; dir = 1; }
                    break;
                }
                default: pick = (int) (rng.next() % (uint32_t) total); break;
            }
            const int nn = juce::jlimit (0, 127, notes[(size_t) (pick % N)] + 12 * (pick / N) + tr);
            out.push_back ({ pos, nn, vel, true });
            sounding[nSounding++] = nn;
        }
        offBeat = gate >= 0.99f ? -1.0 : start + SL * juce::jmax (0.02f, gate);
    }

    std::vector<int> held, latched;
    int keysDown = 0;
    float lastVel = 0.8f;
    int sounding[16] {};
    int nSounding = 0;
    int64_t stepK = 0;
    bool running = false;
    int idx = 0, dir = 1;
    double offBeat = -1, internalBeat = 0;
    Rng rng { 1234 };
};
}
