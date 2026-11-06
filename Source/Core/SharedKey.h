#pragma once
#include <juce_core/juce_core.h>

namespace muze::sharedkey
{
    // Small file-based bridge between Muze Auto Key (on the instrumental/master) and Muze (on the vocal).
    struct Info
    {
        bool  valid = false;
        int   key = 0;
        int   scale = 1;
        float confidence = 0.f;
        juce::String source;
        juce::Time time;
    };

    juce::File muzeDataDirectory();      // ~/Library/Application Support/Muze | %APPDATA%/Muze | ~/.config/Muze
    juce::File presetsDirectory();       // user presets
    juce::File communityDirectory();     // shared presets
    juce::File file();                   // autokey.json
    bool write (int key, int scale, float confidence, const juce::String& source);
    Info read();
}
