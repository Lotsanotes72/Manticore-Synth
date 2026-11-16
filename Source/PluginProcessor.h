#pragma once
#include <JuceHeader.h>
#include "Params.h"
#include "Presets.h"
#include "dsp/Engine.h"

namespace mc
{
class ManticoreProcessor : public juce::AudioProcessor
{
public:
    ManticoreProcessor();
    ~ManticoreProcessor() override = default;

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
    void shapesChanged();                                // push to the engine
    std::atomic<int> shapesVersion { 0 };

    // ---- A/B and undo
    void abSelect (int slot);
    void abCopyToOther();
    int abCurrent() const { return abSlot; }

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    juce::ValueTree uiState { "UI" };
    dsp::Engine engine;

    static void resetShapes (dsp::MsegData*, dsp::MapData*);
    juce::ValueTree makeStateTree();
    void restoreStateTree (const juce::ValueTree&);

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void resetAllParamsToDefault();
    juce::ValueTree shapesToTree() const;
    void shapesFromTree (const juce::ValueTree&);

    int currentProgram = 0;
    juce::String presetName { "Init" }, presetKey { "" };
    juce::ValueTree abStore[2];
    int abSlot = 0;
};
}
