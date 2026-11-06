#include "SharedKey.h"

namespace muze::sharedkey
{

juce::File muzeDataDirectory()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Muze");
    dir.createDirectory();
    return dir;
}

juce::File presetsDirectory()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Muze").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

juce::File communityDirectory()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Muze").getChildFile ("Community");
    d.createDirectory();
    return d;
}

juce::File file()
{
    return muzeDataDirectory().getChildFile ("autokey.json");
}

bool write (int key, int scale, float confidence, const juce::String& source)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("key", key);
    obj->setProperty ("scale", scale);
    obj->setProperty ("confidence", confidence);
    obj->setProperty ("source", source);
    obj->setProperty ("time", juce::Time::getCurrentTime().toMilliseconds());
    return file().replaceWithText (juce::JSON::toString (juce::var (obj)));
}

Info read()
{
    Info info;
    auto f = file();
    if (! f.existsAsFile()) return info;
    auto v = juce::JSON::parse (f.loadFileAsString());
    if (auto* o = v.getDynamicObject())
    {
        info.valid = o->hasProperty ("key");
        info.key = (int) o->getProperty ("key");
        info.scale = (int) o->getProperty ("scale");
        info.confidence = (float) o->getProperty ("confidence");
        info.source = o->getProperty ("source").toString();
        info.time = juce::Time ((juce::int64) o->getProperty ("time"));
    }
    return info;
}

} // namespace muze::sharedkey
