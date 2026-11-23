#include "Pages.h"

namespace mc::ui
{
namespace
{
struct Lay
{
    std::vector<int> top;                    // choices / toggles
    std::vector<std::vector<int>> rows;      // knobs
};

const Lay& layoutFor (Kind k)
{
    static std::map<Kind, Lay> m {
        { Kind::Osc,   { { op::engine, op::table, op::fx1Type, op::fx2Type, op::freeRun },
                         { { op::wave, op::tune, op::fine, op::vol, op::pan, op::keytrack },
                           { op::uniVoices, op::uniDetune, op::uniWidth, op::phase, op::fx1Amt, op::fx2Amt },
                           { op::partials, op::bright, op::oddEven, op::stretch, op::pDecay } } } },
        { Kind::FM,    { { fmp::modShape }, { { fmp::tune, fmp::fine, fmp::ratio, fmp::depth, fmp::feedback }, { fmp::inputFm, fmp::vol, fmp::pan, fmp::keytrack } } } },
        { Kind::Noise, { { np::type }, { { np::color, np::density, np::width }, { np::vol, np::pan } } } },
        { Kind::Exc,   { { xp::mode }, { { xp::decay, xp::color, xp::rate }, { xp::level, xp::width } } } },
        { Kind::Res,   { { rp::profile }, { { rp::tune, rp::fine, rp::decay, rp::damping, rp::bright }, { rp::spread, rp::input, rp::mix } } } },
        { Kind::Comb,  { { cp::mode }, { { cp::tune, cp::fine, cp::feedback, cp::damping }, { cp::input, cp::mix, cp::vol } } } },
        { Kind::Vcf,   { { vp::model, vp::response }, { { vp::cutoff, vp::reso, vp::drive, vp::keytrack }, { vp::spread, vp::mix } } } },
        { Kind::Shape, { { sp::mode }, { { sp::drive, sp::bias, sp::tone }, { sp::mix, sp::out } } } },
        { Kind::Ring,  { { gp::source }, { { gp::tune, gp::fine }, { gp::keytrack, gp::mix } } } },
        { Kind::Eq,    { {}, { { ep::lowFreq, ep::lowGain, ep::midFreq, ep::midGain }, { ep::midQ, ep::highFreq, ep::highGain } } } },
        { Kind::Env,   { { envp::oneShot }, { { envp::delay, envp::attack, envp::decay, envp::sustain, envp::release }, { envp::slope, envp::velocity, envp::keyscale } } } },
        { Kind::Lfo,   { { lfop::wave, lfop::mode, lfop::division, lfop::sync, lfop::unipolar }, { { lfop::rate, lfop::phase, lfop::amount }, { lfop::delay, lfop::fade } } } },
        { Kind::Mseg,  { { msp::loop, msp::sync, msp::bipolar }, { { msp::time, msp::velocity, msp::amount } } } },
        { Kind::Map,   { { mpp::input, mpp::mode, mpp::division }, { { mpp::steps, mpp::smooth, mpp::amount } } } },
        { Kind::Math,  { { mathp::srcA, mathp::srcB, mathp::opr }, { { mathp::amount } } } },
        { Kind::ModFx, { { mfx::mode, mfx::character }, { { mfx::rate, mfx::depth, mfx::feedback, mfx::voices }, { mfx::centre, mfx::width, mfx::mix } } } },
        { Kind::Delay, { { dly::mode, dly::divL, dly::divR, dly::style, dly::sync }, { { dly::timeMs, dly::feedback, dly::lowCut, dly::highCut }, { dly::modDepth, dly::diffusion, dly::mix } } } },
        { Kind::Reverb, { { rvb::model }, { { rvb::size, rvb::decay, rvb::predelay, rvb::damping, rvb::diffusion }, { rvb::modDepth, rvb::width, rvb::lowCut, rvb::mix } } } },
        { Kind::Texture, { { txp::freeze }, { { txp::grain, txp::density, txp::pitch, txp::spread, txp::reverse }, { txp::feedback, txp::jitter, txp::damping, txp::mix } } } },
        { Kind::Comp,  { {}, { { cmp::threshold, cmp::ratio, cmp::attack, cmp::release }, { cmp::knee, cmp::makeup, cmp::mix } } } },
        { Kind::FxEq,  { {}, { { feq::lowFreq, feq::lowGain, feq::m1Freq, feq::m1Gain, feq::m1Q }, { feq::m2Freq, feq::m2Gain, feq::m2Q, feq::highFreq, feq::highGain } } } },
        { Kind::Dist,  { { dsp_::mode }, { { dsp_::drive, dsp_::tone, dsp_::bias }, { dsp_::mix, dsp_::out } } } },
        { Kind::FxFilter, { { ffp::type }, { { ffp::cutoff, ffp::reso }, { ffp::drive, ffp::mix } } } },
    };
    return m[k];
}

bool isFxKind (Kind k) { return (int) k >= (int) Kind::ModFx; }
bool isModKind (Kind k) { return k == Kind::Env || k == Kind::Lfo || k == Kind::Mseg || k == Kind::Map || k == Kind::Math; }

juce::String whereIs (APVTS& s, Kind k, int inst)
{
    const bool fx = isFxKind (k);
    const auto& entries = fx ? fxEntries() : gridEntries();
    for (int l = 0; l < 4; ++l)
        for (int sl = 0; sl < 4; ++sl)
        {
            const int e = juce::roundToInt (getParam (s, fx ? id::fxSlot (l, sl) : id::slot (l, sl)));
            if (e > 0 && entries[(size_t) e].kind == k && entries[(size_t) e].inst == inst)
            {
                if (fx) { const char* b[] { "MAIN", "AUX 1", "AUX 2", "AUX 3" }; return juce::String (b[l]) + " SLOT " + juce::String (sl + 1); }
                return "LANE " + juce::String (l + 1) + " " + dot() + " SLOT " + juce::String (sl + 1);
            }
        }
    return {};
}
}

// ======================================================================= LockButton
LockButton::LockButton (ManticoreProcessor& p, int b, const juce::String& l, const juce::String& what)
    : juce::Button ("lock"), proc (p), bit (b), label (l)
{
    setTooltip ("Padlock: " + what + " Mutate, Breed and Haunt leave a locked section alone.");
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

int LockButton::preferredWidth (float s) const
{
    const int icon = juce::roundToInt (18 * s);
    if (label.isEmpty()) return icon;
    return icon + juce::roundToInt (6 * s) + juce::roundToInt (juce::GlyphArrangement::getStringWidth (Fonts::label (12.5f * s), label.toUpperCase()));
}

void LockButton::paintButton (juce::Graphics& g, bool over, bool)
{
    const float s = scaleOf (*this);
    const bool on = proc.isLocked (bit);
    const auto c = on ? col::amber : (over ? col::text : col::faint.brighter (0.35f));
    auto r = getLocalBounds().toFloat();
    const float icon = juce::jmin (r.getHeight(), 18.0f * s);
    drawPadlock (g, r.removeFromLeft (icon).withSizeKeepingCentre (icon * 0.82f, icon * 0.82f), on, c);
    if (label.isNotEmpty())
    {
        r.removeFromLeft (5.0f * s);
        g.setFont (Fonts::label (12.5f * s));
        g.setColour (c);
        g.drawText (label.toUpperCase(), r, juce::Justification::centredLeft);
    }
}

void LockButton::clicked()
{
    proc.setLocked (bit, ! proc.isLocked (bit));
    repaint();
}

// ======================================================================= ModuleEditor
ModuleEditor::ModuleEditor (Ctx& c, bool wd) : ctx (c), withDisplay (wd), display (c.proc, c.live), mseg (c.proc, c.live), mapEd (c.proc, c.live)
{
    addChildComponent (display);
    addChildComponent (mseg);
    addChildComponent (mapEd);
    addChildComponent (mutBtn);
    mutBtn.onClick = [this]
    {
        if (kind == Kind::Count) return;
        juce::String msg;
        ctx.proc.mutateModule (kind, inst, msg);
        ctx.status (msg, 90);
    };
}

void ModuleEditor::setModule (Kind k, int i)
{
    if (k == kind && i == inst) return;
    kind = k; inst = i;
    knobs.clear(); tops.clear(); knobLocals.clear(); rowSizes.clear();
    auto& s = ctx.proc.apvts;
    const auto& lay = layoutFor (k);
    for (int local : lay.top)
    {
        const auto& d = params()[(size_t) pidx (k, i, local)];
        if (d.type == ParamDef::Bool)
        {
            auto t = std::make_unique<Toggle> (d.caption.toUpperCase(), kindColour (k));
            t->attach (s, d.id);
            t->getProperties().set ("local", local);
            addAndMakeVisible (*t);
            tops.push_back (std::move (t));
        }
        else
        {
            auto c = std::make_unique<Choice>();
            c->attach (s, d.id);
            c->getProperties().set ("local", local);
            c->getProperties().set ("caption", d.caption);
            addAndMakeVisible (*c);
            tops.push_back (std::move (c));
        }
    }
    for (const auto& row : lay.rows)
    {
        rowSizes.push_back ((int) row.size());
        for (int local : row)
        {
            auto kb = std::make_unique<Knob>();
            kb->attach (s, pid (k, i, local));
            kb->setAccent (kindColour (k));
            addAndMakeVisible (*kb);
            knobs.push_back (std::move (kb));
            knobLocals.push_back (local);
        }
    }
    mutBtn.setVisible (true);
    display.setVisible (withDisplay && k != Kind::Mseg && k != Kind::Map);
    mseg.setVisible (withDisplay && k == Kind::Mseg);
    mapEd.setVisible (withDisplay && k == Kind::Map);
    display.setModule (k, i);
    mseg.setIndex (i);
    mapEd.setIndex (i);
    resized();
    tick();
    repaint();
}

void ModuleEditor::refreshModulation()
{
    for (auto& k : knobs) k->refreshModulation();
}

void ModuleEditor::tick()
{
    if (kind == Kind::Count) return;
    display.tick();
    if (mseg.isVisible()) mseg.repaint();
    auto V = [&] (int local) { return ctx.live.v (kind, inst, local); };
    for (size_t n = 0; n < knobs.size(); ++n)
    {
        const int local = knobLocals[n];
        bool en = true;
        switch (kind)
        {
            case Kind::Osc:
            {
                const bool add = V (op::engine) > 0.5f;
                if (local == op::partials || local == op::bright || local == op::oddEven || local == op::stretch || local == op::pDecay) en = add;
                if (local == op::uniVoices || local == op::uniDetune) en = ! add;
                if (local == op::phase) en = V (op::freeRun) < 0.5f;
                if (local == op::fx1Amt) en = V (op::fx1Type) > 0.5f;
                if (local == op::fx2Amt) en = V (op::fx2Type) > 0.5f;
                break;
            }
            case Kind::Lfo: if (local == lfop::rate) en = V (lfop::sync) < 0.5f; break;
            case Kind::Delay: if (local == dly::timeMs) en = V (dly::sync) < 0.5f; break;
            case Kind::Exc: if (local == xp::rate) en = juce::roundToInt (V (xp::mode)) == 4; break;
            case Kind::Noise: if (local == np::density) en = V (np::type) > 3.5f; break;
            case Kind::Ring: if (local == gp::tune || local == gp::fine || local == gp::keytrack) en = V (gp::source) < 2.5f; break;
            case Kind::ModFx: if (local == mfx::feedback) en = V (mfx::mode) > 0.5f; break;
            default: break;
        }
        if (knobs[n]->isEnabled() != en) knobs[n]->setEnabled (en);
    }
    for (auto& t : tops)
    {
        const int local = (int) t->getProperties()["local"];
        bool en = true;
        if (kind == Kind::Lfo && local == lfop::division) en = V (lfop::sync) > 0.5f;
        if (kind == Kind::Delay && (local == dly::divL || local == dly::divR)) en = V (dly::sync) > 0.5f;
        if (kind == Kind::Map && local == mpp::division) en = V (mpp::mode) > 0.5f;
        if (kind == Kind::Map && local == mpp::input) en = V (mpp::mode) < 0.5f;
        if (t->isEnabled() != en) t->setEnabled (en);
    }
}

void ModuleEditor::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    drawPanel (g, getLocalBounds().toFloat(), s);
    if (kind == Kind::Count) return;
    auto t = titleArea;
    g.setFont (Fonts::title (19.0f * s));
    g.setColour (kindColour (kind));
    const auto name = moduleName (kind, inst);
    g.drawText (name, t, juce::Justification::centredLeft);
    const int w = juce::roundToInt (juce::GlyphArrangement::getStringWidth (Fonts::title (19.0f * s), name)) + juce::roundToInt (10 * s);
    g.setFont (Fonts::label (14.0f * s));
    g.setColour (col::dim);
    g.drawText (juce::String (kindInfo (kind).longName).toUpperCase(), t.withTrimmedLeft (w), juce::Justification::centredLeft);
    if (! isModKind (kind))
    {
        const auto where = whereIs (ctx.proc.apvts, kind, inst);
        g.setColour (where.isEmpty() ? col::red.withAlpha (0.85f) : col::teal.withAlpha (0.8f));
        g.drawText (where.isEmpty() ? juce::String ("NOT IN THE GRID - right-click a slot to place it") : where,
                    t.withRight (mutBtn.getX() - juce::roundToInt (12 * s)), juce::Justification::centredRight);
    }
    // captions over the choice boxes
    for (auto& c : tops)
        if (auto* cb = dynamic_cast<Choice*> (c.get()))
            drawCaption (g, cb->getBounds().translated (0, -juce::roundToInt (15 * s)).withHeight (juce::roundToInt (14 * s)),
                         cb->getProperties()["caption"].toString(), s);
}

void ModuleEditor::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds().reduced (S (12), S (8));
    titleArea = r.removeFromTop (S (26));
    mutBtn.setBounds (titleArea.withLeft (titleArea.getRight() - S (78)).reduced (0, S (1)));
    r.removeFromTop (S (6));
    if (kind == Kind::Count) return;
    if (withDisplay)
    {
        const float frac = (kind == Kind::Mseg || kind == Kind::Map) ? 0.56f : (isModKind (kind) ? 0.42f : 0.40f);
        auto d = r.removeFromLeft (juce::roundToInt ((float) r.getWidth() * frac));
        display.setBounds (d); mseg.setBounds (d); mapEd.setBounds (d);
        r.removeFromLeft (S (14));
    }
    // top row: choices and toggles (wrap when needed)
    if (! tops.empty())
    {
        r.removeFromTop (S (15));
        auto row = r.removeFromTop (S (26));
        for (auto& t : tops)
        {
            const bool isToggle = dynamic_cast<Toggle*> (t.get()) != nullptr;
            int w = isToggle ? S (84) : S (124);
            if (row.getWidth() < w)
            {
                r.removeFromTop (S (19));
                row = r.removeFromTop (S (26));
            }
            t->setBounds (row.removeFromLeft (w).withTrimmedRight (S (8)));
        }
        r.removeFromTop (S (6));
    }
    // knob rows
    const int nRows = (int) rowSizes.size();
    if (nRows == 0) return;
    int maxRow = 0;
    for (int n : rowSizes) maxRow = juce::jmax (maxRow, n);
    const int cellW = juce::jmin (S (86), r.getWidth() / juce::jmax (1, maxRow));
    const int cellH = juce::jmin (S (88), r.getHeight() / nRows);
    size_t k = 0;
    for (int row = 0; row < nRows; ++row)
    {
        auto rr = r.removeFromTop (cellH);
        for (int c = 0; c < rowSizes[(size_t) row]; ++c)
            knobs[k++]->setBounds (rr.removeFromLeft (cellW).reduced (S (2), 0));
    }
}

