#include "PresetBrowserPanel.h"

namespace vocalyst::ui
{

PresetBrowserPanel::PresetBrowserPanel (VocalystProcessor& p) : processor (p)
{
    for (auto* b : { &catMain, &catAdlib, &catUser }) addAndMakeVisible (b);
    catMain.onClick  = [this] { setCategory (MainVocal); };
    catAdlib.onClick = [this] { setCategory (Adlib); };
    catUser.onClick  = [this] { setCategory (User); };

    addAndMakeVisible (saveBtn); addAndMakeVisible (deleteBtn); addAndMakeVisible (folderBtn);
    saveBtn.onClick = [this] { saveUserPreset(); };
    deleteBtn.onClick = [this] { deleteUserPreset(); };
    folderBtn.onClick = [] { sharedkey::presetsDirectory().revealToUser(); };

    title.setFont (font (13.f, true));
    title.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (title);
    description.setFont (font (12.f));
    description.setColour (juce::Label::textColourId, colours::textDim());
    description.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (description);

    genreBox.setModel (&genreModel);
    genreBox.setRowHeight (30);
    presetBox.setModel (&presetModel);
    presetBox.setRowHeight (44);
    addAndMakeVisible (genreBox);
    addAndMakeVisible (presetBox);

    refreshUserPresets();
    const bool adlib = (bool) processor.getUiProperty ("presetAdlib", false);
    setCategory (adlib ? Adlib : MainVocal);
    processor.addVocalystListener (this);
}

PresetBrowserPanel::~PresetBrowserPanel() { processor.removeVocalystListener (this); }

void PresetBrowserPanel::refreshUserPresets()
{
    userPresets.clear(); userFiles.clear();
    for (auto& f : sharedkey::presetsDirectory().findChildFiles (juce::File::findFiles, false, "*" + presets::fileExtension()))
    {
        bool ok = false;
        auto pr = Preset::fromJsonString (f.loadFileAsString(), &ok);
        if (ok) { userPresets.push_back (pr); userFiles.push_back (f); }
    }
    if (category == User) { rebuildGenres(); }
}

void PresetBrowserPanel::setCategory (int c)
{
    category = c;
    catMain.setActive (c == MainVocal); catAdlib.setActive (c == Adlib); catUser.setActive (c == User);
    deleteBtn.setVisible (c == User);
    rebuildGenres();
}

void PresetBrowserPanel::rebuildGenres()
{
    genreList.clear();
    genreList.add ("All");
    if (category == User)
    {
        for (auto& p : userPresets) if (p.genre.isNotEmpty() && ! genreList.contains (p.genre)) genreList.add (p.genre);
    }
    else
        genreList.addArray (presets::genres (category == Adlib));
    genreBox.updateContent();
    genreBox.selectRow (0, false, true);
    rebuildVisible();
}

void PresetBrowserPanel::rebuildVisible()
{
    visible.clear(); visibleUserIndex.clear();
    const int row = genreBox.getSelectedRow();
    const juce::String genre = row > 0 && row < genreList.size() ? genreList[row] : juce::String();
    if (category == User)
    {
        for (int i = 0; i < (int) userPresets.size(); ++i)
            if (genre.isEmpty() || userPresets[(size_t) i].genre == genre) { visible.push_back (&userPresets[(size_t) i]); visibleUserIndex.push_back (i); }
    }
    else
    {
        for (auto& p : presets::factory())
            if (p.adlib == (category == Adlib) && (genre.isEmpty() || p.genre == genre)) { visible.push_back (&p); visibleUserIndex.push_back (-1); }
    }
    title.setText (juce::String ((int) visible.size()) + (category == Adlib ? " AD-LIB PRESETS" : category == User ? " USER PRESETS" : " MAIN VOCAL PRESETS"), juce::dontSendNotification);
    presetBox.updateContent();
    presetChanged();
}

void PresetBrowserPanel::presetChanged()
{
    const auto name = processor.getCurrentPresetName();
    int sel = -1;
    for (int i = 0; i < (int) visible.size(); ++i) if (visible[(size_t) i]->name == name) sel = i;
    if (sel >= 0) presetBox.selectRow (sel, true, true); else presetBox.deselectAllRows();
    if (sel >= 0) description.setText (visible[(size_t) sel]->description, juce::dontSendNotification);
    else description.setText (category == User ? "Your saved presets live in Documents/Vocalyst/Presets." : "Click a preset to load it. Every preset is a full chain you can edit in the MIX tab.", juce::dontSendNotification);
    presetBox.repaint();
}

void PresetBrowserPanel::loadVisible (int row)
{
    if (row < 0 || row >= (int) visible.size()) return;
    processor.loadPreset (*visible[(size_t) row]);
}

void PresetBrowserPanel::saveUserPreset()
{
    auto* w = new juce::AlertWindow ("Save user preset", "Give your chain a name.", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", processor.getCurrentPresetName().replace (" *", ""), "Name");
    w->addTextEditor ("genre", processor.getUiProperty ("presetGenre", "Custom").toString(), "Genre");
    w->addTextEditor ("desc", "", "Description");
    w->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<PresetBrowserPanel> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        auto* self = safe.getComponent();
        auto& proc = self->processor;
        const auto name = w->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        auto preset = proc.captureCurrentPreset (name, w->getTextEditorContents ("genre").trim(), w->getTextEditorContents ("desc").trim(),
                                                      juce::SystemStats::getFullUserName(), (bool) proc.getUiProperty ("presetAdlib", false));
        auto file = sharedkey::presetsDirectory().getChildFile (juce::File::createLegalFileName (name) + presets::fileExtension());
        file.replaceWithText (preset.toJsonString());
        proc.setUiProperty ("presetName", name);
        self->refreshUserPresets();
        self->setCategory (User);
    }), true);
}

