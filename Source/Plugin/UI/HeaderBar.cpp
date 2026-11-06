#include "HeaderBar.h"

namespace vocalyst::ui
{

HeaderBar::HeaderBar (VocalystProcessor& p) : processor (p)
{
    for (auto* b : { &prevBtn, &nextBtn })
    {
        addAndMakeVisible (b);
        b->setColour (juce::TextButton::buttonColourId, colours::panelLight());
    }
    prevBtn.onClick = [this] { processor.prevPreset(); };
    nextBtn.onClick = [this] { processor.nextPreset(); };

    presetName.setFont (font (15.f, true));
    presetName.setJustificationType (juce::Justification::centredLeft);
    presetMeta.setFont (font (11.f));
    presetMeta.setJustificationType (juce::Justification::centredLeft);
    presetMeta.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (presetName);
    addAndMakeVisible (presetMeta);

    auto setupTab = [this] (PillButton& b, int tab)
    {
        addAndMakeVisible (b);
        b.onClick = [this, tab] { setTab (tab); if (onTabChanged) onTabChanged (tab); };
    };
    setupTab (tabPresets, Presets);
    setupTab (tabMix, Mix);
    setupTab (tabCommunity, Community);

    keyLabel.setText ("KEY", juce::dontSendNotification);
    keyLabel.setFont (font (11.f, true));
    keyLabel.setColour (juce::Label::textColourId, colours::textDim());
    keyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (keyLabel);

    for (int i = 0; i < scales::keyNames().size(); ++i) keyBox.addItem (scales::keyNames()[i], i + 1);
    for (int i = 0; i < scales::scaleNames().size(); ++i) scaleBox.addItem (scales::scaleNames()[i], i + 1);
    addAndMakeVisible (keyBox);
    addAndMakeVisible (scaleBox);
    if (auto* kp = processor.apvts.getParameter (ids::key))   keyAtt   = std::make_unique<juce::ComboBoxParameterAttachment> (*kp, keyBox);
    if (auto* sp = processor.apvts.getParameter (ids::scale)) scaleAtt = std::make_unique<juce::ComboBoxParameterAttachment> (*sp, scaleBox);

    addAndMakeVisible (autoKeyBtn);
    autoKeyBtn.onClick = [this] { showAutoKeyMenu(); };
    addAndMakeVisible (analyzeBtn);
    analyzeBtn.setColour (juce::TextButton::buttonColourId, colours::accent2().withAlpha (0.25f));
    analyzeBtn.onClick = [this] { if (onAnalyze) onAnalyze(); };

    processor.addVocalystListener (this);
    refresh();
    startTimerHz (4);
}

HeaderBar::~HeaderBar()
{
    processor.removeVocalystListener (this);
}

void HeaderBar::setTab (int tab)
{
    currentTab = tab;
    tabPresets.setActive (tab == Presets);
    tabMix.setActive (tab == Mix);
    tabCommunity.setActive (tab == Community);
}

void HeaderBar::presetChanged() { refresh(); }
void HeaderBar::keyChanged() { refresh(); }
void HeaderBar::timerCallback() { refresh(); }

void HeaderBar::refresh()
{
    presetName.setText (processor.getCurrentPresetName(), juce::dontSendNotification);
    const auto genre = processor.getUiProperty ("presetGenre", "").toString();
    const bool adlib = (bool) processor.getUiProperty ("presetAdlib", false);
    juce::String meta = genre.isNotEmpty() ? genre + (adlib ? "  |  Ad-lib" : "  |  Main Vocal") : "User preset";
    const int idx = processor.getCurrentFactoryIndex();
    if (idx >= 0) meta += "  |  " + juce::String (idx + 1) + "/" + juce::String ((int) presets::factory().size());
    presetMeta.setText (meta, juce::dontSendNotification);
    autoKeyBtn.setColour (juce::TextButton::buttonColourId, processor.isAutoKeyLinked() ? colours::ok().withAlpha (0.35f) : colours::panelLight());
    autoKeyBtn.setButtonText (processor.isAutoKeyLinked() ? "AUTO KEY *" : "AUTO KEY");
}

void HeaderBar::showAutoKeyMenu()
{
    juce::PopupMenu m;
    auto shared = sharedkey::read();
    juce::String sharedText = "Pull key from Vocalyst Auto Key";
    if (shared.valid)
        sharedText += " (" + scales::keyName (shared.key, shared.scale) + ", " + juce::String ((int) (shared.confidence * 100.f)) + "%)";
    m.addSectionHeader ("Auto Key");
    m.addItem (1, sharedText, shared.valid);
    m.addItem (2, "Link: follow Vocalyst Auto Key automatically", true, processor.isAutoKeyLinked());
    m.addSeparator();
    auto est = processor.inputKeyDetector().estimate();
    juce::String detText = "Detect key from this vocal";
    if (est.valid) detText += " (" + scales::keyName (est.key, est.scale) + ", " + juce::String ((int) (est.confidence * 100.f)) + "%)";
    else detText += " (listening... play some audio)";
    m.addItem (3, detText, est.valid);
    m.addItem (4, "Reset vocal key detection");
    m.addSeparator();
    m.addItem (5, "Scale: Chromatic (no key)");
    juce::Component::SafePointer<HeaderBar> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (autoKeyBtn), [safe] (int r)
    {
        if (safe == nullptr) return;
        auto* self = safe.getComponent();
        auto& proc = self->processor;
        switch (r)
        {
            case 1: proc.applySharedKey(); break;
            case 2: proc.setAutoKeyLinked (! proc.isAutoKeyLinked()); if (proc.isAutoKeyLinked()) proc.applySharedKey(); break;
            case 3: { auto e = proc.inputKeyDetector().estimate(); if (e.valid) proc.setKey (e.key, e.scale); break; }
            case 4: proc.inputKeyDetector().reset(); break;
            case 5: proc.setKey (proc.getKey(), scales::Chromatic); break;
            default: break;
        }
        self->refresh();
    });
}

