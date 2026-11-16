#include "LookAndFeel.h"

namespace mc::ui
{
static juce::Typeface::Ptr tf (const void* d, size_t n)
{
    return juce::Typeface::createSystemTypefaceFor (d, n);
}
static juce::Typeface::Ptr cinzel()  { static auto t = tf (BinaryData::CinzelBold_ttf, BinaryData::CinzelBold_ttfSize); return t; }
static juce::Typeface::Ptr rajMed()  { static auto t = tf (BinaryData::RajdhaniMedium_ttf, BinaryData::RajdhaniMedium_ttfSize); return t; }
static juce::Typeface::Ptr rajSemi() { static auto t = tf (BinaryData::RajdhaniSemiBold_ttf, BinaryData::RajdhaniSemiBold_ttfSize); return t; }
static juce::Typeface::Ptr rajBold() { static auto t = tf (BinaryData::RajdhaniBold_ttf, BinaryData::RajdhaniBold_ttfSize); return t; }

juce::Font Fonts::title (float h) { return juce::Font (juce::FontOptions (cinzel()).withHeight (h)); }
juce::Font Fonts::label (float h) { return juce::Font (juce::FontOptions (rajSemi()).withHeight (h)); }
juce::Font Fonts::value (float h) { return juce::Font (juce::FontOptions (rajMed()).withHeight (h)); }
juce::Font Fonts::bold (float h)  { return juce::Font (juce::FontOptions (rajBold()).withHeight (h)); }

McLookAndFeel::McLookAndFeel()
{
    setColour (juce::Label::textColourId, col::text);
    setColour (juce::Slider::textBoxTextColourId, col::text);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, col::teal.withAlpha (0.35f));
    setColour (juce::TextEditor::textColourId, col::text);
    setColour (juce::TextEditor::backgroundColourId, col::bg);
    setColour (juce::TextEditor::highlightColourId, col::teal.withAlpha (0.35f));
    setColour (juce::TextEditor::focusedOutlineColourId, col::teal);
    setColour (juce::CaretComponent::caretColourId, col::amber);
    setColour (juce::ComboBox::textColourId, col::text);
    setColour (juce::ComboBox::backgroundColourId, col::panelHi);
    setColour (juce::ComboBox::outlineColourId, col::edge);
    setColour (juce::ComboBox::arrowColourId, col::amber);
    setColour (juce::PopupMenu::backgroundColourId, col::panel);
    setColour (juce::PopupMenu::textColourId, col::text);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::teal.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TooltipWindow::backgroundColourId, col::bg);
    setColour (juce::TooltipWindow::textColourId, col::text);
    setColour (juce::TooltipWindow::outlineColourId, col::teal);
    setColour (juce::ResizableWindow::backgroundColourId, col::bg);
    setColour (juce::AlertWindow::backgroundColourId, col::panel);
    setColour (juce::AlertWindow::textColourId, col::text);
    setColour (juce::AlertWindow::outlineColourId, col::teal);
    setColour (juce::TextButton::buttonColourId, col::panelHi);
    setColour (juce::TextButton::textColourOffId, col::text);
}

static juce::Colour accentOf (const juce::Component& c, juce::Colour def)
{
    const auto v = c.getProperties()["accent"];
    return v.isVoid() ? def : juce::Colour ((juce::uint32) (juce::int64) v);
}

void McLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& sl)
{
    const auto accent = accentOf (sl, col::teal);
    const bool enabled = sl.isEnabled();
    auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (2.0f * s);
    const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f;
    const auto c = area.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    // ticks
    for (int i = 0; i <= 20; ++i)
    {
        const float a = a0 + (a1 - a0) * (float) i / 20.0f;
        const float r1 = r * 0.93f, r2 = r * (i % 5 == 0 ? 1.0f : 0.97f);
        g.setColour (i % 5 == 0 ? col::faint.brighter (0.2f) : col::faint);
        g.drawLine (c.x + r1 * std::sin (a), c.y - r1 * std::cos (a), c.x + r2 * std::sin (a), c.y - r2 * std::cos (a), 1.0f * s);
    }

    const float tr = r * 0.80f, th = juce::jmax (2.0f, 3.2f * s);
    juce::Path track;
    track.addCentredArc (c.x, c.y, tr, tr, 0.0f, a0, a1, true);
    g.setColour (col::edge);
    g.strokePath (track, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const bool bipolar = (bool) sl.getProperties()["bipolar"];
    const float from = bipolar ? (a0 + a1) * 0.5f : a0;
    juce::Path val;
    val.addCentredArc (c.x, c.y, tr, tr, 0.0f, juce::jmin (from, ang), juce::jmax (from, ang), true);
    g.setColour (enabled ? accent : accent.withSaturation (0.1f).withAlpha (0.4f));
    g.strokePath (val, juce::PathStrokeType (th, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // modulation range from the matrix (amber, just outside the value track)
    const float mp = (float) sl.getProperties().getWithDefault ("modPos", 0.0f), mn = (float) sl.getProperties().getWithDefault ("modNeg", 0.0f);
    if (mp != 0.0f || mn != 0.0f)
    {
        const float lo = juce::jlimit (0.0f, 1.0f, pos + mn), hi = juce::jlimit (0.0f, 1.0f, pos + mp);
        if (hi > lo)
        {
            juce::Path m;
            const float mr = r * 0.95f;
            m.addCentredArc (c.x, c.y, mr, mr, 0.0f, a0 + lo * (a1 - a0), a0 + hi * (a1 - a0), true);
            g.setColour (col::amber.withAlpha (enabled ? 0.9f : 0.4f));
            g.strokePath (m, juce::PathStrokeType (juce::jmax (1.5f, 2.2f * s), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
    }

    const float kr = r * 0.60f;
    juce::ColourGradient body (col::panelHi.brighter (0.15f), c.x, c.y - kr, col::bg, c.x, c.y + kr, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - kr, c.y - kr, 2 * kr, 2 * kr);
    g.setColour (col::edge.brighter (0.3f));
    g.drawEllipse (c.x - kr, c.y - kr, 2 * kr, 2 * kr, 1.0f * s);

    g.setColour (enabled ? col::amber : col::dim);
    g.drawLine (c.x + kr * 0.25f * std::sin (ang), c.y - kr * 0.25f * std::cos (ang),
                c.x + kr * 0.92f * std::sin (ang), c.y - kr * 0.92f * std::cos (ang), juce::jmax (1.5f, 2.4f * s));
}

void McLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                      juce::Slider::SliderStyle, juce::Slider& sl)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float cy = r.getCentreY();
    g.setColour (col::edge);
    g.fillRoundedRectangle (r.getX(), cy - 2 * s, r.getWidth(), 4 * s, 2 * s);
    g.setColour (accentOf (sl, col::teal));
    if ((bool) sl.getProperties()["bipolar"])
    {
        const float mid = r.getCentreX();
        g.fillRoundedRectangle (juce::jmin (mid, pos), cy - 2 * s, std::abs (pos - mid), 4 * s, 2 * s);
        g.setColour (col::faint);
        g.fillRect (mid - 0.5f, cy - 5 * s, 1.0f, 10 * s);
    }
    else
        g.fillRoundedRectangle (r.getX(), cy - 2 * s, pos - r.getX(), 4 * s, 2 * s);
    g.setColour (col::amber);
    g.fillEllipse (pos - 5 * s, cy - 5 * s, 10 * s, 10 * s);
}

juce::Label* McLookAndFeel::createSliderTextBox (juce::Slider& sl)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (sl);
    l->setColour (juce::Label::textColourId, col::text);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setJustificationType (juce::Justification::centred);
    return l;
}

juce::Font McLookAndFeel::getLabelFont (juce::Label& l)
{
    if (dynamic_cast<juce::Slider*> (l.getParentComponent()) != nullptr) return Fonts::value (14.0f * s);
    return Fonts::label (14.0f * s);
}

void McLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& l)
{
    if (l.isBeingEdited()) return;
    g.setColour (l.findColour (juce::Label::textColourId).withMultipliedAlpha (l.isEnabled() ? 1.0f : 0.5f));
    g.setFont (getLabelFont (l));
    g.drawFittedText (l.getText(), l.getLocalBounds(), l.getJustificationType(), 1, 0.9f);
}

void McLookAndFeel::fillTextEditorBackground (juce::Graphics& g, int w, int h, juce::TextEditor&)
{
    g.setColour (col::bg);
    g.fillRoundedRectangle (0, 0, (float) w, (float) h, 3.0f * s);
    g.setColour (col::teal);
    g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1, (float) h - 1, 3.0f * s, 1.0f);
}

void McLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    const auto accent = accentOf (b, col::teal);
    const bool on = b.getToggleState();
    const float rad = 4.0f * s;
    g.setColour (on ? accent.withAlpha (down ? 0.45f : 0.28f) : (over ? col::panelHi.brighter (0.12f) : col::panelHi));
    g.fillRoundedRectangle (r, rad);
    g.setColour (on ? accent : (over ? accent.withAlpha (0.6f) : col::edge.brighter (0.15f)));
    g.drawRoundedRectangle (r, rad, on ? 1.5f : 1.0f);
}

juce::Font McLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return Fonts::label (juce::jmin (15.0f * s, (float) h * 0.62f)); }

void McLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool)
{
    const auto accent = accentOf (b, col::teal);
    const bool on = b.getToggleState();
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (! b.isEnabled() ? col::faint : on ? accent.brighter (0.4f) : (over ? col::text : col::text.withAlpha (0.85f)));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (3, 0), juce::Justification::centred, 1, 0.8f);
}

void McLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& cb)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (cb.isMouseOver (true) ? col::panelHi.brighter (0.1f) : col::panelHi);
    g.fillRoundedRectangle (r, 4.0f * s);
    g.setColour (cb.hasKeyboardFocus (true) ? col::teal : col::edge.brighter (0.15f));
    g.drawRoundedRectangle (r, 4.0f * s, 1.0f);
    const float a = juce::jmin (6.0f * s, (float) h * 0.22f);
    const float ax = (float) w - 10.0f * s, ay = (float) h * 0.5f;
    juce::Path p;
    p.addTriangle (ax - a, ay - a * 0.5f, ax + a, ay - a * 0.5f, ax, ay + a * 0.6f);
    g.setColour (col::amber);
    g.fillPath (p);
}

juce::Font McLookAndFeel::getComboBoxFont (juce::ComboBox& cb) { return Fonts::label (juce::jmin (15.0f * s, (float) cb.getHeight() * 0.62f)); }

void McLookAndFeel::positionComboBoxText (juce::ComboBox& cb, juce::Label& l)
{
    l.setBounds (juce::roundToInt (6 * s), 1, cb.getWidth() - juce::roundToInt (24 * s), cb.getHeight() - 2);
    l.setFont (getComboBoxFont (cb));
    l.setJustificationType (juce::Justification::centredLeft);
}

juce::Font McLookAndFeel::getPopupMenuFont() { return Fonts::label (16.0f * s); }

void McLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panel);
    g.setColour (col::teal.withAlpha (0.6f));
    g.drawRect (0, 0, w, h, 1);
}

void McLookAndFeel::drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area, bool sep, bool active, bool hi,
                                       bool ticked, bool sub, const juce::String& text, const juce::String& shortcut,
                                       const juce::Drawable*, const juce::Colour* tc)
{
    if (sep)
    {
        g.setColour (col::edge);
        g.fillRect (area.reduced (6, 0).withHeight (1).withY (area.getCentreY()));
        return;
    }
    auto r = area.reduced (2, 0);
    if (hi && active)
    {
        g.setColour (col::teal.withAlpha (0.22f));
        g.fillRoundedRectangle (r.toFloat(), 3.0f);
    }
    g.setFont (getPopupMenuFont());
    auto textArea = r.reduced (juce::roundToInt (22 * s), 0);
    g.setColour (! active ? col::faint : tc != nullptr ? *tc : (hi ? juce::Colours::white : col::text));
    g.drawFittedText (text, textArea, juce::Justification::centredLeft, 1);
    if (ticked)
    {
        g.setColour (col::amber);
        const float d = 6.0f * s;
        g.fillEllipse ((float) r.getX() + 8 * s, (float) r.getCentreY() - d * 0.5f, d, d);
    }
    if (sub)
    {
        juce::Path p;
        const float x = (float) r.getRight() - 12 * s, y = (float) r.getCentreY();
        p.addTriangle (x, y - 4 * s, x, y + 4 * s, x + 5 * s, y);
        g.setColour (col::amber);
        g.fillPath (p);
    }
    if (shortcut.isNotEmpty())
    {
        g.setColour (col::dim);
        g.drawText (shortcut, textArea, juce::Justification::centredRight);
    }
}

void McLookAndFeel::drawPopupMenuSectionHeader (juce::Graphics& g, const juce::Rectangle<int>& area, const juce::String& t)
{
    g.setFont (Fonts::bold (14.0f * s));
    g.setColour (col::amber);
    g.drawFittedText (t.toUpperCase(), area.reduced (juce::roundToInt (10 * s), 0), juce::Justification::bottomLeft, 1);
}

static juce::TextLayout tipLayout (const juce::String& text, float s)
{
    juce::AttributedString a;
    a.setJustification (juce::Justification::topLeft);
    a.append (text, Fonts::value (15.0f * s), col::text);
    juce::TextLayout tl;
    tl.createLayoutWithBalancedLineLengths (a, 340.0f * s);
    return tl;
}

juce::Rectangle<int> McLookAndFeel::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    const auto tl = tipLayout (text, s);
    const int w = (int) std::ceil (tl.getWidth() + 20.0f * s), h = (int) std::ceil (tl.getHeight() + 14.0f * s);
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 18,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h).constrainedWithin (parent);
}

void McLookAndFeel::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h);
    g.setColour (col::bg.withAlpha (0.97f));
    g.fillRoundedRectangle (r, 5.0f * s);
    g.setColour (col::teal.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f * s, 1.0f);
    g.setColour (col::amber);
    g.fillRect (r.getX() + 6 * s, r.getY() + 0.5f, 18 * s, 2.0f * s);
    tipLayout (text, s).draw (g, r.reduced (10.0f * s, 7.0f * s));
}

