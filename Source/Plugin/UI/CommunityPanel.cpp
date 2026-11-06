#include "CommunityPanel.h"

namespace muze::ui
{

CommunityPanel::CommunityPanel (MuzeProcessor& p) : processor (p)
{
    list.setModel (&model);
    list.setRowHeight (48);
    addAndMakeVisible (list);
    for (auto* b : { &loadBtn, &shareBtn, &importBtn, &folderBtn, &refreshBtn }) addAndMakeVisible (b);
    shareBtn.setColour (juce::TextButton::buttonColourId, colours::accent().withAlpha (0.3f));
    loadBtn.onClick = [this] { load (list.getSelectedRow()); };
    shareBtn.onClick = [this] { shareCurrent(); };
    importBtn.onClick = [this] { importFile(); };
    folderBtn.onClick = [] { sharedkey::communityDirectory().revealToUser(); };
    refreshBtn.onClick = [this] { refresh(); };
    heading.setFont (font (11.f, true));
    heading.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (heading);
    info.setFont (font (12.f));
    info.setColour (juce::Label::textColourId, colours::textDim());
    info.setJustificationType (juce::Justification::centredLeft);
    info.setText ("Community presets are .muzepreset files in " + sharedkey::communityDirectory().getFullPathName()
                  + ". Share yours, or drop files from other Muze users there and press Refresh.", juce::dontSendNotification);
    addAndMakeVisible (info);
    refresh();
}

void CommunityPanel::refresh()
{
    items.clear();
    auto files = sharedkey::communityDirectory().findChildFiles (juce::File::findFiles, false, "*" + presets::fileExtension());
    files.sort();
    for (auto& f : files)
    {
        bool ok = false;
        auto pr = Preset::fromJsonString (f.loadFileAsString(), &ok);
        if (ok) items.push_back ({ pr, f });
    }
    heading.setText ("COMMUNITY  -  " + juce::String ((int) items.size()) + " shared preset" + (items.size() == 1 ? "" : "s"), juce::dontSendNotification);
    list.updateContent();
    list.repaint();
}

void CommunityPanel::load (int row)
{
    if (row < 0 || row >= (int) items.size()) return;
    processor.loadPreset (items[(size_t) row].preset);
}

void CommunityPanel::shareCurrent()
{
    auto* w = new juce::AlertWindow ("Share preset", "Export the current chain to the community folder.", juce::MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", processor.getCurrentPresetName().replace (" *", ""), "Name");
    w->addTextEditor ("author", juce::SystemStats::getFullUserName(), "Author");
    w->addTextEditor ("genre", processor.getUiProperty ("presetGenre", "Custom").toString(), "Genre");
    w->addTextEditor ("desc", "", "Description");
    w->addButton ("Share", 1, juce::KeyPress (juce::KeyPress::returnKey));
    w->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    juce::Component::SafePointer<CommunityPanel> safe (this);
    w->enterModalState (true, juce::ModalCallbackFunction::create ([safe, w] (int result)
    {
        if (result != 1 || safe == nullptr) return;
        auto* self = safe.getComponent();
        auto& proc = self->processor;
        const auto name = w->getTextEditorContents ("name").trim();
        if (name.isEmpty()) return;
        auto preset = proc.captureCurrentPreset (name, w->getTextEditorContents ("genre").trim(), w->getTextEditorContents ("desc").trim(),
                                                      w->getTextEditorContents ("author").trim(), (bool) proc.getUiProperty ("presetAdlib", false));
        auto file = sharedkey::communityDirectory().getChildFile (juce::File::createLegalFileName (name) + presets::fileExtension());
        file.replaceWithText (preset.toJsonString());
        self->refresh();
    }), true);
}

void CommunityPanel::importFile()
{
    chooser = std::make_unique<juce::FileChooser> ("Import Muze preset", juce::File(), "*" + presets::fileExtension() + ";*.json");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectMultipleItems,
        [this] (const juce::FileChooser& fc)
        {
            for (auto& f : fc.getResults())
            {
                bool ok = false;
                Preset::fromJsonString (f.loadFileAsString(), &ok);
                if (ok) f.copyFileTo (sharedkey::communityDirectory().getChildFile (f.getFileNameWithoutExtension() + presets::fileExtension()));
            }
            refresh();
        });
}

void CommunityPanel::Model::paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected)
{
    if (row < 0 || row >= (int) owner.items.size()) return;
    auto& it = owner.items[(size_t) row];
    auto r = juce::Rectangle<int> (0, 0, w, h).reduced (4, 2);
    g.setColour (selected ? colours::accent().withAlpha (0.3f) : colours::panelLight());
    g.fillRoundedRectangle (r.toFloat(), 6.f);
    auto inner = r.reduced (12, 4);
    g.setColour (colours::text());
    g.setFont (font (13.5f, true));
    g.drawText (it.preset.name, inner.removeFromTop (inner.getHeight() / 2 + 2), juce::Justification::centredLeft);
    g.setColour (colours::textDim());
    g.setFont (font (11.5f));
    juce::String sub = "by " + (it.preset.author.isNotEmpty() ? it.preset.author : "unknown");
    if (it.preset.genre.isNotEmpty()) sub += "  -  " + it.preset.genre;
    if (it.preset.description.isNotEmpty()) sub += "  -  " + it.preset.description;
    g.drawText (sub, inner, juce::Justification::centredLeft);
    g.setFont (font (10.5f));
    g.drawText (it.file.getLastModificationTime().toString (true, false), r.removeFromRight (110).reduced (8, 0), juce::Justification::centredRight);
}

void CommunityPanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::bg());
    if (items.empty())
    {
        g.setColour (colours::textDim());
        g.setFont (font (14.f));
        g.drawFittedText ("No shared presets yet.\nPress SHARE CURRENT PRESET to publish your chain to the community folder,\nor IMPORT FILE to add a .muzepreset someone sent you.",
                          list.getBounds(), juce::Justification::centred, 4);
    }
}

void CommunityPanel::resized()
{
    auto r = getLocalBounds().reduced (14, 10);
    auto top = r.removeFromTop (30);
    heading.setBounds (top.removeFromLeft (280));
    refreshBtn.setBounds (top.removeFromRight (90)); top.removeFromRight (6);
    folderBtn.setBounds (top.removeFromRight (110)); top.removeFromRight (6);
    importBtn.setBounds (top.removeFromRight (110)); top.removeFromRight (6);
    shareBtn.setBounds (top.removeFromRight (190)); top.removeFromRight (6);
    loadBtn.setBounds (top.removeFromRight (70));
    r.removeFromTop (8);
    info.setBounds (r.removeFromBottom (24));
    r.removeFromBottom (4);
    list.setBounds (r);
}

} // namespace muze::ui
