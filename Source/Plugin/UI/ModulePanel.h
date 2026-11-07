#pragma once
#include "VocalystLookAndFeel.h"
#include "../VocalystProcessor.h"

namespace vocalyst::ui
{

// Knobs for the currently selected module.
class ModulePanel : public juce::Component
{
public:
    explicit ModulePanel (VocalystProcessor&);
    void setModule (ModuleId id);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    VocalystProcessor& processor;
    ModuleId current = ModuleId::Count;
    juce::Label title, description, category;
    juce::ToggleButton power;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAtt;
    juce::TextButton removeBtn { "REMOVE FROM CHAIN" };
    juce::ComboBox presetBox;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::Label>> comboLabels;
    std::vector<std::unique_ptr<juce::ComboBoxParameterAttachment>> comboAtts;
};

} // namespace vocalyst::ui
