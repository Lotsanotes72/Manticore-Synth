#include "Pages.h"

namespace mc::ui
{
// ======================================================================= MatrixPanel
MatrixPanel::MatrixPanel (Ctx& c) : ctx (c)
{
    auto& s = ctx.proc.apvts;
    for (int i = 0; i < kNumMatrix; ++i)
    {
        auto* a = srcs.add (new Choice());
        a->attach (s, id::mtx (i, "src"), "Modulation source for slot " + juce::String (i + 1) + ".");
        addAndMakeVisible (a);
        auto* v = vias.add (new Choice());
        v->attach (s, id::mtx (i, "via"), "Optional: a second source that scales this slot (Mod Wheel here makes the wheel fade the modulation in).");
        addAndMakeVisible (v);
        auto* d = dsts.add (new DestPicker());
        d->attach (s, id::mtx (i, "dst"));
        addAndMakeVisible (d);
        auto* sl = amts.add (new juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight));
        sl->getProperties().set ("bipolar", true);
        sl->getProperties().set ("accent", (juce::int64) col::amber.getARGB());
        sl->setTooltip ("Modulation amount. Negative values invert the source. Double-click to reset.");
        sl->setDoubleClickReturnValue (true, 0.0);
        addAndMakeVisible (sl);
        amtAtt.push_back (std::make_unique<APVTS::SliderAttachment> (s, id::mtx (i, "amt"), *sl));
    }
}

void MatrixPanel::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    for (int col = 0; col < 2; ++col)
    {
        const auto first = srcs[col * 12]->getBounds();
        const int y = first.getY() - juce::roundToInt (17 * s);
        drawCaption (g, { first.getX(), y, first.getWidth(), juce::roundToInt (14 * s) }, "Source", s);
        const auto v = vias[col * 12]->getBounds();
        drawCaption (g, { v.getX(), y, v.getWidth(), juce::roundToInt (14 * s) }, "Via", s);
        const auto d = dsts[col * 12]->getBounds();
        drawCaption (g, { d.getX(), y, d.getWidth(), juce::roundToInt (14 * s) }, "Target", s);
        const auto a = amts[col * 12]->getBounds();
        drawCaption (g, { a.getX(), y, a.getWidth(), juce::roundToInt (14 * s) }, "Amount", s);
    }
    g.setFont (Fonts::bold (13.0f * s));
    for (int i = 0; i < kNumMatrix; ++i)
    {
        const auto b = srcs[i]->getBounds();
        const bool active = getParam (ctx.proc.apvts, id::mtx (i, "src")) > 0.5f && getParam (ctx.proc.apvts, id::mtx (i, "dst")) > 0.5f;
        g.setColour (active ? col::amber : col::faint);
        g.drawText (juce::String (i + 1), juce::Rectangle<int> (b.getX() - juce::roundToInt (24 * s), b.getY(), juce::roundToInt (20 * s), b.getHeight()),
                    juce::Justification::centredRight);
    }
}

void MatrixPanel::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds().reduced (S (4), 0);
    r.removeFromTop (S (18));
    const int colW = r.getWidth() / 2;
    const int rowH = juce::jmin (S (29), r.getHeight() / 12);
    for (int col = 0; col < 2; ++col)
    {
        auto c = r.withX (r.getX() + col * colW).withWidth (colW - S (10));
        c.removeFromLeft (S (26));
        for (int k = 0; k < 12; ++k)
        {
            const int i = col * 12 + k;
            auto row = c.removeFromTop (rowH).reduced (0, S (2));
            const int w = row.getWidth();
            srcs[i]->setBounds (row.removeFromLeft ((int) (w * 0.22f)).withTrimmedRight (S (4)));
            vias[i]->setBounds (row.removeFromLeft ((int) (w * 0.20f)).withTrimmedRight (S (4)));
            dsts[i]->setBounds (row.removeFromLeft ((int) (w * 0.33f)).withTrimmedRight (S (6)));
            amts[i]->setTextBoxStyle (juce::Slider::TextBoxRight, false, S (46), row.getHeight());
            amts[i]->setBounds (row);
        }
    }
}

