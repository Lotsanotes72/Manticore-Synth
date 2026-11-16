// Offline test / preset-render / screenshot tool for Manticore (Linux build only, not shipped).
#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace mc;

static int failures = 0;
#define CHECK(cond, msg) do { if (! (cond)) { ++failures; std::cout << "  FAIL: " << msg << "\n"; } else std::cout << "  ok: " << msg << "\n"; } while (0)

struct Stats { float peak = 0, rms = 0, loudest = -120, tail = 0; bool finite = true; };

static void setP (ManticoreProcessor& p, const juce::String& id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    jassert (prm != nullptr);
    if (prm) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

static void setC (ManticoreProcessor& p, const juce::String& id, const juce::String& choice)
{
    const int i = paramIndex (id);
    jassert (i >= 0);
    setP (p, id, (float) params()[(size_t) i].choices.indexOf (choice));
}

// Render a phrase: chord, then a short melody. Returns stats and optionally the audio.
static Stats renderPhrase (ManticoreProcessor& p, double sr, juce::AudioBuffer<float>* keep = nullptr, bool melody = true)
{
    const int block = 256;
    const int total = (int) (sr * (melody ? 6.0 : 3.5));
    p.prepareToPlay (sr, block);
    juce::AudioBuffer<float> out (2, total);
    out.clear();
    struct E { int pos; int note; bool on; };
    std::vector<E> ev;
    const int chord[] { 48, 55, 60, 63 };
    for (int n : chord) { ev.push_back ({ 0, n, true }); ev.push_back ({ (int) (sr * 2.0), n, false }); }
    if (melody)
    {
        const int mel[] { 60, 63, 67, 70, 67, 72 };
        for (int k = 0; k < 6; ++k)
        {
            ev.push_back ({ (int) (sr * (3.4 + 0.3 * k)), mel[k], true });
            ev.push_back ({ (int) (sr * (3.4 + 0.3 * k + 0.26)), mel[k], false });
        }
    }
    juce::AudioBuffer<float> buf (2, block);
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin (block, total - pos);
        buf.setSize (2, n, false, false, true);
        buf.clear();
        juce::MidiBuffer midi;
        for (auto& e : ev)
            if (e.pos >= pos && e.pos < pos + n)
                midi.addEvent (e.on ? juce::MidiMessage::noteOn (1, e.note, (juce::uint8) 100) : juce::MidiMessage::noteOff (1, e.note), e.pos - pos);
        p.processBlock (buf, midi);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, n);
    }
    Stats st;
    double sum = 0;
    const int win = (int) (sr * 0.4);
    double wsum = 0;
    for (int i = 0; i < total; ++i)
    {
        const float l = out.getSample (0, i), r = out.getSample (1, i);
        if (! std::isfinite (l) || ! std::isfinite (r)) { st.finite = false; continue; }
        st.peak = juce::jmax (st.peak, std::abs (l), std::abs (r));
        const double e = 0.5 * ((double) l * l + (double) r * r);
        sum += e;
        wsum += e;
        if (i >= win)
        {
            const float ol = out.getSample (0, i - win), orr = out.getSample (1, i - win);
            wsum -= 0.5 * ((double) ol * ol + (double) orr * orr);
            st.loudest = juce::jmax (st.loudest, (float) (10.0 * std::log10 (juce::jmax (1e-12, wsum / win))));
        }
    }
    st.rms = (float) (10.0 * std::log10 (juce::jmax (1e-12, sum / total)));
    double tail = 0;
    for (int i = total - (int) (sr * 0.2); i < total; ++i) tail = juce::jmax (tail, (double) std::abs (out.getSample (0, i)));
    st.tail = (float) tail;
    if (keep) *keep = out;
    return st;
}

static float estimatePitch (const juce::AudioBuffer<float>& b, double sr, int from, int len)
{
    int crossings = 0, first = -1, last = -1;
    for (int i = from + 1; i < from + len; ++i)
        if (b.getSample (0, i - 1) <= 0 && b.getSample (0, i) > 0) { if (first < 0) first = i; last = i; ++crossings; }
    if (crossings < 2) return 0;
    return (float) ((crossings - 1) * sr / (double) (last - first));
}

static void writeWav (const juce::AudioBuffer<float>& b, double sr, const juce::File& f)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
    if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

