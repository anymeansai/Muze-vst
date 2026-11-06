#include "MuzeEditor.h"

namespace muze
{

MuzeEditor::MuzeEditor (MuzeProcessor& p)
    : AudioProcessorEditor (p), processor (p),
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
    showTab ((int) processor.getUiProperty ("tab", 1));
}

MuzeEditor::~MuzeEditor()
{
    setLookAndFeel (nullptr);
}

void MuzeEditor::showTab (int tab)
{
    tab = juce::jlimit (0, 2, tab);
    processor.setUiProperty ("tab", tab);
    header.setTab (tab);
    presetsPanel.setVisible (tab == ui::HeaderBar::Presets);
    mixView.setVisible (tab == ui::HeaderBar::Mix);
    community.setVisible (tab == ui::HeaderBar::Community);
    if (tab == ui::HeaderBar::Community) community.refresh();
    if (tab == ui::HeaderBar::Presets) presetsPanel.refreshUserPresets();
}

void MuzeEditor::setSimpleMode (bool simple)
{
    processor.setSimpleMode (simple);
    mixView.refreshMode();
}

void MuzeEditor::paint (juce::Graphics& g)
{
    g.fillAll (ui::colours::bg());
}

void MuzeEditor::resized()
{
    auto r = getLocalBounds();
    header.setBounds (r.removeFromTop (64));
    footer.setBounds (r.removeFromBottom (72));
    presetsPanel.setBounds (r);
    mixView.setBounds (r);
    community.setBounds (r);
    analyze.setBounds (getLocalBounds());
}

} // namespace muze