void HeaderBar::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel());
    g.setColour (colours::outline());
    g.drawLine (0.f, (float) getHeight() - 0.5f, (float) getWidth(), (float) getHeight() - 0.5f);

    // logo
    auto logo = getLocalBounds().removeFromLeft (168).reduced (14, 10);
    g.setColour (colours::accent());
    g.fillRoundedRectangle (logo.removeFromLeft (30).toFloat().reduced (2.f), 7.f);
    g.setColour (colours::text());
    g.setFont (font (19.f, true));
    g.drawText ("VOCALYST", logo.withTrimmedLeft (6), juce::Justification::centredLeft);
}

void HeaderBar::resized()
{
    auto r = getLocalBounds().reduced (10, 10);
    r.removeFromLeft (158);

    // preset navigation
    prevBtn.setBounds (r.removeFromLeft (30).reduced (0, 6));
    r.removeFromLeft (4);
    auto nameArea = r.removeFromLeft (200);
    presetName.setBounds (nameArea.removeFromTop (nameArea.getHeight() / 2 + 2));
    presetMeta.setBounds (nameArea);
    r.removeFromLeft (4);
    nextBtn.setBounds (r.removeFromLeft (30).reduced (0, 6));

    // right side: analyze, auto key, key boxes
    analyzeBtn.setBounds (r.removeFromRight (80).reduced (0, 6));
    r.removeFromRight (8);
    autoKeyBtn.setBounds (r.removeFromRight (88).reduced (0, 6));
    r.removeFromRight (8);
    scaleBox.setBounds (r.removeFromRight (110).reduced (0, 6));
    r.removeFromRight (4);
    keyBox.setBounds (r.removeFromRight (56).reduced (0, 6));
    keyLabel.setBounds (r.removeFromRight (34).reduced (0, 6));
    r.removeFromRight (10);

    // tabs centred in the remaining space
    auto tabs = r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 12, 300), r.getHeight()).reduced (0, 6);
    const int w = tabs.getWidth() / 3;
    tabPresets.setBounds (tabs.removeFromLeft (w).reduced (2, 0));
    tabMix.setBounds (tabs.removeFromLeft (w).reduced (2, 0));
    tabCommunity.setBounds (tabs.reduced (2, 0));
}

} // namespace vocalyst::ui
