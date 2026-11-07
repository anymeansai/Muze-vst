#pragma once
#include "VocalystLookAndFeel.h"
#include "../VocalystProcessor.h"

namespace vocalyst::ui
{

// Clickable one-octave keyboard: each key toggles whether Tune may snap to that pitch class.
class NoteKey : public juce::Button
{
public:
    NoteKey (int pitchClass, bool black) : juce::Button (scales::keyNames()[pitchClass]), pc (pitchClass), isBlack (black) { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
    bool black() const { return isBlack; }
private:
    int pc; bool isBlack;
};

// Scrolling detected-vs-corrected pitch graph.
class PitchGraph : public juce::Component, private juce::Timer
{
public:
    explicit PitchGraph (VocalystProcessor& p) : processor (p) {}
    void paint (juce::Graphics&) override;
    void visibilityChanged() override { if (isVisible()) startTimerHz (30); else stopTimer(); }
private:
    void timerCallback() override { repaint(); }
    VocalystProcessor& processor;
    float centre = 60.f;
};

// The TUNE tab: key, scale, note selection, retune controls and the pitch graph.
class TuneView : public juce::Component, private VocalystProcessor::Listener
{
public:
    explicit TuneView (VocalystProcessor&);
    ~TuneView() override;
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    void chainChanged() override { refresh(); }
    void keyChanged() override { refresh(); }
    void presetChanged() override { refresh(); }
    void refresh();
    void applyQuick (float speed, float humanize, float flex);

    VocalystProcessor& processor;
    juce::Label title, keyLabel, scaleLabel, rangeLabel, hint, notInChain;
    juce::ComboBox keyBox, scaleBox, rangeBox;
    std::unique_ptr<juce::ComboBoxParameterAttachment> keyAtt, scaleAtt, rangeAtt;
    std::vector<std::unique_ptr<NoteKey>> keys;
    std::vector<std::unique_ptr<juce::ButtonParameterAttachment>> keyAtts;
    PillButton midiBtn { "FOLLOW MIDI NOTES" };
    std::unique_ptr<juce::ButtonParameterAttachment> midiAtt;
    juce::TextButton naturalBtn { "NATURAL" }, tightBtn { "TIGHT" }, hardBtn { "HARD" }, addBtn { "ADD TUNE TO CHAIN" };
    juce::ToggleButton power;
    std::unique_ptr<juce::ButtonParameterAttachment> powerAtt;
    std::vector<std::unique_ptr<Knob>> knobs;
    PitchGraph graph;
};

} // namespace vocalyst::ui
