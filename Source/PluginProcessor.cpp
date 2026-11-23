#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <set>

namespace mc
{
juce::AudioProcessorValueTreeState::ParameterLayout ManticoreProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& d : params())
    {
        const juce::ParameterID pid { d.id, 1 };
        if (d.type == ParamDef::Choice)
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, d.choices, (int) d.def));
        else if (d.type == ParamDef::Bool)
            // on/off switches as two-state choices: their value is always snapped, which hosts and validators prefer
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, d.name, juce::StringArray { "Off", "On" }, d.def > 0.5f ? 1 : 0));
        else
        {
            auto def = d;
            layout.add (std::make_unique<juce::AudioParameterFloat> (
                pid, d.name, d.range(), d.def,
                juce::AudioParameterFloatAttributes()
                    .withStringFromValueFunction ([def] (float v, int) { return def.format (v); })
                    .withValueFromStringFunction ([def] (const juce::String& s)
                    {
                        auto t = s.trim();
                        float v = t.getFloatValue();
                        if (def.unit == "%" || def.unit == "%b") v *= 0.01f;
                        else if (def.unit == "Hz" && t.containsIgnoreCase ("k")) v *= 1000.0f;
                        else if (def.unit == "s" && t.endsWithIgnoreCase ("ms")) v *= 0.001f;
                        else if (def.unit == "pan") { if (t.endsWithIgnoreCase ("L")) v = -v * 0.01f; else if (t.endsWithIgnoreCase ("R")) v *= 0.01f; }
                        return v;
                    })));
        }
    }
    return layout;
}

ManticoreProcessor::ManticoreProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MANTICORE", createLayout())
{
    engine.bind (apvts);
    resetShapes (mseg, map);
    pushShapes();
    PresetBank::get();
    paramClasses();
    abStore[0] = makeStateTree();
    abStore[1] = abStore[0].createCopy();
    for (auto* p : getParameters()) p->addListener (this);
    pushHistory();
    startTimerHz (5);
}

ManticoreProcessor::~ManticoreProcessor()
{
    stopTimer();
    for (auto* p : getParameters()) p->removeListener (this);
}

bool ManticoreProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono();
}

void ManticoreProcessor::prepareToPlay (double sr, int block)
{
    engine.prepare (sr, block);
}

void ManticoreProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    engine.process (buffer, midi, getPlayHead());
}

juce::AudioProcessorEditor* ManticoreProcessor::createEditor() { return new ManticoreEditor (*this); }

const juce::String ManticoreProcessor::getProgramName (int i)
{
    const auto& f = PresetBank::get().factory();
    return juce::isPositiveAndBelow (i, (int) f.size()) ? f[(size_t) i].name : juce::String ("Init");
}

// ------------------------------------------------------------------ shapes
void ManticoreProcessor::resetShapes (dsp::MsegData* m, dsp::MapData* mp)
{
    for (int i = 0; i < 4; ++i)
    {
        m[i] = dsp::MsegData();
        for (int k = 0; k < kMapSteps; ++k) mp[i].v[k] = juce::jmin (1.0f, (float) k / 15.0f * 2.0f - 1.0f);
    }
}

void ManticoreProcessor::pushShapes()
{
    for (int i = 0; i < 4; ++i) { engine.setMseg (i, mseg[i]); engine.setMap (i, map[i]); }
    ++shapesVersion;
}

void ManticoreProcessor::shapesChanged()
{
    pushShapes();
    shapeEdited = true;
    lastShapeEdit = juce::Time::getMillisecondCounterHiRes();
}

static juce::ValueTree shapesTreeOf (const dsp::MsegData* mseg, const dsp::MapData* map)
{
    juce::ValueTree t ("Shapes");
    for (int i = 0; i < 4; ++i)
    {
        juce::ValueTree m ("Mseg");
        m.setProperty ("index", i, nullptr);
        juce::StringArray pts;
        for (int k = 0; k < mseg[i].n; ++k) pts.add (juce::String (mseg[i].x[k], 4) + "," + juce::String (mseg[i].y[k], 4) + "," + juce::String (mseg[i].c[k], 3));
        m.setProperty ("points", pts.joinIntoString (";"), nullptr);
        m.setProperty ("loopStart", mseg[i].loopStart, nullptr);
        m.setProperty ("loopEnd", mseg[i].loopEnd, nullptr);
        t.appendChild (m, nullptr);
        juce::ValueTree mp ("Map");
        mp.setProperty ("index", i, nullptr);
        juce::StringArray vals;
        for (int k = 0; k < kMapSteps; ++k) vals.add (juce::String (map[i].v[k], 4));
        mp.setProperty ("values", vals.joinIntoString (","), nullptr);
        t.appendChild (mp, nullptr);
    }
    return t;
}

