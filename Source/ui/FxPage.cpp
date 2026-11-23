#include "Pages.h"

namespace mc::ui
{
FxPage::FxPage (Ctx& c) : ctx (c), grid (c, true), editor (c, true), flow (c)
{
    addAndMakeVisible (grid);
    addAndMakeVisible (editor);
    addAndMakeVisible (flow);
    auto& s = ctx.proc.apvts;
    for (int a = 0; a < 3; ++a)
    {
        auto* kb = returns.add (new Knob ("Aux " + juce::String (a + 1) + " Ret"));
        kb->attach (s, id::auxRet (a));
        kb->setAccent (col::amber);
        addAndMakeVisible (kb);
    }
    for (int l = 0; l < 4; ++l)
        for (int a = 0; a < 3; ++a)
        {
            auto* kb = sends.add (new Knob ("L" + juce::String (l + 1) + " > A" + juce::String (a + 1)));
            kb->compact = true;
            kb->attach (s, id::lane (l, ("send" + juce::String (a + 1)).toRawUTF8()));
            kb->setAccent (l % 2 == 0 ? col::teal : col::amber);
            addAndMakeVisible (kb);
        }
}

void FxPage::showFx (Kind k, int i) { editor.setModule (k, i); }

void FxPage::tick()
{
    grid.tick (editor.getKind(), editor.getInst());
    editor.tick();
    if (ctx.live.version % 3 == 0) flow.repaint();
}

void FxPage::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    drawPanel (g, retArea.toFloat(), s);
    drawPanel (g, sendArea.toFloat(), s);
    drawPanelTitle (g, retArea.reduced (S (12), S (8)).removeFromTop (S (18)), "Aux Returns", s);
    drawPanelTitle (g, sendArea.reduced (S (12), S (8)).removeFromTop (S (18)), "Lane Sends", s);
    g.setFont (Fonts::value (13.5f * s));
    g.setColour (col::dim);
    auto t = retArea.reduced (S (12), S (8));
    t.removeFromTop (S (20));
    g.drawFittedText ("Each lane sends to the three aux buses. Aux buses run their own effect chains and return into the Main bus, "
                      "whose effects process the whole mix last.", t.removeFromRight (t.getWidth() / 2).reduced (S (6), 0),
                      juce::Justification::centredLeft, 5);
}

void FxPage::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds();
    auto left = r.removeFromLeft (S (560));
    r.removeFromLeft (S (8));
    gridArea = left.removeFromTop (S (380));
    grid.setBounds (gridArea);
    left.removeFromTop (S (8));
    retArea = left.removeFromTop (S (128));
    left.removeFromTop (S (8));
    sendArea = left;
    {
        auto a = retArea.reduced (S (12), S (8));
        a.removeFromTop (S (20));
        auto k = a.removeFromLeft (a.getWidth() / 2);
        const int w = k.getWidth() / 3;
        for (auto* kb : returns) kb->setBounds (k.removeFromLeft (w).reduced (S (4), 0));
    }
    {
        auto a = sendArea.reduced (S (12), S (8));
        a.removeFromTop (S (22));
        const int rh = juce::jmin (S (70), a.getHeight() / 2);
        const int cw = a.getWidth() / 6;
        for (int i = 0; i < 12; ++i)
        {
            const int row = i / 6, c = i % 6;
            sends[i]->setBounds (a.getX() + c * cw, a.getY() + row * rh, cw, rh);
        }
    }
    editorArea = r.removeFromTop (S (430));
    editor.setBounds (editorArea);
    r.removeFromTop (S (8));
    flow.setBounds (r);
}

