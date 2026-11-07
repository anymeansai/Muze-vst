#include "VocalAnalyzer.h"

namespace vocalyst
{
using namespace dsp;

static float bandEdge (int i) { return 50.f * std::pow (2.f, (float) i / 3.f); }   // 30 bands of 1/3 octave from 50 Hz
float VocalAnalyzer::bandCentreHz (int band) { return std::sqrt (bandEdge (band) * bandEdge (band + 1)); }

void VocalAnalyzer::prepare (double sampleRate)
{
    sr = sampleRate;
    lowLp.set (120.f, 0.707f, sr);
    mudBp.set (280.f, 0.8f, sr);
    presBp.set (3500.f, 0.8f, sr);
    sibBp.set (7000.f, 0.9f, sr);
    airHp.set (10000.f, 0.707f, sr);
    frameLen = (int) (sr * 0.025);
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * (float) i / (float) fftSize);
    for (int b = 0; b < numBands; ++b)
    {
        binLo[(size_t) b] = juce::jmax (1, (int) std::floor (bandEdge (b) * fftSize / sr));
        binHi[(size_t) b] = juce::jmin (fftSize / 2 - 1, (int) std::floor (bandEdge (b + 1) * fftSize / sr));
        if (binHi[(size_t) b] < binLo[(size_t) b]) binHi[(size_t) b] = binLo[(size_t) b];
    }
    reset();
}

void VocalAnalyzer::reset()
{
    lowLp.reset(); mudBp.reset(); presBp.reset(); sibBp.reset(); airHp.reset();
    framePos = 0; eTot = eLow = eMud = ePres = eSib = eAir = framePeak = 0.f; lowSlow = 0.f; stRms = 0.f;
    sumSq = sumLow = sumMud = sumPres = sumSib = sumAir = 0.f;
    peak = 0.f; noiseFloor = 1.f; audibleFrames = 0.f; activeFrames = 0.f; plosiveFrames = 0.f;
    for (auto& h : hist) h.store (0);
    for (auto& b : bandPower) b.store (0.f);
    spectrumFrames.store (0);
    fifo.fill (0.f); fifoPos = 0;
}

void VocalAnalyzer::push (const float* mono, int n)
{
    for (int i = 0; i < n; ++i)
    {
        const float x = mono[i];
        const float l = lowLp.lowpass (x), m = mudBp.bandpass (x), pr = presBp.bandpass (x), s = sibBp.bandpass (x), a = airHp.highpass (x);
        eTot += x * x; eLow += l * l; eMud += m * m; ePres += pr * pr; eSib += s * s; eAir += a * a;
        framePeak = juce::jmax (framePeak, std::abs (x));
        fifo[(size_t) fifoPos] = x;
        if (++fifoPos >= fftSize) { analyseSpectrum(); fifoPos = 0; }
        if (++framePos >= frameLen) finishFrame();
    }
}

void VocalAnalyzer::finishFrame()
{
    const float power = eTot / (float) frameLen;
    const float rms = std::sqrt (power);
    const float a = 1.f - std::exp (-(float) frameLen / (0.4f * (float) sr));
    stRms += a * (power - stRms);

    if (rms > 1.0e-4f)                      // above -80 dBFS: something audible is playing
    {
        audibleFrames.store (audibleFrames.load() + 1.f);
        noiseFloor.store (juce::jmin (noiseFloor.load(), rms));
    }
    if (rms > 0.01f)                        // above -40 dBFS: the singer is active
    {
        activeFrames.store (activeFrames.load() + 1.f);
        sumSq.store (sumSq.load() + eTot);
        sumLow.store (sumLow.load() + eLow);
        sumMud.store (sumMud.load() + eMud);
        sumPres.store (sumPres.load() + ePres);
        sumSib.store (sumSib.load() + eSib);
        sumAir.store (sumAir.load() + eAir);
        const float stDb = gainToDb (std::sqrt (stRms));
        const int bin = juce::jlimit (0, histBins - 1, (int) ((stDb + 90.f) * 2.f));
        hist[(size_t) bin].fetch_add (1);
        // plosive: low band jumps 10 dB above its own slow average within one frame
        const float lowRms = std::sqrt (eLow / (float) frameLen);
        if (lowSlow > 1.0e-5f && lowRms > lowSlow * 3.16f && lowRms > 0.05f) plosiveFrames.store (plosiveFrames.load() + 1.f);
        lowSlow += 0.1f * (lowRms - lowSlow);
    }
    peak.store (juce::jmax (peak.load(), framePeak));
    framePos = 0; eTot = eLow = eMud = ePres = eSib = eAir = framePeak = 0.f;
}

