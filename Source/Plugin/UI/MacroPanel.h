#pragma once
#include "VocalystLookAndFeel.h"
#include "../VocalystProcessor.h"

namespace vocalyst::ui
{

// Simple mode: six macro knobs that bend the loaded preset without touching its settings.
class MacroPanel : public juce::Component, private VocalystProcessor::Listener
{
public:
    explicit MacroPanel (VocalystProcessor&);
    ~MacroPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void chainChanged() override { repaint(); }
    VocalystProcessor& processor;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<juce::String> captions;
    juce::Label heading, sub;
};

} // namespace vocalyst::ui