juce::ValueTree ManticoreProcessor::shapesToTree() const { return shapesTreeOf (mseg, map); }

void ManticoreProcessor::shapesFromTree (const juce::ValueTree& t, dsp::MsegData* ms, dsp::MapData* mp)
{
    resetShapes (ms, mp);
    if (! t.isValid()) return;
    for (auto c : t)
    {
        const int i = juce::jlimit (0, 3, (int) c.getProperty ("index", 0));
        if (c.hasType ("Mseg"))
        {
            juce::StringArray pts;
            pts.addTokens (c.getProperty ("points").toString(), ";", "");
            pts.removeEmptyStrings();
            if (pts.size() >= 2)
            {
                auto& d = ms[i];
                d.n = juce::jmin (kMsegMaxPoints, pts.size());
                for (int k = 0; k < d.n; ++k)
                {
                    juce::StringArray v;
                    v.addTokens (pts[k], ",", "");
                    d.x[k] = v[0].getFloatValue(); d.y[k] = v[1].getFloatValue(); d.c[k] = v[2].getFloatValue();
                }
                d.loopStart = juce::jlimit (0, d.n - 1, (int) c.getProperty ("loopStart", 1));
                d.loopEnd = juce::jlimit (0, d.n - 1, (int) c.getProperty ("loopEnd", 2));
            }
        }
        else if (c.hasType ("Map"))
        {
            juce::StringArray v;
            v.addTokens (c.getProperty ("values").toString(), ",", "");
            for (int k = 0; k < kMapSteps && k < v.size(); ++k) mp[i].v[k] = juce::jlimit (-1.0f, 1.0f, v[k].getFloatValue());
        }
    }
}

// ------------------------------------------------------------------ state
juce::ValueTree ManticoreProcessor::makeStateTree()
{
    auto state = apvts.copyState();
    state.removeChild (state.getChildWithName ("Shapes"), nullptr);
    state.appendChild (shapesToTree(), nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("presetKey", presetKey, nullptr);
    return state;
}

void ManticoreProcessor::restoreStateTree (const juce::ValueTree& t, bool keepPerform)
{
    if (! t.isValid()) return;
    auto copy = t.createCopy();
    shapesFromTree (copy.getChildWithName ("Shapes"), mseg, map);
    pushShapes();
    presetName = copy.getProperty ("presetName", "Init").toString();
    presetKey = copy.getProperty ("presetKey", "").toString();
    if (presetKey.startsWith ("F:"))      // a factory preset renamed in an update
    {
        const auto now = PresetBank::get().currentName (presetKey.substring (2));
        if (now != presetKey.substring (2))
        {
            if (presetName == presetKey.substring (2)) presetName = now;
            presetKey = "F:" + now;
        }
    }
    copy.removeChild (copy.getChildWithName ("Shapes"), nullptr);
    copy.removeChild (copy.getChildWithName ("UI"), nullptr);
    // performance settings (Haunt) keep their current values; parameters missing from older states go to their defaults
    // (APVTS would otherwise reset anything missing to its default)
    std::set<juce::String> have;
    for (int k = copy.getNumChildren(); --k >= 0;)
    {
        auto c = copy.getChild (k);
        if (! c.hasType ("PARAM")) continue;
        const auto pidStr = c.getProperty ("id").toString();
        const int idx = paramIndex (pidStr);
        if (keepPerform && idx >= 0 && isPerformParam (idx)) { copy.removeChild (k, nullptr); continue; }
        have.insert (pidStr);
    }
    const auto& defs = params();
    for (int i = 0; i < (int) defs.size(); ++i)
    {
        const bool perform = isPerformParam (i);
        if (have.count (defs[(size_t) i].id) > 0) continue;
        float value = defs[(size_t) i].def;
        if (perform)
            if (auto* r = apvts.getRawParameterValue (defs[(size_t) i].id)) value = r->load();
        juce::ValueTree c ("PARAM");
        c.setProperty ("id", defs[(size_t) i].id, nullptr);
        c.setProperty ("value", value, nullptr);
        copy.appendChild (c, nullptr);
    }
    apvts.replaceState (copy);
}

void ManticoreProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = makeStateTree();
    state.appendChild (uiState.createCopy(), nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, dest);
}

void ManticoreProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto t = juce::ValueTree::fromXml (*xml);
        if (! t.hasType (apvts.state.getType())) return;
        auto ui = t.getChildWithName ("UI");
        if (ui.isValid()) uiState.copyPropertiesFrom (ui, nullptr);
        lockBits = (uint32_t) (int) uiState.getProperty ("locks", 0);
        engine.locks = lockBits;
        restoreStateTree (t, false);
        abStore[abSlot] = makeStateTree();
        history.clear();
        histIndex = -1;
        pushHistory();
    }
}

// ------------------------------------------------------------------ patches
Patch ManticoreProcessor::currentPatch() const
{
    Patch p;
    const auto& defs = params();
    p.v.resize (defs.size());
    for (size_t i = 0; i < defs.size(); ++i)
        if (auto* r = apvts.getRawParameterValue (defs[i].id)) p.v[i] = r->load();
    for (int k = 0; k < 4; ++k) { p.mseg[k] = mseg[k]; p.map[k] = map[k]; }
    return p;
}

void ManticoreProcessor::applyPatch (const Patch& p)
{
    const auto& defs = params();
    for (int i = 0; i < (int) defs.size(); ++i)
    {
        if (isPerformParam (i)) continue;
        auto* prm = apvts.getParameter (defs[(size_t) i].id);
        auto* raw = apvts.getRawParameterValue (defs[(size_t) i].id);
        if (prm == nullptr || raw == nullptr) continue;
        if (raw->load() != p.v[(size_t) i]) prm->setValueNotifyingHost (prm->convertTo0to1 (p.v[(size_t) i]));
    }
    for (int k = 0; k < 4; ++k) { mseg[k] = p.mseg[k]; map[k] = p.map[k]; }
    pushShapes();
}

Patch ManticoreProcessor::patchFromTree (const juce::ValueTree& t)
{
    auto p = defaultPatch();
    for (const auto& c : t)
    {
        if (! c.hasType ("PARAM")) continue;
        const int idx = paramIndex (c.getProperty ("id").toString());
        if (idx >= 0) p.v[(size_t) idx] = (float) c.getProperty ("value", p.v[(size_t) idx]);
    }
    shapesFromTree (t.getChildWithName ("Shapes"), p.mseg, p.map);
    return p;
}

Patch ManticoreProcessor::patchFromPresetVar (const juce::var& v)
{
    auto p = defaultPatch();
    if (auto* obj = v["p"].getDynamicObject())
        for (const auto& kv : obj->getProperties())
        {
            const int idx = paramIndex (kv.name.toString());
            if (idx < 0) continue;
            const auto& d = params()[(size_t) idx];
            float value;
            if (kv.value.isString())
            {
                const int ci = d.choices.indexOf (kv.value.toString(), true);
                if (ci < 0) continue;
                value = (float) ci;
            }
            else if (kv.value.isBool()) value = (bool) kv.value ? 1.0f : 0.0f;
            else value = (float) (double) kv.value;
            p.v[(size_t) idx] = d.range().snapToLegalValue (value);
        }
    if (auto* ms = v["mseg"].getArray())
        for (const auto& m : *ms)
        {
            const int i = juce::jlimit (0, 3, (int) m["i"]);
            auto& d = p.mseg[i];
            if (auto* pts = m["pts"].getArray())
            {
                d.n = juce::jlimit (2, kMsegMaxPoints, pts->size());
                for (int k = 0; k < d.n; ++k)
                {
                    const auto& pt = (*pts)[k];
                    d.x[k] = (float) (double) pt[0]; d.y[k] = (float) (double) pt[1]; d.c[k] = pt.size() > 2 ? (float) (double) pt[2] : 0.0f;
                }
            }
            d.loopStart = juce::jlimit (0, d.n - 1, (int) m.getProperty ("ls", 1));
            d.loopEnd = juce::jlimit (0, d.n - 1, (int) m.getProperty ("le", d.n - 1));
        }
    if (auto* mp = v["map"].getArray())
        for (const auto& m : *mp)
        {
            const int i = juce::jlimit (0, 3, (int) m["i"]);
            if (auto* vals = m["v"].getArray())
                for (int k = 0; k < kMapSteps && k < vals->size(); ++k) p.map[i].v[k] = juce::jlimit (-1.0f, 1.0f, (float) (double) (*vals)[k]);
        }
    return p;
}