// ======================================================================= ModRack
ModRack::ModRack (Ctx& c) : ctx (c), editor (c, true), matrix (c)
{
    const char* fam[] { "ENV", "LFO", "MSEG", "MAP", "MATH", "MATRIX" };
    const Kind kinds[] { Kind::Env, Kind::Lfo, Kind::Mseg, Kind::Map, Kind::Math, Kind::Count };
    const char* tips[] { "Envelopes (ENV 1 is the amp envelope).", "Low frequency oscillators.", "Multi-segment envelopes you draw yourself.",
                         "Mappers: a drawn curve or a step sequencer.", "Mod math: combine, smooth or quantize two sources.",
                         "The modulation matrix: 24 slots connecting any source to any target. Tip: right-click any knob to add a slot for it." };
    for (int f = 0; f < 6; ++f)
    {
        auto* b = fams.add (new juce::TextButton (fam[f]));
        b->setClickingTogglesState (true);
        b->setRadioGroupId (771);
        b->getProperties().set ("accent", (juce::int64) (f == 5 ? col::amber : kindColour (kinds[f])).getARGB());
        b->setTooltip (tips[f]);
        b->onClick = [this, f] { select (f == 5 ? 20 : f * 4 + lastNum); };
        addAndMakeVisible (b);
    }
    for (int i = 0; i < 4; ++i)
    {
        auto* b = nums.add (new juce::TextButton (juce::String (i + 1)));
        b->setClickingTogglesState (true);
        b->setRadioGroupId (772);
        b->getProperties().set ("accent", (juce::int64) col::teal.getARGB());
        b->setTooltip ("Which of the four modulators of this family to edit.");
        b->onClick = [this, i] { lastNum = i; if (current != 20) select ((current / 4) * 4 + i); };
        addAndMakeVisible (b);
    }
    addAndMakeVisible (editor);
    addChildComponent (matrix);
    select ((int) ctx.proc.uiState.getProperty ("modTab", 0));
}

void ModRack::select (int t)
{
    current = juce::jlimit (0, 20, t);
    ctx.proc.uiState.setProperty ("modTab", current, nullptr);
    fams[current == 20 ? 5 : current / 4]->setToggleState (true, juce::dontSendNotification);
    for (auto* n : nums) n->setVisible (current != 20);
    if (current != 20)
    {
        lastNum = current % 4;
        nums[lastNum]->setToggleState (true, juce::dontSendNotification);
        const Kind kinds[] { Kind::Env, Kind::Lfo, Kind::Mseg, Kind::Map, Kind::Math };
        for (auto* n : nums) n->getProperties().set ("accent", (juce::int64) kindColour (kinds[current / 4]).getARGB());
        editor.setModule (kinds[current / 4], current % 4);
        editor.setVisible (true);
        matrix.setVisible (false);
    }
    else
    {
        editor.setVisible (false);
        matrix.setVisible (true);
    }
    repaint();
}

void ModRack::tick()
{
    if (editor.isVisible()) editor.tick();
    else matrix.repaint();
}

void ModRack::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    drawPanel (g, getLocalBounds().toFloat(), s);
    drawPanelTitle (g, getLocalBounds().reduced (juce::roundToInt (12 * s), juce::roundToInt (8 * s)).removeFromTop (juce::roundToInt (22 * s)), "Modulation Rack", s);
}

void ModRack::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds().reduced (S (12), S (8));
    auto head = r.removeFromTop (S (24));
    head.removeFromLeft (S (170));
    for (auto* f : fams) f->setBounds (head.removeFromLeft (f->getButtonText() == "MATRIX" ? S (78) : S (62)).withTrimmedRight (S (4)));
    head.removeFromLeft (S (18));
    for (auto* n : nums) n->setBounds (head.removeFromLeft (S (34)).withTrimmedRight (S (4)));
    r.removeFromTop (S (6));
    editor.setBounds (r.expanded (S (12), S (8)).withTrimmedTop (S (8)));
    matrix.setBounds (r);
}
}
