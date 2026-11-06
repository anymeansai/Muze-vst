#include "VocalystEditor.h"

namespace vocalyst
{

VocalystEditor::VocalystEditor (VocalystProcessor& p)
    : AudioProcessorEditor (p), vocalystProcessor (p),
      header (p), footer (p), presetsPanel (p), mixView (p), community (p), analyze (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (header);
    addAndMakeVisible (footer);
    addAndMakeVisible (presetsPanel);
    addAndMakeVisible (mixView);
    addAndMakeVisible (community);
    addChildComponent (analyze);

    header.onTabChanged = [this] (int t) { showTab (t); };
    header.onAnalyze = [this] { analyze.open(); };
    analyze.onClose = [this] { analyze.setVisible (false); };

    setSize (defaultWidth, defaultHeight);
    showTab ((int) vocalystProcessor.getUiProperty ("tab", 1));
}

VocalystEditor::~VocalystEditor()
{
    setLookAndFeel (nullptr);
}

void VocalystEditor::showTab (int tab)
{
    tab = juce::jlimit (0, 2, tab);
    vocalystProcessor.setUiProperty ("tab", tab);
    header.setTab (tab);
    presetsPanel.setVisible (tab == ui::HeaderBar::Presets);
    mixView.setVisible (tab == ui::HeaderBar::Mix);
    community.setVisible (tab == ui::HeaderBar::Community);
    if (tab == ui::HeaderBar::Community) community.refresh();
    if (tab == ui::HeaderBar::Presets) presetsPanel.refreshUserPresets();
}

void VocalystEditor::setSimpleMode (bool simple)
{
    vocalystProcessor.setSimpleMode (simple);
    mixView.refreshMode();
}

void VocalystEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::colours::bg());
}

void VocalystEditor::resized()
{
    auto r = getLocalBounds();
    header.setBounds (r.removeFromTop (64));
    footer.setBounds (r.removeFromBottom (72));
    presetsPanel.setBounds (r);
    mixView.setBounds (r);
    community.setBounds (r);
    analyze.setBounds (getLocalBounds());
}

} // namespace vocalyst
