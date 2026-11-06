#pragma once
#include "MuzeLookAndFeel.h"
#include "../MuzeProcessor.h"

namespace muze::ui
{

// Simple mode: six macro knobs that bend the loaded preset without touching its settings.
class MacroPanel : public juce::Component, private MuzeProcessor::Listener
{
public:
    explicit MacroPanel (MuzeProcessor&);
    ~MacroPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void chainChanged() override { repaint(); }
    MuzeProcessor& processor;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<juce::String> captions;
    juce::Label heading, sub;
};

} // namespace muze::ui
