#include "MeterStrip.h"

namespace vocalyst::ui
{

MeterStrip::MeterStrip (VocalystProcessor& p) : processor (p)
{
    inSmooth.fill (-100.f); outSmooth.fill (-100.f);
    widthSmooth.fill (0.f); corrSmooth.fill (1.f);
}

void MeterStrip::timerCallback()
{
    processor.meters().getSnapshot (snap);
    hasData = snap.valid;
    for (size_t k = 0; k < inSmooth.size(); ++k)
    {
        // instant rise, ~25 dB/s fall
        inSmooth[k]  = snap.inDb[k]  > inSmooth[k]  ? snap.inDb[k]  : juce::jmax (snap.inDb[k],  inSmooth[k]  - 0.9f);
        outSmooth[k] = snap.outDb[k] > outSmooth[k] ? snap.outDb[k] : juce::jmax (snap.outDb[k], outSmooth[k] - 0.9f);
    }
    for (size_t b = 0; b < widthSmooth.size(); ++b)
    {
        widthSmooth[b] += 0.25f * (snap.width[b] - widthSmooth[b]);
        corrSmooth[b]  += 0.25f * (snap.corr[b]  - corrSmooth[b]);
    }
    repaint();
}

float MeterStrip::xForHz (float hz, juce::Rectangle<float> a) const
{
    const float t = std::log (juce::jlimit (30.f, 20000.f, hz) / 30.f) / std::log (20000.f / 30.f);
    return a.getX() + t * a.getWidth();
}

void MeterStrip::paint (juce::Graphics& g)
{
    if (! hasData)   // first paint before the timer has ticked
    {
        processor.meters().getSnapshot (snap);
        hasData = snap.valid;
        if (hasData) { inSmooth = snap.inDb; outSmooth = snap.outDb; widthSmooth = snap.width; corrSmooth = snap.corr; }
    }
    auto bounds = getLocalBounds().toFloat();
    g.setColour (colours::panel());
    g.fillRoundedRectangle (bounds, 10.f);
    g.setColour (colours::outline());
    g.drawRoundedRectangle (bounds.reduced (0.5f), 10.f, 1.f);

    auto r = getLocalBounds().reduced (14, 8);
    auto head = r.removeFromTop (16);
    g.setColour (colours::textDim());
    g.setFont (font (10.5f, true));
    g.drawText ("SPECTRUM  -  input (grey) vs output (violet)", head.removeFromLeft (360), juce::Justification::centredLeft);
    const juce::String loud = "IN " + juce::String (processor.dryLoudnessDb(), 1) + " dB   OUT " + juce::String (processor.wetLoudnessDb(), 1) + " dB"
                            + (processor.isMatchOn() ? "   MATCH " + juce::String (processor.matchGainDbNow(), 1) + " dB" : "");
    g.drawText (loud, head, juce::Justification::centredRight);
    r.removeFromTop (4);

    auto imager = r.removeFromRight (juce::jmax (260, r.getWidth() * 36 / 100));
    r.removeFromRight (16);
    auto spec = r.toFloat();

    // ---- spectrum grid
    g.setColour (colours::bg());
    g.fillRoundedRectangle (spec, 6.f);
    const float dbTop = 0.f, dbBottom = -90.f;
    auto yForDb = [&] (float db) { return spec.getY() + (dbTop - juce::jlimit (dbBottom, dbTop, db)) / (dbTop - dbBottom) * spec.getHeight(); };
    g.setFont (font (9.5f));
    for (float f : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f })
    {
        const float x = xForHz (f, spec);
        g.setColour (colours::outline().withAlpha (0.6f));
        g.drawVerticalLine ((int) x, spec.getY(), spec.getBottom());
        g.setColour (colours::textDim());
        g.drawText (f >= 1000.f ? juce::String (f / 1000.f, 0) + "k" : juce::String ((int) f), (int) x + 3, (int) spec.getBottom() - 13, 40, 12, juce::Justification::centredLeft);
    }
    for (float db : { -12.f, -24.f, -36.f, -48.f, -60.f, -72.f })
    {
        g.setColour (colours::outline().withAlpha (0.4f));
        g.drawHorizontalLine ((int) yForDb (db), spec.getX(), spec.getRight());
        g.setColour (colours::textDim());
        g.drawText (juce::String ((int) db), (int) spec.getX() + 2, (int) yForDb (db) - 11, 30, 11, juce::Justification::centredLeft);
    }

