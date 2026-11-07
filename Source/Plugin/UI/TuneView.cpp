#include "TuneView.h"

namespace vocalyst::ui
{

void NoteKey::paintButton (juce::Graphics& g, bool highlighted, bool)
{
    auto r = getLocalBounds().toFloat().reduced (1.f);
    const bool on = getToggleState();
    juce::Colour base = isBlack ? juce::Colour (0xff1a1a22) : juce::Colour (0xffe8e8ee);
    if (on) base = isBlack ? colours::accent().darker (0.35f) : colours::accent();
    if (highlighted) base = base.brighter (0.1f);
    g.setColour (base);
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (on ? colours::accent().brighter (0.3f) : colours::outline());
    g.drawRoundedRectangle (r, 4.f, 1.f);
    g.setColour (on ? juce::Colours::white : (isBlack ? colours::textDim() : juce::Colour (0xff55555f)));
    g.setFont (font (isBlack ? 10.f : 12.f, true));
    g.drawText (getName(), r.withTrimmedTop (r.getHeight() * 0.55f), juce::Justification::centred);
}

void PitchGraph::paint (juce::Graphics& g)
{
    auto area = getLocalBounds().toFloat();
    g.setColour (colours::bg());
    g.fillRoundedRectangle (area, 8.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (area.reduced (0.5f), 8.f, 1.f);

    auto& trace = processor.pitchTrace();
    const int head = trace.head.load();
    const int n = PitchTrace::size;

    // auto-centre on the recent sung pitch
    float sum = 0.f; int cnt = 0;
    for (int i = 0; i < 120; ++i)
    {
        const float v = trace.inMidi[(size_t) ((head - 1 - i + n) % n)].load();
        if (v > 0.f) { sum += v; ++cnt; }
    }
    if (cnt > 0) centre += 0.1f * (sum / (float) cnt - centre);
    const float span = 14.f;   // semitones shown top to bottom
    auto yFor = [&] (float midi) { return area.getBottom() - 10.f - ((midi - (centre - span * 0.5f)) / span) * (area.getHeight() - 20.f); };
    const int mask = processor.getNoteMask();

    // note lines
    g.setFont (font (10.f, true));
    for (int m = (int) std::floor (centre - span * 0.5f); m <= (int) std::ceil (centre + span * 0.5f); ++m)
    {
        const bool allowed = (mask >> ((m % 12 + 12) % 12)) & 1;
        const float y = yFor ((float) m);
        if (y < area.getY() + 4.f || y > area.getBottom() - 4.f) continue;
        g.setColour (allowed ? colours::outline().brighter (0.25f) : colours::outline().withAlpha (0.35f));
        g.drawHorizontalLine ((int) y, area.getX() + 6.f, area.getRight() - 44.f);
        g.setColour (allowed ? colours::text().withAlpha (0.8f) : colours::textDim().withAlpha (0.5f));
        g.drawText (scales::keyNames()[(m % 12 + 12) % 12] + juce::String (m / 12 - 1), (int) area.getRight() - 40, (int) y - 7, 36, 14, juce::Justification::centredLeft);
    }

    // traces: oldest on the left
    const float w = area.getWidth() - 50.f;
    juce::Path inPath, outPath;
    bool inOpen = false, outOpen = false;
    for (int i = 0; i < n; ++i)
    {
        const int idx = (head + i) % n;
        const float x = area.getX() + 6.f + (float) i / (float) (n - 1) * w;
        const float vi = trace.inMidi[(size_t) idx].load(), vo = trace.outMidi[(size_t) idx].load();
        if (vi > 0.f) { if (inOpen) inPath.lineTo (x, yFor (vi)); else { inPath.startNewSubPath (x, yFor (vi)); inOpen = true; } } else inOpen = false;
        if (vo > 0.f) { if (outOpen) outPath.lineTo (x, yFor (vo)); else { outPath.startNewSubPath (x, yFor (vo)); outOpen = true; } } else outOpen = false;
    }
    g.setColour (colours::textDim().withAlpha (0.8f));
    g.strokePath (inPath, juce::PathStrokeType (1.4f));
    g.setColour (colours::accent2());
    g.strokePath (outPath, juce::PathStrokeType (2.0f));

    // readout
    const float inNow = trace.inNow.load(), outNow = trace.outNow.load();
    juce::String text = "no pitch";
    if (inNow > 0.f)
    {
        const int ni = (int) std::lround (inNow);
        const int cents = (int) std::lround ((inNow - (float) ni) * 100.f);
        const int no = (int) std::lround (outNow);
        text = "IN  " + scales::keyNames()[(ni % 12 + 12) % 12] + juce::String (ni / 12 - 1) + " " + (cents >= 0 ? "+" : "") + juce::String (cents) + " ct"
             + "     OUT  " + scales::keyNames()[(no % 12 + 12) % 12] + juce::String (no / 12 - 1) + "  (" + juce::String (trace.inHz.load(), 1) + " Hz in)";
    }
    g.setColour (colours::panel().withAlpha (0.85f));
    g.fillRoundedRectangle (area.getX() + 8.f, area.getY() + 8.f, 360.f, 20.f, 5.f);
    g.setColour (colours::text());
    g.setFont (font (12.f, true));
    g.drawText (text, (int) area.getX() + 16, (int) area.getY() + 8, 350, 20, juce::Justification::centredLeft);
}

//==============================================================================
TuneView::TuneView (VocalystProcessor& p) : processor (p), graph (p)
{
    title.setText ("TUNE", juce::dontSendNotification);
    title.setFont (font (11.f, true));
    title.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (title);
    hint.setText ("Pick the song key (or pull it from Vocalyst Auto Key), click keys to allow or forbid notes, then set how hard it corrects.", juce::dontSendNotification);
    hint.setFont (font (12.f));
    hint.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (hint);

    for (auto* l : { &keyLabel, &scaleLabel, &rangeLabel })
    {
        l->setFont (font (10.5f, true));
        l->setColour (juce::Label::textColourId, colours::textDim());
        addAndMakeVisible (l);
    }
    keyLabel.setText ("KEY", juce::dontSendNotification);
    scaleLabel.setText ("SCALE", juce::dontSendNotification);
    rangeLabel.setText ("VOICE RANGE", juce::dontSendNotification);

    for (int i = 0; i < 12; ++i) keyBox.addItem (scales::keyNames()[i], i + 1);
    for (int i = 0; i < scales::scaleNames().size(); ++i) scaleBox.addItem (scales::scaleNames()[i], i + 1);
    juce::StringArray ranges; ranges.addTokens ("Auto,Soprano,Alto / Tenor,Low Male,Bass", ",", "");
    for (int i = 0; i < ranges.size(); ++i) rangeBox.addItem (ranges[i], i + 1);
    addAndMakeVisible (keyBox); addAndMakeVisible (scaleBox); addAndMakeVisible (rangeBox);
    if (auto* kp = processor.apvts.getParameter (ids::key))   keyAtt   = std::make_unique<juce::ComboBoxParameterAttachment> (*kp, keyBox);
    if (auto* sp = processor.apvts.getParameter (ids::scale)) scaleAtt = std::make_unique<juce::ComboBoxParameterAttachment> (*sp, scaleBox);
    if (auto* rp = processor.apvts.getParameter (paramId (ModuleId::Tune, "range"))) rangeAtt = std::make_unique<juce::ComboBoxParameterAttachment> (*rp, rangeBox);

    static const bool blackKeys[12] = { false, true, false, true, false, false, true, false, true, false, true, false };
    for (int i = 0; i < 12; ++i)
    {
        auto k = std::make_unique<NoteKey> (i, blackKeys[i]);
        addAndMakeVisible (*k);
        if (auto* np = processor.apvts.getParameter (ids::note (i)))
            keyAtts.push_back (std::make_unique<juce::ButtonParameterAttachment> (*np, *k));
        keys.push_back (std::move (k));
    }
    for (auto& k : keys) if (k->black()) k->toFront (false);

    midiBtn.setClickingTogglesState (true);
    addAndMakeVisible (midiBtn);
    if (auto* mp = processor.apvts.getParameter (paramId (ModuleId::Tune, "midi")))
        midiAtt = std::make_unique<juce::ButtonParameterAttachment> (*mp, midiBtn);

    for (auto* b : { &naturalBtn, &tightBtn, &hardBtn }) addAndMakeVisible (b);
    naturalBtn.onClick = [this] { applyQuick (120.f, 40.f, 20.f); };
    tightBtn.onClick   = [this] { applyQuick (20.f, 0.f, 0.f); };
    hardBtn.onClick    = [this] { applyQuick (0.f, 0.f, 0.f); };

    addBtn.setColour (juce::TextButton::buttonColourId, colours::accent());
    addBtn.onClick = [this] { auto order = processor.getChainOrder(); processor.addModule (ModuleId::Tune, order.empty() ? -1 : juce::jmin (1, (int) order.size())); };
    addAndMakeVisible (addBtn);
    notInChain.setText ("Tune is not in the signal chain yet.", juce::dontSendNotification);
    notInChain.setFont (font (12.5f, true));
    notInChain.setColour (juce::Label::textColourId, colours::danger());
    addAndMakeVisible (notInChain);

    power.setClickingTogglesState (true);
    addAndMakeVisible (power);
    if (auto* pp = processor.apvts.getParameter (enableId (ModuleId::Tune)))
        powerAtt = std::make_unique<juce::ButtonParameterAttachment> (*pp, power);

    for (const char* key : { "amount", "speed", "humanize", "flex", "vibrato", "transpose" })
    {
        auto* spec = findParam (ModuleId::Tune, key);
        auto knob = std::make_unique<Knob> (spec != nullptr ? spec->name : key, processor.apvts.getParameter (paramId (ModuleId::Tune, key)), categoryColour (ModuleCategory::Clean));
        addAndMakeVisible (*knob);
        knobs.push_back (std::move (knob));
    }
    addAndMakeVisible (graph);
    processor.addVocalystListener (this);
    refresh();
}

TuneView::~TuneView() { processor.removeVocalystListener (this); }

void TuneView::applyQuick (float speed, float humanize, float flex)
{
    auto set = [this] (const char* key, float v) { if (auto* p = processor.apvts.getParameter (paramId (ModuleId::Tune, key))) p->setValueNotifyingHost (p->convertTo0to1 (v)); };
    set ("speed", speed); set ("humanize", humanize); set ("flex", flex); set ("amount", 100.f);
}

void TuneView::refresh()
{
    const bool inChain = processor.isInChain (ModuleId::Tune);
    addBtn.setVisible (! inChain);
    notInChain.setVisible (! inChain);
    repaint();
}

void TuneView::paint (juce::Graphics& g)
{
    g.fillAll (colours::bg());
    auto r = getLocalBounds().reduced (14, 10);
    r.removeFromTop (44);
    auto left = r.removeFromLeft (330);
    g.setColour (colours::panel());
    g.fillRoundedRectangle (left.toFloat(), 10.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (left.toFloat().reduced (0.5f), 10.f, 1.f);
    r.removeFromLeft (10);
    auto right = r.removeFromRight (300);
    g.setColour (colours::panel());
    g.fillRoundedRectangle (right.toFloat(), 10.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (right.toFloat().reduced (0.5f), 10.f, 1.f);
    g.setColour (colours::textDim());
    g.setFont (font (10.5f, true));
    g.drawText ("NOTES VOCALYST MAY SNAP TO  -  click to toggle", left.reduced (16, 12).withTrimmedTop (106).removeFromTop (14), juce::Justification::centredLeft);
    g.setFont (font (11.f));
    auto legend = left.reduced (16, 12).withTrimmedTop (290);
    g.drawFittedText ("NATURAL  slow retune, keeps vibrato and slides, forgives off notes.\nTIGHT  modern pop/R&B: quick, still human.\nHARD  instant snap, the classic hard-tuned effect.\n\nAmount  how far toward the note.   Retune Speed  how fast.\nHumanize  slows down on held notes.   Flex-Tune  leaves notes further than this alone.\nVibrato  scale the singer's own vibrato.   Transpose  shift the whole part.\nFollow MIDI  held MIDI notes become the allowed notes.", legend, juce::Justification::topLeft, 12);
    g.drawText ("CORRECTION", right.reduced (16, 12).removeFromTop (14), juce::Justification::centredLeft);
    g.drawText ("PITCH  -  sung (grey) vs corrected (cyan)", r.removeFromTop (14), juce::Justification::centredLeft);
}

void TuneView::resized()
{
    auto r = getLocalBounds().reduced (14, 10);
    auto head = r.removeFromTop (44);
    title.setBounds (head.removeFromTop (16));
    hint.setBounds (head.removeFromTop (18));

    auto left = r.removeFromLeft (330).reduced (16, 12);
    auto row = left.removeFromTop (18);
    keyLabel.setBounds (row.removeFromLeft (90)); row.removeFromLeft (8); scaleLabel.setBounds (row);
    row = left.removeFromTop (30);
    keyBox.setBounds (row.removeFromLeft (90)); row.removeFromLeft (8); scaleBox.setBounds (row);
    left.removeFromTop (8);
    row = left.removeFromTop (18);
    rangeLabel.setBounds (row);
    row = left.removeFromTop (30);
    rangeBox.setBounds (row.removeFromLeft (150)); row.removeFromLeft (8); midiBtn.setBounds (row.reduced (0, 1));
    left.removeFromTop (20);
    auto kb = left.removeFromTop (110);
    // white keys
    static const int whites[7] = { 0, 2, 4, 5, 7, 9, 11 };
    static const int blacks[5] = { 1, 3, 6, 8, 10 };
    const int ww = kb.getWidth() / 7;
    for (int i = 0; i < 7; ++i) keys[(size_t) whites[i]]->setBounds (kb.getX() + i * ww, kb.getY(), ww, kb.getHeight());
    const int bw = (int) (ww * 0.62f);
    static const float blackPos[5] = { 1.f, 2.f, 4.f, 5.f, 6.f };
    for (int i = 0; i < 5; ++i) keys[(size_t) blacks[i]]->setBounds (kb.getX() + (int) (blackPos[i] * (float) ww) - bw / 2, kb.getY(), bw, (int) (kb.getHeight() * 0.6f));
    left.removeFromTop (12);
    row = left.removeFromTop (30);
    const int qw = (row.getWidth() - 12) / 3;
    naturalBtn.setBounds (row.removeFromLeft (qw)); row.removeFromLeft (6);
    tightBtn.setBounds (row.removeFromLeft (qw)); row.removeFromLeft (6);
    hardBtn.setBounds (row);

    r.removeFromLeft (10);
    auto right = r.removeFromRight (300).reduced (16, 12);
    right.removeFromTop (18);
    auto prow = right.removeFromTop (30);
    power.setBounds (prow.removeFromLeft (26).withSizeKeepingCentre (22, 22));
    prow.removeFromLeft (8);
    notInChain.setBounds (prow);
    right.removeFromTop (6);
    addBtn.setBounds (right.removeFromTop (30));
    right.removeFromTop (8);
    const int kw = right.getWidth() / 2;
    const int kh = juce::jmin (120, right.getHeight() / 3);
    for (size_t i = 0; i < knobs.size(); ++i)
        knobs[i]->setBounds (right.getX() + (int) (i % 2) * kw, right.getY() + (int) (i / 2) * kh, kw, kh);

    r.removeFromRight (10);
    r.removeFromTop (18);
    graph.setBounds (r);
}

} // namespace vocalyst::ui