// ======================================================================= SlotTile
SlotTile::SlotTile (Ctx& c, bool f, int l, int sl) : ctx (c), fx (f), lane (l), slot (sl)
{
    pid = fx ? id::fxSlot (l, sl) : id::slot (l, sl);
    setTooltip (fx ? "Effect slot. Click to edit the effect, right-click (or click an empty slot) to choose one."
                   : "Grid slot. Signal flows down the lane. Click to edit the module, right-click (or click an empty slot) to choose one.");
}

int SlotTile::entry() const { return juce::roundToInt (getParam (ctx.proc.apvts, pid)); }

void SlotTile::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto r = getLocalBounds().toFloat().reduced (2.0f * s);
    const int e = entry();
    const auto& entries = fx ? fxEntries() : gridEntries();
    g.setColour (e > 0 ? col::panelHi : col::bg2);
    g.fillRoundedRectangle (r, 5.0f * s);
    if (e <= 0 || e >= (int) entries.size())
    {
        g.setColour (col::edge);
        g.drawRoundedRectangle (r, 5.0f * s, 1.0f);
        g.setFont (Fonts::label (13.0f * s));
        g.setColour (col::faint);
        g.drawText ("+", r, juce::Justification::centred);
        return;
    }
    const auto& ge = entries[(size_t) e];
    const auto c = fx ? col::teal : kindColour (ge.kind);
    // duplicate check (only the first use of a module is active)
    bool dup = false;
    for (int l = 0; l < 4 && ! dup; ++l)
        for (int sl = 0; sl < 4; ++sl)
        {
            if (l == lane && sl == slot) { l = 4; break; }
            if (juce::roundToInt (getParam (ctx.proc.apvts, fx ? id::fxSlot (l, sl) : id::slot (l, sl))) == e) { dup = true; break; }
        }
    juce::ColourGradient grad (c.withAlpha (selected ? 0.32f : 0.16f), r.getX(), r.getY(), col::panelHi.withAlpha (0.0f), r.getRight(), r.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (r, 5.0f * s);
    g.setColour (c);
    g.fillRoundedRectangle (r.getX(), r.getY() + 4 * s, 3.0f * s, r.getHeight() - 8 * s, 1.5f * s);
    g.setColour (selected ? c : col::edge.brighter (0.2f));
    g.drawRoundedRectangle (r, 5.0f * s, selected ? 1.8f : 1.0f);
    auto tr = r.reduced (9.0f * s, 4.0f * s);
    g.setFont (Fonts::bold (15.0f * s));
    g.setColour (dup ? col::red : col::text);
    g.drawText (moduleName (ge.kind, ge.inst), tr.removeFromTop (tr.getHeight() * 0.55f), juce::Justification::bottomLeft);
    // sub label: the module's most telling setting
    juce::String sub;
    const auto& L = ctx.live;
    switch (ge.kind)
    {
        case Kind::Osc: sub = tableNames()[L.c (Kind::Osc, ge.inst, op::table)]; if (L.c (Kind::Osc, ge.inst, op::engine) == 1) sub = "Add: " + sub; break;
        case Kind::FM: sub = "x" + juce::String (L.v (Kind::FM, ge.inst, fmp::ratio), 2); break;
        case Kind::Vcf: sub = params()[(size_t) pidx (Kind::Vcf, ge.inst, vp::model)].choices[L.c (Kind::Vcf, ge.inst, vp::model)]
                            + " " + params()[(size_t) pidx (Kind::Vcf, ge.inst, vp::response)].choices[L.c (Kind::Vcf, ge.inst, vp::response)]; break;
        case Kind::Res: sub = params()[(size_t) pidx (Kind::Res, ge.inst, rp::profile)].choices[L.c (Kind::Res, ge.inst, rp::profile)]; break;
        case Kind::Shape: sub = params()[(size_t) pidx (Kind::Shape, ge.inst, sp::mode)].choices[L.c (Kind::Shape, ge.inst, sp::mode)]; break;
        case Kind::Noise: sub = params()[(size_t) pidx (Kind::Noise, ge.inst, np::type)].choices[L.c (Kind::Noise, ge.inst, np::type)]; break;
        case Kind::Exc: sub = params()[(size_t) pidx (Kind::Exc, ge.inst, xp::mode)].choices[L.c (Kind::Exc, ge.inst, xp::mode)]; break;
        case Kind::Comb: sub = L.c (Kind::Comb, ge.inst, cp::mode) == 1 ? "Diffuse" : "Comb"; break;
        case Kind::Ring: sub = params()[(size_t) pidx (Kind::Ring, ge.inst, gp::source)].choices[L.c (Kind::Ring, ge.inst, gp::source)]; break;
        case Kind::ModFx: sub = params()[(size_t) pidx (Kind::ModFx, ge.inst, mfx::mode)].choices[L.c (Kind::ModFx, ge.inst, mfx::mode)]; break;
        case Kind::Reverb: sub = L.c (Kind::Reverb, ge.inst, rvb::model) == 1 ? "Lush" : "Classic"; break;
        case Kind::Dist: sub = params()[(size_t) pidx (Kind::Dist, ge.inst, dsp_::mode)].choices[L.c (Kind::Dist, ge.inst, dsp_::mode)]; break;
        case Kind::Delay: sub = params()[(size_t) pidx (Kind::Delay, ge.inst, dly::mode)].choices[L.c (Kind::Delay, ge.inst, dly::mode)]; break;
        default: sub = kindInfo (ge.kind).longName; break;
    }
    if (dup) sub = "Duplicate - inactive";
    g.setFont (Fonts::value (12.5f * s));
    g.setColour (dup ? col::red : col::dim);
    g.drawFittedText (sub, tr.toNearestInt(), juce::Justification::topLeft, 1, 0.7f);
}

