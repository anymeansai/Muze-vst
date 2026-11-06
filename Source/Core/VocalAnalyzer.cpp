#include "VocalAnalyzer.h"

namespace muze
{
using namespace dsp;

void VocalAnalyzer::prepare (double sampleRate)
{
    sr = sampleRate;
    lowLp.set (120.f, 0.707f, sr);
    mudBp.set (280.f, 0.8f, sr);
    presBp.set (3500.f, 0.8f, sr);
    sibBp.set (7000.f, 0.9f, sr);
    airHp.set (10000.f, 0.707f, sr);
    frameLen = (int) (sr * 0.025);
    reset();
}

void VocalAnalyzer::reset()
{
    lowLp.reset(); mudBp.reset(); presBp.reset(); sibBp.reset(); airHp.reset();
    framePos = 0; eTot = eLow = eMud = ePres = eSib = eAir = framePeak = 0.f;
    sumSq = sumLow = sumMud = sumPres = sumSib = sumAir = 0.f;
    peak = 0.f; noiseFloor = 1.f; frames = 0.f; activeFrames = 0.f;
}

void VocalAnalyzer::push (const float* mono, int n)
{
    for (int i = 0; i < n; ++i)
    {
        const float x = mono[i];
        const float l = lowLp.lowpass (x), m = mudBp.bandpass (x), pr = presBp.bandpass (x), s = sibBp.bandpass (x), a = airHp.highpass (x);
        eTot += x * x; eLow += l * l; eMud += m * m; ePres += pr * pr; eSib += s * s; eAir += a * a;
        framePeak = juce::jmax (framePeak, std::abs (x));
        if (++framePos >= frameLen) finishFrame();
    }
}

void VocalAnalyzer::finishFrame()
{
    const float rms = std::sqrt (eTot / (float) frameLen);
    frames.store (frames.load() + 1.f);
    if (rms > 1.0e-4f) // above -80 dBFS: count towards the noise floor
        noiseFloor.store (juce::jmin (noiseFloor.load(), rms));
    if (rms > 0.01f)    // above -40 dBFS: the singer is active
    {
        activeFrames.store (activeFrames.load() + 1.f);
        sumSq.store (sumSq.load() + eTot);
        sumLow.store (sumLow.load() + eLow);
        sumMud.store (sumMud.load() + eMud);
        sumPres.store (sumPres.load() + ePres);
        sumSib.store (sumSib.load() + eSib);
        sumAir.store (sumAir.load() + eAir);
    }
    peak.store (juce::jmax (peak.load(), framePeak));
    framePos = 0; eTot = eLow = eMud = ePres = eSib = eAir = framePeak = 0.f;
}

VocalAnalyzer::Stats VocalAnalyzer::snapshot() const
{
    Stats s;
    const float af = activeFrames.load();
    s.seconds = frames.load() * (float) frameLen / (float) sr;
    if (af < 8.f) return s;   // need at least ~0.2 s of singing
    const float tot = juce::jmax (1.0e-12f, sumSq.load());
    s.valid = true;
    s.rmsDb = gainToDb (std::sqrt (tot / (af * (float) frameLen)));
    s.peakDb = gainToDb (peak.load());
    s.noiseFloorDb = noiseFloor.load() < 1.f ? gainToDb (noiseFloor.load()) : -100.f;
    s.crestDb = s.peakDb - s.rmsDb;
    s.lowRatio = sumLow.load() / tot;
    s.mudRatio = sumMud.load() / tot;
    s.presenceRatio = sumPres.load() / tot;
    s.sibilanceRatio = sumSib.load() / tot;
    s.airRatio = sumAir.load() / tot;
    return s;
}

std::vector<VocalAnalyzer::Suggestion> VocalAnalyzer::suggest (const Stats& s)
{
    std::vector<Suggestion> out;
    if (! s.valid) return out;

    // Low end
    {
        Suggestion g;
        const float f = s.lowRatio > 0.12f ? 130.f : (s.lowRatio > 0.05f ? 100.f : 80.f);
        g.text = s.lowRatio > 0.12f ? "Heavy low-end rumble detected. Low Cut raised to " + juce::String ((int) f) + " Hz."
                                    : "Low Cut set to " + juce::String ((int) f) + " Hz to keep the bottom clean.";
        g.values = { { "lowcut_freq", f } };
        g.modules = { "lowcut" };
        out.push_back (g);
    }
    // Noise / gate
    if (s.noiseFloorDb > -70.f)
    {
        Suggestion g;
        const float th = juce::jlimit (-70.f, -25.f, s.noiseFloorDb + 8.f);
        g.text = "Room noise around " + juce::String ((int) s.noiseFloorDb) + " dB. Gate threshold set to " + juce::String ((int) th) + " dB.";
        g.values = { { "gate_thresh", th } };
        g.modules = { "gate" };
        out.push_back (g);
    }
    // Sibilance
    if (s.sibilanceRatio > 0.08f)
    {
        Suggestion g;
        const float amt = s.sibilanceRatio > 0.16f ? 14.f : 9.f;
        g.text = "Sharp S sounds detected. De-Esser engaged with " + juce::String ((int) amt) + " dB of range.";
        g.values = { { "deess_amount", amt }, { "deess_thresh", juce::jlimit (-50.f, -15.f, s.rmsDb - 6.f) } };
        g.modules = { "deess" };
        out.push_back (g);
    }
    // Dynamics
    {
        Suggestion g;
        const float ratio = s.crestDb > 20.f ? 6.f : (s.crestDb > 14.f ? 4.f : 3.f);
        const float th = juce::jlimit (-50.f, -8.f, s.rmsDb + 2.f);
        const float makeup = juce::jlimit (0.f, 12.f, -14.f - s.rmsDb);
        g.text = "Dynamic range of " + juce::String ((int) s.crestDb) + " dB. Compressor set to " + juce::String ((int) ratio)
               + ":1 at " + juce::String ((int) th) + " dB with " + juce::String ((int) makeup) + " dB makeup.";
        g.values = { { "comp_thresh", th }, { "comp_ratio", ratio }, { "comp_makeup", makeup } };
        g.modules = { "comp" };
        out.push_back (g);
    }
    // Tone
    {
        Suggestion g;
        float mud = 0.f, pres = 0.f, air = 0.f;
        juce::StringArray notes;
        if (s.mudRatio > 0.35f) { mud = -3.f; notes.add ("boxy low-mids cut 3 dB at 280 Hz"); }
        if (s.presenceRatio < 0.08f) { pres = 2.5f; notes.add ("presence lifted 2.5 dB at 3.5 kHz"); }
        if (s.airRatio < 0.01f) { air = 4.f; notes.add ("air added above 12 kHz"); }
        if (notes.isEmpty()) { g.text = "Tonal balance looks healthy. EQ left flat."; }
        else g.text = "EQ: " + notes.joinIntoString (", ") + ".";
        g.values = { { "eq_mid1gain", mud }, { "eq_mid1freq", 280.f }, { "eq_mid2gain", pres }, { "eq_mid2freq", 3500.f } };
        g.modules = { "eq" };
        if (air > 0.f) { g.values.push_back ({ "air_amount", air }); g.modules.push_back ("air"); }
        out.push_back (g);
    }
    return out;
}

} // namespace muze
