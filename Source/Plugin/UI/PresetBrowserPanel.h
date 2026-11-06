#pragma once
#include "VocalystLookAndFeel.h"
#include "../VocalystProcessor.h"

namespace vocalyst::ui
{

class PresetBrowserPanel : public juce::Component,
                           private VocalystProcessor::Listener
{
public:
    explicit PresetBrowserPanel (VocalystProcessor&);
    ~PresetBrowserPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void refreshUserPresets();

private:
    enum Category { MainVocal = 0, Adlib = 1, User = 2 };

    struct GenreModel : juce::ListBoxModel
    {
        PresetBrowserPanel& owner;
        explicit GenreModel (PresetBrowserPanel& o) : owner (o) {}
        int getNumRows() override { return owner.genreList.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override;
        void selectedRowsChanged (int) override { owner.rebuildVisible(); }
    };
    struct PresetModel : juce::ListBoxModel
    {
        PresetBrowserPanel& owner;
        explicit PresetModel (PresetBrowserPanel& o) : owner (o) {}
        int getNumRows() override { return (int) owner.visible.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override;
        void listBoxItemClicked (int row, const juce::MouseEvent&) override { owner.loadVisible (row); }
        void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { owner.loadVisible (row); }
    };

    void presetChanged() override;
    void setCategory (int c);
    void rebuildGenres();
    void rebuildVisible();
    void loadVisible (int row);
    void saveUserPreset();
    void deleteUserPreset();

    VocalystProcessor& processor;
    PillButton catMain { "MAIN VOCAL" }, catAdlib { "AD-LIBS" }, catUser { "USER" };
    juce::TextButton saveBtn { "SAVE AS USER PRESET" }, deleteBtn { "DELETE" }, folderBtn { "OPEN FOLDER" };
    juce::Label title, description;
    GenreModel genreModel { *this };
    PresetModel presetModel { *this };
    juce::ListBox genreBox, presetBox;
    juce::StringArray genreList;
    std::vector<Preset> userPresets;
    std::vector<juce::File> userFiles;
    std::vector<const Preset*> visible;
    std::vector<int> visibleUserIndex;    // index into userPresets for User category (-1 otherwise)
    int category = MainVocal;
};

} // namespace vocalyst::ui
