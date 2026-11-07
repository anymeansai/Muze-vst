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

    // Absolute 12-bit pitch-class mask for a key + scale (bit n set => pitch class n allowed)
    int  maskForKey (int key, int scaleIndex);
    // Nearest allowed note to a (fractional) midi pitch, given an absolute pitch-class mask.
    int  nearestNoteInMask (float midi, int mask12);
    // Nearest scale note to a (fractional) midi pitch. Returns a midi note number.
    int  nearestScaleNote (float midi, int key, int scaleIndex);

    juce::String keyName (int key, int scaleIndex);   // "F# Minor"
    inline constexpr int Chromatic = 0, Major = 1, Minor = 2;
}

} // namespace vocalyst
