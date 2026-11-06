#pragma once
#include "MuzeLookAndFeel.h"
#include "../MuzeProcessor.h"

namespace muze::ui
{

// Overlay showing what the analyzer heard and the auto-mix suggestions it derived.
class AnalyzePanel : public juce::Component, private juce::Timer
{
public:
    explicit AnalyzePanel (MuzeProcessor&);
    std::function<void()> onClose;
    void open();
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override {}
private:
    void timerCallback() override;
    void refresh();
    MuzeProcessor& processor;
    VocalAnalyzer::Stats stats;
    std::vector<VocalAnalyzer::Suggestion> suggestions;
    juce::TextButton applyBtn { "APPLY SUGGESTIONS" }, resetBtn { "LISTEN AGAIN" }, closeBtn { "CLOSE" };
};

} // namespace muze::ui
