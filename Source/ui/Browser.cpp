#include "Pages.h"

namespace mc::ui
{
Browser::Browser (Ctx& c) : ctx (c)
{
    list.setModel (this);
    list.setColour (juce::ListBox::backgroundColourId, col::bg);
    list.setColour (juce::ListBox::outlineColourId, col::edge);
    list.setOutlineThickness (1);
    list.setRowHeight (26);
    addAndMakeVisible (list);

    search.setTextToShowWhenEmpty ("Search names, categories and descriptions...", col::faint);
    search.addListener (this);
    search.setTooltip ("Type to filter presets.");
    addAndMakeVisible (search);

    juce::StringArray catNames { "All", "Favourites", "User" };
    catNames.addArray (PresetBank::categories());
    for (const auto& n : catNames)
    {
        auto* b = cats.add (new juce::TextButton (n));
        b->setClickingTogglesState (true);
        b->setRadioGroupId (881);
        b->getProperties().set ("accent", (juce::int64) (n == "Favourites" ? col::amber : col::teal).getARGB());
        b->setTooltip (n == "Favourites" ? juce::String ("Presets you have starred. Star a preset with the star icon in the list, this panel or the header.")
                                         : "Show " + n.toLowerCase() + " presets.");
        b->onClick = [this, n] { category = n; refresh(); };
        addAndMakeVisible (b);
    }
    cats[0]->setToggleState (true, juce::dontSendNotification);
    for (const auto& gname : PresetBank::genres())
    {
        auto* b = genres.add (new juce::TextButton (gname));
        b->setClickingTogglesState (true);
        b->getProperties().set ("accent", (juce::int64) col::amber.getARGB());
        b->setTooltip ("Only show presets tagged " + gname + ". Several tags together show presets that match any of them.");
        b->onClick = [this] { refresh(); };
        addAndMakeVisible (b);
    }
    loadBtn.onClick = [this] { load (list.getSelectedRow()); };
    saveBtn.onClick = [this] { if (onSave) onSave(); };
    favBtn.setTooltip ("Add or remove this preset from your favourites.");
    favBtn.onClick = [this]
    {
        if (juce::isPositiveAndBelow (infoRow, (int) shown.size()))
        {
            PresetBank::get().setFavourite (shown[(size_t) infoRow].key(), favBtn.getToggleState());
            if (category == "Favourites") refresh();
            list.repaint();
        }
    };
    addAndMakeVisible (loadBtn);
    addAndMakeVisible (saveBtn);
    addAndMakeVisible (favBtn);
    refresh();
}

void Browser::refresh()
{
    all = PresetBank::get().factory();
    for (auto& u : PresetBank::get().userPresets()) all.push_back (u);
    juce::StringArray tagFilter;
    for (auto* b : genres) if (b->getToggleState()) tagFilter.add (b->getButtonText());
    const auto q = search.getText().trim();
    shown.clear();
    for (const auto& p : all)
    {
        if (category == "Favourites" && ! PresetBank::get().isFavourite (p.key())) continue;
        if (category == "User" && ! p.isUser) continue;
        if (category != "All" && category != "Favourites" && category != "User" && p.category != category) continue;
        if (! tagFilter.isEmpty())
        {
            bool any = false;
            for (auto& t : tagFilter) if (p.tags.contains (t)) { any = true; break; }
            if (! any) continue;
        }
        if (q.isNotEmpty() && ! (p.name.containsIgnoreCase (q) || p.category.containsIgnoreCase (q) || p.description.containsIgnoreCase (q)
                                 || p.tags.joinIntoString (" ").containsIgnoreCase (q)))
            continue;
        shown.push_back (p);
    }
    list.updateContent();
    // keep the current preset selected if it is visible
    const auto key = ctx.proc.currentPresetKey();
    suppressLoad = true;
    list.deselectAllRows();
    infoRow = -1;
    for (int i = 0; i < (int) shown.size(); ++i)
        if (shown[(size_t) i].key() == key) { list.selectRow (i); list.scrollToEnsureRowIsOnscreen (i); infoRow = i; break; }
    suppressLoad = false;
    favBtn.setToggleState (infoRow >= 0 && PresetBank::get().isFavourite (shown[(size_t) infoRow].key()), juce::dontSendNotification);
    repaint();
}

void Browser::step (int dir)
{
    if (shown.empty()) refresh();
    if (shown.empty()) return;
    int cur = -1;
    const auto key = ctx.proc.currentPresetKey();
    for (int i = 0; i < (int) shown.size(); ++i) if (shown[(size_t) i].key() == key) { cur = i; break; }
    const int n = (int) shown.size();
    const int next = cur < 0 ? (dir > 0 ? 0 : n - 1) : (cur + dir + n) % n;
    load (next);
}

void Browser::load (int row)
{
    if (! juce::isPositiveAndBelow (row, (int) shown.size())) return;
    const auto& p = shown[(size_t) row];
    if (p.isUser) ctx.proc.loadUserPreset (p.file);
    else ctx.proc.loadFactoryPreset (p.factoryIndex);
    infoRow = row;
    suppressLoad = true;
    list.selectRow (row);
    list.scrollToEnsureRowIsOnscreen (row);
    suppressLoad = false;
    favBtn.setToggleState (PresetBank::get().isFavourite (p.key()), juce::dontSendNotification);
    if (onLoaded) onLoaded();
    repaint();
}

void Browser::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (! juce::isPositiveAndBelow (row, (int) shown.size())) return;
    const float s = scaleOf (*this);
    const auto& p = shown[(size_t) row];
    if (selected) { g.setColour (col::teal.withAlpha (0.22f)); g.fillRect (0, 0, w, h); }
    else if (row % 2) { g.setColour (col::panel.withAlpha (0.6f)); g.fillRect (0, 0, w, h); }
    const bool fav = PresetBank::get().isFavourite (p.key());
    drawStar (g, juce::Rectangle<float> (4.0f * s, (float) h * 0.18f, (float) h * 0.64f, (float) h * 0.64f), fav, fav ? col::amber : col::faint);
    g.setFont (Fonts::label (15.5f * s));
    g.setColour (selected ? juce::Colours::white : col::text);
    const int x0 = (int) ((float) h * 0.64f + 12.0f * s);
    g.drawFittedText (p.name, x0, 0, (int) (w * 0.42f), h, juce::Justification::centredLeft, 1);
    g.setFont (Fonts::value (13.5f * s));
    g.setColour (col::dim);
    g.drawText (p.category, (int) (w * 0.48f), 0, (int) (w * 0.24f), h, juce::Justification::centredLeft);
    g.setColour (col::amber.withAlpha (0.85f));
    g.drawFittedText (p.tags.joinIntoString ("  " + dot() + "  "), (int) (w * 0.72f), 0, (int) (w * 0.27f), h, juce::Justification::centredLeft, 1);
}