void VocalAnalyzer::analyseSpectrum()
{
    float energy = 0.f;
    for (int i = 0; i < fftSize; ++i) { const float v = fifo[(size_t) i] * window[(size_t) i]; fftData[(size_t) i] = v; energy += v * v; }
    if (energy / fftSize < 1.0e-5f) return;   // only frames where the singer is active (about -40 dBFS)
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.f);
    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);
    for (int b = 0; b < numBands; ++b)
    {
        float pw = 0.f;
        for (int k = binLo[(size_t) b]; k <= binHi[(size_t) b]; ++k) pw += fftData[(size_t) k] * fftData[(size_t) k];
        pw /= (float) (binHi[(size_t) b] - binLo[(size_t) b] + 1);
        bandPower[(size_t) b].store (bandPower[(size_t) b].load() + pw);
    }
    spectrumFrames.fetch_add (1);
}

VocalAnalyzer::Stats VocalAnalyzer::snapshot() const
{
    Stats s;
    const float af = activeFrames.load();
    s.seconds = audibleFrames.load() * (float) frameLen / (float) sr;
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
    s.sibilanceLevelDb = gainToDb (std::sqrt (sumSib.load() / (af * (float) frameLen)));
    s.plosiveRatio = plosiveFrames.load() / af;

    // loudness percentiles from the histogram
    int total = 0;
    for (auto& h : hist) total += h.load();
    auto percentile = [&] (float pc)
    {
        int target = (int) (pc * (float) total), acc = 0;
        for (int i = 0; i < histBins; ++i) { acc += hist[(size_t) i].load(); if (acc >= target) return (float) i * 0.5f - 90.f; }
        return 0.f;
    };
    if (total > 0) { s.loudP10 = percentile (0.10f); s.loudP50 = percentile (0.50f); s.loudP90 = percentile (0.90f); }
    s.dynamicRangeDb = s.loudP90 - s.loudP10;

    // long-term spectrum (dB, normalised so the 300 Hz - 3 kHz core sits around 0 dB)
    const int frames = spectrumFrames.load();
    if (frames > 2)
    {
        float core = 0.f; int coreN = 0;
        for (int b = 0; b < numBands; ++b)
        {
            const float db = gainToDb (std::sqrt (bandPower[(size_t) b].load() / (float) frames));
            s.spectrumDb[(size_t) b] = db;
            const float fc = bandCentreHz (b);
            if (fc > 300.f && fc < 3000.f) { core += db; ++coreN; }
        }
        const float ref = coreN > 0 ? core / (float) coreN : 0.f;
        for (auto& v : s.spectrumDb) v -= ref;
        for (int b = 0; b < numBands; ++b)    // 5-band moving average = the broad trend
        {
            float acc = 0.f; int n = 0;
            for (int k = b - 2; k <= b + 2; ++k) if (k >= 0 && k < numBands) { acc += s.spectrumDb[(size_t) k]; ++n; }
            s.trendDb[(size_t) b] = acc / (float) n;
        }
        // slope between 200 Hz and 8 kHz by least squares on log2(f)
        float sx = 0.f, sy = 0.f, sxx = 0.f, sxy = 0.f; int n = 0;
        for (int b = 0; b < numBands; ++b)
        {
            const float fc = bandCentreHz (b);
            if (fc < 200.f || fc > 8000.f) continue;
            const float x = std::log2 (fc), y = s.spectrumDb[(size_t) b];
            sx += x; sy += y; sxx += x * x; sxy += x * y; ++n;
        }
        if (n > 2) s.tiltDbPerOct = (sxy - sx * sy / (float) n) / juce::jmax (1.0e-6f, sxx - sx * sx / (float) n);
    }
    return s;
}