void McLookAndFeel::drawCornerResizer (juce::Graphics& g, int w, int h, bool over, bool drag)
{
    g.setColour ((over || drag) ? col::amber : col::amberDim);
    for (int i = 1; i <= 3; ++i)
    {
        const float o = (float) i * (float) w / 4.0f;
        g.drawLine ((float) w - o, (float) h, (float) w, (float) h - o, 1.5f);
    }
}

void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float s, bool ornaments)
{
    juce::ColourGradient grad (col::panel, r.getX(), r.getY(), col::bg2, r.getX(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 6.0f * s);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r.reduced (0.5f), 6.0f * s, 1.0f);
    if (! ornaments) return;
    g.setColour (col::teal.withAlpha (0.35f));
    g.fillRect (r.getX() + 14 * s, r.getY() + 0.5f, r.getWidth() - 28 * s, 1.0f);
    auto diamond = [&] (float x, float y)
    {
        juce::Path p;
        const float d = 4.0f * s;
        p.addQuadrilateral (x, y - d, x + d, y, x, y + d, x - d, y);
        g.setColour (col::amber.withAlpha (0.8f));
        g.fillPath (p);
    };
    diamond (r.getX() + 9 * s, r.getY() + 1.0f);
    diamond (r.getRight() - 9 * s, r.getY() + 1.0f);
}

void drawEmblem (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto c = r.getCentre();
    const float R = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    g.setColour (col::bg);
    g.fillEllipse (c.x - R, c.y - R, 2 * R, 2 * R);
    // four lane arcs around the ring
    for (int i = 0; i < 4; ++i)
    {
        const float a0 = juce::MathConstants<float>::twoPi * (float) i / 4.0f + 0.16f;
        const float a1 = a0 + juce::MathConstants<float>::twoPi / 4.0f - 0.32f;
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, R * 0.84f, R * 0.84f, 0.0f, a0, a1, true);
        g.setColour (i % 2 == 0 ? col::teal : col::amber);
        g.strokePath (arc, juce::PathStrokeType (R * 0.14f, juce::PathStrokeType::curved, juce::PathStrokeType::butt));
    }
    g.setColour (col::edge.brighter (0.4f));
    g.drawEllipse (c.x - R * 0.98f, c.y - R * 0.98f, R * 1.96f, R * 1.96f, juce::jmax (1.0f, R * 0.04f));
    // the manticore's scorpion tail: overlapping segments arcing up and over into a barbed stinger
    const int n = R < 20.0f ? 6 : 9;
    const float tr = R * 0.34f, cy0 = c.y - R * 0.02f;
    float lx = 0, ly = 0, lth = 0;
    for (int k = 0; k < n; ++k)
    {
        const float t = (float) k / (float) (n - 1);
        const float th = juce::degreesToRadians (228.0f - 250.0f * t);
        const float x = c.x + tr * std::cos (th), y = cy0 - tr * std::sin (th);
        const float sz = R * (0.15f - 0.075f * t);
        g.setColour (col::amber);
        g.fillEllipse (x - sz, y - sz, sz * 2, sz * 2);
        g.setColour (col::bg);
        g.drawEllipse (x - sz, y - sz, sz * 2, sz * 2, juce::jmax (1.0f, R * 0.025f));
        lx = x; ly = y; lth = th;
    }
    const float dx = std::sin (lth), dy = std::cos (lth), px = -dy, py = dx;
    const float sz = R * 0.085f, L = R * 0.30f;
    juce::Path sting;
    sting.startNewSubPath (lx + px * sz, ly + py * sz);
    sting.lineTo (lx + dx * L * 0.55f + px * sz * 0.6f, ly + dy * L * 0.55f + py * sz * 0.6f);
    sting.lineTo (lx + dx * L - px * L * 0.35f, ly + dy * L - py * L * 0.35f);
    sting.lineTo (lx + dx * L * 0.45f - px * sz * 0.9f, ly + dy * L * 0.45f - py * sz * 0.9f);
    sting.lineTo (lx - px * sz, ly - py * sz);
    sting.closeSubPath();
    g.setColour (col::teal);
    g.fillPath (sting);
}

void drawStar (juce::Graphics& g, juce::Rectangle<float> r, bool filled, juce::Colour c)
{
    juce::Path p;
    const auto ctr = r.getCentre();
    const float R = juce::jmin (r.getWidth(), r.getHeight()) * 0.5f;
    p.addStar (ctr, 5, R * 0.42f, R, 0.0f);
    g.setColour (c);
    if (filled) g.fillPath (p);
    else g.strokePath (p, juce::PathStrokeType (juce::jmax (1.0f, R * 0.12f), juce::PathStrokeType::mitered));
}
}
