#include "PluginEditor.h"

namespace mc
{
using namespace ui;

static bool isFx (Kind k) { return (int) k >= (int) Kind::ModFx; }
static bool isMod (Kind k) { return k == Kind::Env || k == Kind::Lfo || k == Kind::Mseg || k == Kind::Map || k == Kind::Math; }

ManticoreEditor::ManticoreEditor (ManticoreProcessor& p)
    : AudioProcessorEditor (p), proc (p),
      ctx { p, live, [this] (Kind k, int i) { selectModule (k, i); }, [this] (const juce::String& s, int t) { setStatus (s, t); } },
      synth (ctx), rack (ctx), fx (ctx), arp (ctx), xy (ctx), browser (ctx)
{
    setLookAndFeel (&lnf);
    tips.setLookAndFeel (&lnf);
    live.refresh (proc.apvts);

    synth.rackHolder().addAndMakeVisible (rack);
    synth.rackRefresh = [this] { rack.refreshModulation(); };
    addAndMakeVisible (synth);
    addChildComponent (fx);
    addChildComponent (arp);
    addChildComponent (xy);
    addChildComponent (browser);

    int t = 0;
    for (auto* b : { &synthTab, &fxTab, &arpTab, &xyTab, &browseTab })
    {
        b->setClickingTogglesState (true);
        b->setRadioGroupId (991);
        b->getProperties().set ("accent", (juce::int64) col::amber.getARGB());
        const int page = t++;
        b->onClick = [this, page] { setPage (page); };
        addAndMakeVisible (b);
    }
    synthTab.setTooltip ("The synth: generator grid, module editor and modulation rack.");
    fxTab.setTooltip ("The effects grid: Main bus plus three aux send buses.");
    arpTab.setTooltip ("The step arpeggiator.");
    xyTab.setTooltip ("The XY vector mixer: four pads that each morph up to eight targets at once.");
    browseTab.setTooltip ("Browse, audition and favourite presets.");

    for (juce::Component* c : std::initializer_list<juce::Component*> { &prevBtn, &nextBtn, &presetBtn, &favBtn, &saveBtn, &initBtn,
                                                                       &aBtn, &bBtn, &copyBtn, &undoBtn, &redoBtn })
        addAndMakeVisible (c);
    favBtn.setTooltip ("Favourite: star the current preset so it shows in the browser's Favourites list.");
    favBtn.onClick = [this]
    {
        const auto key = proc.currentPresetKey();
        if (key.isEmpty()) { favBtn.setToggleState (false, juce::dontSendNotification); setStatus ("Save the sound first to favourite it.", 80); return; }
        PresetBank::get().setFavourite (key, favBtn.getToggleState());
        browser.refresh();
        setStatus (favBtn.getToggleState() ? "Added to Favourites." : "Removed from Favourites.", 60);
    };
    prevBtn.onClick = [this] { browser.step (-1); };
    nextBtn.onClick = [this] { browser.step (1); };
    presetBtn.onClick = [this] { setPage (page == 4 ? 0 : 4); };
    saveBtn.onClick = [this] { savePresetDialog(); };
    initBtn.onClick = [this] { proc.loadInit(); browser.refresh(); setStatus ("Init patch loaded.", 60); };
    aBtn.onClick = [this] { proc.abSelect (0); refreshAB(); };
    bBtn.onClick = [this] { proc.abSelect (1); refreshAB(); };
    copyBtn.onClick = [this] { proc.abCopyToOther(); setStatus (juce::String ("Copied to ") + (proc.abCurrent() == 0 ? "B." : "A."), 60); };
    undoBtn.onClick = [this] { proc.undo(); };
    redoBtn.onClick = [this] { proc.redo(); };
    // ---- creature toolbar
    for (juce::Component* c : std::initializer_list<juce::Component*> { &mutateBtn, &mutStr, &breedBtn, &breedFavBtn, &hauntBtn, &hauntDepth,
                                                                       &hauntSpeed, &followBtn, &captureBtn })
        addAndMakeVisible (c);
    mutStr.setRange (0.0, 1.0, 0.01);
    mutStr.setValue (proc.mutateStrength(), juce::dontSendNotification);
    mutStr.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
    mutStr.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue() * 0.01; };
    mutStr.onValueChange = [this] { proc.setMutateStrength ((float) mutStr.getValue()); };
    mutStr.setDoubleClickReturnValue (true, 0.35);
    mutStr.setTooltip ("Mutate Strength: small values nudge the sound, large values transform it (and may move modules in the grid "
                       "unless the Layout padlock is closed). Also used by each module's own MUTATE button.");
    mutStr.getProperties().set ("accent", (juce::int64) col::amber.getARGB());
    mutStr.updateText();
    for (auto* sl : { &hauntDepth, &hauntSpeed }) sl->getProperties().set ("accent", (juce::int64) col::ghost.getARGB());
    depthAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id::hauntDepth, hauntDepth);
    speedAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id::hauntSpeed, hauntSpeed);
    hauntDepth.setTooltip (tooltipFor (id::hauntDepth));
    hauntSpeed.setTooltip (tooltipFor (id::hauntSpeed));
    hauntBtn.attach (proc.apvts, id::hauntOn);
    followBtn.attach (proc.apvts, id::hauntFollow);
    mutateBtn.onClick = [this] { juce::String m; const bool ok = proc.mutate (m); runTool (ok, m); };
    breedBtn.onClick = [this] { juce::String m; const bool ok = proc.breed (m); runTool (ok, m); };
    breedFavBtn.onClick = [this] { juce::String m; const bool ok = proc.breedWithFavourite (m); runTool (ok, m); };
    captureBtn.onClick = [this] { juce::String m; const bool ok = proc.capture (m); runTool (ok, m); };

    browser.onLoaded = [this] { setStatus ("Loaded " + proc.currentPresetName() + ".", 50); };
    browser.onSave = [this] { savePresetDialog(); };
    refreshAB();
    lastPreset = proc.currentPresetName();
    presetBtn.setButtonText (lastPreset);
    favBtn.setToggleState (PresetBank::get().isFavourite (proc.currentPresetKey()), juce::dontSendNotification);

    const int sk = (int) proc.uiState.getProperty ("selKind", (int) Kind::Osc);
    const int si = (int) proc.uiState.getProperty ("selInst", 0);
    synth.showModule (Kind::Osc, 0);
    fx.showFx (Kind::Reverb, 0);
    selectModule ((Kind) juce::jlimit (0, kNumKinds - 1, sk), si);
    setPage ((int) proc.uiState.getProperty ("page", 0));

    setResizable (true, true);
    getConstrainer()->setFixedAspectRatio ((double) kBaseW / (double) kBaseH);
    setResizeLimits (kBaseW * 6 / 10, kBaseH * 6 / 10, kBaseW * 2, kBaseH * 2);
    const int w = (int) proc.uiState.getProperty ("w", kBaseW * 85 / 100);
    setSize (juce::jlimit (kBaseW * 6 / 10, kBaseW * 2, w), juce::jlimit (kBaseH * 6 / 10, kBaseH * 2, w * kBaseH / kBaseW));
    startTimerHz (20);
}