void PresetBrowserPanel::deleteUserPreset()
{
    const int row = presetBox.getSelectedRow();
    if (category != User || row < 0 || row >= (int) visibleUserIndex.size()) return;
    const int idx = visibleUserIndex[(size_t) row];
    if (idx < 0 || idx >= (int) userFiles.size()) return;
    userFiles[(size_t) idx].deleteFile();
    refreshUserPresets();
    rebuildGenres();
}

void PresetBrowserPanel::GenreModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= owner.genreList.size()) return;
    auto r = juce::Rectangle<int> (0, 0, w, h).reduced (4, 2);
    if (selected) { g.setColour (colours::accent().withAlpha (0.25f)); g.fillRoundedRectangle (r.toFloat(), 6.f); }
    g.setColour (selected ? colours::text() : colours::textDim());
    g.setFont (font (13.f, selected));
    g.drawText (owner.genreList[row], r.reduced (10, 0), juce::Justification::centredLeft);
}

void PresetBrowserPanel::PresetModel::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= (int) owner.visible.size()) return;
    auto& p = *owner.visible[(size_t) row];
    auto r = juce::Rectangle<int> (0, 0, w, h).reduced (4, 2);
    g.setColour (selected ? colours::accent().withAlpha (0.3f) : colours::panelLight());
    g.fillRoundedRectangle (r.toFloat(), 6.f);
    if (selected) { g.setColour (colours::accent()); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 6.f, 1.f); }
    auto inner = r.reduced (12, 4);
    g.setColour (colours::text());
    g.setFont (font (13.5f, true));
    g.drawText (p.name, inner.removeFromTop (inner.getHeight() / 2 + 2), juce::Justification::centredLeft);
    g.setColour (colours::textDim());
    g.setFont (font (11.5f));
    juce::String sub = p.description;
    if (p.author.isNotEmpty() && p.author != "Vocalyst") sub = "by " + p.author + "  -  " + sub;
    g.drawText (sub, inner, juce::Justification::centredLeft);
    // chain length chip
    g.setColour (colours::outline());
    auto chip = r.removeFromRight (64).reduced (6, 10);
    g.fillRoundedRectangle (chip.toFloat(), 4.f);
    g.setColour (colours::textDim());
    g.setFont (font (10.5f, true));
    g.drawText (juce::String ((int) p.chain.size()) + " FX", chip, juce::Justification::centred);
}

void PresetBrowserPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::bg());
}

void PresetBrowserPanel::resized()
{
    auto r = getLocalBounds().reduced (14, 10);
    auto top = r.removeFromTop (30);
    auto cats = top.removeFromLeft (330);
    const int cw = cats.getWidth() / 3;
    catMain.setBounds (cats.removeFromLeft (cw).reduced (2, 0));
    catAdlib.setBounds (cats.removeFromLeft (cw).reduced (2, 0));
    catUser.setBounds (cats.reduced (2, 0));
    folderBtn.setBounds (top.removeFromRight (110));
    top.removeFromRight (6);
    deleteBtn.setBounds (top.removeFromRight (80));
    top.removeFromRight (6);
    saveBtn.setBounds (top.removeFromRight (170));
    r.removeFromTop (8);
    auto bottom = r.removeFromBottom (22);
    description.setBounds (bottom);
    r.removeFromBottom (4);
    auto left = r.removeFromLeft (150);
    genreBox.setBounds (left);
    r.removeFromLeft (10);
    title.setBounds (r.removeFromTop (20));
    presetBox.setBounds (r);
}

} // namespace vocalyst::ui
