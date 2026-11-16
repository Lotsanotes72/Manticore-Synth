#pragma once
#include "Widgets.h"
#include "../PluginProcessor.h"

namespace mc::ui
{
// Mirror of the parameter values for the editor's previews (refreshed by the editor timer).
struct LiveParams
{
    dsp::Shared sh;
    LiveParams();
    bool refresh (APVTS&);       // true if anything changed
    float v (const juce::String& id) const;
    float v (Kind k, int inst, int local) const { return sh.val[(size_t) pidx (k, inst, local)]; }
    int c (Kind k, int inst, int local) const { return juce::roundToInt (v (k, inst, local)); }
    int version = 0;
};

// Plot shown at the left of a module panel. Draws whichever module it is pointed at.
class ModuleDisplay : public juce::Component
{
public:
    ModuleDisplay (ManticoreProcessor& p, LiveParams& lp) : proc (p), live (lp) {}
    void setModule (Kind k, int i) { kind = k; inst = i; repaint(); }
    void paint (juce::Graphics&) override;
    void tick();                 // called by the editor timer
private:
    void drawWave (juce::Graphics&, juce::Rectangle<float>, const std::vector<float>&, juce::Colour, float thickness = 2.0f);
    void drawResponse (juce::Graphics&, juce::Rectangle<float>, const std::function<float (float)>& magDb, float lo, float hi, juce::Colour);
    void drawTransfer (juce::Graphics&, juce::Rectangle<float>, const std::function<float (float)>& f, juce::Colour);
    ManticoreProcessor& proc;
    LiveParams& live;
    Kind kind = Kind::Osc;
    int inst = 0;
    int lastVersion = -1;
    float lfoDot = 0;
};

// Drawable multi-segment envelope.
class MsegEditor : public juce::Component, public juce::SettableTooltipClient
{
public:
    MsegEditor (ManticoreProcessor& p, LiveParams& lp);
    void setIndex (int i) { index = i; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
private:
    juce::Rectangle<float> plot() const;
    float viewLen() const;
    juce::Point<float> toScreen (float x, float y) const;
    juce::Point<float> fromScreen (juce::Point<float>) const;
    int hitPoint (juce::Point<float>) const;
    int hitHandle (juce::Point<float>) const;
    void changed();
    void templates();
    ManticoreProcessor& proc;
    LiveParams& live;
    int index = 0, drag = -1, dragHandle = -1, hover = -1;
    float dragStartC = 0, dragStartY = 0, frozenLen = 4;
};

// Drawable mapper / step-sequencer bars.
class MapEditor : public juce::Component, public juce::SettableTooltipClient
{
public:
    MapEditor (ManticoreProcessor& p, LiveParams& lp);
    void setIndex (int i) { index = i; repaint(); }
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
private:
    juce::Rectangle<float> plot() const;
    void setAt (juce::Point<float>);
    void templates();
    ManticoreProcessor& proc;
    LiveParams& live;
    int index = 0, lastCell = -1;
    float lastVal = 0;
};
}
