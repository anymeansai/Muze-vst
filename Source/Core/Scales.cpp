#include "Scales.h"
#include <cmath>

namespace vocalyst::scales
{

const juce::StringArray& keyNames()
{
    static const juce::StringArray names { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return names;
}

const juce::StringArray& scaleNames()
{
    static const juce::StringArray names { "Chromatic", "Major", "Minor", "Harmonic Minor", "Dorian",
                                           "Mixolydian", "Major Pentatonic", "Minor Pentatonic", "Blues" };
    return names;
}

int scaleMask (int scaleIndex)
{
    static const int masks[kNumScales] = {
        0b111111111111,                                   // Chromatic
        (1<<0)|(1<<2)|(1<<4)|(1<<5)|(1<<7)|(1<<9)|(1<<11), // Major
        (1<<0)|(1<<2)|(1<<3)|(1<<5)|(1<<7)|(1<<8)|(1<<10), // Natural minor
        (1<<0)|(1<<2)|(1<<3)|(1<<5)|(1<<7)|(1<<8)|(1<<11), // Harmonic minor
        (1<<0)|(1<<2)|(1<<3)|(1<<5)|(1<<7)|(1<<9)|(1<<10), // Dorian
        (1<<0)|(1<<2)|(1<<4)|(1<<5)|(1<<7)|(1<<9)|(1<<10), // Mixolydian
        (1<<0)|(1<<2)|(1<<4)|(1<<7)|(1<<9),                // Major pentatonic
        (1<<0)|(1<<3)|(1<<5)|(1<<7)|(1<<10),               // Minor pentatonic
        (1<<0)|(1<<3)|(1<<5)|(1<<6)|(1<<7)|(1<<10)         // Blues
    };
    return masks[juce::jlimit (0, kNumScales - 1, scaleIndex)];
}

bool noteInScale (int midiNote, int key, int scaleIndex)
{
    const int rel = ((midiNote - key) % 12 + 12) % 12;
    return (scaleMask (scaleIndex) >> rel) & 1;
}

int maskForKey (int key, int scaleIndex)
{
    const int m = scaleMask (scaleIndex);
    int out = 0;
    for (int i = 0; i < 12; ++i)
        if ((m >> i) & 1)
            out |= 1 << (((i + key) % 12 + 12) % 12);
    return out;
}

int nearestScaleNote (float midi, int key, int scaleIndex)
{
    return nearestNoteInMask (midi, maskForKey (key, scaleIndex));
}

int nearestNoteInMask (float midi, int mask12)
{
    if ((mask12 & 0xFFF) == 0) mask12 = 0xFFF;
    const int base = (int) std::lround (midi);
    int best = base;
    float bestDist = 1.0e9f;
    for (int d = -6; d <= 6; ++d)
    {
        const int n = base + d;
        if (! ((mask12 >> ((n % 12 + 12) % 12)) & 1))
            continue;
        const float dist = std::abs ((float) n - midi);
        if (dist < bestDist - 1.0e-4f || (std::abs (dist - bestDist) < 1.0e-4f && n < best))
        {
            bestDist = dist;
            best = n;
        }
    }
    return best;
}

juce::String keyName (int key, int scaleIndex)
{
    return keyNames()[juce::jlimit (0, 11, key)] + " " + scaleNames()[juce::jlimit (0, kNumScales - 1, scaleIndex)];
}

} // namespace vocalyst::scales
