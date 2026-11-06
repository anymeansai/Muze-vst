#include "MuzeLookAndFeel.h"

namespace muze::ui
{

MuzeLookAndFeel::MuzeLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, colours::bg());
    setColour (juce::Label::textColourId, colours::text());
    setColour (juce::Slider::textBoxTextColourId, colours::text());
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::rotarySliderFillColourId, colours::accent());
    setColour (juce::Slider::rotarySliderOutlineColourId, colours::outline());
    setColour (juce::Slider::thumbColourId, colours::text());
    setColour (juce::TextButton::buttonColourId, colours::panelLight());
    setColour (juce::TextButton::buttonOnColourId, colours::accent());
    setColour (juce::TextButton::textColourOffId, colours::text());
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::ComboBox::backgroundColourId, colours::panelLight());
    setColour (juce::ComboBox::textColourId, colours::text());
    setColour (juce::ComboBox::outlineColourId, colours::outline());
    setColour (juce::ComboBox::arrowColourId, colours::textDim());
    setColour (juce::PopupMenu::backgroundColourId, colours::panelLight());
    setColour (juce::PopupMenu::textColourId, colours::text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colours::accent());
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::PopupMenu::headerTextColourId, colours::textDim());
    setColour (juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, colours::bg());
    setColour (juce::TextEditor::textColourId, colours::text());
    setColour (juce::TextEditor::outlineColourId, colours::outline());
    setColour (juce::TextEditor::focusedOutlineColourId, colours::accent());
    setColour (juce::TextEditor::highlightColourId, colours::accent().withAlpha (0.4f));
    setColour (juce::AlertWindow::backgroundColourId, colours::panel());
    setColour (juce::AlertWindow::textColourId, colours::text());
    setColour (juce::AlertWindow::outlineColourId, colours::outline());
    setColour (juce::ScrollBar::thumbColourId, colours::outline());
    setColour (juce::ToggleButton::textColourId, colours::text());
    setColour (juce::ToggleButton::tickColourId, colours::accent());
}

void MuzeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
    const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    bounds = bounds.withSizeKeepingCentre (size, size);
    const float radius = size * 0.5f;
    const float lineW = juce::jmax (2.5f, size * 0.07f);
    const float arcR = radius - lineW * 0.5f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // body
    g.setColour (colours::panelLight());
    g.fillEllipse (bounds.reduced (lineW * 1.6f));
    g.setColour (colours::outline());
    g.drawEllipse (bounds.reduced (lineW * 1.6f), 1.f);

    // track
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, startAngle, endAngle, true);
    g.setColour (s.findColour (juce::Slider::rotarySliderOutlineColourId));
    g.strokePath (track, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // value arc (bipolar if the range crosses zero)
    const double mn = s.getMinimum(), mx = s.getMaximum();
    const bool bipolar = mn < 0.0 && mx > 0.0;
    juce::Path value;
    const float zeroAngle = bipolar ? startAngle + (float) ((0.0 - mn) / (mx - mn)) * (endAngle - startAngle) : startAngle;
    if (std::abs (angle - zeroAngle) > 0.001f)
    {
        value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.f, juce::jmin (zeroAngle, angle), juce::jmax (zeroAngle, angle), true);
        g.setColour (s.isEnabled() ? s.findColour (juce::Slider::rotarySliderFillColourId) : colours::textDim());
        g.strokePath (value, juce::PathStrokeType (lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // pointer
    const float pr = radius - lineW * 2.4f;
    juce::Point<float> p1 (centre.x + (pr * 0.45f) * std::sin (angle), centre.y - (pr * 0.45f) * std::cos (angle));
    juce::Point<float> p2 (centre.x + pr * std::sin (angle), centre.y - pr * std::cos (angle));
    g.setColour (colours::text());
    g.drawLine ({ p1, p2 }, juce::jmax (2.f, lineW * 0.7f));
}

void MuzeLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bgColour, bool highlighted, bool down)
{
    auto bounds = b.getLocalBounds().toFloat().reduced (0.5f);
    const float r = juce::jmin (8.f, bounds.getHeight() * 0.5f);
    juce::Colour c = bgColour;
    if (auto* pill = dynamic_cast<PillButton*> (&b))
        if (pill->isActive()) c = colours::accent();
    if (b.getToggleState()) c = colours::accent();
    if (down) c = c.brighter (0.15f);
    else if (highlighted) c = c.brighter (0.08f);
    g.setColour (c);
    g.fillRoundedRectangle (bounds, r);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (bounds, r, 1.f);
}

void MuzeLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    bool active = b.getToggleState();
    if (auto* pill = dynamic_cast<PillButton*> (&b)) active = active || pill->isActive();
    g.setColour (active ? juce::Colours::white : colours::text().withAlpha (b.isEnabled() ? 1.f : 0.4f));
    g.drawFittedText (b.getButtonText(), b.getLocalBounds().reduced (6, 0), juce::Justification::centred, 1);
}

void MuzeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    // drawn as a small power switch
    auto r = b.getLocalBounds().toFloat();
    const float s = juce::jmin (r.getWidth(), r.getHeight()) - 2.f;
    auto circle = r.withSizeKeepingCentre (s, s);
    const bool on = b.getToggleState();
    g.setColour (on ? colours::ok() : colours::panelLight());
    g.fillEllipse (circle);
    g.setColour (on ? colours::ok().brighter() : (highlighted ? colours::textDim() : colours::outline()));
    g.drawEllipse (circle, 1.2f);
    // power glyph
    auto inner = circle.reduced (s * 0.28f);
    juce::Path p;
    p.addCentredArc (inner.getCentreX(), inner.getCentreY(), inner.getWidth() * 0.5f, inner.getHeight() * 0.5f, 0.f, 0.6f, juce::MathConstants<float>::twoPi - 0.6f, true);
    g.setColour (on ? colours::bg() : colours::textDim());
    g.strokePath (p, juce::PathStrokeType (1.6f));
    g.drawLine (inner.getCentreX(), inner.getY() - 1.f, inner.getCentreX(), inner.getCentreY(), 1.6f);
}

void MuzeLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 6.f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 6.f, 1.f);
    juce::Path arrow;
    const float ax = (float) w - 14.f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.f, ay - 2.f, ax + 4.f, ay - 2.f, ax, ay + 3.f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

void MuzeLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 26, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

void MuzeLookAndFeel::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.setColour (colours::panelLight());
    g.fillRoundedRectangle (0.f, 0.f, (float) w, (float) h, 6.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (0.5f, 0.5f, (float) w - 1.f, (float) h - 1.f, 6.f, 1.f);
}

juce::Label* MuzeLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (font (11.5f));
    l->setJustificationType (juce::Justification::centred);
    l->setColour (juce::Label::textColourId, colours::textDim());
    l->setColour (juce::Label::outlineWhenEditingColourId, colours::accent());
    l->setColour (juce::Label::backgroundWhenEditingColourId, colours::bg());
    l->setColour (juce::Label::textWhenEditingColourId, colours::text());
    return l;
}

void MuzeLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    auto r = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    auto track = r.withSizeKeepingCentre ((float) w, 4.f);
    g.setColour (colours::outline());
    g.fillRoundedRectangle (track, 2.f);
    g.setColour (s.findColour (juce::Slider::rotarySliderFillColourId));
    g.fillRoundedRectangle (track.withWidth (pos - (float) x), 2.f);
    g.setColour (colours::text());
    g.fillEllipse (juce::Rectangle<float> (12.f, 12.f).withCentre ({ pos, r.getCentreY() }));
}

void MuzeLookAndFeel::drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool, bool)
{
    juce::Rectangle<float> thumb = vertical ? juce::Rectangle<float> ((float) x + 2.f, (float) thumbStart, (float) w - 4.f, (float) thumbSize)
                                            : juce::Rectangle<float> ((float) thumbStart, (float) y + 2.f, (float) thumbSize, (float) h - 4.f);
    g.setColour (colours::outline());
    g.fillRoundedRectangle (thumb, 3.f);
}

//==============================================================================
Knob::Knob (const juce::String& titleText, juce::RangedAudioParameter* param, juce::Colour accent)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 16);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setPopupDisplayEnabled (false, false, nullptr);
    addAndMakeVisible (slider);
    title.setText (titleText, juce::dontSendNotification);
    title.setFont (font (12.f, true));
    title.setJustificationType (juce::Justification::centred);
    title.setColour (juce::Label::textColourId, colours::textDim());
    addAndMakeVisible (title);
    if (param != nullptr)
        attachment = std::make_unique<juce::SliderParameterAttachment> (*param, slider);
    else
        setEnabled (false);
}

void Knob::setAccent (juce::Colour c) { slider.setColour (juce::Slider::rotarySliderFillColourId, c); repaint(); }

void Knob::resized()
{
    auto r = getLocalBounds();
    title.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

} // namespace muze::ui
