#pragma once
#include <juce_core/juce_core.h>

namespace vocalyst
{

// Musical keys / scales shared by Tune and Auto Key.
namespace scales
{
    constexpr int kNumKeys   = 12;
    constexpr int kNumScales = 9;

    const juce::StringArray& keyNames();      // C, C#, D ...
    const juce::StringArray& scaleNames();    // Chromatic, Major, Minor ...
    int  scaleMask (int scaleIndex);          // bit n set => semitone n (relative to key) is in the scale
    bool noteInScale (int midiNote, int key, int scaleIndex);

    // Nearest scale note to a (fractional) midi pitch. Returns a midi note number.
    int nearestScaleNote (float midi, int key, int scaleIndex);

    juce::String keyName (int key, int scaleIndex);   // "F# Minor"
    inline constexpr int Chromatic = 0, Major = 1, Minor = 2;
}

} // namespace vocalyst