ManticoreEditor::~ManticoreEditor()
{
    stopTimer();
    tips.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void ManticoreEditor::setPage (int p)
{
    page = juce::jlimit (0, 4, p);
    proc.uiState.setProperty ("page", page, nullptr);
    synth.setVisible (page == 0);
    fx.setVisible (page == 1);
    arp.setVisible (page == 2);
    xy.setVisible (page == 3);
    browser.setVisible (page == 4);
    juce::TextButton* tabs[] { &synthTab, &fxTab, &arpTab, &xyTab, &browseTab };
    tabs[page]->setToggleState (true, juce::dontSendNotification);
    if (page == 4) browser.refresh();
}

void ManticoreEditor::runTool (bool ok, const juce::String& msg)
{
    setStatus (msg, ok ? 90 : 140);
    refreshAB();
    for (auto* c : synth.getChildren()) c->repaint();
    if (page == 4) browser.refresh();
}

void ManticoreEditor::sweepGhosts (juce::Component& c, bool clear)
{
    for (auto* ch : c.getChildren())
    {
        if (! ch->isVisible()) continue;
        if (auto* kb = dynamic_cast<ui::Knob*> (ch))
        {
            const int i = kb->paramIndexCached();
            kb->setGhost (clear || i < 0 ? 0.0f : proc.engine.hauntOffset (i));
        }
        else sweepGhosts (*ch, clear);
    }
}

void ManticoreEditor::selectModule (Kind k, int inst)
{
    if (isMod (k))
    {
        const int fam = k == Kind::Env ? 0 : k == Kind::Lfo ? 1 : k == Kind::Mseg ? 2 : k == Kind::Map ? 3 : 4;
        rack.select (fam * 4 + inst);
        return;
    }
    proc.uiState.setProperty ("selKind", (int) k, nullptr);
    proc.uiState.setProperty ("selInst", inst, nullptr);
    if (isFx (k)) fx.showFx (k, inst);
    else synth.showModule (k, inst);
}

void ManticoreEditor::refreshAB()
{
    aBtn.setToggleState (proc.abCurrent() == 0, juce::dontSendNotification);
    bBtn.setToggleState (proc.abCurrent() == 1, juce::dontSendNotification);
    copyBtn.setButtonText (proc.abCurrent() == 0 ? "A > B" : "B > A");
    aBtn.setTooltip ("A/B comparison: slot A holds " + proc.abName (0) + ".");
    bBtn.setTooltip ("A/B comparison: slot B holds " + proc.abName (1) + ".");
    const auto other = proc.abName (1 - proc.abCurrent());
    breedBtn.setTooltip ("Breed: a new sound made from " + proc.currentPresetName() + " and " + other + " (the other A/B slot). Each section comes "
                         "from one parent with some values blended towards the other, and the child replaces the current slot. "
                         "Padlocked sections stay as they are.");
}

void ManticoreEditor::savePresetDialog()
{
    auto* w = new juce::AlertWindow ("Save Preset", "Save the current sound as a user preset.", juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel (&lnf);
    w->addTextEditor ("name", proc.currentPresetName(), "Name:");
    juce::StringArray cats = PresetBank::categories();
    cats.insert (0, "User");
    w->addComboBox ("cat", cats, "Category:");
    w->addTextEditor ("tags", "", "Genre tags (comma separated, e.g. Metal, Game):");
    w->addTextEditor ("desc", "", "Description:");
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<ManticoreEditor> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int r)
    {
        if (r != 1 || safe == nullptr) return;
        const auto name = w->getTextEditorContents ("name");
        const auto cat = w->getComboBoxComponent ("cat")->getText();
        if (safe->proc.saveUserPreset (name, cat, w->getTextEditorContents ("tags"), w->getTextEditorContents ("desc")))
        {
            safe->setStatus ("Saved " + safe->proc.currentPresetName() + ".", 100);
            safe->browser.refresh();
        }
        else safe->setStatus ("Could not save the preset.", 100);
    }), true);
}

