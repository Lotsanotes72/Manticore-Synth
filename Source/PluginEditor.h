#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Pages.h"

namespace mc
{
class ManticoreEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int kBaseW = 1400, kBaseH = 944;

    explicit ManticoreEditor (ManticoreProcessor&);
    ~ManticoreEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

    // used by the screenshot tests
    void setPage (int p);
    void selectModule (Kind k, int inst);
    void selectModTab (int t) { rack.select (t); }
    void tickForTests() { timerCallback(); }

private:
    void timerCallback() override;
    void savePresetDialog();
    void setStatus (const juce::String& s, int ticks) { status = s; statusTicks = ticks; repaint (statusArea); }
    void refreshAB();
    void runTool (bool ok, const juce::String& msg);
    void sweepGhosts (juce::Component&, bool clear);

    ManticoreProcessor& proc;
    ui::McLookAndFeel lnf;
    juce::TooltipWindow tips { this, 600 };
    ui::LiveParams live;
    ui::Ctx ctx;
    ui::SynthPage synth;
    ui::ModRack rack;
    ui::FxPage fx;
    ui::ArpPage arp;
    ui::XyPage xy;
    ui::Browser browser;

    juce::TextButton synthTab { "SYNTH" }, fxTab { "EFFECTS" }, arpTab { "ARP" }, xyTab { "XY" }, browseTab { "BROWSE" };
    ui::Action prevBtn { "<", ui::col::teal, "Previous preset (in the browser's current list)." };
    ui::Action nextBtn { ">", ui::col::teal, "Next preset (in the browser's current list)." };
    ui::Action presetBtn { "Init", ui::col::teal, "Current preset. Click to open the preset browser." };
    ui::StarButton favBtn;
    ui::Action saveBtn { "SAVE", ui::col::teal, "Save the current sound as a user preset (Documents\\Manticore\\Presets)." };
    ui::Action initBtn { "INIT", ui::col::teal, "Start from the basic Init patch: one oscillator into one filter." };
    ui::Action aBtn { "A", ui::col::amber, "A/B comparison: switch to setting A." };
    ui::Action bBtn { "B", ui::col::amber, "A/B comparison: switch to setting B." };
    ui::Action copyBtn { "A > B", ui::col::amber, "Copy the current setting into the other A/B slot." };
    ui::Action undoBtn { "UNDO", ui::col::teal, "Undo: step back through preset changes, edits, Mutate, Breed and Capture (48 steps)." };
    ui::Action redoBtn { "REDO", ui::col::teal, "Redo: step forward again." };

    // ---- creature toolbar
    ui::Action mutateBtn { "MUTATE", ui::col::amber, "Mutate: a random variation of the whole sound at the chosen Strength. Pitch is protected, "
                                                     "padlocked sections are left alone, macro routings are kept and the level is matched." };
    juce::Slider mutStr { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    ui::Action breedBtn { "BREED", ui::col::amber, "Breed: a new sound made from slots A and B. Each section comes from one parent with some values "
                                                   "blended towards the other, and the child replaces the current slot. Padlocked sections stay as they are." };
    ui::Action breedFavBtn { "BREED", ui::col::amber, "Breed with a Favourite: puts a random starred preset into the other slot and breeds it with the "
                                                      "current sound." };
    ui::Toggle hauntBtn { "HAUNT", ui::col::ghost };
    juce::Slider hauntDepth { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    juce::Slider hauntSpeed { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAtt, speedAtt;
    ui::Toggle followBtn { "FOLLOW", ui::col::ghost };
    ui::Action captureBtn { "CAPTURE", ui::col::ghost, "Capture: keeps what Haunt is playing right now as the new knob settings, then turns Haunt off." };

    juce::Rectangle<int> emblemArea, titleArea, statusArea, toolArea, strCap, depthCap, speedCap;
    std::vector<int> toolSeps;
    bool ghostsShown = false;
    juce::String status, lastPreset;
    int statusTicks = 0, page = 0, lastMatrixHash = 0, lastShapes = -1;
};
}
