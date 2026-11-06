#pragma once
#include "DspUtil.h"

namespace vocalyst::dsp
{

// Time-domain pitch shifter built from two crossfaded delay taps (Doppler style).
// When a period hint is supplied, the window snaps to a whole number of periods so
// both taps stay phase-aligned, which keeps small corrections nearly artifact-free.
class PitchShifter
{
public:
    void prepare (double sampleRate, int numChannels);
    void reset();

    void setRatio (float newRatio)        { targetRatio = juce::jlimit (0.25f, 4.0f, newRatio); }
    void setPeriodHint (float periodSamples); // 0 = none
    void setNominalWindowMs (float ms)    { nominalWindow = (float) (ms * 0.001 * sr); }

    // process one frame (all channels), in -> out may alias
    void processFrame (const float* in, float* out, int numChannels);

    int   latencySamples() const          { return (int) (nominalWindow * 0.5f); }
    float currentRatio() const            { return ratio; }

private:
    double sr = 44100.0;
    int    channels = 2;
    std::vector<DelayLine> lines;
    float  nominalWindow = 1764.f, window = 1764.f, targetWindow = 1764.f;
    float  ratio = 1.f, targetRatio = 1.f;
    float  phase = 0.f;
    float  windowSmooth = 0.999f, ratioSmooth = 0.995f;
};

} // namespace vocalyst::dsp