juce::ValueTree ManticoreProcessor::treeFromPatch (const Patch& p, const juce::String& name, const juce::String& key)
{
    auto t = makeStateTree();
    for (auto c : t)
    {
        if (! c.hasType ("PARAM")) continue;
        const int idx = paramIndex (c.getProperty ("id").toString());
        if (idx >= 0) c.setProperty ("value", p.v[(size_t) idx], nullptr);
    }
    t.removeChild (t.getChildWithName ("Shapes"), nullptr);
    t.appendChild (shapesTreeOf (p.mseg, p.map), nullptr);
    t.setProperty ("presetName", name, nullptr);
    t.setProperty ("presetKey", key, nullptr);
    return t;
}

// ------------------------------------------------------------------ presets
void ManticoreProcessor::loadInit()
{
    applyPatch (defaultPatch());
    presetName = "Init";
    presetKey = "";
    engine.reset();
    pushHistory();
}

void ManticoreProcessor::applyPresetVar (const juce::var& v)
{
    applyPatch (patchFromPresetVar (v));
}

void ManticoreProcessor::loadFactoryPreset (int index)
{
    auto& bank = PresetBank::get();
    if (! juce::isPositiveAndBelow (index, bank.numFactory())) { loadInit(); return; }
    applyPresetVar (bank.factoryData (index));
    currentProgram = index;
    presetName = bank.factory()[(size_t) index].name;
    presetKey = bank.factory()[(size_t) index].key();
    engine.reset();
    pushHistory();
}

bool ManticoreProcessor::loadUserPreset (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr) return false;
    auto t = juce::ValueTree::fromXml (*xml);
    if (! t.hasType (apvts.state.getType())) return false;
    restoreStateTree (t, true);
    presetName = f.getFileNameWithoutExtension();
    presetKey = "U:" + presetName;
    engine.reset();
    pushHistory();
    return true;
}

bool ManticoreProcessor::saveUserPreset (const juce::String& name, const juce::String& category, const juce::String& tags, const juce::String& desc)
{
    const auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto dir = PresetBank::userFolder();
    dir.createDirectory();
    presetName = clean;
    presetKey = "U:" + clean;
    auto t = makeStateTree();
    // performance settings are not part of a preset
    for (int k = t.getNumChildren(); --k >= 0;)
    {
        const int idx = paramIndex (t.getChild (k).getProperty ("id").toString());
        if (idx >= 0 && isPerformParam (idx)) t.removeChild (k, nullptr);
    }
    if (auto xml = t.createXml())
    {
        xml->setAttribute ("category", category);
        xml->setAttribute ("tags", tags);
        xml->setAttribute ("description", desc);
        return xml->writeTo (dir.getChildFile (clean + ".mtpreset"));
    }
    return false;
}

// ------------------------------------------------------------------ A/B
void ManticoreProcessor::abSelect (int slot)
{
    if (slot == abSlot) return;
    abStore[abSlot] = makeStateTree();
    abSlot = slot;
    restoreStateTree (abStore[abSlot], true);
    pushHistory();
}

void ManticoreProcessor::abCopyToOther()
{
    abStore[1 - abSlot] = makeStateTree();
}

juce::String ManticoreProcessor::abName (int slot) const
{
    if (slot == abSlot) return presetName;
    return abStore[slot].getProperty ("presetName", "Init").toString();
}

// ------------------------------------------------------------------ undo / redo
void ManticoreProcessor::pushHistory()
{
    auto t = makeStateTree();
    for (int k = t.getNumChildren(); --k >= 0;)
    {
        const int idx = paramIndex (t.getChild (k).getProperty ("id").toString());
        if (idx >= 0 && isPerformParam (idx)) t.removeChild (k, nullptr);
    }
    if (histIndex >= 0 && histIndex < (int) history.size() && history[(size_t) histIndex].isEquivalentTo (t)) return;
    history.erase (history.begin() + (histIndex + 1), history.end());
    history.push_back (t);
    while (history.size() > 49) history.erase (history.begin());
    histIndex = (int) history.size() - 1;
}

