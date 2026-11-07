#include "ModulePanel.h"

namespace vocalyst::ui
{

ModulePanel::ModulePanel (VocalystProcessor& p) : processor (p)
{
    title.setFont (font (18.f, true));
    addAndMakeVisible (title);
    description.setFont (font (12.f));
    description.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (description);
    category.setFont (font (10.5f, true));
    category.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (category);
    power.setClickingTogglesState (true);
    addAndMakeVisible (power);
    removeBtn.onClick = [this] { if (current != ModuleId::Count) processor.removeModule (current); };
    addAndMakeVisible (removeBtn);
    presetBox.setTextWhenNothingSelected ("PRESET");
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedId() - 1;
        if (idx >= 0 && current != ModuleId::Count) processor.applyModulePreset (current, idx);
        presetBox.setSelectedId (0, juce::dontSendNotification);
    };
    addAndMakeVisible (presetBox);
    setModule (ModuleId::Count);
}

void ModulePanel::setModule (ModuleId id)
{
    current = id;
    knobs.clear(); combos.clear(); comboLabels.clear(); comboAtts.clear(); powerAtt.reset();
    const bool valid = id != ModuleId::Count;
    power.setVisible (valid); removeBtn.setVisible (valid); category.setVisible (valid); presetBox.setVisible (valid);
    presetBox.clear (juce::dontSendNotification);
    if (! valid)
    {
        title.setText ("No module selected", juce::dontSendNotification);
        description.setText ("Click a tile in the chain above, or press + ADD to insert one of the 27 effects.", juce::dontSendNotification);
        resized(); repaint();
        return;
    }
    auto& spec = moduleSpec (id);
    title.setText (spec.name, juce::dontSendNotification);
    description.setText (spec.description, juce::dontSendNotification);
    category.setText (juce::String (categoryName (spec.category)).toUpperCase(), juce::dontSendNotification);
    category.setColour (juce::Label::textColourId, categoryColour (spec.category));
    for (int i = 0; i < (int) spec.presets.size(); ++i) presetBox.addItem (spec.presets[(size_t) i].name, i + 1);
    presetBox.setEnabled (! spec.presets.empty());
    if (auto* param = processor.apvts.getParameter (enableId (id)))
        powerAtt = std::make_unique<juce::ButtonParameterAttachment> (*param, power);
    for (auto& ps : spec.params)
    {
        auto* param = processor.apvts.getParameter (paramId (id, ps.key));
        if (ps.choices != nullptr)
        {
            auto box = std::make_unique<juce::ComboBox>();
            juce::StringArray choices; choices.addTokens (ps.choices, ",", "");
            for (int i = 0; i < choices.size(); ++i) box->addItem (choices[i], i + 1);
            addAndMakeVisible (*box);
            if (param != nullptr) comboAtts.push_back (std::make_unique<juce::ComboBoxParameterAttachment> (*param, *box));
            auto lbl = std::make_unique<juce::Label>();
            lbl->setText (ps.name, juce::dontSendNotification);
            lbl->setFont (font (12.f, true));
            lbl->setJustificationType (juce::Justification::centred);
            lbl->setColour (juce::Label::textColourId, colours::textDim());
            addAndMakeVisible (*lbl);
            combos.push_back (std::move (box));
            comboLabels.push_back (std::move (lbl));
        }
        else
        {
            auto k = std::make_unique<Knob> (ps.name, param, categoryColour (spec.category));
            addAndMakeVisible (*k);
            knobs.push_back (std::move (k));
        }
    }
    resized(); repaint();
}

void ModulePanel::paint (juce::Graphics& g)
{
    g.setColour (colours::panel());
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.f, 1.f);
}

void ModulePanel::resized()
{
    auto r = getLocalBounds().reduced (16, 12);
    auto head = r.removeFromTop (44);
    if (power.isVisible()) { power.setBounds (head.removeFromLeft (26).withSizeKeepingCentre (22, 22)); head.removeFromLeft (8); }
    removeBtn.setBounds (head.removeFromRight (160).reduced (0, 8));
    head.removeFromRight (8);
    presetBox.setBounds (head.removeFromRight (150).reduced (0, 8));
    head.removeFromRight (8);
    category.setBounds (head.removeFromRight (90).reduced (0, 12));
    title.setBounds (head.removeFromTop (24));
    description.setBounds (head);
    r.removeFromTop (6);

    const int total = (int) knobs.size() + (int) combos.size();
    if (total == 0) return;
    const int perRow = juce::jmin (total, juce::jmax (1, r.getWidth() / 96));
    const int rows = (total + perRow - 1) / perRow;
    const int cellW = r.getWidth() / perRow;
    const int cellH = juce::jmin (150, r.getHeight() / rows);
    int idx = 0;
    auto cellFor = [&] (int i) { return juce::Rectangle<int> (r.getX() + (i % perRow) * cellW, r.getY() + (i / perRow) * cellH, cellW, cellH); };
    for (size_t c = 0; c < combos.size(); ++c)
    {
        auto cell = cellFor (idx++).reduced (6, 8);
        comboLabels[c]->setBounds (cell.removeFromTop (16));
        combos[c]->setBounds (cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 110), 26));
    }
    for (auto& k : knobs)
        k->setBounds (cellFor (idx++).reduced (4, 2));
}

} // namespace vocalyst::ui