// ======================================================================= signal flow overview
void FlowPanel::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return v * s; };
    drawPanel (g, getLocalBounds().toFloat(), s);
    auto area = getLocalBounds().toFloat().reduced (S (14), S (10));
    drawPanelTitle (g, area.removeFromTop (S (18)).toNearestInt(), "Signal Flow", s);
    area.removeFromTop (S (8));
    auto& ap = ctx.proc.apvts;
    const auto& entries = fxEntries();
    auto busUnits = [&] (int b)
    {
        juce::StringArray u;
        for (int sl = 0; sl < kFxSlots; ++sl)
        {
            const int e = juce::roundToInt (getParam (ap, id::fxSlot (b, sl)));
            if (e > 0 && e < (int) entries.size()) u.add (moduleName (entries[(size_t) e].kind, entries[(size_t) e].inst));
        }
        return u.isEmpty() ? juce::String ("(no effects)") : u.joinIntoString ("  >  ");
    };
    const float colW = area.getWidth();
    const float laneX = area.getX(), laneW = colW * 0.18f;
    const float busX = area.getX() + colW * 0.30f, busW = colW * 0.40f;
    const float mainX = area.getX() + colW * 0.78f, mainW = colW * 0.22f;
    const float rowH = area.getHeight() / 4.0f;
    juce::Rectangle<float> laneR[4], busR[3];
    for (int l = 0; l < 4; ++l) laneR[l] = { laneX, area.getY() + rowH * (float) l + S (6), laneW, rowH - S (12) };
    for (int a = 0; a < 3; ++a) busR[a] = { busX, area.getY() + rowH * (float) (a + 1) + S (6), busW, rowH - S (12) };
    const juce::Rectangle<float> mainBus { busX, area.getY() + S (6), busW, rowH - S (12) };
    const juce::Rectangle<float> outR { mainX, area.getY() + S (6), mainW, rowH - S (12) };

    auto box = [&] (juce::Rectangle<float> r, const juce::String& title, const juce::String& sub, juce::Colour c, bool active)
    {
        g.setColour (active ? col::panelHi : col::bg2);
        g.fillRoundedRectangle (r, S (5));
        g.setColour (active ? c : col::edge);
        g.drawRoundedRectangle (r, S (5), active ? 1.5f : 1.0f);
        auto t = r.reduced (S (8), S (4));
        g.setFont (Fonts::bold (S (14)));
        g.setColour (active ? c : col::faint);
        g.drawText (title, t.removeFromTop (t.getHeight() * 0.5f), juce::Justification::bottomLeft);
        g.setFont (Fonts::value (S (12.5f)));
        g.setColour (active ? col::dim : col::faint);
        g.drawFittedText (sub, t.toNearestInt(), juce::Justification::topLeft, 1, 0.7f);
    };
    auto wire = [&] (juce::Point<float> a, juce::Point<float> b, juce::Colour c, float amount)
    {
        if (amount <= 0.001f) return;
        juce::Path p;
        p.startNewSubPath (a);
        const float mx = (a.x + b.x) * 0.5f;
        p.cubicTo (mx, a.y, mx, b.y, b.x, b.y);
        g.setColour (c.withAlpha (0.15f + 0.75f * juce::jmin (1.0f, amount)));
        g.strokePath (p, juce::PathStrokeType (S (1.0f + 2.0f * juce::jmin (1.0f, amount))));
    };
    bool laneUsed[4] {};
    for (int l = 0; l < 4; ++l)
    {
        for (int sl = 0; sl < 4; ++sl) if (getParam (ap, id::slot (l, sl)) > 0.5f) laneUsed[l] = true;
        if (getParam (ap, id::lane (l, "in")) > 0.5f) laneUsed[l] = true;
    }
    for (int l = 0; l < 4; ++l)
    {
        if (! laneUsed[l]) continue;
        const float vol = getParam (ap, id::lane (l, "vol"));
        wire (laneR[l].getCentre().withX (laneR[l].getRight()), mainBus.getCentre().withX (mainBus.getX()), col::teal, vol > -59.0f ? 0.8f : 0.0f);
        for (int a = 0; a < 3; ++a)
            wire (laneR[l].getCentre().withX (laneR[l].getRight()), busR[a].getCentre().withX (busR[a].getX()), col::amber,
                  getParam (ap, id::lane (l, ("send" + juce::String (a + 1)).toRawUTF8())));
    }
    for (int a = 0; a < 3; ++a)
    {
        bool fed = false;
        for (int l = 0; l < 4; ++l) if (laneUsed[l] && getParam (ap, id::lane (l, ("send" + juce::String (a + 1)).toRawUTF8())) > 0.001f) fed = true;
        if (! fed || getParam (ap, id::auxRet (a)) <= -59.0f) continue;
        juce::Path p;
        const float x0 = busR[a].getRight(), y0 = busR[a].getCentreY();
        const float xr = busR[a].getRight() + S (12 + 8 * a);
        p.startNewSubPath (x0, y0);
        p.lineTo (xr, y0);
        p.lineTo (xr, mainBus.getCentreY() + S (6 + 5 * a));
        p.lineTo (mainBus.getRight(), mainBus.getCentreY() + S (6 + 5 * a));
        g.setColour (col::amber.withAlpha (0.6f));
        g.strokePath (p, juce::PathStrokeType (S (1.5f), juce::PathStrokeType::mitered));
    }
    wire (mainBus.getCentre().withX (mainBus.getRight()), outR.getCentre().withX (outR.getX()), col::teal, 1.0f);

    for (int l = 0; l < 4; ++l)
    {
        juce::StringArray mods;
        for (int sl = 0; sl < 4; ++sl)
        {
            const int e = juce::roundToInt (getParam (ap, id::slot (l, sl)));
            if (e > 0) mods.add (moduleName (gridEntries()[(size_t) e].kind, gridEntries()[(size_t) e].inst));
        }
        box (laneR[l], "LANE " + juce::String (l + 1), mods.isEmpty() ? juce::String ("empty") : mods.joinIntoString (" > "), l % 2 == 0 ? col::teal : col::amber, laneUsed[l]);
    }
    box (mainBus, "MAIN BUS", busUnits (0), col::teal, true);
    for (int a = 0; a < 3; ++a)
        box (busR[a], "AUX " + juce::String (a + 1), busUnits (a + 1), col::amber, getParam (ap, id::fxSlot (a + 1, 0)) > 0.5f
             || getParam (ap, id::fxSlot (a + 1, 1)) > 0.5f || getParam (ap, id::fxSlot (a + 1, 2)) > 0.5f || getParam (ap, id::fxSlot (a + 1, 3)) > 0.5f);
    box (outR, "OUTPUT", "Master " + params()[(size_t) paramIndex (id::master)].format (getParam (ap, id::master)), col::teal, true);
}

