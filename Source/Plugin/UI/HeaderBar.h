#pragma once
#include "VocalystLookAndFeel.h"
#include "../VocalystProcessor.h"

namespace vocalyst::ui
{

class HeaderBar : public juce::Component,
                  private VocalystProcessor::Listener,
                  private juce::Timer
{
public:
    enum Tab { Presets = 0, Mix = 1, Community = 2 };

    explicit HeaderBar (VocalystProcessor&);
    ~HeaderBar() override;

    std::function<void (int)> onTabChanged;
    std::function<void()>     onAnalyze;

    void setTab (int tab);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void presetChanged() override;
    void keyChanged() override;
    void timerCallback() override;
    void refresh();
    void showAutoKeyMenu();

    VocalystProcessor& processor;
    juce::TextButton prevBtn { "<" }, nextBtn { ">" };
    juce::Label presetName, presetMeta;
    PillButton tabPresets { "PRESETS" }, tabMix { "MIX" }, tabCommunity { "COMMUNITY" };
    juce::Label keyLabel;
    juce::ComboBox keyBox, scaleBox;
    std::unique_ptr<juce::ComboBoxParameterAttachment> keyAtt, scaleAtt;
    juce::TextButton autoKeyBtn { "AUTO KEY" }, analyzeBtn { "ANALYZE" };
    int currentTab = 0;
};

} // namespace vocalyst::ui
