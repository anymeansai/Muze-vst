#pragma once
#include "ChainStrip.h"
#include "ModulePanel.h"
#include "MacroPanel.h"

namespace vocalyst::ui
{

// The MIX tab: Simple mode shows macros, Advanced mode shows the chain editor.
class MixView : public juce::Component
{
public:
    explicit MixView (VocalystProcessor&);
    void refreshMode();
    void resized() override;
    void paint (juce::Graphics&) override;
private:
    VocalystProcessor& processor;
    PillButton modeSimple { "SIMPLE" }, modeAdvanced { "ADVANCED" };
    juce::Label modeLabel;
    ChainStrip chain;
    ModulePanel modulePanel;
    MacroPanel macros;
};

} // namespace vocalyst::ui
