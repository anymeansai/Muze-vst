#pragma once
#include "MuzeLookAndFeel.h"
#include "../MuzeProcessor.h"

namespace muze::ui
{

// Shared presets: a local library folder (Documents/Muze/Community) that users exchange
// as .muzepreset files. No online backend is involved.
class CommunityPanel : public juce::Component
{
public:
    explicit CommunityPanel (MuzeProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();
private:
    struct Model : juce::ListBoxModel
    {
        CommunityPanel& owner;
        explicit Model (CommunityPanel& o) : owner (o) {}
        int getNumRows() override { return (int) owner.items.size(); }
        void paintListBoxItem (int row, juce::Graphics& g, int w, int h, bool selected) override;
        void listBoxItemDoubleClicked (int row, const juce::MouseEvent&) override { owner.load (row); }
    };
    struct Item { Preset preset; juce::File file; };

    void load (int row);
    void shareCurrent();
    void importFile();

    MuzeProcessor& processor;
    Model model { *this };
    juce::ListBox list;
    std::vector<Item> items;
    juce::TextButton loadBtn { "LOAD" }, shareBtn { "SHARE CURRENT PRESET" }, importBtn { "IMPORT FILE" },
                     folderBtn { "OPEN FOLDER" }, refreshBtn { "REFRESH" };
    juce::Label heading, info;
    std::unique_ptr<juce::FileChooser> chooser;
};

} // namespace muze::ui