void Browser::listBoxItemClicked (int row, const juce::MouseEvent& e)
{
    if (! juce::isPositiveAndBelow (row, (int) shown.size())) return;
    const float s = scaleOf (*this);
    if (e.x < (int) ((float) list.getRowHeight() * 0.64f + 10.0f * s))
    {
        const auto key = shown[(size_t) row].key();
        PresetBank::get().setFavourite (key, ! PresetBank::get().isFavourite (key));
        if (row == infoRow) favBtn.setToggleState (PresetBank::get().isFavourite (key), juce::dontSendNotification);
        if (category == "Favourites") refresh();
        list.repaint();
        return;
    }
}

void Browser::selectedRowsChanged (int row)
{
    if (suppressLoad || row < 0) return;
    load (row);    // single click auditions the preset
}

void Browser::paint (juce::Graphics& g)
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    drawPanel (g, leftArea.toFloat(), s);
    drawPanel (g, infoArea.toFloat(), s);
    auto l = leftArea.reduced (S (12), S (8));
    drawPanelTitle (g, l.removeFromTop (S (18)), "Categories", s);
    const auto genreTop = genres[0]->getBounds().getY() - S (22);
    drawPanelTitle (g, { l.getX(), genreTop, l.getWidth(), S (18) }, "Genre Tags", s);
    // list header
    const auto lb = list.getBounds();
    g.setFont (Fonts::bold (13.0f * s));
    g.setColour (col::dim);
    const int hy = lb.getY() - S (20);
    g.drawText ("NAME", lb.getX() + S (34), hy, S (200), S (18), juce::Justification::centredLeft);
    g.drawText ("CATEGORY", lb.getX() + (int) (lb.getWidth() * 0.48f), hy, S (120), S (18), juce::Justification::centredLeft);
    g.drawText ("GENRES", lb.getX() + (int) (lb.getWidth() * 0.72f), hy, S (120), S (18), juce::Justification::centredLeft);
    g.setColour (col::amber);
    g.drawText (juce::String ((int) shown.size()) + " PRESETS", lb.getX(), hy, lb.getWidth() - S (6), S (18), juce::Justification::centredRight);

    auto info = infoArea.reduced (S (16), S (12));
    drawPanelTitle (g, info.removeFromTop (S (18)), "Preset", s);
    info.removeFromTop (S (8));
    if (juce::isPositiveAndBelow (infoRow, (int) shown.size()))
    {
        const auto& p = shown[(size_t) infoRow];
        g.setFont (Fonts::title (22.0f * s));
        g.setColour (col::text);
        g.drawFittedText (p.name, info.removeFromTop (S (60)), juce::Justification::topLeft, 2);
        g.setFont (Fonts::label (15.0f * s));
        g.setColour (col::teal);
        g.drawText (p.category.toUpperCase(), info.removeFromTop (S (20)), juce::Justification::centredLeft);
        g.setColour (col::amber);
        g.drawFittedText (p.tags.joinIntoString ("  " + dot() + "  "), info.removeFromTop (S (20)), juce::Justification::centredLeft, 1);
        info.removeFromTop (S (10));
        g.setFont (Fonts::value (15.0f * s));
        g.setColour (col::text.withAlpha (0.9f));
        g.drawFittedText (p.description, info.removeFromTop (S (200)), juce::Justification::topLeft, 10);
        g.setFont (Fonts::value (13.0f * s));
        g.setColour (col::dim);
        g.drawText (p.isUser ? juce::String ("User preset") : "By " + p.author, info.removeFromTop (S (18)), juce::Justification::centredLeft);
    }
    else
    {
        g.setFont (Fonts::value (15.0f * s));
        g.setColour (col::dim);
        g.drawFittedText ("Click a preset to hear it. Use the arrow keys to step through the list. Click the star beside any name to add it to your Favourites.",
                          info.removeFromTop (S (120)), juce::Justification::topLeft, 6);
    }
}

