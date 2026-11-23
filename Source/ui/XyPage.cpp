#include "Pages.h"

namespace mc::ui
{
// ======================================================================= XyPad
XyPad::XyPad (Ctx& c, int p) : ctx (c), pad (p)
{
    setTooltip ("XY " + juce::String (pad + 1) + ": drag to morph every target assigned below. The centre leaves the sound as stored. "
                "Double-click to return to the centre.");
    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

juce::Rectangle<float> XyPad::padArea() const { return getLocalBounds().toFloat().reduced (2.0f); }

void XyPad::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    const auto r = padArea();
    g.setColour (col::bg);
    g.fillRoundedRectangle (r, 6.0f * s);
    // grid
    for (int k = 1; k < 8; ++k)
    {
        const float fx = r.getX() + r.getWidth() * (float) k / 8.0f, fy = r.getY() + r.getHeight() * (float) k / 8.0f;
        g.setColour (k == 4 ? col::edge.brighter (0.35f) : col::edge.withAlpha (0.55f));
        g.drawLine (fx, r.getY() + 3, fx, r.getBottom() - 3, k == 4 ? 1.2f : 1.0f);
        g.drawLine (r.getX() + 3, fy, r.getRight() - 3, fy, k == 4 ? 1.2f : 1.0f);
    }
    g.setColour (col::edge.brighter (0.2f));
    g.drawRoundedRectangle (r, 6.0f * s, 1.0f);
    g.setFont (Fonts::label (13.0f * s));
    g.setColour (col::faint.brighter (0.3f));
    g.drawText ("X", r.reduced (8 * s, 4 * s), juce::Justification::bottomRight);
    g.drawText ("Y", r.reduced (8 * s, 4 * s), juce::Justification::topLeft);

    const float x = ctx.live.v (id::xyPos (pad, 0)), y = ctx.live.v (id::xyPos (pad, 1));
    auto toPt = [&] (float px, float py) { return juce::Point<float> (r.getX() + px * r.getWidth(), r.getBottom() - py * r.getHeight()); };
    const auto p = toPt (x, y);
    const auto ctr = toPt (0.5f, 0.5f);
    // line from the centre: how far the pad pushes the targets
    g.setColour (col::teal.withAlpha (0.35f));
    g.drawLine (ctr.x, ctr.y, p.x, p.y, 1.5f * s);
    g.setColour (col::teal.withAlpha (0.18f));
    g.drawLine (p.x, r.getY() + 2, p.x, r.getBottom() - 2, 1.0f);
    g.drawLine (r.getX() + 2, p.y, r.getRight() - 2, p.y, 1.0f);

    // Haunt ghost
    const int ix = paramIndex (id::xyPos (pad, 0)), iy = paramIndex (id::xyPos (pad, 1));
    const float gx = ctx.proc.engine.hauntOffset (ix), gy = ctx.proc.engine.hauntOffset (iy);
    if (std::abs (gx) > 0.002f || std::abs (gy) > 0.002f)
    {
        const auto gp = toPt (juce::jlimit (0.0f, 1.0f, x + gx), juce::jlimit (0.0f, 1.0f, y + gy));
        g.setColour (col::ghost.withAlpha (0.5f));
        g.drawLine (p.x, p.y, gp.x, gp.y, 1.2f * s);
        const float gr = 7.0f * s;
        g.setColour (col::ghost.withAlpha (0.85f));
        g.drawEllipse (gp.x - gr, gp.y - gr, gr * 2, gr * 2, 1.6f * s);
    }
    const float pr = 9.0f * s;
    g.setColour (col::teal.withAlpha (0.25f));
    g.fillEllipse (p.x - pr * 1.6f, p.y - pr * 1.6f, pr * 3.2f, pr * 3.2f);
    g.setColour (col::teal);
    g.fillEllipse (p.x - pr, p.y - pr, pr * 2, pr * 2);
    g.setColour (col::amber);
    g.drawEllipse (p.x - pr, p.y - pr, pr * 2, pr * 2, 2.0f * s);
}

void XyPad::setFrom (juce::Point<float> pt)
{
    const auto r = padArea();
    const float x = juce::jlimit (0.0f, 1.0f, (pt.x - r.getX()) / r.getWidth());
    const float y = juce::jlimit (0.0f, 1.0f, (r.getBottom() - pt.y) / r.getHeight());
    if (auto* px = ctx.proc.apvts.getParameter (id::xyPos (pad, 0))) px->setValueNotifyingHost (x);
    if (auto* py = ctx.proc.apvts.getParameter (id::xyPos (pad, 1))) py->setValueNotifyingHost (y);
    repaint();
}

void XyPad::mouseDown (const juce::MouseEvent& e)
{
    for (int a = 0; a < 2; ++a) if (auto* p = ctx.proc.apvts.getParameter (id::xyPos (pad, a))) p->beginChangeGesture();
    setFrom (e.position);
}

void XyPad::mouseDrag (const juce::MouseEvent& e) { setFrom (e.position); }

void XyPad::mouseUp (const juce::MouseEvent&)
{
    for (int a = 0; a < 2; ++a) if (auto* p = ctx.proc.apvts.getParameter (id::xyPos (pad, a))) p->endChangeGesture();
}

void XyPad::mouseDoubleClick (const juce::MouseEvent&)
{
    for (int a = 0; a < 2; ++a) setParam (ctx.proc.apvts, id::xyPos (pad, a), 0.5f);
    repaint();
}

void XyPad::tick()
{
    const juce::Point<float> now (ctx.live.v (id::xyPos (pad, 0)), ctx.live.v (id::xyPos (pad, 1)));
    const juce::Point<float> gh (ctx.proc.engine.hauntOffset (paramIndex (id::xyPos (pad, 0))),
                                 ctx.proc.engine.hauntOffset (paramIndex (id::xyPos (pad, 1))));
    if (now != last || gh.getDistanceFrom (lastGhost) > 0.001f) { last = now; lastGhost = gh; repaint(); }
}

// ======================================================================= XyPage
XyPage::XyPage (Ctx& c) : ctx (c),
    xyLock (c.proc, kLockXY, "XY pads", "keeps the XY pad targets and amounts as they are (Haunt won't drift the pads either).")
{
    auto& s = ctx.proc.apvts;
    for (int n = 0; n < kNumXY; ++n)
    {
        addAndMakeVisible (pads.add (new XyPad (ctx, n)));
        for (int a = 0; a < 2; ++a)
            for (int k = 0; k < kXYTargets; ++k)
            {
                auto* d = dsts.add (new DestPicker());
                d->attach (s, id::xyTarget (n, a, k, "dst"));
                d->setTooltip (juce::String ("Target moved by the ") + (a == 0 ? "X (horizontal)" : "Y (vertical)") + " axis of XY " + juce::String (n + 1)
                               + ". Click to choose. Tip: right-click any knob and choose Assign to XY pad.");
                addAndMakeVisible (d);
                auto* sl = amts.add (new juce::Slider (juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight));
                sl->getProperties().set ("bipolar", true);
                sl->getProperties().set ("accent", (juce::int64) (a == 0 ? col::teal : col::amber).getARGB());
                sl->setTooltip ("How far the target moves when the pad reaches the edge. Negative moves it the other way. Double-click to reset.");
                sl->setDoubleClickReturnValue (true, 0.0);
                addAndMakeVisible (sl);
                amtAtt.push_back (std::make_unique<APVTS::SliderAttachment> (s, id::xyTarget (n, a, k, "amt"), *sl));
            }
    }
    addAndMakeVisible (xyLock);
}

void XyPage::tick()
{
    for (auto* p : pads) p->tick();
}

void XyPage::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    drawPanel (g, headArea.toFloat(), s);
    auto h = headArea.reduced (S (12), S (8));
    drawPanelTitle (g, h.removeFromTop (S (18)), "XY Vector Mixer", s);
    g.setFont (Fonts::value (13.5f * s));
    g.setColour (col::dim);
    g.drawFittedText ("Each pad moves up to four targets per axis at once, so one gesture can morph a whole patch. The centre leaves the sound "
                      "as stored. Pad positions are automatable and are also XY sources in the Mod Matrix (and they can drive effects).",
                      h.withTrimmedRight (S (140)), juce::Justification::centredLeft, 2, 0.9f);
    for (int n = 0; n < kNumXY; ++n)
    {
        drawPanel (g, padAreas[n].toFloat(), s);
        auto a = padAreas[n].reduced (S (12), S (8));
        drawPanelTitle (g, a.removeFromTop (S (18)), "XY " + juce::String (n + 1), s, n % 2 == 0 ? col::teal : col::amber);
        for (int ax = 0; ax < 2; ++ax)
        {
            const auto first = dsts[(n * 2 + ax) * kXYTargets]->getBounds();
            drawCaption (g, { first.getX(), first.getY() - S (17), padAreas[n].getRight() - first.getX() - S (12), S (14) },
                         ax == 0 ? "X axis targets  (left  -  right)" : "Y axis targets  (down  -  up)", s, ax == 0 ? col::teal : col::amber);
        }
        const auto last = amts[(n * 2 + 1) * kXYTargets + kXYTargets - 1]->getBounds();
        g.setFont (Fonts::value (12.5f * s));
        g.setColour (col::faint.brighter (0.4f));
        g.drawFittedText ("Double-click the pad to recentre it.\nRight-click any knob and choose Assign to XY pad.",
                          juce::Rectangle<int> (padAreas[n].getX() + S (12), last.getBottom() + S (14), padAreas[n].getWidth() - S (24), S (36)),
                          juce::Justification::centredTop, 2, 0.9f);
    }
}