void SlotTile::mouseDown (const juce::MouseEvent& e)
{
    const int en = entry();
    if (e.mods.isPopupMenu() || en <= 0) { showMenu(); return; }
    const auto& ge = (fx ? fxEntries() : gridEntries())[(size_t) en];
    ctx.selectModule (ge.kind, ge.inst);
}

void SlotTile::showMenu()
{
    auto& s = ctx.proc.apvts;
    const auto& entries = fx ? fxEntries() : gridEntries();
    // where is each entry used?
    std::vector<juce::String> usedAt (entries.size());
    std::vector<juce::String> usedPid (entries.size());
    for (int l = 0; l < 4; ++l)
        for (int sl = 0; sl < 4; ++sl)
        {
            const auto p = fx ? id::fxSlot (l, sl) : id::slot (l, sl);
            const int e = juce::roundToInt (getParam (s, p));
            if (e > 0 && usedAt[(size_t) e].isEmpty() && ! (l == lane && sl == slot))
            {
                usedAt[(size_t) e] = fx ? juce::String ("bus ") + juce::String (l + 1) + " slot " + juce::String (sl + 1)
                                        : "L" + juce::String (l + 1) + " S" + juce::String (sl + 1);
                usedPid[(size_t) e] = p;
            }
        }
    juce::PopupMenu m;
    m.addSectionHeader (fx ? "Choose an effect" : "Choose a module");
    m.addItem (1, "Empty slot");
    m.addSeparator();
    Kind cur = Kind::Count;
    juce::PopupMenu sub;
    auto flush = [&] { if (cur != Kind::Count) m.addSubMenu (kindInfo (cur).longName, sub); sub = {}; };
    for (size_t e = 1; e < entries.size(); ++e)
    {
        if (entries[e].kind != cur) { flush(); cur = entries[e].kind; }
        auto label = moduleName (entries[e].kind, entries[e].inst);
        if (usedAt[e].isNotEmpty()) label << "   (move from " << usedAt[e] << ")";
        sub.addItem ((int) e + 1, label, true, (int) e == entry());
    }
    flush();
    juce::Component::SafePointer<SlotTile> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe, usedPid] (int r)
    {
        if (safe == nullptr || r == 0) return;
        auto& st = safe->ctx.proc.apvts;
        const int e = r - 1;
        if (e > 0 && usedPid[(size_t) e].isNotEmpty()) setParam (st, usedPid[(size_t) e], 0);
        setParam (st, safe->pid, (float) e);
        if (e > 0)
        {
            const auto& ge = (safe->fx ? fxEntries() : gridEntries())[(size_t) e];
            safe->ctx.selectModule (ge.kind, ge.inst);
        }
        if (safe->onPick) safe->onPick();
    });
}