// ------------------------------------------------------------------ commands
static void dumpParams (const juce::File& f)
{
    juce::Array<juce::var> arr;
    for (const auto& d : params())
    {
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", d.id);
        o->setProperty ("type", d.type == ParamDef::Choice ? "choice" : d.type == ParamDef::Bool ? "bool" : "float");
        o->setProperty ("min", d.min); o->setProperty ("max", d.max); o->setProperty ("def", d.def);
        o->setProperty ("unit", d.unit);
        o->setProperty ("name", d.name);
        o->setProperty ("tip", d.tip);
        o->setProperty ("caption", d.caption);
        juce::Array<juce::var> ch;
        for (auto& c : d.choices) ch.add (c);
        o->setProperty ("choices", ch);
        o->setProperty ("mod", d.mod);
        arr.add (juce::var (o));
    }
    f.replaceWithText (juce::JSON::toString (juce::var (arr)));
    std::cout << "wrote " << params().size() << " parameters, " << destParams().size() << " mod destinations\n";
}

static void unitTests()
{
    const double sr = 48000.0;
    {
        std::cout << "Init patch\n";
        ManticoreProcessor p;
        p.loadInit();
        juce::AudioBuffer<float> b;
        auto st = renderPhrase (p, sr, &b, false);
        CHECK (st.finite, "finite output");
        CHECK (st.peak > 0.05f && st.peak < 2.0f, "audible, sane level (peak " + juce::String (st.peak, 3) + ")");
        // single note pitch check with a sine
        ManticoreProcessor q;
        q.loadInit();
        setC (q, "osc1_table", "Classic");
        setP (q, "osc1_wave", 0.0f);
        setP (q, id::slot (0, 1), 0);
        q.prepareToPlay (sr, 512);
        juce::AudioBuffer<float> buf (2, 512);
        juce::AudioBuffer<float> all (2, 48128);
        for (int pos = 0; pos < 48000; pos += 512)
        {
            buf.clear();
            juce::MidiBuffer m;
            if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (1, 69, (juce::uint8) 100), 0);
            q.processBlock (buf, m);
            for (int c = 0; c < 2; ++c) all.copyFrom (c, pos, buf, c, 0, 512);
        }
        const float f = estimatePitch (all, sr, 4800, 38000);
        CHECK (std::abs (f - 440.0f) < 1.0f, "A4 sine pitch = " + juce::String (f, 2) + " Hz");
    }
    {
        std::cout << "Every generator module in the grid\n";
        const auto& entries = gridEntries();
        for (size_t e = 1; e < entries.size(); e += 1)
        {
            const auto ge = entries[e];
            if (ge.inst != 0) continue;
            ManticoreProcessor p;
            p.loadInit();
            // OSC 1 in slot 1, the module under test in slot 2 (or slot 1 for generators)
            const bool gen = ge.kind == Kind::Osc || ge.kind == Kind::FM || ge.kind == Kind::Noise || ge.kind == Kind::Exc;
            if (gen) { setP (p, id::slot (0, 0), (float) e); setP (p, id::slot (0, 1), 0); }
            else { setP (p, id::slot (0, 0), ge.kind == Kind::Res || ge.kind == Kind::Comb ? 11.0f : 1.0f); setP (p, id::slot (0, 1), (float) e); }
            auto st = renderPhrase (p, sr, nullptr, false);
            CHECK (st.finite && st.peak > 0.005f && st.peak < 4.0f, moduleName (ge.kind, 0) + " peak " + juce::String (st.peak, 3));
        }
        // every filter model and response
        const auto& vdef = params()[(size_t) paramIndex ("vcf1_model")];
        for (int m = 0; m < vdef.choices.size(); ++m)
            for (int r = 0; r < 12; r += 1)
            {
                ManticoreProcessor p;
                p.loadInit();
                setP (p, "vcf1_model", (float) m);
                setP (p, "vcf1_response", (float) r);
                setP (p, "vcf1_reso", 0.95f);
                setP (p, "vcf1_drive", 0.6f);
                auto st = renderPhrase (p, sr, nullptr, false);
                if (! (st.finite && st.peak < 8.0f))
                {
                    ++failures;
                    std::cout << "  FAIL: filter " << vdef.choices[m] << " response " << r << " peak " << st.peak << "\n";
                }
            }
        std::cout << "  ok: all filter models x responses finite at high resonance\n";
    }
    {
        std::cout << "Every effect on the main bus\n";
        const auto& entries = fxEntries();
        for (size_t e = 1; e < entries.size(); ++e)
        {
            if (entries[e].inst != 0) continue;
            ManticoreProcessor p;
            p.loadInit();
            setP (p, id::fxSlot (0, 0), (float) e);
            auto st = renderPhrase (p, sr, nullptr, false);
            CHECK (st.finite && st.peak > 0.01f && st.peak < 4.0f, moduleName (entries[e].kind, 0) + " peak " + juce::String (st.peak, 3));
        }
    }
    {
        std::cout << "Modulation\n";
        ManticoreProcessor p;
        p.loadInit();
        setC (p, "mtx1_src", "LFO 1");
        setP (p, "mtx1_dst", (float) destOfParam (paramIndex ("vcf1_cutoff")));
        setP (p, "mtx1_amt", 0.5f);
        setC (p, "mtx2_src", "MSEG 1");
        setP (p, "mtx2_dst", (float) destOfParam (paramIndex ("osc1_wave")));
        setP (p, "mtx2_amt", 1.0f);
        setC (p, "mtx3_src", "Macro A");
        setP (p, "mtx3_dst", (float) destOfParam (paramIndex ("reverb1_mix")));
        setP (p, "mtx3_amt", 1.0f);
        auto st = renderPhrase (p, sr, nullptr, false);
        CHECK (st.finite && st.peak > 0.05f, "LFO/MSEG/Macro routes render");
    }
    {
        std::cout << "Arpeggiator\n";
        ManticoreProcessor p;
        p.loadInit();
        setP (p, id::arpOn, 1);
        juce::AudioBuffer<float> b;
        auto st = renderPhrase (p, sr, &b, false);
        CHECK (st.finite && st.peak > 0.05f, "arp produces sound");
        int onsets = 0; float prev = 0;
        for (int i = 0; i < (int) (sr * 2.0); i += 256)
        {
            float pk = 0;
            for (int k = 0; k < 256; ++k) pk = juce::jmax (pk, std::abs (b.getSample (0, i + k)));
            if (pk > 0.05f && prev < pk * 0.5f) ++onsets;
            prev = pk;
        }
        CHECK (onsets >= 6, "arp steps heard: " + juce::String (onsets));
    }
    {
        std::cout << "Mono / legato\n";
        ManticoreProcessor p;
        p.loadInit();
        setC (p, id::vmode, "Legato");
        setP (p, id::glide, 0.1f);
        auto st = renderPhrase (p, sr, nullptr, true);
        CHECK (st.finite && st.peak > 0.05f, "legato with glide renders");
    }
    {
        std::cout << "State round trip\n";
        ManticoreProcessor a;
        if (PresetBank::get().numFactory() > 3) a.loadFactoryPreset (3); else a.loadInit();
        a.mseg[1].y[1] = 0.33f;
        a.shapesChanged();
        juce::MemoryBlock mb;
        a.getStateInformation (mb);
        ManticoreProcessor b;
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        bool same = true;
        for (const auto& d : params())
            if (std::abs (a.apvts.getRawParameterValue (d.id)->load() - b.apvts.getRawParameterValue (d.id)->load()) > 1e-4f * juce::jmax (1.0f, std::abs (a.apvts.getRawParameterValue (d.id)->load()))) { same = false; std::cout << "    differs: " << d.id << "\n"; }
        CHECK (same, "all parameters restored");
        CHECK (std::abs (b.mseg[1].y[1] - 0.33f) < 1e-3f, "MSEG shape restored");
        CHECK (b.currentPresetName() == a.currentPresetName(), "preset name restored");
    }
    {
        std::cout << "Sample rates\n";
        for (double r : { 44100.0, 96000.0, 192000.0 })
        {
            ManticoreProcessor p;
            p.loadInit();
            auto st = renderPhrase (p, r, nullptr, false);
            CHECK (st.finite && st.peak > 0.05f, juce::String (r, 0) + " Hz renders");
        }
    }
}