void Browser::resized()
{
    const float s = scaleOf (*this);
    auto S = [s] (float v) { return juce::roundToInt (v * s); };
    list.setRowHeight (S (26));
    auto r = getLocalBounds();
    leftArea = r.removeFromLeft (S (240));
    r.removeFromLeft (S (8));
    infoArea = r.removeFromRight (S (330));
    r.removeFromRight (S (8));
    {
        auto l = leftArea.reduced (S (12), S (8));
        l.removeFromTop (S (24));
        for (auto* b : cats) { b->setBounds (l.removeFromTop (S (27)).reduced (0, S (2))); }
        l.removeFromTop (S (28));
        const int w = l.getWidth() / 2;
        for (int i = 0; i < genres.size(); ++i)
            genres[i]->setBounds (l.getX() + (i % 2) * w, l.getY() + (i / 2) * S (30), w - S (4), S (26));
    }
    search.setBounds (r.removeFromTop (S (30)));
    search.setFont (Fonts::value (15.0f * s));
    r.removeFromTop (S (26));
    list.setBounds (r);
    auto info = infoArea.reduced (S (16), S (12));
    auto bottom = info.removeFromBottom (S (34));
    favBtn.setBounds (bottom.removeFromLeft (S (34)));
    bottom.removeFromLeft (S (10));
    loadBtn.setBounds (bottom.removeFromLeft (S (100)));
    bottom.removeFromLeft (S (8));
    saveBtn.setBounds (bottom.removeFromLeft (S (120)));
}
}