// ======================================================================= GridPanel
GridPanel::GridPanel (Ctx& c, bool f) : ctx (c), fx (f)
{
    for (int l = 0; l < 4; ++l)
        for (int s = 0; s < 4; ++s)
            addAndMakeVisible (tiles.add (new SlotTile (ctx, fx, l, s)));
    if (! fx)
        for (int l = 0; l < 4; ++l)
        {
            auto* cb = inputs.add (new Choice());
            cb->attach (ctx.proc.apvts, id::lane (l, "in"));
            addAndMakeVisible (cb);
        }
    if (fx) locks.add (new LockButton (ctx.proc, kLockFx, "Effects", "keeps the effect chains, effect settings and aux returns as they are."));
    else
    {
        locks.add (new LockButton (ctx.proc, kLockGrid, "Layout", "keeps every module in its slot (and the lane inputs), so Mutate and Breed can't move, add or remove modules."));
        for (int l = 0; l < 4; ++l)
            locks.add (new LockButton (ctx.proc, kLockLane1 + l, {}, "keeps the settings of the modules in Lane " + juce::String (l + 1) + " and its mixer controls."));
    }
    for (auto* b : locks) addAndMakeVisible (b);
}

void GridPanel::tick (Kind selKind, int selInst)
{
    const auto& entries = fx ? fxEntries() : gridEntries();
    for (int i = 0; i < 16; ++i)
    {
        const int e = tiles[i]->entry();
        tiles[i]->setSelected (e > 0 && entries[(size_t) e].kind == selKind && entries[(size_t) e].inst == selInst);
        if (e != lastEntries[(size_t) i]) { lastEntries[(size_t) i] = e; tiles[i]->repaint(); }
    }
    if (ctx.live.version % 4 == 0) for (auto* t : tiles) t->repaint();
}

