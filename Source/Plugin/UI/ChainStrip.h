#pragma once
#include "VocalystLookAndFeel.h"
#include "../VocalystProcessor.h"

namespace vocalyst::ui
{

// Horizontal strip of module tiles: drag to re-order, power toggle, remove, "+ ADD".
class ChainStrip : public juce::Component,
                   private VocalystProcessor::Listener,
                   private juce::AsyncUpdater
{
public:
    explicit ChainStrip (VocalystProcessor&);
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

    VocalystProcessor& processor;
    juce::Viewport viewport;
    juce::Component container;
    std::vector<std::unique_ptr<Tile>> tiles;
    juce::TextButton addBtn { "+ ADD" };
    ModuleId selected = ModuleId::Count;
    Tile* dragging = nullptr;
    int dragIndex = -1;
};

} // namespace vocalyst::ui
