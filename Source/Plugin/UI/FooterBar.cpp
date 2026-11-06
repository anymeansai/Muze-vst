#include "FooterBar.h"

namespace muze::ui
{

void LevelMeter::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();
    const float gap = 3.f;
    const float h = (r.getHeight() - gap) * 0.5f;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto bar = juce::Rectangle<float> (r.getX(), r.getY() + (float) ch * (h + gap), r.getWidth(), h);
        g.setColour (colours::bg());
        g.fillRoundedRectangle (bar, 2.f);
        const float db = juce::Decibels::gainToDecibels (lv[ch], -60.f);
        const float frac = juce::jlimit (0.f, 1.f, (db + 60.f) / 60.f);
        juce::ColourGradient grad (colours::ok(), bar.getX(), 0.f, colours::danger(), bar.getRight(), 0.f, false);
        grad.addColour (0.8, colours::accent2());
        g.setGradientFill (grad);
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * frac), 2.f);
    }
}

FooterBar::FooterBar (MuzeProcessor& p)
    : processor (p),
      inKnob ("INPUT", p.apvts.getParameter (ids::inGain), colours::accent2()),
      mixKnob ("MIX", p.apvts.getParameter (ids::mix)),
      outKnob ("OUTPUT", p.apvts.getParameter (ids::outGain), colours::accent2())
{
    addAndMakeVisible (inKnob); addAndMakeVisible (mixKnob); addAndMakeVisible (outKnob);
    addAndMakeVisible (inMeter); addAndMakeVisible (outMeter);
    latencyLabel.setFont (font (11.f));
    latencyLabel.setColour (juce::Label::textColourId, colours::textDim());
    latencyLabel.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (latencyLabel);
    hint.setFont (font (11.f));
    hint.setColour (juce::Label::textColourId, colours::textDim());
    hint.setJustificationType (juce::Justification::centred);
    hint.setText ("Raw to finished vocal in seconds. Pick a preset, set the key, press Analyze.", juce::dontSendNotification);
    addAndMakeVisible (hint);
    timerCallback();
    startTimerHz (30);
}

void FooterBar::timerCallback()
{
    inMeter.setLevels (processor.inputLevel (0), processor.inputLevel (1));
    outMeter.setLevels (processor.outputLevel (0), processor.outputLevel (1));
    const int lat = (int) processor.currentLatencySamples();
    latencyLabel.setText (lat > 0 ? "Latency " + juce::String (lat) + " smp" : "Zero latency", juce::dontSendNotification);
}

void FooterBar::paint (juce::Graphics& g)
{
    g.fillAll (colours::panel());
    g.setColour (colours::outline());
    g.drawLine (0.f, 0.5f, (float) getWidth(), 0.5f);
}

void FooterBar::resized()
{
    auto r = getLocalBounds().reduced (12, 4);
    inKnob.setBounds (r.removeFromLeft (70));
    inMeter.setBounds (r.removeFromLeft (110).reduced (6, 22));
    r.removeFromLeft (10);
    latencyLabel.setBounds (r.removeFromRight (110));
    outMeter.setBounds (r.removeFromRight (110).reduced (6, 22));
    outKnob.setBounds (r.removeFromRight (70));
    mixKnob.setBounds (r.removeFromRight (70));
    hint.setBounds (r);
}

} // namespace muze::ui