void GridPanel::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    drawPanel (g, getLocalBounds().toFloat(), s);
    auto r = getLocalBounds().reduced (juce::roundToInt (12 * s), juce::roundToInt (8 * s));
    drawPanelTitle (g, r.removeFromTop (juce::roundToInt (20 * s)), fx ? "FX Grid" : "Generator Grid", s);
    const char* fxNames[] { "MAIN", "AUX 1", "AUX 2", "AUX 3" };
    for (int l = 0; l < 4; ++l)
    {
        const auto t = tiles[l * 4]->getBounds();
        drawCaption (g, juce::Rectangle<int> (t.getX(), t.getY() - juce::roundToInt ((fx ? 18 : 44) * s), t.getWidth(), juce::roundToInt (16 * s)),
                     fx ? juce::String (fxNames[l]) : "LANE " + juce::String (l + 1), s, l % 2 == 0 ? col::teal : col::amber);
        // flow chevrons between tiles
        for (int sl = 0; sl < 4; ++sl)
        {
            const auto b = tiles[l * 4 + sl]->getBounds().toFloat();
            const float cx = b.getCentreX(), y = b.getBottom() + 1.0f * s;
            juce::Path p;
            p.addTriangle (cx - 4 * s, y, cx + 4 * s, y, cx, y + 4 * s);
            g.setColour (col::faint);
            g.fillPath (p);
        }
        const auto last = tiles[l * 4 + 3]->getBounds();
        drawCaption (g, juce::Rectangle<int> (last.getX(), last.getBottom() + juce::roundToInt (5 * s), last.getWidth(), juce::roundToInt (14 * s)),
                     fx ? (l == 0 ? juce::String ("OUT") : juce::String ("TO MAIN")) : juce::String ("OUT"), s, col::faint);
    }
}

