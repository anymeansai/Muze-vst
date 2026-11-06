#pragma once
#include "AutoKeyProcessor.h"
#include "../Plugin/UI/MuzeLookAndFeel.h"

namespace muze
{

class AutoKeyEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AutoKeyEditor (AutoKeyProcessor&);
    ~AutoKeyEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    AutoKeyProcessor& processor;
    ui::MuzeLookAndFeel lnf;
    KeyDetector::Result result;
    juce::TextButton sendBtn { "SEND TO MUZE" }, resetBtn { "RESET" };
    ui::PillButton autoBtn { "AUTO SEND" };
    juce::Label status;
};

} // namespace muze