void ManticoreProcessor::undo()
{
    if (! canUndo()) return;
    --histIndex;
    restoreStateTree (history[(size_t) histIndex], true);
}

void ManticoreProcessor::redo()
{
    if (! canRedo()) return;
    ++histIndex;
    restoreStateTree (history[(size_t) histIndex], true);
}

void ManticoreProcessor::parameterGestureChanged (int, bool starting)
{
    if (starting) ++gestures;
    else
    {
        gestures = juce::jmax (0, gestures.load() - 1);
        gestureEnded = true;
    }
}

void ManticoreProcessor::timerCallback()
{
    if (gestureEnded.load() && gestures.load() <= 0)
    {
        gestureEnded = false;
        pushHistory();
    }
    if (shapeEdited && juce::Time::getMillisecondCounterHiRes() - lastShapeEdit > 350.0)
    {
        shapeEdited = false;
        pushHistory();
    }
}

// ------------------------------------------------------------------ padlocks
void ManticoreProcessor::setLocked (int bit, bool on)
{
    if (on) lockBits |= (1u << bit); else lockBits &= ~(1u << bit);
    uiState.setProperty ("locks", (int) lockBits, nullptr);
    engine.locks = lockBits;
}

// ------------------------------------------------------------------ measuring and level matching
ManticoreProcessor::Loudness ManticoreProcessor::measure (const Patch& p)
{
    const double sr = 44100.0;
    const int block = 256;
    // a fresh engine every time, so every random generator and oscillator phase starts the same way
    if (meterVals == nullptr) meterVals = std::make_unique<std::vector<std::atomic<float>>> (p.v.size());
    meter = std::make_unique<dsp::Engine>();
    meter->bindValues (*meterVals);
    meter->prepare (sr, block);
    for (size_t i = 0; i < p.v.size(); ++i) (*meterVals)[i].store (p.v[i]);
    (*meterVals)[(size_t) paramIndex (id::hauntOn)].store (0.0f);
    for (int k = 0; k < 4; ++k) { meter->setMseg (k, p.mseg[k]); meter->setMap (k, p.map[k]); }

    const int total = (int) (sr * 2.4), offAt = (int) (sr * 1.6), win = (int) (sr * 0.4);
    juce::AudioBuffer<float> buf (2, block);
    std::vector<double> energy ((size_t) total, 0.0);
    Loudness L;
    const int chord[] { 48, 55, 60, 63 };
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin (block, total - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        juce::MidiBuffer midi;
        if (pos == 0) for (int c : chord) midi.addEvent (juce::MidiMessage::noteOn (1, c, (juce::uint8) 100), 0);
        if (offAt >= pos && offAt < pos + n) for (int c : chord) midi.addEvent (juce::MidiMessage::noteOff (1, c), offAt - pos);
        meter->process (buf, midi, nullptr);
        for (int i = 0; i < n; ++i)
        {
            const float l = buf.getSample (0, i), r = buf.getSample (1, i);
            if (! std::isfinite (l) || ! std::isfinite (r)) { L.finite = false; continue; }
            L.peak = juce::jmax (L.peak, std::abs (l), std::abs (r));
            energy[(size_t) (pos + i)] = 0.5 * ((double) l * l + (double) r * r);
        }
    }
    double w = 0;
    for (int i = 0; i < total; ++i)
    {
        w += energy[(size_t) i];
        if (i >= win) w -= energy[(size_t) (i - win)];
        if (i >= win - 1) L.loud = juce::jmax (L.loud, (float) (10.0 * std::log10 (juce::jmax (1e-12, w / win))));
    }
    if (! L.finite || L.peak > 30.0f) { L.finite = false; L.loud = -120.0f; }
    return L;
}