static void renderPresets (const juce::File& outJson, const juce::File& wavDir)
{
    auto& bank = PresetBank::get();
    juce::Array<juce::var> arr;
    const double sr = 48000.0;
    if (wavDir != juce::File()) wavDir.createDirectory();
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (int i = 0; i < bank.numFactory(); ++i)
    {
        ManticoreProcessor p;
        p.loadFactoryPreset (i);
        juce::AudioBuffer<float> b;
        const auto t1 = juce::Time::getMillisecondCounterHiRes();
        auto st = renderPhrase (p, sr, wavDir != juce::File() ? &b : nullptr, true);
        const auto ms = juce::Time::getMillisecondCounterHiRes() - t1;
        auto* o = new juce::DynamicObject();
        o->setProperty ("name", bank.factory()[(size_t) i].name);
        o->setProperty ("peak", st.peak);
        o->setProperty ("rms", st.rms);
        o->setProperty ("loudest", st.loudest);
        o->setProperty ("tail", st.tail);
        o->setProperty ("finite", st.finite);
        o->setProperty ("cpu", ms / 6000.0);
        arr.add (juce::var (o));
        if (wavDir != juce::File()) writeWav (b, sr, wavDir.getChildFile (juce::String (i).paddedLeft ('0', 3) + ".wav"));
        if (i % 50 == 0) std::cout << "  rendered " << i << "/" << bank.numFactory() << "\n" << std::flush;
    }
    outJson.replaceWithText (juce::JSON::toString (juce::var (arr)));
    std::cout << "rendered " << bank.numFactory() << " presets in " << (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0 << " s\n";
}

static void screenshots (const juce::File& dir, float scale, const juce::File& list)
{
    // each line: preset|page|kindIndex|inst|modTab|file|scale
    dir.createDirectory();
    juce::StringArray lines;
    lines.addLines (list.loadFileAsString());
    for (auto line : lines)
    {
        juce::StringArray f;
        f.addTokens (line, "|", "");
        if (f.size() < 6) continue;
        ManticoreProcessor p;
        const int idx = PresetBank::get().findFactory (f[0]);
        if (idx >= 0) p.loadFactoryPreset (idx); else p.loadInit();
        std::unique_ptr<ManticoreEditor> ed (dynamic_cast<ManticoreEditor*> (p.createEditor()));
        const float sc = f.size() > 6 ? f[6].getFloatValue() : scale;
        ed->setSize ((int) (ManticoreEditor::kBaseW * sc), (int) (ManticoreEditor::kBaseH * sc));
        ed->setPage (f[1].getIntValue());
        ed->selectModule ((Kind) f[2].getIntValue(), f[3].getIntValue());
        ed->selectModTab (f[4].getIntValue());
        for (auto* c : ed->getChildren()) c->resized();
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::FileOutputStream os (dir.getChildFile (f[5] + ".png"));
        os.setPosition (0); os.truncate();
        juce::PNGImageFormat().writeImageToStream (img, os);
        std::cout << "  " << f[5] << "\n";
    }
}

static void cpuTest()
{
    auto& bank = PresetBank::get();
    std::vector<std::pair<double, juce::String>> res;
    for (int i = 0; i < bank.numFactory(); i += juce::jmax (1, bank.numFactory() / 40))
    {
        ManticoreProcessor p;
        p.loadFactoryPreset (i);
        const auto t = juce::Time::getMillisecondCounterHiRes();
        renderPhrase (p, 48000.0, nullptr, true);
        res.push_back ({ (juce::Time::getMillisecondCounterHiRes() - t) / 6000.0, bank.factory()[(size_t) i].name });
    }
    std::sort (res.begin(), res.end());
    for (auto& [c, n] : res) std::cout << "  " << juce::String (c * 100.0, 1) << "% realtime  " << n << "\n";
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String cmd = argc > 1 ? argv[1] : "unit";
    if (cmd == "params") dumpParams (juce::File (argv[2]));
    else if (cmd == "unit") { unitTests(); std::cout << (failures == 0 ? "ALL PASSED\n" : juce::String (failures) + " FAILURES\n"); }
    else if (cmd == "render") renderPresets (juce::File (argv[2]), argc > 3 ? juce::File (argv[3]) : juce::File());
    else if (cmd == "gui") screenshots (juce::File (argv[2]), 1.0f, juce::File (argv[3]));
    else if (cmd == "cpu") cpuTest();
    else if (cmd == "one")
    {
        ManticoreProcessor p;
        p.loadFactoryPreset (PresetBank::get().findFactory (argv[2]));
        juce::AudioBuffer<float> b;
        auto st = renderPhrase (p, 48000.0, &b, true);
        writeWav (b, 48000.0, juce::File (argv[3]));
        std::cout << "peak " << st.peak << " loudest " << st.loudest << "\n";
    }
    juce::DeletedAtShutdown::deleteAll();
    return failures == 0 ? 0 : 1;
}
