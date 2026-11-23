#pragma once
#include <JuceHeader.h>
#include "Params.h"
#include "Presets.h"
#include "Evolve.h"
#include "dsp/Engine.h"

namespace mc
{
class ManticoreProcessor : public juce::AudioProcessor,
                           private juce::AudioProcessorParameter::Listener,
                           private juce::Timer
{
public:
    ManticoreProcessor();
    ~ManticoreProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Manticore"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return juce::jmax (1, PresetBank::get().numFactory()); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override { loadFactoryPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- presets
    void loadFactoryPreset (int index);
    bool loadUserPreset (const juce::File&);
    bool saveUserPreset (const juce::String& name, const juce::String& category, const juce::String& tags, const juce::String& desc);
    void loadInit();
    juce::String currentPresetName() const { return presetName; }
    juce::String currentPresetKey() const { return presetKey; }
    void setPresetName (const juce::String& n) { presetName = n; }
    void applyPresetVar (const juce::var& v);           // factory JSON object

    // ---- shapes (MSEG curves and mapper bars), edited on the message thread
    dsp::MsegData mseg[4];
    dsp::MapData map[4];
    void shapesChanged();                                // an edit from the UI: push to the engine and remember it for Undo
    std::atomic<int> shapesVersion { 0 };

    // ---- A/B
    void abSelect (int slot);
    void abCopyToOther();
    int abCurrent() const { return abSlot; }
    juce::String abName (int slot) const;

    // ---- Undo / Redo (48 steps: preset changes, edits, Mutate, Breed, Capture)
    void pushHistory();
    bool canUndo() const { return histIndex > 0; }
    bool canRedo() const { return histIndex >= 0 && histIndex < (int) history.size() - 1; }
    void undo();
    void redo();

    // ---- padlocks
    bool isLocked (int bit) const { return mc::locked (lockBits, bit); }
    void setLocked (int bit, bool on);
    uint32_t locks() const { return lockBits; }

    // ---- Mutate / Breed / Haunt
    float mutateStrength() const { return (float) uiState.getProperty ("mutStr", 0.35f); }
    void setMutateStrength (float s) { uiState.setProperty ("mutStr", juce::jlimit (0.0f, 1.0f, s), nullptr); }
    bool mutate (juce::String& message);                                  // the whole patch, padlocks respected
    bool mutateModule (Kind, int inst, juce::String& message);            // one module, padlocks ignored
    bool breed (juce::String& message);                                   // current slot x other slot -> current slot
    bool breedWithFavourite (juce::String& message);                      // a random starred preset goes into the other slot first
    bool capture (juce::String& message);                                 // keep what Haunt is playing now
    bool isHaunting() const;

    juce::AudioProcessorValueTreeState apvts;
    juce::ValueTree uiState { "UI" };
    dsp::Engine engine;

    static void resetShapes (dsp::MsegData*, dsp::MapData*);
    juce::ValueTree makeStateTree();
    void restoreStateTree (const juce::ValueTree&, bool keepPerform = true);

    // ---- patch snapshots (used by the tools and the tests)
    Patch currentPatch() const;
    void applyPatch (const Patch&);
    static Patch patchFromTree (const juce::ValueTree&);
    static Patch patchFromPresetVar (const juce::var&);
    juce::ValueTree treeFromPatch (const Patch&, const juce::String& name, const juce::String& key);
    struct Loudness
    {
        float loud = -120.0f, peak = 0.0f;
        bool finite = true;
        bool audible() const { return finite && loud > -55.0f; }
        // a candidate close enough to the reference to be level-matched cleanly (no collapse, no runaway feedback)
        bool acceptableAgainst (float ref) const
        {
            if (! audible() || peak >= 4.0f || loud < ref - 15.0f || loud > ref + 12.0f) return false;
            return loud + 20.0f * std::log10 (0.89f / juce::jmax (1.0e-6f, peak)) > ref - 2.0f;   // can reach the target without clipping
        }
    };
    Loudness measure (const Patch&);
    void levelMatch (Patch&, float targetDb, const Loudness& now);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::ValueTree shapesToTree() const;
    static void shapesFromTree (const juce::ValueTree&, dsp::MsegData*, dsp::MapData*);
    void pushShapes();
    bool finishTool (Patch& cand, const Loudness& base, float target, const juce::String& newName);

    void parameterValueChanged (int, float) override {}
    void parameterGestureChanged (int, bool starting) override;
    void timerCallback() override;

    int currentProgram = 0;
    juce::String presetName { "Init" }, presetKey { "" };
    juce::ValueTree abStore[2];
    int abSlot = 0;

    std::vector<juce::ValueTree> history;
    int histIndex = -1;
    std::atomic<int> gestures { 0 };
    std::atomic<bool> gestureEnded { false };
    bool shapeEdited = false;
    double lastShapeEdit = 0;

    uint32_t lockBits = 0;
    juce::Random rng;

    // offline measuring engine (message thread only)
    std::unique_ptr<dsp::Engine> meter;
    std::unique_ptr<std::vector<std::atomic<float>>> meterVals;
    uint64_t cachedHash = 0;
    Loudness cachedLoud;
};
}
