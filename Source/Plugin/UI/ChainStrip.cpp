#include "ChainStrip.h"

namespace muze::ui
{

class ChainStrip::Tile : public juce::Component, public juce::SettableTooltipClient
{
public:
    Tile (ChainStrip& o, MuzeProcessor& p, ModuleId mid) : owner (o), processor (p), id (mid)
    {
        auto& spec = moduleSpec (id);
        power.setClickingTogglesState (true);
        addAndMakeVisible (power);
        if (auto* param = processor.apvts.getParameter (enableId (id)))
            powerAtt = std::make_unique<juce::ButtonParameterAttachment> (*param, power);
        power.onStateChange = [this] { repaint(); };
        remove.setButtonText ("x");
        remove.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        remove.onClick = [this] { processor.removeModule (id); };
        addAndMakeVisible (remove);
        setTooltip (spec.description);
        setInterceptsMouseClicks (true, true);
    }

    ModuleId moduleId() const { return id; }
    void setSelected (bool s) { sel = s; repaint(); }

    void paint (juce::Graphics& g) override
    {
        auto& spec = moduleSpec (id);
        auto r = getLocalBounds().toFloat().reduced (1.f);
        const auto cat = categoryColour (spec.category);
        const bool on = power.getToggleState();
        g.setColour (sel ? colours::panelLight().brighter (0.08f) : colours::panelLight());
        g.fillRoundedRectangle (r, 8.f);
        g.setColour (sel ? colours::accent() : (dragged ? colours::accent2() : colours::outline()));
        g.drawRoundedRectangle (r, 8.f, sel || dragged ? 1.5f : 1.f);
        // category stripe
        g.setColour (on ? cat : cat.withAlpha (0.3f));
        g.fillRoundedRectangle (r.getX() + 10.f, r.getBottom() - 8.f, r.getWidth() - 20.f, 3.f, 1.5f);
        // name
        g.setColour (on ? colours::text() : colours::textDim());
        g.setFont (font (13.f, true));
        g.drawFittedText (spec.name, getLocalBounds().reduced (8, 0).withTrimmedTop (26).withTrimmedBottom (14), juce::Justification::centred, 2);
        // drag handle dots
        g.setColour (colours::outline().brighter (0.3f));
        for (int i = 0; i < 3; ++i) g.fillEllipse ((float) getWidth() * 0.5f - 7.f + (float) i * 6.f, 10.f, 3.f, 3.f);
    }

    void resized() override
    {
        power.setBounds (6, 5, 18, 18);
        remove.setBounds (getWidth() - 24, 4, 20, 20);
    }

    void mouseDown (const juce::MouseEvent&) override { owner.setSelected (id); if (owner.onSelect) owner.onSelect (id); }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (! dragged && e.getDistanceFromDragStart() > 6) { dragged = true; owner.tileDragStart (this); }
        if (dragged) owner.tileDragged (this, e.getEventRelativeTo (getParentComponent()).x);
    }
    void mouseUp (const juce::MouseEvent&) override { if (dragged) { dragged = false; owner.tileDragEnd (this); } }

    bool dragged = false;
private:
    ChainStrip& owner;
    MuzeProcessor& processor;
    ModuleId id;
    juce::ToggleButton power;
    juce::TextButton remove;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAtt;
    bool sel = false;
};

//==============================================================================
ChainStrip::ChainStrip (MuzeProcessor& p) : processor (p)
{
    viewport.setViewedComponent (&container, false);
    viewport.setScrollBarsShown (false, true, false, true);
    addAndMakeVisible (viewport);
    addBtn.setColour (juce::TextButton::buttonColourId, colours::accent().withAlpha (0.25f));
    addBtn.onClick = [this] { showAddMenu(); };
    container.addAndMakeVisible (addBtn);
    processor.addMuzeListener (this);
    rebuild();
}

ChainStrip::~ChainStrip() { cancelPendingUpdate(); processor.removeMuzeListener (this); }

void ChainStrip::chainChanged()
{
    if (dragging == nullptr) triggerAsyncUpdate();
}

void ChainStrip::handleAsyncUpdate()
{
    if (dragging == nullptr) rebuild();
}

