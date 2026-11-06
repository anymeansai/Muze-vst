#include "Presets.h"
#include "../Core/ModuleRegistry.h"

namespace muze
{

extern const char* const kFactoryPresetData[];
extern const int kNumFactoryPresets;

juce::var Preset::toJson() const
{
    auto* o = new juce::DynamicObject();
    o->setProperty ("name", name);
    o->setProperty ("genre", genre);
    o->setProperty ("description", description);
    o->setProperty ("author", author);
    o->setProperty ("adlib", adlib);
    o->setProperty ("format", "muze-preset-1");
    juce::Array<juce::var> ch;
    for (auto& c : chain) ch.add (c);
    o->setProperty ("chain", ch);
    auto* vals = new juce::DynamicObject();
    for (auto& kv : values) vals->setProperty (juce::Identifier (kv.first), kv.second);
    o->setProperty ("values", juce::var (vals));
    return juce::var (o);
}

Preset Preset::fromJson (const juce::var& v, bool* ok)
{
    Preset p;
    auto* o = v.getDynamicObject();
    if (o == nullptr || ! o->hasProperty ("name") || ! o->hasProperty ("chain"))
    {
        if (ok) *ok = false;
        return p;
    }
    p.name = o->getProperty ("name").toString();
    p.genre = o->getProperty ("genre").toString();
    p.description = o->getProperty ("description").toString();
    p.author = o->getProperty ("author").toString();
    p.adlib = (bool) o->getProperty ("adlib");
    if (auto* arr = o->getProperty ("chain").getArray())
        for (auto& c : *arr)
            if (findModuleByKey (c.toString()) != nullptr)
                p.chain.push_back (c.toString());
    if (auto* vals = o->getProperty ("values").getDynamicObject())
        for (auto& prop : vals->getProperties())
            p.values.push_back ({ prop.name.toString(), (float) prop.value });
    if (ok) *ok = true;
    return p;
}

namespace presets
{

Preset parseCompact (const juce::String& encoded)
{
    Preset p;
    juce::StringArray parts;
    parts.addTokens (encoded, "|", "");
    if (parts.size() < 6) return p;
    p.name = parts[0].trim();
    p.genre = parts[1].trim();
    p.adlib = parts[2].trim() == "adlib";
    p.description = parts[3].trim();
    p.author = "Muze";
    juce::StringArray chain;
    chain.addTokens (parts[4], ",", "");
    for (auto& c : chain)
    {
        auto key = c.trim();
        if (key.isNotEmpty() && findModuleByKey (key) != nullptr)
            p.chain.push_back (key);
    }
    juce::StringArray kvs;
    kvs.addTokens (parts[5], ";", "");
    for (auto& kv : kvs)
    {
        auto t = kv.trim();
        if (! t.containsChar ('=')) continue;
        p.values.push_back ({ t.upToFirstOccurrenceOf ("=", false, false).trim(),
                              t.fromFirstOccurrenceOf ("=", false, false).trim().getFloatValue() });
    }
    return p;
}

const std::vector<Preset>& factory()
{
    static const std::vector<Preset> list = []
    {
        std::vector<Preset> v;
        v.reserve ((size_t) kNumFactoryPresets);
        for (int i = 0; i < kNumFactoryPresets; ++i)
            v.push_back (parseCompact (kFactoryPresetData[i]));
        return v;
    }();
    return list;
}

juce::StringArray genres (bool adlib)
{
    juce::StringArray g;
    for (auto& p : factory())
        if (p.adlib == adlib && ! g.contains (p.genre))
            g.add (p.genre);
    return g;
}

int count (bool adlib)
{
    int n = 0;
    for (auto& p : factory()) if (p.adlib == adlib) ++n;
    return n;
}

int indexOf (const juce::String& name)
{
    auto& f = factory();
    for (int i = 0; i < (int) f.size(); ++i)
        if (f[(size_t) i].name == name) return i;
    return -1;
}

juce::String fileExtension() { return ".muzepreset"; }

} // namespace presets
} // namespace muze
