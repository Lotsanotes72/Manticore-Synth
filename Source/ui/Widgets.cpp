#include "Widgets.h"

namespace mc::ui
{
float scaleOf (const juce::Component& c)
{
    if (auto* lf = dynamic_cast<McLookAndFeel*> (&c.getLookAndFeel())) return lf->s;
    return 1.0f;
}

void drawCaption (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text, float s, juce::Colour c)
{
    g.setFont (Fonts::label (13.0f * s));
    g.setColour (c);
    g.drawFittedText (text.toUpperCase(), r, juce::Justification::centred, 1, 0.75f);
}

void drawPanelTitle (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& text, float s, juce::Colour c)
{
    g.setFont (Fonts::bold (14.0f * s));
    g.setColour (c);
    g.drawText (text.toUpperCase(), r, juce::Justification::centredLeft);
}

void setParam (APVTS& s, const juce::String& id, float v)
{
    if (auto* p = s.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (v));
        p->endChangeGesture();
    }
}

float getParam (APVTS& s, const juce::String& id)
{
    if (auto* r = s.getRawParameterValue (id)) return r->load();
    return 0.0f;
}

juce::PopupMenu sourceMenu (int firstId)
{
    juce::PopupMenu m, envs, lfos, msegs, maps, maths, perf;
    const auto names = sourceNames();
    for (int s = 1; s < sNumSources; ++s)
    {
        auto* target = &perf;
        if (s >= sEnv1 && s < sEnv1 + 4) target = &envs;
        else if (s >= sLfo1 && s < sLfo1 + 4) target = &lfos;
        else if (s >= sMseg1 && s < sMseg1 + 4) target = &msegs;
        else if (s >= sMap1 && s < sMap1 + 4) target = &maps;
        else if (s >= sMath1 && s < sMath1 + 4) target = &maths;
        target->addItem (firstId + s, names[s]);
    }
    m.addSubMenu ("Envelopes", envs);
    m.addSubMenu ("LFOs", lfos);
    m.addSubMenu ("MSEGs", msegs);
    m.addSubMenu ("Mappers", maps);
    m.addSubMenu ("Mod Math", maths);
    m.addSubMenu ("Performance", perf);
    return m;
}

static bool sourceBipolar (int s)
{
    return (s >= sLfo1 && s < sLfo1 + 4) || (s >= sMap1 && s < sMap1 + 4) || (s >= sMath1 && s < sMath1 + 4)
        || s == sKey || s == sPitchBend || s == sRandom || s == sAlternate;
}

// ------------------------------------------------------------------ Knob
Knob::Knob (const juce::String& c) : caption (c)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setVelocityBasedMode (false);
    slider.setMouseDragSensitivity (220);
    slider.addMouseListener (this, false);
    addAndMakeVisible (slider);
}

void Knob::attach (APVTS& s, const juce::String& id, const juce::String& tip)
{
    att.reset();
    apvts = &s;
    paramId = id;
    att = std::make_unique<APVTS::SliderAttachment> (s, id, slider);
    if (auto* p = s.getParameter (id)) slider.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
    const int idx = paramIndex (id);
    juce::String t = tip.isNotEmpty() ? tip : (idx >= 0 ? params()[(size_t) idx].tip : juce::String());
    if (caption.isEmpty() && idx >= 0) caption = params()[(size_t) idx].caption;
    if (idx >= 0)
    {
        const auto& d = params()[(size_t) idx];
        setBipolar (d.min < 0 && d.max > 0 && std::abs (d.min + d.max) < 1e-4f);
        if (destOfParam (idx) > 0) t += "\nRight-click to modulate.";
    }
    slider.setTooltip (t + "\nDouble-click the knob to reset, double-click the value to type one.");
    setTooltip (t);
    refreshModulation();
}

void Knob::refreshModulation()
{
    float pos = 0, neg = 0;
    const int idx = paramIndex (paramId);
    const int d = destOfParam (idx);
    if (apvts != nullptr && d > 0)
        for (int i = 0; i < kNumMatrix; ++i)
        {
            if (juce::roundToInt (getParam (*apvts, id::mtx (i, "dst"))) != d) continue;
            const int src = juce::roundToInt (getParam (*apvts, id::mtx (i, "src")));
            const float a = getParam (*apvts, id::mtx (i, "amt"));
            if (src <= 0 || a == 0.0f) continue;
            if (sourceBipolar (src)) { pos += std::abs (a); neg -= std::abs (a); }
            else if (a > 0) pos += a; else neg += a;
        }
    auto& pr = slider.getProperties();
    const bool changed = (float) pr.getWithDefault ("modPos", 0.0f) != pos || (float) pr.getWithDefault ("modNeg", 0.0f) != neg;
    pr.set ("modPos", pos);
    pr.set ("modNeg", neg);
    if (changed) slider.repaint();
}

void Knob::resized()
{
    const float s = scaleOf (*this);
    auto r = getLocalBounds();
    const int cap = juce::roundToInt ((compact ? 13 : 16) * s);
    r.removeFromTop (cap);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, r.getWidth(), juce::roundToInt ((compact ? 15 : 17) * s));
    slider.setBounds (r);
}

