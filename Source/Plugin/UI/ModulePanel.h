#pragma once
#include "MuzeLookAndFeel.h"
#include "../MuzeProcessor.h"

namespace muze::ui
{

// Knobs for the currently selected module.
class ModulePanel : public juce::Component
{
public:
    explicit ModulePanel (MuzeProcessor&);
    void setModule (ModuleId id);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    MuzeProcessor& processor;
    ModuleId current = ModuleId::Count;
    juce::Label title, description, category;
    juce::ToggleButton power;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAtt;
    juce::TextButton removeBtn { "REMOVE FROM CHAIN" };
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<juce::ComboBox>> combos;
    std::vector<std::unique_ptr<juce::Label>> comboLabels;
    std::vector<std::unique_ptr<juce::ComboBoxParameterAttachment>> comboAtts;
};

} // namespace muze::ui
