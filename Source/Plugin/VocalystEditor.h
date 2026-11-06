#pragma once
#include "VocalystProcessor.h"
#include "UI/VocalystLookAndFeel.h"
#include "UI/HeaderBar.h"
#include "UI/FooterBar.h"
#include "UI/PresetBrowserPanel.h"
#include "UI/MixView.h"
#include "UI/CommunityPanel.h"
#include "UI/AnalyzePanel.h"

namespace vocalyst
{

class VocalystEditor final : public juce::AudioProcessorEditor
{
public:
    explicit VocalystEditor (VocalystProcessor&);
    ~VocalystEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void showTab (int tab);
    void setSimpleMode (bool simple);
    void openAnalyze() { analyze.open(); }

    static constexpr int defaultWidth = 1120, defaultHeight = 660;

private:
    VocalystProcessor& vocalystProcessor;
    ui::VocalystLookAndFeel lnf;
    ui::HeaderBar header;
    ui::FooterBar footer;
    ui::PresetBrowserPanel presetsPanel;
    ui::MixView mixView;
    ui::CommunityPanel community;
    ui::AnalyzePanel analyze;
    juce::TooltipWindow tooltips { this, 600 };
};

} // namespace vocalyst
