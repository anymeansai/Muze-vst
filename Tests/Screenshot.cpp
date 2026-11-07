// Headless UI check: renders the Vocalyst editor (every tab / mode) and the Auto Key editor to PNG.
// Usage: VocalystScreenshot <output-dir>

#include "../Source/Plugin/VocalystProcessor.h"
#include "../Source/Plugin/VocalystEditor.h"
#include "../Source/AutoKey/AutoKeyProcessor.h"
#include "../Source/AutoKey/AutoKeyEditor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class ScreenshotApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "VocalystScreenshot"; }
    const juce::String getApplicationVersion() override { return "1.0"; }

    static void save (juce::Component& c, const juce::File& f)
    {
        auto img = c.createComponentSnapshot (c.getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        juce::FileOutputStream os (f);
        if (os.openedOk()) { os.setPosition (0); os.truncate(); png.writeImageToStream (img, os); }
        std::cout << "wrote " << f.getFullPathName() << "\n";
    }

    void initialise (const juce::String& cmd) override
    {
        juce::File dir (cmd.trim().unquoted());
        if (cmd.trim().isEmpty()) dir = juce::File::getCurrentWorkingDirectory();
        dir.createDirectory();

        vocalyst::VocalystProcessor proc;
        proc.prepareToPlay (44100.0, 512);
        // feed some audio so meters / analyzer / key detector have content
        juce::AudioBuffer<float> buf (2, 512); juce::MidiBuffer midi; double ph = 0.0; juce::Random rng;
        for (int b = 0; b < 300; ++b)
        {
            for (int i = 0; i < 512; ++i) { const float v = 0.3f * (float) std::sin (ph) + 0.03f * (rng.nextFloat() - 0.5f); ph += 2.0 * juce::MathConstants<double>::pi * 233.0 / 44100.0; buf.setSample (0, i, v); buf.setSample (1, i, v); }
            proc.processBlock (buf, midi);
        }
        proc.loadFactoryPreset (vocalyst::presets::indexOf ("Pop - Dry Intimate"));

        auto window = std::make_unique<juce::DocumentWindow> ("Vocalyst", juce::Colours::black, 0);
        auto* editor = new vocalyst::VocalystEditor (proc);
        window->setContentOwned (editor, true);
        window->setVisible (true);

        proc.setSimpleMode (true);  editor->showTab (1); editor->setSimpleMode (true);  save (*editor, dir.getChildFile ("vocalyst_mix_simple.png"));
        proc.setSimpleMode (false); editor->setSimpleMode (false);                    save (*editor, dir.getChildFile ("vocalyst_mix_advanced.png"));
        proc.setUiProperty ("meters", true); editor->setMetersVisible (true);            save (*editor, dir.getChildFile ("vocalyst_meters.png"));
        editor->setMetersVisible (false);
        editor->showTab (0);  save (*editor, dir.getChildFile ("vocalyst_presets.png"));
        editor->showTab (2);  save (*editor, dir.getChildFile ("vocalyst_community.png"));
        editor->showTab (2);  save (*editor, dir.getChildFile ("vocalyst_tune.png"));
        editor->showTab (1);  editor->openAnalyze(); save (*editor, dir.getChildFile ("vocalyst_analyze.png"));
        window->clearContentComponent();
        window.reset();

        vocalyst::AutoKeyProcessor ak;
        ak.prepareToPlay (44100.0, 512);
        for (int b = 0; b < 300; ++b)
        {
            for (int i = 0; i < 512; ++i)
            {
                const double t = (b * 512 + i) / 44100.0;
                float v = 0.f;
                for (double f : { 220.0, 261.63, 329.63, 440.0 }) v += 0.1f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * f * t);
                buf.setSample (0, i, v); buf.setSample (1, i, v);
            }
            ak.processBlock (buf, midi);
        }
        auto akWindow = std::make_unique<juce::DocumentWindow> ("Auto Key", juce::Colours::black, 0);
        auto* akEditor = new vocalyst::AutoKeyEditor (ak);
        akWindow->setContentOwned (akEditor, true);
        akWindow->setVisible (true);
        save (*akEditor, dir.getChildFile ("autokey.png"));
        akWindow->clearContentComponent();
        akWindow.reset();
        quit();
    }

    void shutdown() override {}
};

START_JUCE_APPLICATION (ScreenshotApp)