void ManticoreProcessor::levelMatch (Patch& p, float target, const Loudness& now)
{
    if (! now.audible()) return;
    float g = target - now.loud;
    g = juce::jmin (g, 20.0f * std::log10 (0.89f / juce::jmax (1.0e-6f, now.peak)));
    g = juce::jlimit (-24.0f, 18.0f, g);
    if (std::abs (g) < 0.2f) return;
    GridMap gm;
    gm.build (p.v.data());
    std::vector<int> lanes;
    for (int l = 0; l < kNumLanes; ++l)
    {
        const int vi = paramIndex (id::lane (l, "vol"));
        if (gm.laneActive[l] && ! isLocked (kLockLane1 + l) && p.v[(size_t) vi] > -59.0f) lanes.push_back (vi);
    }
    const int mi = paramIndex (id::master);
    float rest = g;
    if (g < 0 && ! lanes.empty())
    {
        float top = -60.0f;
        for (int vi : lanes) top = juce::jmax (top, p.v[(size_t) vi]);
        const float step = juce::jmax (g, -40.0f - top);
        for (int vi : lanes) p.v[(size_t) vi] = juce::jlimit (-60.0f, 6.0f, p.v[(size_t) vi] + step);
        rest = g - step;
    }
    float master = p.v[(size_t) mi] + rest;
    if (master > 6.0f && ! lanes.empty())
    {
        float extra = master - 6.0f, room = 12.0f;
        for (int vi : lanes) room = juce::jmin (room, 6.0f - p.v[(size_t) vi]);
        const float up = juce::jlimit (0.0f, extra, room);
        for (int vi : lanes) p.v[(size_t) vi] += up;
    }
    p.v[(size_t) mi] = juce::jlimit (-60.0f, 6.0f, master);
}

bool ManticoreProcessor::finishTool (Patch& cand, const Loudness& lc, float target, const juce::String& newName)
{
    if (lc.audible())
    {
        levelMatch (cand, target, lc);
        const auto l2 = measure (cand);
        if (l2.audible() && std::abs (l2.loud - target) > 1.0f) levelMatch (cand, target, l2);
    }
    applyPatch (cand);
    presetName = newName;
    presetKey = "";
    cachedHash = currentPatch().hash();
    cachedLoud = Loudness { target, juce::jmin (0.89f, lc.peak), true };
    pushHistory();
    return true;
}

// ------------------------------------------------------------------ Mutate / Breed / Capture
bool ManticoreProcessor::mutate (juce::String& msg)
{
    const auto base = currentPatch();
    const auto lb = (base.hash() == cachedHash) ? cachedLoud : measure (base);
    float st = mutateStrength();
    Patch cand;
    Loudness lc;
    bool ok = false, changed = false;
    for (int attempt = 0; attempt < 5 && ! ok; ++attempt)
    {
        cand = base;
        MutateOptions o;
        o.strength = juce::jmax (0.02f, st);
        o.locks = lockBits;
        mutatePatch (cand, o, rng);
        if (cand.sameAs (base)) { st = juce::jmin (1.0f, st * 1.4f + 0.05f); continue; }
        changed = true;
        lc = measure (cand);
        if (lb.audible() ? lc.acceptableAgainst (lb.loud) : lc.finite) ok = true;
        else st *= 0.8f;
    }
    if (! ok)
    {
        msg = changed ? "Mutate couldn't find a playable variation. Try a lower strength." : "Nothing to mutate: unlock a section first.";
        return false;
    }
    finishTool (cand, lc, lb.audible() ? lb.loud : -14.5f, mutatedName (presetName));
    msg = "Mutated at " + juce::String (juce::roundToInt (mutateStrength() * 100.0f)) + "%.";
    return true;
}

bool ManticoreProcessor::mutateModule (Kind k, int inst, juce::String& msg)
{
    const auto base = currentPatch();
    const auto lb = (base.hash() == cachedHash) ? cachedLoud : measure (base);
    float st = mutateStrength();
    Patch cand;
    Loudness lc;
    bool ok = false;
    for (int attempt = 0; attempt < 5 && ! ok; ++attempt)
    {
        cand = base;
        MutateOptions o;
        o.strength = juce::jmax (0.02f, st);
        o.moduleOnly = true;
        o.kind = k;
        o.inst = inst;
        mutatePatch (cand, o, rng);
        if (cand.sameAs (base)) { st = juce::jmin (1.0f, st * 1.4f + 0.05f); continue; }
        lc = measure (cand);
        if (lb.audible() ? lc.acceptableAgainst (lb.loud) : lc.finite) ok = true;
        else st *= 0.8f;
    }
    if (! ok) { msg = "Couldn't find a playable variation of " + moduleName (k, inst) + ". Try a lower strength."; return false; }
    finishTool (cand, lc, lb.audible() ? lb.loud : -14.5f, mutatedName (presetName));
    msg = "Mutated " + moduleName (k, inst) + ".";
    return true;
}