void ManticoreEditor::timerCallback()
{
    live.refresh (proc.apvts);
    const auto pk = proc.engine.peakL.exchange (0.0f), pr = proc.engine.peakR.exchange (0.0f);
    synth.meterL.setLevel (juce::Decibels::gainToDecibels (pk, -120.0f));
    synth.meterR.setLevel (juce::Decibels::gainToDecibels (pr, -120.0f));
    undoBtn.setEnabled (proc.canUndo());
    redoBtn.setEnabled (proc.canRedo());

    const auto name = proc.currentPresetName();
    if (name != lastPreset)
    {
        presetBtn.setButtonText (name);
        lastPreset = name;
        refreshAB();
        favBtn.setToggleState (PresetBank::get().isFavourite (proc.currentPresetKey()), juce::dontSendNotification);
        for (auto* c : synth.getChildren()) c->repaint();
    }

    // matrix changes -> refresh the amber modulation rings
    int hash = 17;
    for (int i = 0; i < kNumMatrix; ++i)
        hash = hash * 31 + juce::roundToInt (live.v (id::mtx (i, "src")) * 7 + live.v (id::mtx (i, "dst")) * 131 + live.v (id::mtx (i, "amt")) * 1000);

    for (int n = 0; n < kNumXY; ++n)
        for (int a = 0; a < 2; ++a)
            for (int k = 0; k < kXYTargets; ++k)
                hash = hash * 31 + juce::roundToInt (live.v (id::xyTarget (n, a, k, "dst")) * 131 + live.v (id::xyTarget (n, a, k, "amt")) * 1000);
    if (hash != lastMatrixHash) { lastMatrixHash = hash; synth.refreshModulation(); }

    if (page == 0) { synth.tick(); rack.tick(); }
    else if (page == 1) fx.tick();
    else if (page == 2) arp.tick();
    else if (page == 3) xy.tick();

    // Haunt: ghost pointers on the visible knobs
    const bool haunting = proc.engine.hauntLevel.load() > 0.0f;
    if (haunting || ghostsShown) sweepGhosts (*this, ! haunting);
    ghostsShown = haunting;
    captureBtn.setEnabled (haunting || proc.isHaunting());

    if (statusTicks > 0 && --statusTicks == 0) status.clear();
    repaint (statusArea);
}