void GridPanel::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds().reduced (S (12), S (8));
    r.removeFromTop (S (20));
    r.removeFromTop (S (fx ? 20 : 20));
    if (! fx)
    {
        auto ir = r.removeFromTop (S (24));
        const int cw = ir.getWidth() / 4;
        for (int l = 0; l < 4; ++l) inputs[l]->setBounds (ir.removeFromLeft (cw).reduced (S (3), 0));
        r.removeFromTop (S (6));
    }
    r.removeFromBottom (S (20));
    const int cw = r.getWidth() / 4, ch = r.getHeight() / 4;
    for (int l = 0; l < 4; ++l)
        for (int sl = 0; sl < 4; ++sl)
            tiles[l * 4 + sl]->setBounds (r.getX() + l * cw, r.getY() + sl * ch, cw, ch - S (6));
    // padlocks: the section lock at the right of the title, lane locks beside the lane captions
    auto title = getLocalBounds().reduced (S (12), S (8)).removeFromTop (S (20));
    locks[0]->setBounds (title.removeFromRight (locks[0]->preferredWidth (s)).reduced (0, S (1)));
    if (! fx)
        for (int l = 0; l < 4; ++l)
        {
            const auto t = tiles[l * 4]->getBounds();
            locks[1 + l]->setBounds (t.getRight() - S (24), t.getY() - S (45), S (18), S (18));
        }
}

