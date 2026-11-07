#pragma once
#include "VocalystLookAndFeel.h"
#include "../VocalystProcessor.h"

namespace vocalyst::ui
{

// Toggleable meter strip: input/output spectrum analyzer, per-band stereo width and loudness.
class MeterStrip : public juce::Component, private juce::Timer
{
public:
    explicit MeterStrip (VocalystProcessor&);
    void paint (juce::Graphics&) override;
    void visibilityChanged() override { if (isVisible()) startTimerHz (30); else stopTimer(); }
private:
    void timerCallback() override;
    float xForHz (float hz, juce::Rectangle<float> area) const;
    VocalystProcessor& processor;
    MeterEngine::Snapshot snap;
    std::array<float, MeterEngine::numBins> inSmooth {}, outSmooth {};
    std::array<float, MeterEngine::numBands> widthSmooth {}, corrSmooth {};
    bool hasData = false;
};

} // namespace vocalyst::ui
