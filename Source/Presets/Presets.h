#pragma once
#include <juce_core/juce_core.h>
#include <vector>

namespace vocalyst
{

struct Preset
{
    juce::String name, genre, description, author;
    bool adlib = false;
    std::vector<juce::String> chain;                       // ordered module keys
    std::vector<std::pair<juce::String, float>> values;    // parameter id -> value (display units)

    juce::var  toJson() const;
    static Preset fromJson (const juce::var& v, bool* ok = nullptr);
    juce::String toJsonString() const { return juce::JSON::toString (toJson()); }
    static Preset fromJsonString (const juce::String& s, bool* ok = nullptr) { return fromJson (juce::JSON::parse (s), ok); }
};

namespace presets
{
    const std::vector<Preset>& factory();                  // 59 main + 37 adlib
    juce::StringArray genres (bool adlib);                 // in display order
    int  count (bool adlib);
    int  indexOf (const juce::String& name);               // -1 if not a factory preset
    Preset parseCompact (const juce::String& encoded);     // "name|genre|cat|desc|chain|k=v;k=v"
    juce::String fileExtension();                          // ".vocalystpreset"
}

} // namespace vocalyst