void XyPage::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds();
    headArea = r.removeFromTop (S (70));
    r.removeFromTop (S (8));
    {
        auto t = headArea.reduced (S (12), S (8)).removeFromTop (S (18));
        xyLock.setBounds (t.removeFromRight (xyLock.preferredWidth (s)));
    }
    const int gap = S (8);
    const int w = (r.getWidth() - gap * (kNumXY - 1)) / kNumXY;
    for (int n = 0; n < kNumXY; ++n)
    {
        padAreas[n] = r.withX (r.getX() + n * (w + gap)).withWidth (w);
        auto a = padAreas[n].reduced (S (12), S (8));
        a.removeFromTop (S (24));
        const int rowH = S (30);
        const int listH = 2 * (S (22) + kXYTargets * rowH) + S (8);
        const int side = juce::jmin (a.getWidth(), a.getHeight() - listH - S (10));
        pads[n]->setBounds (a.removeFromTop (side).withSizeKeepingCentre (side, side));
        a.removeFromTop (S (10));
        for (int ax = 0; ax < 2; ++ax)
        {
            a.removeFromTop (S (22));
            for (int k = 0; k < kXYTargets; ++k)
            {
                const int i = (n * 2 + ax) * kXYTargets + k;
                auto row = a.removeFromTop (rowH).reduced (0, S (3));
                dsts[i]->setBounds (row.removeFromLeft ((int) (row.getWidth() * 0.58f)).withTrimmedRight (S (6)));
                amts[i]->setTextBoxStyle (juce::Slider::TextBoxRight, false, S (42), row.getHeight());
                amts[i]->setBounds (row);
            }
            a.removeFromTop (S (8));
        }
    }
}
}
