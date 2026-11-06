#include "MacroPanel.h"

namespace muze::ui
{

MacroPanel::MacroPanel (MuzeProcessor& p) : processor (p)
{
    struct M { const char* id; const char* name; const char* caption; juce::Colour c; };
    const M macros[] = {
        { ids::macroTune,  "TUNE",  "Pitch correction strength and speed",        categoryColour (ModuleCategory::Clean) },
        { ids::macroClean, "CLEAN", "Gate, de-esser and compression intensity",    categoryColour (ModuleCategory::Dynamics) },
        { ids::macroTone,  "TONE",  "Dark  <->  bright tilt",                      categoryColour (ModuleCategory::Tone) },
        { ids::macroDrive, "DRIVE", "Saturation, distortion and exciter",          categoryColour (ModuleCategory::Color) },
        { ids::macroWidth, "WIDTH", "Doubler, widener and chorus",                  categoryColour (ModuleCategory::Modulation) },
        { ids::macroSpace, "SPACE", "Reverb, delay and slap amount",               categoryColour (ModuleCategory::Space) },
    };
    for (auto& m : macros)
    {
        auto k = std::make_unique<Knob> (m.name, processor.apvts.getParameter (m.id), m.c);
        k->title.setFont (font (14.f, true));
        k->title.setColour (juce::Label::textColourId, colours::text());
        addAndMakeVisible (*k);
        knobs.push_back (std::move (k));
        captions.push_back (m.caption);
    }
    heading.setText ("SIMPLE MODE", juce::dontSendNotification);
    heading.setFont (font (11.f, true));
    heading.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (heading);
    sub.setText ("Six macros shape the loaded preset. 50 % = exactly as the preset was designed. Switch to ADVANCED for the full chain.", juce::dontSendNotification);
    sub.setFont (font (12.f));
    sub.setColour (juce::Label::textColourId, colours::textDim());
    sub.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (sub);
    processor.addListener (this);
}

MacroPanel::~MacroPanel() { processor.removeListener (this); }

void MacroPanel::paint (juce::Graphics& g)
{
    g.setColour (colours::panel());
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.f, 1.f);

    // captions under knobs
    g.setFont (font (11.f));
    g.setColour (colours::textDim());
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto b = knobs[i]->getBounds();
        g.drawFittedText (captions[i], b.getX(), b.getBottom() + 2, b.getWidth(), 30, juce::Justification::centredTop, 2);
    }

    // chain overview chips
    auto order = processor.getChainOrder();
    auto area = getLocalBounds().reduced (16, 10).removeFromBottom (52);
    g.setColour (colours::textDim());
    g.setFont (font (10.5f, true));
    g.drawText ("ACTIVE CHAIN", area.removeFromTop (14), juce::Justification::centredLeft);
    area.removeFromTop (4);
    int x = area.getX();
    for (auto id : order)
    {
        auto& spec = moduleSpec (id);
        const bool on = processor.apvts.getRawParameterValue (enableId (id))->load() > 0.5f;
        const int w = (int) juce::GlyphArrangement::getStringWidth (font (11.f, true), spec.name) + 22;
        if (x + w > area.getRight()) break;
        auto chip = juce::Rectangle<int> (x, area.getY(), w, 22);
        g.setColour (on ? categoryColour (spec.category).withAlpha (0.18f) : colours::panelLight());
        g.fillRoundedRectangle (chip.toFloat(), 11.f);
        g.setColour (on ? categoryColour (spec.category) : colours::outline());
        g.drawRoundedRectangle (chip.toFloat().reduced (0.5f), 11.f, 1.f);
        g.setColour (on ? colours::text() : colours::textDim());
        g.setFont (font (11.f, true));
        g.drawText (spec.name, chip, juce::Justification::centred);
        x += w + 6;
    }
}

void MacroPanel::resized()
{
    auto r = getLocalBounds().reduced (16, 10);
    heading.setBounds (r.removeFromTop (16));
    r.removeFromBottom (56);
    sub.setBounds (r.removeFromBottom (20));
    r.removeFromBottom (30); // captions
    auto knobArea = r.withSizeKeepingCentre (juce::jmin (r.getWidth(), 900), juce::jmin (r.getHeight(), 170));
    const int w = knobArea.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
        k->setBounds (knobArea.removeFromLeft (w).reduced (8, 0));
}

} // namespace muze::ui
