#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/Pages.h"

namespace mc
{
class ManticoreEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int kBaseW = 1400, kBaseH = 900;

    explicit ManticoreEditor (ManticoreProcessor&);
    ~ManticoreEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // used by the screenshot tests
    void setPage (int p);
    void selectModule (Kind k, int inst);
    void selectModTab (int t) { rack.select (t); }

private:
    void timerCallback() override;
    void savePresetDialog();
    void setStatus (const juce::String& s, int ticks) { status = s; statusTicks = ticks; repaint (statusArea); }
    void refreshAB();

    ManticoreProcessor& proc;
    ui::McLookAndFeel lnf;
    juce::TooltipWindow tips { this, 600 };
    ui::LiveParams live;
    ui::Ctx ctx;
    ui::SynthPage synth;
    ui::ModRack rack;
    ui::FxPage fx;
    ui::ArpPage arp;
    ui::Browser browser;

    juce::TextButton synthTab { "SYNTH" }, fxTab { "EFFECTS" }, arpTab { "ARP" }, browseTab { "BROWSE" };
    ui::Action prevBtn { "<", ui::col::teal, "Previous preset (in the browser's current list)." };
    ui::Action nextBtn { ">", ui::col::teal, "Next preset (in the browser's current list)." };
    ui::Action presetBtn { "Init", ui::col::teal, "Current preset. Click to open the preset browser." };
    ui::StarButton favBtn;
    ui::Action saveBtn { "SAVE", ui::col::teal, "Save the current sound as a user preset (Documents\\Manticore\\Presets)." };
    ui::Action initBtn { "INIT", ui::col::teal, "Start from the basic Init patch: one oscillator into one filter." };
    ui::Action aBtn { "A", ui::col::amber, "A/B comparison: switch to setting A." };
    ui::Action bBtn { "B", ui::col::amber, "A/B comparison: switch to setting B." };
    ui::Action copyBtn { "A > B", ui::col::amber, "Copy the current setting into the other A/B slot." };
    ui::Action undoBtn { "UNDO", ui::col::teal, "Undo the last change." };
    ui::Action redoBtn { "REDO", ui::col::teal, "Redo the last undone change." };

    juce::Rectangle<int> emblemArea, titleArea, statusArea;
    juce::String status, lastPreset;
    int statusTicks = 0, page = 0, lastMatrixHash = 0, lastShapes = -1;
};
}
