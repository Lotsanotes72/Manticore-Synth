#pragma once
#include <JuceHeader.h>

namespace mc::ui
{
namespace col
{
    const juce::Colour bg       { 0xff0a1013 };
    const juce::Colour bg2      { 0xff0f171b };
    const juce::Colour panel    { 0xff131d22 };
    const juce::Colour panelHi  { 0xff19262c };
    const juce::Colour edge     { 0xff27383f };
    const juce::Colour text     { 0xffdce8e5 };
    const juce::Colour dim      { 0xff86a09f };
    const juce::Colour faint    { 0xff4a6066 };
    const juce::Colour teal     { 0xff1fd0bf };
    const juce::Colour tealDim  { 0xff11766d };
    const juce::Colour amber    { 0xffffb33f };
    const juce::Colour amberDim { 0xff94621f };
    const juce::Colour red      { 0xffff5c4d };
    const juce::Colour ghost    { 0xff9cc9ff };      // Haunt: pale spectral blue

    inline juce::Colour band (int b)
    {
        static const juce::Colour c[] { juce::Colour (0xff22d6c3), juce::Colour (0xffffb33f), juce::Colour (0xff52b6ff),
                                        juce::Colour (0xffff7b47), juce::Colour (0xff86e07c), juce::Colour (0xffeed46b) };
        return c[(b % 6 + 6) % 6];
    }
}

struct Fonts
{
    static juce::Font title (float h);
    static juce::Font label (float h);     // Rajdhani SemiBold
    static juce::Font value (float h);     // Rajdhani Medium
    static juce::Font bold (float h);      // Rajdhani Bold
};

class McLookAndFeel : public juce::LookAndFeel_V4
{
public:
    McLookAndFeel();
    float s = 1.0f;   // UI scale, set by the editor

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    void drawPopupMenuItem (juce::Graphics&, const juce::Rectangle<int>&, bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu, const juce::String& text, const juce::String& shortcut,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;
    void drawPopupMenuSectionHeader (juce::Graphics&, const juce::Rectangle<int>&, const juce::String&) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String&, juce::Point<int>, juce::Rectangle<int>) override;
    void drawTooltip (juce::Graphics&, const juce::String&, int w, int h) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
    void fillTextEditorBackground (juce::Graphics&, int w, int h, juce::TextEditor&) override;
    void drawTextEditorOutline (juce::Graphics&, int, int, juce::TextEditor&) override {}
    void drawCornerResizer (juce::Graphics&, int w, int h, bool over, bool drag) override;
};

// Decorative helpers shared by several components.
void drawPanel (juce::Graphics&, juce::Rectangle<float> r, float s, bool ornaments = true);
void drawEmblem (juce::Graphics&, juce::Rectangle<float> r);
void drawStar (juce::Graphics&, juce::Rectangle<float> r, bool filled, juce::Colour c);
void drawPadlock (juce::Graphics&, juce::Rectangle<float> r, bool closed, juce::Colour c);
}
