#include "PitchShifter.h"

namespace vocalyst::dsp
{

void PitchShifter::prepare (double sampleRate, int numChannels)
{
    sr = sampleRate;
    channels = juce::jmax (1, numChannels);
    nominalWindow = (float) (0.040 * sr);
    window = targetWindow = nominalWindow;
    lines.resize ((size_t) channels);
    for (auto& l : lines)
        l.prepare ((int) (0.12 * sr) + 8);
    windowSmooth = msToCoeff (60.f, sr);
    ratioSmooth  = msToCoeff (2.f, sr);
    reset();
}

void PitchShifter::reset()
{
    for (auto& l : lines) l.reset();
    phase = 0.f;
    ratio = targetRatio = 1.f;
    window = targetWindow = nominalWindow;
}

void PitchShifter::setPeriodHint (float periodSamples)
{
    if (periodSamples <= 0.f)
    {
        targetWindow = nominalWindow;
        return;
    }
    // even number of periods so the two taps (W/2 apart) are in phase
    int n = (int) std::lround (nominalWindow / periodSamples);
    if (n < 2) n = 2;
    if (n & 1) ++n;
    float w = (float) n * periodSamples;
    const float minW = (float) (0.012 * sr), maxW = (float) (0.10 * sr);
    targetWindow = juce::jlimit (minW, maxW, w);
}

void PitchShifter::processFrame (const float* in, float* out, int numChannels)
{
    numChannels = juce::jmin (numChannels, channels);

    ratio  += (1.f - ratioSmooth)  * (targetRatio - ratio);
    window += (1.f - windowSmooth) * (targetWindow - window);

    const float drift = 1.f - ratio;      // ratio > 1 -> delay shrinks -> pitch up
    if (std::abs (drift) < 1.0e-4f)
    {
        // Park the phase on a "pure" tap position so there is no comb filtering at unity.
        const float target = (phase < 0.25f || phase >= 0.75f) ? 0.f : 0.5f;
        const float step = 1.f / (0.05f * (float) sr);
        float d = target - phase;
        if (phase >= 0.75f) d = 1.f - phase; // wrap towards 1 (== 0)
        d = juce::jlimit (-step, step, d);
        phase += d;
    }
    else
    {
        phase += drift / window;
    }
    while (phase >= 1.f) phase -= 1.f;
    while (phase < 0.f)  phase += 1.f;

    float phase2 = phase + 0.5f;
    if (phase2 >= 1.f) phase2 -= 1.f;

    const float d1 = 1.f + phase  * window;
    const float d2 = 1.f + phase2 * window;
    const float s1 = std::sin (kPi * phase);
    const float g1 = s1 * s1;
    const float g2 = 1.f - g1;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto& line = lines[(size_t) ch];
        line.push (in[ch]);
        out[ch] = g1 * line.read (d1) + g2 * line.read (d2);
    }
}

} // namespace vocalyst::dsp
