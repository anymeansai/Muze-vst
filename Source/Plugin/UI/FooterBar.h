#pragma once
#include "VocalystLookAndFeel.h"
#include "../VocalystProcessor.h"

namespace vocalyst::ui
{

class LevelMeter : public juce::Component
{
public:
    void setLevels (float l, float r) { lv[0] = juce::jmax (l, lv[0] * 0.85f); lv[1] = juce::jmax (r, lv[1] * 0.85f); repaint(); }
    void paint (juce::Graphics&) override;
private:
    float lv[2] { 0.f, 0.f };
};

class FooterBar : public juce::Component, private juce::Timer
{
public:
    explicit FooterBar (VocalystProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    VocalystProcessor& processor;
    Knob inKnob, mixKnob, outKnob;
    LevelMeter inMeter, outMeter;
    juce::Label latencyLabel, hint;
};

} // namespace vocalyst::ui