std::vector<VocalAnalyzer::Suggestion> VocalAnalyzer::suggest (const Stats& s)
{
    std::vector<Suggestion> out;
    if (! s.valid) return out;
    auto db1 = [] (float v) { return juce::String (v, 1); };
    auto dbi = [] (float v) { return juce::String ((int) std::round (v)); };

    // peak in the long-term spectrum between two frequencies, relative to the broad trend
    auto bump = [&] (float lo, float hi, float& atHz, float& aboveTrendDb)
    {
        atHz = 0.f; aboveTrendDb = -100.f;
        for (int b = 0; b < numBands; ++b)
        {
            const float fc = bandCentreHz (b);
            if (fc < lo || fc > hi) continue;
            const float d = s.spectrumDb[(size_t) b] - s.trendDb[(size_t) b];
            if (d > aboveTrendDb) { aboveTrendDb = d; atHz = fc; }
        }
    };

    // 1. Low cut
    {
        Suggestion g; g.title = "Low Cut";
        float f = 80.f;
        juce::String why = "Energy below 120 Hz is " + juce::String ((int) (s.lowRatio * 100.f)) + " % of the vocal";
        if (s.plosiveRatio > 0.03f) { f = 120.f; why += " and " + juce::String ((int) (s.plosiveRatio * 100.f)) + " % of frames show plosive thumps"; }
        else if (s.lowRatio > 0.12f) { f = 110.f; why += ", which is rumble, not voice"; }
        else if (s.lowRatio > 0.05f) { f = 95.f; }
        g.text = why + ". High-pass at " + dbi (f) + " Hz keeps the body of the voice and removes what the microphone picked up below it.";
        g.values = { { "lowcut_freq", f } };
        g.modules = { "lowcut" };
        out.push_back (g);
    }
    // 2. Gate
    {
        Suggestion g; g.title = "Gate";
        if (s.noiseFloorDb > -70.f && s.loudP10 > s.noiseFloorDb + 12.f)
        {
            const float th = juce::jlimit (-70.f, -25.f, juce::jmin (s.noiseFloorDb + 10.f, s.loudP10 - 6.f));
            g.text = "Room noise sits around " + dbi (s.noiseFloorDb) + " dB while the quietest sung phrases reach " + dbi (s.loudP10)
                   + " dB. A gate threshold at " + dbi (th) + " dB (10 dB above the noise, well under the quietest singing) mutes the gaps without clipping word endings.";
            g.values = { { "gate_thresh", th }, { "gate_release", 150.f } };
            g.modules = { "gate" };
            out.push_back (g);
        }
        else
        {
            g.text = "Noise floor is " + dbi (s.noiseFloorDb) + " dB, far below the singing. No gate needed; the chain's gate is set to a safe -70 dB.";
            g.values = { { "gate_thresh", -70.f } };
            out.push_back (g);
        }
    }
    // 3. De-esser
    if (s.sibilanceRatio > 0.05f)
    {
        Suggestion g; g.title = "De-Esser";
        const float amt = s.sibilanceRatio > 0.16f ? 14.f : (s.sibilanceRatio > 0.09f ? 10.f : 6.f);
        const float th = juce::jlimit (-50.f, -12.f, s.sibilanceLevelDb + 6.f);
        g.text = "The 5-9 kHz band carries " + juce::String ((int) (s.sibilanceRatio * 100.f)) + " % of the energy (above 5 % is sharp S and T sounds). De-ess at 6.5 kHz, threshold "
               + dbi (th) + " dB (6 dB above the band's average so only the loud S's trigger), up to " + dbi (amt) + " dB of reduction.";
        g.values = { { "deess_freq", 6500.f }, { "deess_amount", amt }, { "deess_thresh", th } };
        g.modules = { "deess" };
        out.push_back (g);
    }
    // 4. Compressor
    {
        Suggestion g; g.title = "Compressor";
        const float dr = s.dynamicRangeDb;
        const float ratio = dr < 8.f ? 2.5f : (dr < 14.f ? 4.f : (dr < 20.f ? 6.f : 8.f));
        const float th = juce::jlimit (-50.f, -8.f, s.loudP50 - 2.f);
        const float attack = s.crestDb > 14.f ? 10.f : 3.f;
        const float release = dr > 14.f ? 120.f : 80.f;
        const float makeup = juce::jlimit (0.f, 14.f, -18.f - (s.loudP50 - (s.loudP90 - th) * (1.f - 1.f / ratio) * 0.5f));
        g.text = "Loud phrases hit " + dbi (s.loudP90) + " dB and quiet ones " + dbi (s.loudP10) + " dB, a " + dbi (dr) + " dB spread. "
               + juce::String (ratio, 1) + ":1 at " + dbi (th) + " dB (just under the typical phrase level) with a " + dbi (attack) + " ms attack"
               + (s.crestDb > 14.f ? " to let the consonants through" : " to grab the peaks") + ", " + dbi (release) + " ms release and "
               + db1 (makeup) + " dB makeup lands the vocal near -18 dBFS average" + (dr > 14.f ? ". The Glue stage then evens out the rest." : ".");
        g.values = { { "comp_thresh", th }, { "comp_ratio", ratio }, { "comp_attack", attack }, { "comp_release", release }, { "comp_makeup", makeup } };
        g.modules = { "comp" };
        if (dr > 14.f) { g.values.push_back ({ "glue_thresh", th - 4.f }); g.values.push_back ({ "glue_ratio", 2.f }); g.modules.push_back ("glue"); }
        out.push_back (g);
    }
    // 5. Tone / EQ from the long-term spectrum
    {
        Suggestion g; g.title = "EQ";
        juce::StringArray notes;
        float lowGain = 0.f, mid1Gain = 0.f, mid1Freq = 280.f, mid1Q = 1.2f, mid2Gain = 0.f, mid2Freq = 3500.f, mid2Q = 1.f, highGain = 0.f, air = 0.f;

        float resHz, resDb;            // narrow resonance anywhere in the body of the voice
        bump (150.f, 1500.f, resHz, resDb);
        if (resDb > 7.f)
        {
            mid1Freq = resHz; mid1Gain = -juce::jlimit (3.f, 5.f, resDb * 0.5f); mid1Q = 3.f;
            notes.add ("a resonance at " + dbi (resHz) + " Hz sticks out " + db1 (resDb) + " dB above the rest, cut " + db1 (-mid1Gain) + " dB with a narrow band");
        }
        else
        {
            float mudHz, mudDb; bump (200.f, 500.f, mudHz, mudDb);
            if (mudDb > 2.5f || s.mudRatio > 0.35f)
            {
                mid1Freq = mudHz > 0.f ? mudHz : 280.f; mid1Gain = -3.f; mid1Q = 1.2f;
                notes.add ("low-mids around " + dbi (mid1Freq) + " Hz are " + db1 (juce::jmax (mudDb, 2.5f)) + " dB over the trend (boxy), cut 3 dB");
            }
        }
        float harshHz, harshDb; bump (2500.f, 5000.f, harshHz, harshDb);
        if (harshDb > 3.f)
        {
            mid2Freq = harshHz; mid2Gain = -2.5f; mid2Q = 1.5f;
            notes.add ("harshness at " + dbi (harshHz) + " Hz (" + db1 (harshDb) + " dB over the trend), cut 2.5 dB");
        }
        else if (s.tiltDbPerOct < -8.f || s.presenceRatio < 0.08f)
        {
            mid2Freq = 3500.f; mid2Gain = 2.5f; mid2Q = 0.9f;
            notes.add ("the spectrum falls " + db1 (-s.tiltDbPerOct) + " dB per octave (dull; a clear vocal is around 6), lift presence 2.5 dB at 3.5 kHz");
        }
        if (s.tiltDbPerOct < -7.f || s.airRatio < 0.01f) { air = 4.f; notes.add ("add 4 dB of air above 12 kHz"); }
        else if (s.tiltDbPerOct > -3.f) { highGain = -2.f; notes.add ("the top end is " + db1 (s.tiltDbPerOct + 6.f) + " dB brighter than a natural voice, shelve 2 dB off above 9 kHz"); }
        if (s.spectrumDb[(size_t) 4] < s.trendDb[(size_t) 6] - 4.f && s.lowRatio < 0.05f) { lowGain = 1.5f; notes.add ("thin below 200 Hz, 1.5 dB low shelf for warmth"); }

        g.text = notes.isEmpty() ? "The long-term spectrum follows a natural vocal curve (" + db1 (-s.tiltDbPerOct) + " dB per octave). EQ left flat."
                                 : "Long-term spectrum: " + notes.joinIntoString ("; ") + ".";
        g.values = { { "eq_lowgain", lowGain }, { "eq_mid1gain", mid1Gain }, { "eq_mid1freq", mid1Freq }, { "eq_mid1q", mid1Q },
                     { "eq_mid2gain", mid2Gain }, { "eq_mid2freq", mid2Freq }, { "eq_mid2q", mid2Q }, { "eq_highgain", highGain }, { "eq_highfreq", 9000.f } };
        g.modules = { "eq" };
        if (air > 0.f) { g.values.push_back ({ "air_amount", air }); g.modules.push_back ("air"); }
        out.push_back (g);
    }
    return out;
}

} // namespace vocalyst
