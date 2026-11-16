#include "PluginProcessor.h"
#include "PluginEditor.h"

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
      apvts (*this, &undo, "MANTICORE", createLayout())
{
    engine.bind (apvts);
    resetShapes (mseg, map);
    shapesChanged();
    PresetBank::get();
    abStore[0] = makeStateTree();
    abStore[1] = abStore[0].createCopy();
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

void ManticoreProcessor::shapesChanged()
{
    for (int i = 0; i < 4; ++i) { engine.setMseg (i, mseg[i]); engine.setMap (i, map[i]); }
    ++shapesVersion;
}

juce::ValueTree ManticoreProcessor::shapesToTree() const
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

void ManticoreProcessor::shapesFromTree (const juce::ValueTree& t)
{
    resetShapes (mseg, map);
    if (! t.isValid()) { shapesChanged(); return; }
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
                auto& d = mseg[i];
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
            for (int k = 0; k < kMapSteps && k < v.size(); ++k) map[i].v[k] = juce::jlimit (-1.0f, 1.0f, v[k].getFloatValue());
        }
    }
    shapesChanged();
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

void ManticoreProcessor::restoreStateTree (const juce::ValueTree& t)
{
    if (! t.isValid()) return;
    auto copy = t.createCopy();
    shapesFromTree (copy.getChildWithName ("Shapes"));
    presetName = copy.getProperty ("presetName", "Init").toString();
    presetKey = copy.getProperty ("presetKey", "").toString();
    copy.removeChild (copy.getChildWithName ("Shapes"), nullptr);
    copy.removeChild (copy.getChildWithName ("UI"), nullptr);
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
        restoreStateTree (t);
        abStore[abSlot] = makeStateTree();
    }
}

// ------------------------------------------------------------------ presets
void ManticoreProcessor::resetAllParamsToDefault()
{
    for (const auto& d : params())
        if (auto* p = apvts.getParameter (d.id))
            p->setValueNotifyingHost (p->getDefaultValue());
}

void ManticoreProcessor::loadInit()
{
    undo.beginNewTransaction ("Init");
    resetAllParamsToDefault();
    resetShapes (mseg, map);
    shapesChanged();
    presetName = "Init";
    presetKey = "";
    engine.reset();
}

void ManticoreProcessor::applyPresetVar (const juce::var& v)
{
    resetAllParamsToDefault();
    if (auto* obj = v["p"].getDynamicObject())
    {
        for (const auto& kv : obj->getProperties())
        {
            const auto pidStr = kv.name.toString();
            const int idx = paramIndex (pidStr);
            if (idx < 0) continue;
            const auto& d = params()[(size_t) idx];
            auto* prm = apvts.getParameter (pidStr);
            if (prm == nullptr) continue;
            float value;
            if (kv.value.isString())
            {
                const int ci = d.choices.indexOf (kv.value.toString(), true);
                if (ci < 0) continue;
                value = (float) ci;
            }
            else if (kv.value.isBool()) value = (bool) kv.value ? 1.0f : 0.0f;
            else value = (float) (double) kv.value;
            prm->setValueNotifyingHost (prm->convertTo0to1 (value));
        }
    }
    resetShapes (mseg, map);
    if (auto* ms = v["mseg"].getArray())
        for (const auto& m : *ms)
        {
            const int i = juce::jlimit (0, 3, (int) m["i"]);
            auto& d = mseg[i];
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
                for (int k = 0; k < kMapSteps && k < vals->size(); ++k) map[i].v[k] = juce::jlimit (-1.0f, 1.0f, (float) (double) (*vals)[k]);
        }
    shapesChanged();
}

void ManticoreProcessor::loadFactoryPreset (int index)
{
    auto& bank = PresetBank::get();
    if (! juce::isPositiveAndBelow (index, bank.numFactory())) { loadInit(); return; }
    undo.beginNewTransaction ("Preset");
    applyPresetVar (bank.factoryData (index));
    currentProgram = index;
    presetName = bank.factory()[(size_t) index].name;
    presetKey = bank.factory()[(size_t) index].key();
    engine.reset();
    undo.clearUndoHistory();
}

bool ManticoreProcessor::loadUserPreset (const juce::File& f)
{
    auto xml = juce::XmlDocument::parse (f);
    if (xml == nullptr) return false;
    auto t = juce::ValueTree::fromXml (*xml);
    if (! t.hasType (apvts.state.getType())) return false;
    undo.beginNewTransaction ("Preset");
    restoreStateTree (t);
    presetName = f.getFileNameWithoutExtension();
    presetKey = "U:" + presetName;
    engine.reset();
    undo.clearUndoHistory();
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
    if (auto xml = t.createXml())
    {
        xml->setAttribute ("category", category);
        xml->setAttribute ("tags", tags);
        xml->setAttribute ("description", desc);
        return xml->writeTo (dir.getChildFile (clean + ".mtpreset"));
    }
    return false;
}

void ManticoreProcessor::abSelect (int slot)
{
    if (slot == abSlot) return;
    abStore[abSlot] = makeStateTree();
    abSlot = slot;
    restoreStateTree (abStore[abSlot]);
}

void ManticoreProcessor::abCopyToOther()
{
    abStore[1 - abSlot] = makeStateTree();
}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new mc::ManticoreProcessor(); }
