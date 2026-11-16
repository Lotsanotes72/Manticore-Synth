#pragma once
#include <JuceHeader.h>

namespace mc
{
struct PresetInfo
{
    juce::String name, category, description, author;
    juce::StringArray tags;            // genre tags: Rock, Metal, Prog, Cinematic, Ambient, Game
    bool isUser = false;
    juce::File file;                   // user presets only
    int factoryIndex = -1;
    juce::String key() const { return isUser ? "U:" + file.getFileNameWithoutExtension() : "F:" + name; }
};

// Factory bank (embedded JSON) and the favourites list.
class PresetBank
{
public:
    static PresetBank& get();
    const std::vector<PresetInfo>& factory() const { return infos; }
    const juce::var& factoryData (int i) const { return data[(size_t) i]; }
    int numFactory() const { return (int) infos.size(); }
    int findFactory (const juce::String& name) const;

    static juce::File rootFolder();     // Documents/Manticore
    static juce::File userFolder();     // Documents/Manticore/Presets
    std::vector<PresetInfo> userPresets() const;

    bool isFavourite (const juce::String& key) const { return favourites.contains (key); }
    void setFavourite (const juce::String& key, bool fav);
    const juce::StringArray& favouriteKeys() const { return favourites; }

    static juce::StringArray categories();
    static juce::StringArray genres();
private:
    PresetBank();
    void loadFavourites();
    void saveFavourites() const;
    std::vector<PresetInfo> infos;
    std::vector<juce::var> data;
    juce::StringArray favourites;
};
}
