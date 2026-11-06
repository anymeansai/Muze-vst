#pragma once
#include "MuzeLookAndFeel.h"
#include "../MuzeProcessor.h"

namespace muze::ui
{

// Horizontal strip of module tiles: drag to re-order, power toggle, remove, "+ ADD".
class ChainStrip : public juce::Component,
                   private MuzeProcessor::Listener,
                   private juce::AsyncUpdater
{
public:
    explicit ChainStrip (MuzeProcessor&);
    ~ChainStrip() override;

    std::function<void (ModuleId)> onSelect;
    void setSelected (ModuleId id);
    ModuleId getSelected() const { return selected; }

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int tileW = 104, tileH = 78, gap = 8;

private:
    class Tile;
    void chainChanged() override;
    void handleAsyncUpdate() override;
    void rebuild();
    void layoutTiles (bool animateDragged);
    void showAddMenu();
    void tileDragStart (Tile*);
    void tileDragged (Tile*, int containerX);
    void tileDragEnd (Tile*);

    MuzeProcessor& processor;
    juce::Viewport viewport;
    juce::Component container;
    std::vector<std::unique_ptr<Tile>> tiles;
    juce::TextButton addBtn { "+ ADD" };
    ModuleId selected = ModuleId::Count;
    Tile* dragging = nullptr;
    int dragIndex = -1;
};

} // namespace muze::ui
