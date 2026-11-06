#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace muze::ui
{

namespace colours
{
    inline juce::Colour bg()         { return juce::Colour (0xff0b0b10); }
    inline juce::Colour panel()      { return juce::Colour (0xff14141b); }
    inline juce::Colour panelLight() { return juce::Colour (0xff1d1d27); }
    inline juce::Colour outline()    { return juce::Colour (0xff2a2a38); }
    inline juce::Colour accent()     { return juce::Colour (0xff8b5cf6); }
    inline juce::Colour accent2()    { return juce::Colour (0xff22d3ee); }
    inline juce::Colour text()       { return juce::Colour (0xffededf2); }
    inline juce::Colour textDim()    { return juce::Colour (0xff8a8a99); }
    inline juce::Colour danger()     { return juce::Colour (0xfff43f5e); }
    inline juce::Colour ok()         { return juce::Colour (0xff4ade80); }
}

inline juce::Font font (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
}

class MuzeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MuzeLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& bg, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return font (13.f); }
    juce::Font getTextButtonFont (juce::TextButton&, int) override { return font (13.f, true); }
    juce::Font getPopupMenuFont() override { return font (14.f); }
    juce::Font getLabelFont (juce::Label& l) override { return l.getFont(); }
    void drawPopupMenuBackground (juce::Graphics&, int w, int h) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float min, float max, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int w, int h, bool vertical, int thumbStart, int thumbSize, bool mouseOver, bool down) override;
    int getDefaultScrollbarWidth() override { return 8; }
};

// Rotary knob bound to a parameter, with a title above and value readout below.
class Knob : public juce::Component
{
public:
    Knob (const juce::String& title, juce::RangedAudioParameter* param, juce::Colour accent = colours::accent());
    void resized() override;
    void setAccent (juce::Colour c);
    juce::Slider slider;
    juce::Label  title;
private:
    std::unique_ptr<juce::SliderParameterAttachment> attachment;
};

// Flat text button with optional "active" highlight
class PillButton : public juce::TextButton
{
public:
    explicit PillButton (const juce::String& buttonText) : juce::TextButton (buttonText) {}
    void setActive (bool a) { active = a; repaint(); }
    bool isActive() const { return active; }
private:
    bool active = false;
};

} // namespace muze::ui
