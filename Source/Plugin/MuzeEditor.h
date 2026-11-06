#pragma once
#include "MuzeProcessor.h"
#include "UI/MuzeLookAndFeel.h"
#include "UI/HeaderBar.h"
#include "UI/FooterBar.h"
#include "UI/PresetBrowserPanel.h"
#include "UI/MixView.h"
#include "UI/CommunityPanel.h"
#include "UI/AnalyzePanel.h"

namespace muze
{

class MuzeEditor final : public juce::AudioProcessorEditor
{
public:
    explicit MuzeEditor (MuzeProcessor&);
    ~MuzeEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void showTab (int tab);
    void setSimpleMode (bool simple);
    void openAnalyze() { analyze.open(); }

    static constexpr int defaultWidth = 1080, defaultHeight = 660;

private:
    MuzeProcessor& processor;
    ui::MuzeLookAndFeel lnf;
    ui::HeaderBar header;
    ui::FooterBar footer;
    ui::PresetBrowserPanel presetsPanel;
    ui::MixView mixView;
    ui::CommunityPanel community;
    ui::AnalyzePanel analyze;
    juce::TooltipWindow tooltips { this, 600 };
};

} // namespace muze