void Knob::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    const int cap = juce::roundToInt ((compact ? 13 : 16) * s);
    g.setFont (Fonts::label ((compact ? 11.5f : 13.0f) * s));
    g.setColour (isEnabled() ? col::dim : col::faint);
    g.drawFittedText (caption.toUpperCase(), getLocalBounds().removeFromTop (cap), juce::Justification::centred, 1, 0.7f);
}

void Knob::mouseDown (const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu() && apvts != nullptr) showModMenu();
}

void Knob::showModMenu()
{
    const int idx = paramIndex (paramId);
    const int d = destOfParam (idx);
    juce::PopupMenu m;
    m.addSectionHeader (idx >= 0 ? params()[(size_t) idx].name : paramId);
    if (d > 0)
    {
        m.addSubMenu ("Modulate with", sourceMenu (100));
        juce::PopupMenu existing;
        bool any = false;
        const auto names = sourceNames();
        for (int i = 0; i < kNumMatrix; ++i)
            if (juce::roundToInt (getParam (*apvts, id::mtx (i, "dst"))) == d && getParam (*apvts, id::mtx (i, "src")) > 0.5f)
            {
                existing.addItem (1000 + i, "Remove slot " + juce::String (i + 1) + ": " + names[juce::roundToInt (getParam (*apvts, id::mtx (i, "src")))]
                                                + " (" + juce::String (juce::roundToInt (getParam (*apvts, id::mtx (i, "amt")) * 100.0f)) + "%)");
                any = true;
            }
        if (any) m.addSubMenu ("Remove modulation", existing);
    }
    m.addItem (1, "Reset to default");
    juce::Component::SafePointer<Knob> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&slider), [safe, d] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& s = *safe->apvts;
        if (r == 1)
        {
            if (auto* p = s.getParameter (safe->paramId)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); }
        }
        else if (r >= 1000)
        {
            const int i = r - 1000;
            setParam (s, id::mtx (i, "src"), 0); setParam (s, id::mtx (i, "via"), 0);
            setParam (s, id::mtx (i, "dst"), 0); setParam (s, id::mtx (i, "amt"), 0);
        }
        else if (r > 100)
        {
            const int src = r - 100;
            for (int i = 0; i < kNumMatrix; ++i)
                if (getParam (s, id::mtx (i, "src")) < 0.5f || getParam (s, id::mtx (i, "dst")) < 0.5f)
                {
                    setParam (s, id::mtx (i, "src"), (float) src);
                    setParam (s, id::mtx (i, "via"), 0);
                    setParam (s, id::mtx (i, "dst"), (float) d);
                    setParam (s, id::mtx (i, "amt"), 0.25f);
                    break;
                }
        }
        safe->refreshModulation();
    });
}

// ------------------------------------------------------------------ Toggle / Choice / Action
Toggle::Toggle (const juce::String& text, juce::Colour accent) : juce::TextButton (text)
{
    setClickingTogglesState (true);
    getProperties().set ("accent", (juce::int64) accent.getARGB());
}

void Toggle::attach (APVTS& s, const juce::String& id, const juce::String& tip)
{
    att.reset();
    att = std::make_unique<APVTS::ButtonAttachment> (s, id, *this);
    const int idx = paramIndex (id);
    if (getButtonText().isEmpty() && idx >= 0) setButtonText (params()[(size_t) idx].caption.toUpperCase());
    setTooltip (tip.isNotEmpty() ? tip : (idx >= 0 ? params()[(size_t) idx].tip : juce::String()));
}

Choice::Choice() { setJustificationType (juce::Justification::centredLeft); }

void Choice::attach (APVTS& s, const juce::String& id, const juce::String& tip)
{
    att.reset();
    clear (juce::dontSendNotification);
    if (auto* p = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter (id))) addItemList (p->choices, 1);
    att = std::make_unique<APVTS::ComboBoxAttachment> (s, id, *this);
    const int idx = paramIndex (id);
    setTooltip (tip.isNotEmpty() ? tip : (idx >= 0 ? params()[(size_t) idx].tip : juce::String()));
}

Action::Action (const juce::String& text, juce::Colour accent, const juce::String& tip) : juce::TextButton (text)
{
    getProperties().set ("accent", (juce::int64) accent.getARGB());
    setTooltip (tip);
}

void StarButton::paintButton (juce::Graphics& g, bool over, bool)
{
    const auto r = getLocalBounds().toFloat().reduced (getHeight() * 0.12f);
    const bool on = getToggleState();
    drawStar (g, r, on, on ? col::amber : (over ? col::amber.withAlpha (0.8f) : col::dim));
}

// ------------------------------------------------------------------ DestPicker
DestPicker::~DestPicker() { if (apvts != nullptr) apvts->removeParameterListener (pid, this); }

void DestPicker::attach (APVTS& s, const juce::String& id)
{
    if (apvts != nullptr) apvts->removeParameterListener (pid, this);
    apvts = &s; pid = id;
    s.addParameterListener (id, this);
    setTooltip ("Modulation target. Click to choose a parameter, grouped by module.");
    repaint();
}

