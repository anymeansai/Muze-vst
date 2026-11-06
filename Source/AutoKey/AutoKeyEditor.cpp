#include "AutoKeyEditor.h"

namespace muze
{
using namespace ui;

AutoKeyEditor::AutoKeyEditor (AutoKeyProcessor& p) : AudioProcessorEditor (p), keyProcessor (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (sendBtn); addAndMakeVisible (resetBtn); addAndMakeVisible (autoBtn);
    sendBtn.setColour (juce::TextButton::buttonColourId, colours::accent());
    sendBtn.onClick = [this] { keyProcessor.sendToMuze(); timerCallback(); };
    resetBtn.onClick = [this] { keyProcessor.detector().reset(); timerCallback(); };
    autoBtn.setActive (keyProcessor.isAutoSend());
    autoBtn.onClick = [this] { keyProcessor.setAutoSend (! keyProcessor.isAutoSend()); autoBtn.setActive (keyProcessor.isAutoSend()); };
    status.setFont (font (11.5f));
    status.setColour (juce::Label::textColourId, colours::textDim());
    status.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (status);
    setSize (420, 330);
    timerCallback();
    startTimerHz (8);
}

AutoKeyEditor::~AutoKeyEditor() { setLookAndFeel (nullptr); }

void AutoKeyEditor::timerCallback()
{
    result = keyProcessor.current();
    auto sent = keyProcessor.lastSentText();
    status.setText (sent.isNotEmpty() ? "Sent " + sent : "Place on the beat or master bus. Muze picks the key up from the AUTO KEY menu.", juce::dontSendNotification);
    repaint();
}

void AutoKeyEditor::paint (juce::Graphics& g)
{
    g.fillAll (colours::bg());
    auto r = getLocalBounds().reduced (18, 14);

    auto head = r.removeFromTop (34);
    g.setColour (colours::accent());
    g.fillRoundedRectangle (head.removeFromLeft (26).toFloat().reduced (2.f), 6.f);
    head.removeFromLeft (8);
    g.setColour (colours::text());
    g.setFont (font (20.f, true));
    g.drawText ("MUZE AUTO KEY", head, juce::Justification::centredLeft);

    r.removeFromTop (6);
    auto keyArea = r.removeFromTop (90);
    g.setColour (colours::panel());
    g.fillRoundedRectangle (keyArea.toFloat(), 10.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (keyArea.toFloat().reduced (0.5f), 10.f, 1.f);
    g.setColour (colours::text());
    g.setFont (font (36.f, true));
    g.drawText (result.valid ? scales::keyName (result.key, result.scale) : "Listening...", keyArea.withTrimmedBottom (26), juce::Justification::centred);
    g.setColour (colours::textDim());
    g.setFont (font (12.f));
    juce::String meta = result.valid ? "Confidence " + juce::String ((int) (result.confidence * 100.f)) + " %   -   " + juce::String (result.seconds, 1) + " s analysed"
                                     : juce::String (result.seconds, 1) + " s analysed";
    g.drawText (meta, keyArea.withTrimmedTop (60), juce::Justification::centred);

    // confidence bar
    r.removeFromTop (8);
    auto bar = r.removeFromTop (6);
    g.setColour (colours::panelLight());
    g.fillRoundedRectangle (bar.toFloat(), 3.f);
    g.setColour (colours::accent2());
    g.fillRoundedRectangle (bar.withWidth ((int) ((float) bar.getWidth() * result.confidence)).toFloat(), 3.f);

    // chroma
    r.removeFromTop (12);
    auto chroma = r.removeFromTop (90);
    g.setColour (colours::textDim());
    g.setFont (font (10.f, true));
    g.drawText ("PITCH CLASS ENERGY", chroma.removeFromTop (14), juce::Justification::centredLeft);
    float mx = 1.0e-6f;
    for (auto v : result.chroma) mx = juce::jmax (mx, v);
    const int w = chroma.getWidth() / 12;
    for (int i = 0; i < 12; ++i)
    {
        auto col = chroma.removeFromLeft (w).reduced (3, 0);
        auto label = col.removeFromBottom (14);
        const float h = (float) col.getHeight() * (result.chroma[(size_t) i] / mx);
        const bool inKey = result.valid && scales::noteInScale (i, result.key, result.scale);
        g.setColour (colours::panelLight());
        g.fillRoundedRectangle (col.toFloat(), 3.f);
        g.setColour (inKey ? colours::accent() : colours::outline().brighter (0.2f));
        g.fillRoundedRectangle (col.withTop (col.getBottom() - (int) h).toFloat(), 3.f);
        g.setColour (result.valid && i == result.key ? colours::text() : colours::textDim());
        g.setFont (font (10.5f, i == result.key));
        g.drawText (scales::keyNames()[i], label, juce::Justification::centred);
    }
}

void AutoKeyEditor::resized()
{
    auto r = getLocalBounds().reduced (18, 14);
    status.setBounds (r.removeFromBottom (18));
    r.removeFromBottom (6);
    auto row = r.removeFromBottom (32);
    const int w = (row.getWidth() - 16) / 3;
    sendBtn.setBounds (row.removeFromLeft (w)); row.removeFromLeft (8);
    autoBtn.setBounds (row.removeFromLeft (w)); row.removeFromLeft (8);
    resetBtn.setBounds (row);
}

} // namespace muze