// ======================================================================= SynthPage
SynthPage::SynthPage (Ctx& c) : ctx (c), grid (c, false), editor (c, true)
{
    addAndMakeVisible (grid);
    addAndMakeVisible (editor);
    addAndMakeVisible (rack);
    auto& s = ctx.proc.apvts;
    for (int l = 0; l < 4; ++l)
    {
        const char* what[] { "vol", "pan", "send1", "send2", "send3" };
        const char* cap[] { "Vol", "Pan", "Aux 1", "Aux 2", "Aux 3" };
        for (int k = 0; k < 5; ++k)
        {
            auto* kb = laneKnobs.add (new Knob (cap[k]));
            kb->compact = k >= 2;
            kb->attach (s, id::lane (l, what[k]));
            kb->setAccent (k >= 2 ? col::amber : (l % 2 == 0 ? col::teal : col::amber));
            addAndMakeVisible (kb);
        }
    }
    for (int m = 0; m < 4; ++m)
    {
        auto* kb = macroKnobs.add (new Knob (juce::String ("Ctrl ") + (char) ('A' + m)));
        kb->attach (s, id::macro (m));
        kb->setAccent (col::amber);
        addAndMakeVisible (kb);
    }
    for (auto [pid, cap] : std::initializer_list<std::pair<const char*, const char*>> { { id::voices, "Voices" }, { id::glide, "Glide" }, { id::pbUp, "Bend +" },
                                                                                        { id::pbDown, "Bend -" }, { id::transpose, "Transpose" }, { id::master, "Master" } })
    {
        auto* kb = voiceKnobs.add (new Knob (cap));
        kb->attach (s, pid);
        kb->setAccent (juce::String (pid) == id::master ? col::amber : col::teal);
        addAndMakeVisible (kb);
    }
    for (auto pid : { id::vmode, id::glideMode })
    {
        auto* cb = voiceChoices.add (new Choice());
        cb->attach (s, pid);
        addAndMakeVisible (cb);
    }
    meterL.caption = "L"; meterR.caption = "R";
    meterL.setTooltip ("Output peak level."); meterR.setTooltip ("Output peak level.");
    addAndMakeVisible (meterL);
    addAndMakeVisible (meterR);
}

