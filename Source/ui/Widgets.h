#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "../Params.h"

namespace mc::ui
{
using APVTS = juce::AudioProcessorValueTreeState;

float scaleOf (const juce::Component&);
inline juce::String dot() { return juce::String (juce::CharPointer_UTF8 ("\xc2\xb7")); }

// Rotary knob with a caption above and an editable value below (double-click the value to type).
// Right-click offers modulation routing; an amber arc shows any modulation applied through the matrix.
class Knob : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit Knob (const juce::String& caption = {});
    void attach (APVTS&, const juce::String& paramId, const juce::String& tip = {});
    void detach() { att.reset(); apvts = nullptr; }
    void setAccent (juce::Colour c) { slider.getProperties().set ("accent", (juce::int64) c.getARGB()); repaint(); }
    void setBipolar (bool b) { slider.getProperties().set ("bipolar", b); }
    void setCaption (const juce::String& c) { caption = c; repaint(); }
    void refreshModulation();           // re-read the matrix for this knob's target
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    juce::Slider slider;
    juce::String paramId;
    bool compact = false;               // smaller caption/value for dense rows
private:
    void showModMenu();
    juce::String caption;
    APVTS* apvts = nullptr;
    std::unique_ptr<APVTS::SliderAttachment> att;
};

class Toggle : public juce::TextButton
{
public:
    Toggle (const juce::String& text = {}, juce::Colour accent = col::teal);
    void attach (APVTS&, const juce::String& paramId, const juce::String& tip = {});
    void detach() { att.reset(); }
private:
    std::unique_ptr<APVTS::ButtonAttachment> att;
};

class Choice : public juce::ComboBox
{
public:
    Choice();
    void attach (APVTS&, const juce::String& paramId, const juce::String& tip = {});
    void detach() { att.reset(); }
private:
    std::unique_ptr<APVTS::ComboBoxAttachment> att;
};

class Action : public juce::TextButton
{
public:
    Action (const juce::String& text = {}, juce::Colour accent = col::teal, const juce::String& tip = {});
};

class StarButton : public juce::Button
{
public:
    StarButton() : juce::Button ("fav") { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool over, bool down) override;
};

// Picks a matrix destination from a menu grouped by module.
class DestPicker : public juce::Component, public juce::SettableTooltipClient, private juce::AudioProcessorValueTreeState::Listener, private juce::AsyncUpdater
{
public:
    DestPicker() = default;
    ~DestPicker() override;
    void attach (APVTS&, const juce::String& paramId);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
private:
    void parameterChanged (const juce::String&, float) override { triggerAsyncUpdate(); }
    void handleAsyncUpdate() override { repaint(); }
    APVTS* apvts = nullptr;
    juce::String pid;
};

void drawCaption (juce::Graphics&, juce::Rectangle<int> r, const juce::String& text, float s, juce::Colour c = col::dim);
void drawPanelTitle (juce::Graphics&, juce::Rectangle<int> r, const juce::String& text, float s, juce::Colour c = col::amber);

class PeakMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    void setLevel (float db);
    void paint (juce::Graphics&) override;
    juce::String caption;
private:
    float level = -120.0f, hold = -120.0f;
    int holdTicks = 0;
};

// Set a parameter from the UI as a single undoable gesture.
void setParam (APVTS&, const juce::String& id, float realValue);
float getParam (APVTS&, const juce::String& id);

// Builds the "modulate with" popup used by knobs.
juce::PopupMenu sourceMenu (int firstId);
}
