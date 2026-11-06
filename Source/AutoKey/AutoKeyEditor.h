#pragma once
#include "AutoKeyProcessor.h"
#include "../Plugin/UI/VocalystLookAndFeel.h"

namespace vocalyst
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
    AutoKeyProcessor& keyProcessor;
    ui::VocalystLookAndFeel lnf;
    KeyDetector::Result result;
    juce::TextButton sendBtn { "SEND TO VOCALYST" }, resetBtn { "RESET" };
    ui::PillButton autoBtn { "AUTO SEND" };
    juce::Label status;
};

} // namespace vocalyst
