#include "Presets.h"

namespace mc
{
PresetBank& PresetBank::get()
{
    static PresetBank b;
    return b;
}

juce::StringArray PresetBank::categories()
{
    return { "Pads", "Atmospheres", "Leads", "Bass", "Keys & Bells", "Plucks", "Arps & Sequences", "Brass & Strings",
             "Choirs & Voices", "Drones", "Cinematic FX", "Textures" };
}

juce::StringArray PresetBank::genres() { return { "Rock", "Metal", "Prog", "Cinematic", "Ambient", "Game" }; }

PresetBank::PresetBank()
{
    juce::MemoryInputStream raw (BinaryData::factory_presets_json_gz, BinaryData::factory_presets_json_gzSize, false);
    juce::GZIPDecompressorInputStream gz (&raw, false, juce::GZIPDecompressorInputStream::gzipFormat);
    const auto text = gz.readEntireStreamAsString();
    const auto root = juce::JSON::parse (text);
    if (auto* arr = root.getArray())
    {
        int i = 0;
        for (const auto& v : *arr)
        {
            PresetInfo p;
            p.name = v["name"].toString();
            p.category = v["cat"].toString();
            p.description = v["desc"].toString();
            p.author = v.hasProperty ("author") ? v["author"].toString() : juce::String ("Fellowship of the String");
            if (auto* t = v["tags"].getArray()) for (const auto& tag : *t) p.tags.add (tag.toString());
            p.factoryIndex = i++;
            if (v.hasProperty ("was")) renamedFrom.set (v["was"].toString(), p.name);
            infos.push_back (p);
            data.push_back (v);
        }
    }
    loadFavourites();
}

juce::String PresetBank::currentName (const juce::String& name) const
{
    return renamedFrom.containsKey (name) ? renamedFrom[name] : name;
}

int PresetBank::findFactory (const juce::String& name) const
{
    for (int i = 0; i < (int) infos.size(); ++i) if (infos[(size_t) i].name == name) return i;
    return -1;
}

juce::File PresetBank::rootFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Manticore");
}

juce::File PresetBank::userFolder() { return rootFolder().getChildFile ("Presets"); }

std::vector<PresetInfo> PresetBank::userPresets() const
{
    std::vector<PresetInfo> out;
    auto files = userFolder().findChildFiles (juce::File::findFiles, false, "*.mtpreset");
    files.sort();
    for (auto& f : files)
    {
        PresetInfo p;
        p.name = f.getFileNameWithoutExtension();
        p.category = "User";
        p.isUser = true;
        p.file = f;
        if (auto xml = juce::XmlDocument::parse (f))
        {
            p.description = xml->getStringAttribute ("description");
            p.tags.addTokens (xml->getStringAttribute ("tags"), ",", "");
            p.tags.removeEmptyStrings();
            const auto cat = xml->getStringAttribute ("category");
            if (cat.isNotEmpty()) p.category = cat;
        }
        out.push_back (p);
    }
    return out;
}

void PresetBank::loadFavourites()
{
    favourites.clear();
    auto f = rootFolder().getChildFile ("Favourites.txt");
    if (f.existsAsFile())
    {
        favourites.addLines (f.loadFileAsString());
        favourites.removeEmptyStrings();
        // factory presets renamed in an update keep their stars
        bool migrated = false;
        for (auto& k : favourites)
            if (k.startsWith ("F:") && renamedFrom.containsKey (k.substring (2)))
            {
                k = "F:" + renamedFrom[k.substring (2)];
                migrated = true;
            }
        favourites.removeDuplicates (false);
        if (migrated) saveFavourites();
    }
}

void PresetBank::saveFavourites() const
{
    rootFolder().createDirectory();
    rootFolder().getChildFile ("Favourites.txt").replaceWithText (favourites.joinIntoString ("\n"));
}

void PresetBank::setFavourite (const juce::String& key, bool fav)
{
    if (fav) favourites.addIfNotAlreadyThere (key);
    else favourites.removeString (key);
    saveFavourites();
}
}