void SynthPage::showModule (Kind k, int i) { editor.setModule (k, i); }

void SynthPage::tick()
{
    grid.tick (editor.getKind(), editor.getInst());
    editor.tick();
}

void SynthPage::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    for (auto a : { laneArea, macroArea, voiceArea }) drawPanel (g, a.toFloat(), s);
    drawPanelTitle (g, laneArea.reduced (S (12), S (8)).removeFromTop (S (18)), "Lane Mixer", s);
    drawPanelTitle (g, macroArea.reduced (S (12), S (8)).removeFromTop (S (18)), "Macros  " + dot() + "  Ctrl A-D", s);
    drawPanelTitle (g, voiceArea.reduced (S (12), S (8)).removeFromTop (S (18)), "Voice & Output", s);
    for (int l = 0; l < 4; ++l)
        drawCaption (g, laneCols[l].withHeight (S (14)), "LANE " + juce::String (l + 1), s, l % 2 == 0 ? col::teal : col::amber);
    // captions for the voice choices
    for (int i = 0; i < voiceChoices.size(); ++i)
        drawCaption (g, voiceChoices[i]->getBounds().translated (0, -S (15)).withHeight (S (14)), i == 0 ? "Voice Mode" : "Glide Mode", s);
}

void SynthPage::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    auto r = getLocalBounds();
    auto left = r.removeFromLeft (S (446));
    r.removeFromLeft (S (8));
    gridArea = left.removeFromTop (S (330));
    grid.setBounds (gridArea);
    left.removeFromTop (S (8));
    laneArea = left.removeFromTop (S (178));
    left.removeFromTop (S (8));
    macroArea = left.removeFromTop (S (118));
    left.removeFromTop (S (8));
    voiceArea = left;

    // lane mixer
    {
        auto a = laneArea.reduced (S (12), S (8));
        a.removeFromTop (S (20));
        const int cw = a.getWidth() / 4;
        for (int l = 0; l < 4; ++l)
        {
            auto col = a.removeFromLeft (cw).reduced (S (3), 0);
            laneCols[l] = col;
            col.removeFromTop (S (16));
            auto top = col.removeFromTop (S (78));
            laneKnobs[l * 5 + 0]->setBounds (top.removeFromLeft (top.getWidth() / 2));
            laneKnobs[l * 5 + 1]->setBounds (top);
            auto bot = col.removeFromTop (S (58));
            const int bw = bot.getWidth() / 3;
            for (int k = 0; k < 3; ++k) laneKnobs[l * 5 + 2 + k]->setBounds (bot.removeFromLeft (bw));
        }
    }
    // macros
    {
        auto a = macroArea.reduced (S (12), S (8));
        a.removeFromTop (S (18));
        const int cw = a.getWidth() / 4;
        for (int m = 0; m < 4; ++m) macroKnobs[m]->setBounds (a.removeFromLeft (cw).reduced (S (8), 0));
    }
    // voice
    {
        auto a = voiceArea.reduced (S (12), S (8));
        a.removeFromTop (S (22));
        auto meters = a.removeFromRight (S (40));
        meterL.setBounds (meters.removeFromLeft (S (16)));
        meters.removeFromLeft (S (6));
        meterR.setBounds (meters.removeFromLeft (S (16)));
        a.removeFromRight (S (8));
        auto row1 = a.removeFromTop (S (42));
        row1.removeFromTop (S (15));
        voiceChoices[0]->setBounds (row1.removeFromLeft (row1.getWidth() / 2).withTrimmedRight (S (8)));
        voiceChoices[1]->setBounds (row1);
        a.removeFromTop (S (6));
        const int cw = a.getWidth() / 6;
        auto ra = a.withHeight (juce::jmin (a.getHeight(), S (84)));
        for (int k = 0; k < 6; ++k) voiceKnobs[k]->setBounds (ra.removeFromLeft (cw));
    }
    editorArea = r.removeFromTop (S (378));
    editor.setBounds (editorArea);
    r.removeFromTop (S (8));
    rack.setBounds (r);
    for (auto* c : rack.getChildren()) c->setBounds (rack.getLocalBounds());
}
}
