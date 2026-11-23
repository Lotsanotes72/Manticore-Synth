#pragma once
#include "Displays.h"

namespace mc::ui
{
// Everything the pages share.
struct Ctx
{
    ManticoreProcessor& proc;
    LiveParams& live;
    std::function<void (Kind, int)> selectModule;       // generator/modulator/fx module chosen in a grid or tab
    std::function<void (const juce::String&, int)> status;
};

// ------------------------------------------------------------------ padlock (Mutate / Breed / Haunt leave a locked section alone)
class LockButton : public juce::Button
{
public:
    LockButton (ManticoreProcessor& p, int bit, const juce::String& label, const juce::String& what);
    void paintButton (juce::Graphics&, bool over, bool down) override;
    void clicked() override;
    int preferredWidth (float s) const;
private:
    ManticoreProcessor& proc;
    int bit;
    juce::String label;
};

// ------------------------------------------------------------------ generic module panel (data-driven)
class ModuleEditor : public juce::Component
{
public:
    ModuleEditor (Ctx&, bool withDisplay = true);
    void setModule (Kind, int inst);
    Kind getKind() const { return kind; }
    int getInst() const { return inst; }
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
    void refreshModulation();
private:
    Ctx& ctx;
    bool withDisplay;
    Kind kind = Kind::Count;
    int inst = -1;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::Component>> tops;       // choices and toggles
    std::vector<int> knobLocals;
    std::vector<int> rowSizes;
    ModuleDisplay display;
    MsegEditor mseg;
    MapEditor mapEd;
    Action mutBtn { "MUTATE", col::amber, "Mutate this module only: a random variation of its settings at the toolbar's Strength. "
                                           "Pitch stays protected and the level is matched. Padlocks don't apply here, since you picked the module." };
    juce::Rectangle<int> titleArea;
};

// ------------------------------------------------------------------ grid tiles
class SlotTile : public juce::Component, public juce::SettableTooltipClient
{
public:
    SlotTile (Ctx&, bool fx, int lane, int slot);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void setSelected (bool b) { if (b != selected) { selected = b; repaint(); } }
    int entry() const;
    std::function<void()> onPick;
private:
    void showMenu();
    Ctx& ctx;
    bool fx;
    int lane, slot;
    juce::String pid;
    bool selected = false;
};

class GridPanel : public juce::Component
{
public:
    GridPanel (Ctx&, bool fx);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick (Kind selKind, int selInst);
private:
    Ctx& ctx;
    bool fx;
    juce::OwnedArray<SlotTile> tiles;
    juce::OwnedArray<Choice> inputs;
    juce::OwnedArray<LockButton> locks;      // grid: layout + lanes 1-4; fx: effects
    std::array<int, 16> lastEntries {};
};

// ------------------------------------------------------------------ synth page
class SynthPage : public juce::Component
{
public:
    explicit SynthPage (Ctx&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
    void showModule (Kind, int);
    void refreshModulation() { editor.refreshModulation(); rackRefresh(); }
    std::function<void()> rackRefresh = [] {};
    juce::Component& rackHolder() { return rack; }
    PeakMeter meterL, meterR;
private:
    Ctx& ctx;
    GridPanel grid;
    ModuleEditor editor;
    juce::Component rack;              // the ModRack is added by the editor
    juce::OwnedArray<Knob> laneKnobs, macroKnobs, voiceKnobs;
    juce::OwnedArray<Choice> voiceChoices;
    juce::Rectangle<int> gridArea, laneArea, macroArea, voiceArea, editorArea;
    juce::Rectangle<int> laneCols[4];
};

// ------------------------------------------------------------------ modulation rack
class MatrixPanel : public juce::Component
{
public:
    explicit MatrixPanel (Ctx&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    Ctx& ctx;
    juce::OwnedArray<Choice> srcs, vias;
    juce::OwnedArray<DestPicker> dsts;
    juce::OwnedArray<juce::Slider> amts;
    std::vector<std::unique_ptr<APVTS::SliderAttachment>> amtAtt;
};

class ModRack : public juce::Component
{
public:
    explicit ModRack (Ctx&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
    void select (int tab);        // 0..19 modulators, 20 matrix
    void refreshModulation() { editor.refreshModulation(); }
private:
    Ctx& ctx;
    juce::OwnedArray<juce::TextButton> fams, nums;
    ModuleEditor editor;
    MatrixPanel matrix;
    LockButton modLock, mtxLock;
    int current = 0, lastNum = 0;
};

// ------------------------------------------------------------------ FX page
class FlowPanel : public juce::Component
{
public:
    explicit FlowPanel (Ctx& c) : ctx (c) {}
    void paint (juce::Graphics&) override;
private:
    Ctx& ctx;
};

class FxPage : public juce::Component
{
public:
    explicit FxPage (Ctx&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
    void showFx (Kind, int);
private:
    Ctx& ctx;
    GridPanel grid;
    ModuleEditor editor;
    FlowPanel flow;
    juce::OwnedArray<Knob> returns, sends;
    juce::Rectangle<int> gridArea, retArea, sendArea, editorArea;
};

// ------------------------------------------------------------------ arpeggiator
class StepBar : public juce::Component, public juce::SettableTooltipClient
{
public:
    StepBar (Ctx&, const juce::String& pid, bool bipolar, juce::Colour c);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    bool active = true;
private:
    void setFrom (const juce::MouseEvent&);
    Ctx& ctx;
    juce::String pid;
    bool bipolar;
    juce::Colour colour;
    bool gesture = false;
};

class ArpPage : public juce::Component
{
public:
    explicit ArpPage (Ctx&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
private:
    Ctx& ctx;
    Toggle on { "ARP ON", col::amber }, hold { "HOLD", col::teal };
    LockButton arpLock;
    juce::OwnedArray<Choice> choices;
    juce::OwnedArray<Knob> knobs;
    juce::OwnedArray<Toggle> stepOn;
    juce::OwnedArray<StepBar> trBars, velBars, gateBars;
    juce::Rectangle<int> topArea, gridArea, rowLabels[4], helpArea;
};

// ------------------------------------------------------------------ XY vector mixer
class XyPad : public juce::Component, public juce::SettableTooltipClient
{
public:
    XyPad (Ctx&, int pad);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void tick();
private:
    void setFrom (juce::Point<float>);
    juce::Rectangle<float> padArea() const;
    Ctx& ctx;
    int pad;
    juce::Point<float> last { -1, -1 }, lastGhost { -9, -9 };
};

class XyPage : public juce::Component
{
public:
    explicit XyPage (Ctx&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void tick();
private:
    Ctx& ctx;
    juce::OwnedArray<XyPad> pads;
    juce::OwnedArray<DestPicker> dsts;
    juce::OwnedArray<juce::Slider> amts;
    std::vector<std::unique_ptr<APVTS::SliderAttachment>> amtAtt;
    LockButton xyLock;
    juce::Rectangle<int> headArea, padAreas[kNumXY];
};

// ------------------------------------------------------------------ preset browser
class Browser : public juce::Component, private juce::ListBoxModel, private juce::TextEditor::Listener
{
public:
    explicit Browser (Ctx&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();                       // rebuild the filtered list
    void step (int dir);                  // previous / next preset
    std::function<void()> onLoaded;
    std::function<void()> onSave;
private:
    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, juce::Graphics&, int w, int h, bool selected) override;
    void listBoxItemClicked (int row, const juce::MouseEvent&) override;
    void selectedRowsChanged (int row) override;
    void returnKeyPressed (int row) override { load (row); }
    void textEditorTextChanged (juce::TextEditor&) override { refresh(); }
    void load (int row);
    Ctx& ctx;
    juce::ListBox list;
    juce::TextEditor search;
    juce::OwnedArray<juce::TextButton> cats, genres;
    Action loadBtn { "LOAD", col::teal, "Load the selected preset." }, saveBtn { "SAVE AS...", col::amber, "Save the current sound as a user preset." };
    StarButton favBtn;
    std::vector<PresetInfo> all, shown;
    juce::String category { "All" };
    juce::Rectangle<int> leftArea, infoArea;
    int infoRow = -1;
    bool suppressLoad = false;
};
}