// ======================================================================= Arp
StepBar::StepBar (Ctx& c, const juce::String& p, bool bp, juce::Colour col_) : ctx (c), pid (p), bipolar (bp), colour (col_)
{
    const int i = paramIndex (p);
    setTooltip (i >= 0 ? params()[(size_t) i].name + ": " + params()[(size_t) i].tip + " Drag to set, double-click to reset." : juce::String());
}

void StepBar::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto r = getLocalBounds().toFloat().reduced (2.0f * s, 1.0f * s);
    g.setColour (col::bg);
    g.fillRoundedRectangle (r, 3.0f * s);
    auto* p = ctx.proc.apvts.getParameter (pid);
    if (p == nullptr) return;
    const float n = p->getValue();
    const auto c = active ? colour : col::faint;
    if (bipolar)
    {
        const float mid = r.getCentreY();
        const float y = r.getBottom() - n * r.getHeight();
        g.setColour (c.withAlpha (0.35f));
        g.fillRect (juce::Rectangle<float> (r.getX(), juce::jmin (mid, y), r.getWidth(), std::abs (y - mid)));
        g.setColour (c);
        g.fillRect (r.getX(), y - 1.0f * s, r.getWidth(), 2.0f * s);
        g.setColour (col::edge);
        g.drawHorizontalLine (juce::roundToInt (mid), r.getX(), r.getRight());
    }
    else
    {
        const float h = n * r.getHeight();
        g.setColour (c.withAlpha (0.35f));
        g.fillRect (r.withTop (r.getBottom() - h));
        g.setColour (c);
        g.fillRect (r.getX(), r.getBottom() - h, r.getWidth(), 2.0f * s);
    }
    g.setFont (Fonts::value (12.0f * s));
    g.setColour (col::text.withAlpha (active ? 0.9f : 0.4f));
    const int idx = paramIndex (pid);
    if (idx >= 0) g.drawText (params()[(size_t) idx].format (p->convertFrom0to1 (n)), getLocalBounds().reduced (0, juce::roundToInt (2 * s)),
                              juce::Justification::centredTop);
}

