#pragma once
#include "VocalystProcessor.h"
#include "UI/VocalystLookAndFeel.h"
#include "UI/HeaderBar.h"
#include "UI/FooterBar.h"
#include "UI/PresetBrowserPanel.h"
#include "UI/MixView.h"
#include "UI/CommunityPanel.h"
#include "UI/AnalyzePanel.h"
#include "UI/TuneView.h"

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
    void setMetersVisible (bool on) { footer.setMetersActive (on); mixView.setMetersVisible (on); }

    static constexpr int defaultWidth = 1160, defaultHeight = 680;

private:
    VocalystProcessor& vocalystProcessor;
    ui::VocalystLookAndFeel lnf;
    ui::HeaderBar header;
    ui::FooterBar footer;
    ui::PresetBrowserPanel presetsPanel;
    ui::MixView mixView;
    ui::TuneView tuneView;
    ui::CommunityPanel community;
    ui::AnalyzePanel analyze;
    juce::TooltipWindow tooltips { this, 600 };
};

} // namespace vocalyst
