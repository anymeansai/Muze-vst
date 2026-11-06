#pragma once
#include <juce_core/juce_core.h>

namespace vocalyst::sharedkey
{
    // Small file-based bridge between Vocalyst Auto Key (on the instrumental/master) and Vocalyst (on the vocal).
    struct Info
    {
        bool  valid = false;
        int   key = 0;
        int   scale = 1;
        float confidence = 0.f;
        juce::String source;
        juce::Time time;
    };

    juce::File vocalystDataDirectory();      // ~/Library/Application Support/Vocalyst | %APPDATA%/Vocalyst | ~/.config/Vocalyst
    juce::File presetsDirectory();       // user presets
    juce::File communityDirectory();     // shared presets
    juce::File file();                   // autokey.json
    bool write (int key, int scale, float confidence, const juce::String& source);
    Info read();
}