void StepBar::setFrom (const juce::MouseEvent& e)
{
    auto* p = ctx.proc.apvts.getParameter (pid);
    if (p == nullptr) return;
    const float v = juce::jlimit (0.0f, 1.0f, 1.0f - (float) e.position.y / (float) getHeight());
    p->setValueNotifyingHost (v);
    repaint();
}

void StepBar::mouseDown (const juce::MouseEvent& e)
{
    if (auto* p = ctx.proc.apvts.getParameter (pid)) { p->beginChangeGesture(); gesture = true; }
    setFrom (e);
}
void StepBar::mouseDrag (const juce::MouseEvent& e) { setFrom (e); }
void StepBar::mouseUp (const juce::MouseEvent&)
{
    if (gesture) if (auto* p = ctx.proc.apvts.getParameter (pid)) p->endChangeGesture();
    gesture = false;
}
void StepBar::mouseDoubleClick (const juce::MouseEvent&)
{
    if (auto* p = ctx.proc.apvts.getParameter (pid)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); }
    repaint();
}

ArpPage::ArpPage (Ctx& c) : ctx (c),
    arpLock (c.proc, kLockArp, "Arp", "keeps the arpeggiator pattern and settings as they are.")
{
    addAndMakeVisible (arpLock);
    auto& s = ctx.proc.apvts;
    on.attach (s, id::arpOn);
    hold.attach (s, id::arpHold);
    addAndMakeVisible (on);
    addAndMakeVisible (hold);
    for (auto p : { id::arpMode, id::arpDiv })
    {
        auto* cb = choices.add (new Choice());
        cb->attach (s, p);
        addAndMakeVisible (cb);
    }
    for (auto p : { id::arpOct, id::arpSteps, id::arpSwing, id::arpGate })
    {
        auto* kb = knobs.add (new Knob());
        kb->attach (s, p);
        kb->setAccent (col::amber);
        addAndMakeVisible (kb);
    }
    for (int i = 0; i < kArpSteps; ++i)
    {
        auto* t = stepOn.add (new Toggle (juce::String (i + 1), col::teal));
        t->attach (s, id::arpStep (i, "on"));
        addAndMakeVisible (t);
        addAndMakeVisible (trBars.add (new StepBar (ctx, id::arpStep (i, "tr"), true, col::amber)));
        addAndMakeVisible (velBars.add (new StepBar (ctx, id::arpStep (i, "vel"), false, col::teal)));
        addAndMakeVisible (gateBars.add (new StepBar (ctx, id::arpStep (i, "gate"), false, juce::Colour (0xff52b6ff))));
    }
}

void ArpPage::tick()
{
    const int steps = juce::roundToInt (getParam (ctx.proc.apvts, id::arpSteps));
    for (int i = 0; i < kArpSteps; ++i)
    {
        const bool act = i < steps && getParam (ctx.proc.apvts, id::arpStep (i, "on")) > 0.5f;
        for (auto* b : { trBars[i], velBars[i], gateBars[i] }) { b->active = act; b->repaint(); }
        stepOn[i]->setAlpha (i < steps ? 1.0f : 0.4f);
    }
}