bool ManticoreProcessor::breed (juce::String& msg)
{
    const auto cur = currentPatch();
    const auto oth = patchFromTree (abStore[1 - abSlot]);
    const auto nameA = presetName, nameB = abName (1 - abSlot);
    if (cur.sameAs (oth))
    {
        msg = juce::String ("Slots A and B hold the same sound. Put a different sound in slot ") + (abSlot == 0 ? "B" : "A") + " first.";
        return false;
    }
    const auto lc = (cur.hash() == cachedHash) ? cachedLoud : measure (cur);
    const auto lo = measure (oth);
    float target = -14.5f;
    if (lc.audible() && lo.audible()) target = 0.5f * (lc.loud + lo.loud);
    else if (lc.audible()) target = lc.loud;
    else if (lo.audible()) target = lo.loud;
    Patch child;
    Loudness lch;
    bool ok = false;
    const bool anyAudible = lc.audible() || lo.audible();
    for (int attempt = 0; attempt < 8 && ! ok; ++attempt)
    {
        child = breedPatches (cur, oth, lockBits, rng);
        lch = measure (child);
        ok = anyAudible ? lch.acceptableAgainst (target) : lch.finite;
    }
    if (! ok) { msg = "These two sounds didn't produce a playable child. Try again or unlock more sections."; return false; }
    finishTool (child, lch, target, bredName (nameA, nameB));
    msg = "Bred " + stripMarker (nameA) + " with " + stripMarker (nameB) + ".";
    return true;
}

bool ManticoreProcessor::breedWithFavourite (juce::String& msg)
{
    auto& bank = PresetBank::get();
    struct Cand { juce::String key, name; };
    std::vector<Cand> cands;
    for (const auto& key : bank.favouriteKeys())
    {
        if (key == presetKey) continue;
        const auto name = key.substring (2);
        if (key.startsWith ("F:") && bank.findFactory (name) >= 0) cands.push_back ({ key, name });
        else if (key.startsWith ("U:") && PresetBank::userFolder().getChildFile (name + ".mtpreset").existsAsFile()) cands.push_back ({ key, name });
    }
    if (cands.empty())
    {
        msg = "Star some presets in the browser first: Breed with a Favourite picks one of them at random.";
        return false;
    }
    const auto& c = cands[(size_t) rng.nextInt ((int) cands.size())];
    juce::ValueTree t;
    if (c.key.startsWith ("F:"))
        t = treeFromPatch (patchFromPresetVar (bank.factoryData (bank.findFactory (c.name))), c.name, c.key);
    else if (auto xml = juce::XmlDocument::parse (PresetBank::userFolder().getChildFile (c.name + ".mtpreset")))
    {
        t = juce::ValueTree::fromXml (*xml);
        t.setProperty ("presetName", c.name, nullptr);
        t.setProperty ("presetKey", c.key, nullptr);
    }
    if (! t.isValid()) { msg = "Couldn't open " + c.name + "."; return false; }
    abStore[1 - abSlot] = t;
    return breed (msg);
}

bool ManticoreProcessor::isHaunting() const
{
    if (auto* r = apvts.getRawParameterValue (id::hauntOn)) return r->load() > 0.5f;
    return false;
}

bool ManticoreProcessor::capture (juce::String& msg)
{
    if (! isHaunting() && engine.hauntLevel.load() < 0.01f)
    {
        msg = "Turn Haunt on first, then press Capture when you hear something you like.";
        return false;
    }
    auto p = currentPatch();
    int moved = 0;
    for (int i = 0; i < (int) p.v.size(); ++i)
    {
        const float off = engine.hauntOffset (i);
        if (off == 0.0f) continue;
        p.setNorm (i, p.norm (i) + off);
        ++moved;
    }
    if (auto* h = apvts.getParameter (id::hauntOn)) h->setValueNotifyingHost (0.0f);
    engine.snapHauntHome();
    applyPatch (p);
    presetName = mutatedName (presetName);
    presetKey = "";
    pushHistory();
    msg = "Captured: " + juce::String (moved) + " drifting controls kept as the new settings. Haunt is off.";
    return true;
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new mc::ManticoreProcessor(); }
