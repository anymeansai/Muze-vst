#include "AnalyzePanel.h"

namespace muze::ui
{

AnalyzePanel::AnalyzePanel (MuzeProcessor& p) : processor (p)
{
    for (auto* b : { &applyBtn, &resetBtn, &closeBtn }) addAndMakeVisible (b);
    applyBtn.setColour (juce::TextButton::buttonColourId, colours::accent());
    applyBtn.onClick = [this] { processor.applySuggestions (suggestions); if (onClose) onClose(); };
    resetBtn.onClick = [this] { processor.analyzer().reset(); refresh(); };
    closeBtn.onClick = [this] { if (onClose) onClose(); };
    setVisible (false);
}

void AnalyzePanel::open()
{
    refresh();
    setVisible (true);
    toFront (true);
    startTimerHz (4);
}

void AnalyzePanel::timerCallback()
{
    if (! isVisible()) { stopTimer(); return; }
    refresh();
}

void AnalyzePanel::refresh()
{
    stats = processor.analyzer().snapshot();
    suggestions = VocalAnalyzer::suggest (stats);
    applyBtn.setEnabled (stats.valid && ! suggestions.empty());
    repaint();
}

void AnalyzePanel::paint (juce::Graphics& g)
{
    g.fillAll (colours::bg().withAlpha (0.82f));
    auto card = getLocalBounds().withSizeKeepingCentre (juce::jmin (getWidth() - 40, 680), juce::jmin (getHeight() - 30, 470));
    g.setColour (colours::panel());
    g.fillRoundedRectangle (card.toFloat(), 12.f);
    g.setColour (colours::accent());
    g.drawRoundedRectangle (card.toFloat().reduced (0.5f), 12.f, 1.f);

    auto r = card.reduced (24, 18);
    g.setColour (colours::text());
    g.setFont (font (20.f, true));
    g.drawText ("ANALYZE", r.removeFromTop (28), juce::Justification::centredLeft);
    g.setColour (colours::textDim());
    g.setFont (font (12.5f));
    g.drawText ("Play the vocal through Muze. The analyzer listens to the raw input and proposes a starting mix.", r.removeFromTop (20), juce::Justification::centredLeft);
    r.removeFromTop (10);

    // stats grid
    auto grid = r.removeFromTop (58);
    struct Cell { juce::String k, v; };
    std::vector<Cell> cells;
    cells.push_back ({ "LISTENED", juce::String (stats.seconds, 1) + " s" });
    cells.push_back ({ "LEVEL (RMS)", stats.valid ? juce::String ((int) stats.rmsDb) + " dB" : "--" });
    cells.push_back ({ "PEAK", stats.valid ? juce::String ((int) stats.peakDb) + " dB" : "--" });
    cells.push_back ({ "NOISE FLOOR", stats.valid && stats.noiseFloorDb > -99.f ? juce::String ((int) stats.noiseFloorDb) + " dB" : "--" });
    cells.push_back ({ "DYNAMICS", stats.valid ? juce::String ((int) stats.crestDb) + " dB" : "--" });
    cells.push_back ({ "SIBILANCE", stats.valid ? juce::String ((int) (stats.sibilanceRatio * 100.f)) + " %" : "--" });
    const int cw = grid.getWidth() / (int) cells.size();
    for (auto& c : cells)
    {
        auto cell = grid.removeFromLeft (cw).reduced (3, 0);
        g.setColour (colours::panelLight());
        g.fillRoundedRectangle (cell.toFloat(), 6.f);
        g.setColour (colours::textDim());
        g.setFont (font (10.f, true));
        g.drawText (c.k, cell.reduced (8, 6).removeFromTop (14), juce::Justification::centredLeft);
        g.setColour (colours::text());
        g.setFont (font (16.f, true));
        g.drawText (c.v, cell.reduced (8, 6).withTrimmedTop (16), juce::Justification::centredLeft);
    }
    r.removeFromTop (12);
    r.removeFromBottom (44); // buttons

    if (! stats.valid)
    {
        g.setColour (colours::accent2());
        g.setFont (font (14.f));
        g.drawFittedText ("Listening... start playback so the singer is heard for at least a second.", r, juce::Justification::centred, 2);
        return;
    }
    g.setColour (colours::textDim());
    g.setFont (font (10.5f, true));
    g.drawText ("SUGGESTIONS", r.removeFromTop (16), juce::Justification::centredLeft);
    for (auto& s : suggestions)
    {
        auto row = r.removeFromTop (34);
        if (row.getHeight() < 30) break;
        g.setColour (colours::ok());
        g.fillEllipse (row.removeFromLeft (14).withSizeKeepingCentre (8, 8).toFloat());
        row.removeFromLeft (8);
        g.setColour (colours::text());
        g.setFont (font (13.f));
        g.drawFittedText (s.text, row, juce::Justification::centredLeft, 2);
    }
}

void AnalyzePanel::resized()
{
    auto card = getLocalBounds().withSizeKeepingCentre (juce::jmin (getWidth() - 40, 680), juce::jmin (getHeight() - 30, 470));
    auto row = card.reduced (24, 18).removeFromBottom (34);
    closeBtn.setBounds (row.removeFromRight (90));
    row.removeFromRight (8);
    resetBtn.setBounds (row.removeFromRight (120));
    applyBtn.setBounds (row.removeFromLeft (190));
}

} // namespace muze::ui
