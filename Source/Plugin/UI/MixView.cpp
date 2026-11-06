#include "MixView.h"

namespace muze::ui
{

MixView::MixView (MuzeProcessor& p) : processor (p), chain (p), modulePanel (p), macros (p)
{
    addAndMakeVisible (modeSimple);
    addAndMakeVisible (modeAdvanced);
    modeSimple.onClick   = [this] { processor.setSimpleMode (true);  refreshMode(); };
    modeAdvanced.onClick = [this] { processor.setSimpleMode (false); refreshMode(); };
    modeLabel.setFont (font (11.f, true));
    modeLabel.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (modeLabel);
    addAndMakeVisible (chain);
    addAndMakeVisible (modulePanel);
    addAndMakeVisible (macros);
    chain.onSelect = [this] (ModuleId id) { modulePanel.setModule (id); processor.setUiProperty ("selectedModule", (int) id); };
    const int sel = (int) processor.getUiProperty ("selectedModule", -1);
    if (sel >= 0 && sel < kNumModules && processor.isInChain ((ModuleId) sel)) { chain.setSelected ((ModuleId) sel); modulePanel.setModule ((ModuleId) sel); }
    else modulePanel.setModule (chain.getSelected());
    refreshMode();
}

void MixView::refreshMode()
{
    const bool simple = processor.isSimpleMode();
    modeSimple.setActive (simple);
    modeAdvanced.setActive (! simple);
    modeLabel.setText (simple ? "SIMPLE MODE  -  six macros shape the preset" : "ADVANCED MODE  -  full control of every effect", juce::dontSendNotification);
    macros.setVisible (simple);
    chain.setVisible (! simple);
    modulePanel.setVisible (! simple);
}

void MixView::resized()
{
    auto r = getLocalBounds().reduced (14, 10);
    auto top = r.removeFromTop (28);
    modeAdvanced.setBounds (top.removeFromRight (96));
    top.removeFromRight (4);
    modeSimple.setBounds (top.removeFromRight (80));
    modeLabel.setBounds (top);
    r.removeFromTop (8);
    macros.setBounds (r);
    chain.setBounds (r.removeFromTop (ChainStrip::tileH + 32));
    r.removeFromTop (10);
    modulePanel.setBounds (r);
}

void MixView::paint (juce::Graphics& g)
{
    g.fillAll (colours::bg());
}

} // namespace muze::ui