void ChainStrip::setSelected (ModuleId id)
{
    selected = id;
    for (auto& t : tiles) t->setSelected (t->moduleId() == id);
}

void ChainStrip::rebuild()
{
    tiles.clear();
    auto order = processor.getChainOrder();
    for (auto id : order)
    {
        auto t = std::make_unique<Tile> (*this, processor, id);
        container.addAndMakeVisible (*t);
        tiles.push_back (std::move (t));
    }
    if (std::find (order.begin(), order.end(), selected) == order.end())
    {
        selected = order.empty() ? ModuleId::Count : order.front();
        if (onSelect) onSelect (selected);
    }
    setSelected (selected);
    layoutTiles (false);
}

void ChainStrip::layoutTiles (bool keepDragged)
{
    const int n = (int) tiles.size();
    const int totalW = n * (tileW + gap) + 90;
    container.setSize (juce::jmax (totalW, viewport.getWidth()), tileH);
    for (int i = 0; i < n; ++i)
    {
        auto* t = tiles[(size_t) i].get();
        if (keepDragged && t == dragging) continue;
        t->setBounds (i * (tileW + gap), 0, tileW, tileH);
    }
    addBtn.setBounds (n * (tileW + gap), (tileH - 34) / 2, 80, 34);
}

void ChainStrip::tileDragStart (Tile* t)
{
    dragging = t;
    dragIndex = -1;
    for (int i = 0; i < (int) tiles.size(); ++i) if (tiles[(size_t) i].get() == t) dragIndex = i;
    t->toFront (false);
}

void ChainStrip::tileDragged (Tile* t, int x)
{
    if (dragging != t || dragIndex < 0) return;
    t->setTopLeftPosition (juce::jlimit (0, container.getWidth() - tileW, x - tileW / 2), 0);
    const int target = juce::jlimit (0, (int) tiles.size() - 1, (x) / (tileW + gap));
    if (target != dragIndex)
    {
        auto moved = std::move (tiles[(size_t) dragIndex]);
        tiles.erase (tiles.begin() + dragIndex);
        tiles.insert (tiles.begin() + target, std::move (moved));
        dragIndex = target;
        layoutTiles (true);
    }
}

void ChainStrip::tileDragEnd (Tile* t)
{
    if (dragging != t) return;
    dragging = nullptr;
    std::vector<ModuleId> order;
    for (auto& tile : tiles) order.push_back (tile->moduleId());
    processor.setChainOrder (order);   // triggers chainChanged -> rebuild
}

void ChainStrip::showAddMenu()
{
    juce::PopupMenu menu;
    for (int c = 0; c < (int) ModuleCategory::NumCategories; ++c)
    {
        juce::PopupMenu sub;
        bool any = false;
        for (auto& m : allModules())
        {
            if ((int) m.category != c) continue;
            const bool inChain = processor.isInChain (m.id);
            sub.addItem ((int) m.id + 1, juce::String (m.name) + "  -  " + m.description, ! inChain, inChain);
            any = true;
        }
        if (any) menu.addSubMenu (categoryName ((ModuleCategory) c), sub);
    }
    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (addBtn), [this] (int r)
    {
        if (r <= 0) return;
        const auto id = (ModuleId) (r - 1);
        // insert before a trailing limiter so it stays last
        int index = -1;
        auto order = processor.getChainOrder();
        if (! order.empty() && order.back() == ModuleId::Limiter && id != ModuleId::Limiter) index = (int) order.size() - 1;
        processor.addModule (id, index);
        setSelected (id);
        if (onSelect) onSelect (id);
    });
}

void ChainStrip::paint (juce::Graphics& g)
{
    g.setColour (colours::panel());
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.f, 1.f);
    g.setColour (colours::textDim());
    g.setFont (font (10.5f, true));
    g.drawText ("SIGNAL CHAIN  -  drag tiles to re-order, click to edit", 14, 4, 400, 14, juce::Justification::centredLeft);
}

void ChainStrip::resized()
{
    viewport.setBounds (getLocalBounds().reduced (10, 0).withTrimmedTop (20).withTrimmedBottom (4));
    layoutTiles (false);
}

} // namespace muze::ui