void ManticoreEditor::paint (juce::Graphics& g)
{
    const float s = lnf.s;
    juce::ColourGradient bg (col::bg2, 0, 0, col::bg, 0, (float) getHeight(), false);
    g.setGradientFill (bg);
    g.fillAll();
    drawEmblem (g, emblemArea.toFloat());
    auto t = titleArea.toFloat();
    g.setFont (Fonts::title (28 * s));
    g.setColour (col::text);
    g.drawText ("MANTICORE", t.removeFromTop (t.getHeight() * 0.62f), juce::Justification::bottomLeft);
    g.setFont (Fonts::label (12.5f * s));
    g.setColour (col::amber);
    g.drawText (juce::CharPointer_UTF8 ("MODULAR CHIMERA SYNTH  \xc2\xb7  FELLOWSHIP OF THE STRING"), t, juce::Justification::topLeft);

    // creature toolbar
    drawPanel (g, toolArea.toFloat(), s, false);
    g.setColour (col::edge);
    for (int x : toolSeps) g.fillRect ((float) x, (float) toolArea.getY() + 8 * s, 1.0f, (float) toolArea.getHeight() - 16 * s);
    g.setFont (Fonts::label (11.5f * s));
    g.setColour (col::dim);
    g.drawText ("STRENGTH", strCap, juce::Justification::centredLeft);
    g.drawText ("DEPTH", depthCap, juce::Justification::centredLeft);
    g.drawText ("SPEED", speedCap, juce::Justification::centredLeft);

    g.setFont (Fonts::value (13.0f * s));
    if (status.isNotEmpty())
    {
        g.setColour (col::amber);
        g.drawFittedText (status, statusArea, juce::Justification::centredRight, 1, 0.85f);
    }
    else
    {
        g.setColour (col::dim);
        const int v = proc.engine.voicesPlaying.load();
        g.drawFittedText (juce::String (v) + (v == 1 ? " voice" : " voices") + "  " + ui::dot() + "  hover any control for help", statusArea,
                          juce::Justification::centredRight, 1, 0.85f);
    }
}

void ManticoreEditor::paintOverChildren (juce::Graphics& g)
{
    // star on the Breed-with-a-Favourite button
    const auto b = breedFavBtn.getBounds().toFloat();
    const float sz = b.getHeight() * 0.5f;
    drawStar (g, juce::Rectangle<float> (b.getRight() - sz - b.getHeight() * 0.3f, b.getCentreY() - sz * 0.5f, sz, sz), true,
              breedFavBtn.isMouseOver() ? col::text : col::amber);
}