void ArpPage::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    drawPanel (g, topArea.toFloat(), s);
    drawPanel (g, gridArea.toFloat(), s);
    drawPanelTitle (g, topArea.reduced (S (12), S (8)).removeFromTop (S (18)), "Arpeggiator", s);
    drawPanelTitle (g, gridArea.reduced (S (12), S (8)).removeFromTop (S (18)), "Steps", s);
    const char* labels[] { "STEP ON", "TRANSPOSE", "VELOCITY", "GATE" };
    for (int i = 0; i < 4; ++i)
    {
        g.setFont (Fonts::bold (13.0f * s));
        g.setColour (i == 1 ? col::amber : i == 3 ? juce::Colour (0xff52b6ff) : col::teal);
        g.drawText (labels[i], rowLabels[i], juce::Justification::centredLeft);
    }
    for (int i = 0; i < choices.size(); ++i)
        drawCaption (g, choices[i]->getBounds().translated (0, -S (15)).withHeight (S (14)), i == 0 ? "Order" : "Rate", s);
    g.setFont (Fonts::value (14.0f * s));
    g.setColour (col::dim);
    g.drawFittedText ("Hold notes and the arpeggiator plays them one step at a time, locked to your DAW's tempo. Each step can rest, transpose, "
                      "change velocity or tie into the next (gate at 100%). Steps sets the pattern length, so a 7-step pattern against "
                      "4/4 drifts in true prog fashion.", helpArea, juce::Justification::topLeft, 4);
}

void ArpPage::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds();
    topArea = r.removeFromTop (S (150));
    r.removeFromTop (S (8));
    gridArea = r;
    {
        auto t = topArea.reduced (S (12), S (8)).removeFromTop (S (18));
        arpLock.setBounds (t.removeFromRight (arpLock.preferredWidth (s)));
    }
    {
        auto a = topArea.reduced (S (12), S (8));
        a.removeFromTop (S (22));
        auto row = a.removeFromLeft (S (130));
        on.setBounds (row.removeFromTop (S (32)).reduced (0, S (2)));
        row.removeFromTop (S (8));
        hold.setBounds (row.removeFromTop (S (32)).reduced (0, S (2)));
        a.removeFromLeft (S (16));
        auto ch = a.removeFromLeft (S (170));
        ch.removeFromTop (S (16));
        choices[0]->setBounds (ch.removeFromTop (S (28)));
        ch.removeFromTop (S (20));
        choices[1]->setBounds (ch.removeFromTop (S (28)));
        a.removeFromLeft (S (16));
        for (auto* kb : knobs) kb->setBounds (a.removeFromLeft (S (90)).withHeight (S (100)));
        a.removeFromLeft (S (20));
        helpArea = a.reduced (0, S (4));
    }
    auto a = gridArea.reduced (S (12), S (8));
    a.removeFromTop (S (24));
    auto labels = a.removeFromLeft (S (110));
    const int onH = S (30);
    const int barH = (a.getHeight() - onH - S (30)) / 3;
    rowLabels[0] = labels.removeFromTop (onH);
    labels.removeFromTop (S (10));
    for (int i = 1; i < 4; ++i) { rowLabels[i] = labels.removeFromTop (barH); labels.removeFromTop (S (10)); }
    const int cw = a.getWidth() / kArpSteps;
    for (int i = 0; i < kArpSteps; ++i)
    {
        auto c = a.withX (a.getX() + i * cw).withWidth (cw).reduced (S (3), 0);
        stepOn[i]->setBounds (c.removeFromTop (onH));
        c.removeFromTop (S (10));
        trBars[i]->setBounds (c.removeFromTop (barH));
        c.removeFromTop (S (10));
        velBars[i]->setBounds (c.removeFromTop (barH));
        c.removeFromTop (S (10));
        gateBars[i]->setBounds (c.removeFromTop (barH));
    }
}
}