void DestPicker::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto r = getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (isMouseOver() ? col::panelHi.brighter (0.1f) : col::panelHi);
    g.fillRoundedRectangle (r, 4.0f * s);
    g.setColour (col::edge.brighter (0.15f));
    g.drawRoundedRectangle (r, 4.0f * s, 1.0f);
    const int d = apvts ? juce::roundToInt (getParam (*apvts, pid)) : 0;
    const auto names = destNames();
    g.setFont (Fonts::label (juce::jmin (15.0f * s, (float) getHeight() * 0.62f)));
    g.setColour (d > 0 ? col::text : col::faint);
    g.drawFittedText (names[d], getLocalBounds().reduced (juce::roundToInt (6 * s), 0).withTrimmedRight (juce::roundToInt (14 * s)),
                      juce::Justification::centredLeft, 1, 0.8f);
    const float a = juce::jmin (6.0f * s, (float) getHeight() * 0.22f);
    const float ax = (float) getWidth() - 10.0f * s, ay = (float) getHeight() * 0.5f;
    juce::Path p;
    p.addTriangle (ax - a, ay - a * 0.5f, ax + a, ay - a * 0.5f, ax, ay + a * 0.6f);
    g.setColour (col::amber);
    g.fillPath (p);
}

void DestPicker::mouseDown (const juce::MouseEvent&)
{
    if (apvts == nullptr) return;
    juce::PopupMenu m;
    m.addItem (1, "None");
    const auto& dp = destParams();
    const auto& defs = params();
    juce::String group;
    juce::PopupMenu sub;
    std::vector<std::pair<juce::String, juce::PopupMenu>> groups;
    for (int k = 0; k < (int) dp.size(); ++k)
    {
        const auto& d = defs[(size_t) dp[(size_t) k]];
        juce::String gname = d.inst >= 0 ? moduleName (d.kind, d.inst) : d.name.upToFirstOccurrenceOf (" ", false, false) + " " +
                             d.name.fromFirstOccurrenceOf (" ", false, false).upToFirstOccurrenceOf (" ", false, false);
        if (d.id.startsWith ("aux")) gname = "Aux Returns";
        if (gname != group)
        {
            if (group.isNotEmpty()) groups.push_back ({ group, sub });
            sub = {};
            group = gname;
        }
        sub.addItem (2 + k, d.caption + (d.global ? juce::String ("  (FX)") : juce::String()));
    }
    if (group.isNotEmpty()) groups.push_back ({ group, sub });
    // fold the per-instance groups into families to keep the menu short
    juce::PopupMenu voiceM, modM, fxM, laneM;
    for (auto& [n, menu] : groups)
    {
        if (n.startsWith ("ENV") || n.startsWith ("LFO") || n.startsWith ("MSEG") || n.startsWith ("MAP") || n.startsWith ("MATH")) modM.addSubMenu (n, menu);
        else if (n.startsWith ("Lane")) laneM.addSubMenu (n, menu);
        else if (n.startsWith ("MODFX") || n.startsWith ("DELAY") || n.startsWith ("REVERB") || n.startsWith ("TEXTURE") || n.startsWith ("COMP")
                 || n.startsWith ("FX") || n.startsWith ("DIST") || n.startsWith ("FILTER") || n.startsWith ("Aux")) fxM.addSubMenu (n, menu);
        else voiceM.addSubMenu (n, menu);
    }
    m.addSubMenu ("Generators & Filters", voiceM);
    m.addSubMenu ("Lanes", laneM);
    m.addSubMenu ("Modulators", modM);
    m.addSubMenu ("Effects (global sources only)", fxM);
    juce::Component::SafePointer<DestPicker> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
    {
        if (safe == nullptr || r == 0) return;
        setParam (*safe->apvts, safe->pid, (float) (r - 1));
    });
}

// ------------------------------------------------------------------ meter
void PeakMeter::setLevel (float db)
{
    level = juce::jmax (db, level - 1.5f);
    if (db >= hold) { hold = db; holdTicks = 45; }
    else if (--holdTicks < 0) hold = juce::jmax (-120.0f, hold - 1.0f);
    repaint();
}

void PeakMeter::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto r = getLocalBounds().toFloat();
    if (caption.isNotEmpty()) drawCaption (g, r.removeFromBottom (14 * s).toNearestInt(), caption, s);
    g.setColour (col::bg);
    g.fillRoundedRectangle (r, 2.0f * s);
    auto map = [&] (float db) { return juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f); };
    const float h = r.getHeight() * map (level);
    juce::ColourGradient gr (col::teal, r.getX(), r.getBottom(), col::red, r.getX(), r.getY(), false);
    gr.addColour (0.75, col::amber);
    g.setGradientFill (gr);
    g.fillRect (r.withTop (r.getBottom() - h).reduced (1.0f * s, 0));
    g.setColour (hold > 0.0f ? col::red : col::amber);
    g.fillRect (r.getX(), r.getBottom() - r.getHeight() * map (hold), r.getWidth(), 1.5f * s);
    g.setColour (col::edge);
    g.drawRoundedRectangle (r, 2.0f * s, 1.0f);
}
}