    if (hasData)
    {
        const double binHz = snap.sampleRate / (double) MeterEngine::fftSize;
        auto buildPath = [&] (const std::array<float, MeterEngine::numBins>& data, bool closed)
        {
            juce::Path path;
            bool started = false;
            for (int k = 1; k < MeterEngine::numBins; ++k)
            {
                const float f = (float) (k * binHz);
                if (f < 30.f || f > 20000.f) continue;
                const float x = xForHz (f, spec), y = yForDb (data[(size_t) k]);
                if (! started) { path.startNewSubPath (closed ? x : x, closed ? spec.getBottom() : y); if (closed) path.lineTo (x, y); started = true; }
                else path.lineTo (x, y);
            }
            if (closed && started) { path.lineTo (spec.getRight(), spec.getBottom()); path.closeSubPath(); }
            return path;
        };
        g.setColour (colours::textDim().withAlpha (0.28f));
        g.fillPath (buildPath (inSmooth, true));
        g.setColour (colours::accent());
        g.strokePath (buildPath (outSmooth, false), juce::PathStrokeType (1.6f));
    }
    else
    {
        g.setColour (colours::textDim());
        g.setFont (font (12.f));
        g.drawText ("Play audio to see the spectrum", spec, juce::Justification::centred);
    }

    // ---- stereo imager: width per band
    g.setColour (colours::textDim());
    g.setFont (font (10.5f, true));
    auto imHead = imager.removeFromTop (14);
    g.drawText ("STEREO WIDTH PER BAND", imHead.removeFromLeft (170), juce::Justification::centredLeft);
    g.drawText ("corr " + juce::String (snap.overallCorr, 2), imHead, juce::Justification::centredRight);
    imager.removeFromTop (2);
    const int rowH = imager.getHeight() / MeterEngine::numBands;
    for (int b = MeterEngine::numBands - 1; b >= 0; --b)
    {
        auto row = imager.removeFromTop (rowH);
        auto label = row.removeFromLeft (52);
        g.setColour (colours::textDim());
        g.setFont (font (9.5f));
        const float lo = MeterEngine::bandLowHz (b), hi = MeterEngine::bandHighHz (b);
        auto fmt = [] (float f) { return f >= 1000.f ? juce::String (f / 1000.f, 0) + "k" : juce::String ((int) f); };
        g.drawText (fmt (lo) + "-" + fmt (hi), label, juce::Justification::centredLeft);
        auto track = row.reduced (0, juce::jmax (2, rowH / 4)).toFloat();
        g.setColour (colours::bg());
        g.fillRoundedRectangle (track, 3.f);
        g.setColour (colours::outline());
        g.drawVerticalLine ((int) track.getCentreX(), track.getY(), track.getBottom());
        const float extent = juce::jlimit (0.f, 1.f, widthSmooth[(size_t) b] * 2.f);   // 0 = mono, 1 = fully wide
        const float half = extent * track.getWidth() * 0.5f;
        const bool outOfPhase = corrSmooth[(size_t) b] < -0.1f;
        g.setColour (outOfPhase ? colours::danger() : colours::accent2().withAlpha (0.85f));
        g.fillRoundedRectangle (track.getCentreX() - half, track.getY(), juce::jmax (2.f, half * 2.f), track.getHeight(), 3.f);
    }
}

} // namespace vocalyst::ui