void ManticoreEditor::resized()
{
    const float s = (float) getWidth() / (float) kBaseW;
    lnf.s = s;
    proc.uiState.setProperty ("w", getWidth(), nullptr);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };

    auto r = getLocalBounds().reduced (S (10));
    auto top = r.removeFromTop (S (56));
    emblemArea = top.removeFromLeft (S (52)).withSizeKeepingCentre (S (48), S (48));
    top.removeFromLeft (S (12));
    titleArea = top.removeFromLeft (S (300));

    auto tabsRow = top.removeFromLeft (S (440)).withSizeKeepingCentre (S (440), S (30)).translated (0, -S (5));
    for (auto* b : { &synthTab, &fxTab, &arpTab, &xyTab, &browseTab }) b->setBounds (tabsRow.removeFromLeft (S (88)).withTrimmedRight (S (4)));
    top.removeFromLeft (S (16));

    auto btnRow = top.withSizeKeepingCentre (top.getWidth(), S (30)).translated (0, -S (5));
    statusArea = top.withTop (btnRow.getBottom() + S (1)).withTrimmedRight (S (2));
    initBtn.setBounds (btnRow.removeFromRight (S (48)));
    btnRow.removeFromRight (S (4));
    saveBtn.setBounds (btnRow.removeFromRight (S (54)));
    btnRow.removeFromRight (S (4));
    favBtn.setBounds (btnRow.removeFromRight (S (30)));
    btnRow.removeFromRight (S (4));
    nextBtn.setBounds (btnRow.removeFromRight (S (28)));
    prevBtn.setBounds (btnRow.removeFromLeft (S (28)));
    btnRow.removeFromLeft (S (4));
    btnRow.removeFromRight (S (4));
    presetBtn.setBounds (btnRow);

    // creature toolbar
    r.removeFromTop (S (6));
    toolArea = r.removeFromTop (S (40));
    {
        toolSeps.clear();
        auto t = toolArea.reduced (S (12), S (5));
        auto btn = [&] (juce::Component& c, int w) { c.setBounds (t.removeFromLeft (S (w)).withSizeKeepingCentre (S (w), S (28))); };
        auto gap = [&] (int w) { t.removeFromLeft (S (w)); };
        auto sep = [&] { gap (14); toolSeps.push_back (t.getX()); gap (14); };
        auto slider = [&] (juce::Slider& sl, juce::Rectangle<int>& cap, int w)
        {
            auto a = t.removeFromLeft (S (w));
            cap = a.removeFromTop (S (12));
            sl.setTextBoxStyle (juce::Slider::TextBoxRight, false, S (40), S (18));
            sl.setBounds (a.withSizeKeepingCentre (a.getWidth(), S (18)));
        };
        btn (mutateBtn, 86); gap (8);
        slider (mutStr, strCap, 150);
        sep();
        btn (breedBtn, 74); gap (4);
        btn (breedFavBtn, 92);
        sep();
        btn (hauntBtn, 80); gap (10);
        slider (hauntDepth, depthCap, 130); gap (8);
        slider (hauntSpeed, speedCap, 130); gap (8);
        btn (followBtn, 74); gap (4);
        btn (captureBtn, 84);
        // right side: A/B and undo
        auto right = [&] (juce::Component& c, int w) { c.setBounds (t.removeFromRight (S (w)).withSizeKeepingCentre (S (w), S (28))); };
        right (redoBtn, 56); t.removeFromRight (S (4));
        right (undoBtn, 56); t.removeFromRight (S (14));
        right (copyBtn, 54); t.removeFromRight (S (4));
        right (bBtn, 30); t.removeFromRight (S (2));
        right (aBtn, 30);
        toolSeps.push_back (aBtn.getX() - S (14));
    }
    r.removeFromTop (S (8));
    for (juce::Component* c : std::initializer_list<juce::Component*> { &synth, &fx, &arp, &xy, &browser }) c->setBounds (r);
    for (auto* c : getChildren()) c->resized();
    for (auto* c : synth.getChildren()) c->resized();
}
}
