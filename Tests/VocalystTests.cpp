// Offline test runner: registry/preset integrity, DSP stability, pitch detection & correction,
// key detection, analyzer, state round-trip. Run: ./VocalystTests  (exit code 0 = all passed)

#include "../Source/Plugin/VocalystProcessor.h"
#include "../Source/DSP/PitchDetector.h"
#include "../Source/Core/KeyDetector.h"
#include "../Source/Core/MeterEngine.h"
#include <juce_events/juce_events.h>
#include <iostream>
#include <cmath>
#include <set>
#include <algorithm>

using namespace vocalyst;

static int failures = 0;
#define CHECK(cond, msg) do { if (cond) std::cout << "  [ok]   " << msg << "\n"; else { std::cout << "  [FAIL] " << msg << "\n"; ++failures; } } while (0)

static void fillSine (juce::AudioBuffer<float>& b, double hz, double sr, double& phase, float amp = 0.3f)
{
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const float v = amp * (float) std::sin (phase);
        for (int ch = 0; ch < b.getNumChannels(); ++ch) b.getWritePointer (ch)[i] = v;
        phase += 2.0 * juce::MathConstants<double>::pi * hz / sr;
        if (phase > 2.0 * juce::MathConstants<double>::pi) phase -= 2.0 * juce::MathConstants<double>::pi;
    }
}

static float measurePitch (const std::vector<float>& signal, double sr, int skipSamples)
{
    dsp::PitchDetector det;
    det.prepare (sr);
    std::vector<float> estimates;
    for (size_t i = 0; i < signal.size(); ++i)
    {
        det.push (signal[i]);
        if (det.hasNewEstimate() && (int) i > skipSamples && det.frequencyHz() > 0.f && det.confidence() > 0.6f)
            estimates.push_back (det.frequencyHz());
    }
    if (estimates.empty()) return 0.f;
    std::sort (estimates.begin(), estimates.end());
    return estimates[estimates.size() / 2];
}

static void testRegistry (VocalystProcessor& p)
{
    std::cout << "Registry\n";
    CHECK (allModules().size() == 27, "27 effect modules registered");
    std::set<juce::String> keys;
    for (auto& m : allModules()) keys.insert (m.key);
    CHECK (keys.size() == 27, "module keys are unique");
    bool ordered = true;
    for (int i = 0; i < kNumModules; ++i) if ((int) allModules()[(size_t) i].id != i) ordered = false;
    CHECK (ordered, "registry order matches ModuleId enum");
    bool allFound = true;
    int count = 0;
    for (auto& m : allModules())
    {
        if (p.apvts.getParameter (enableId (m.id)) == nullptr) allFound = false;
        for (auto& ps : m.params) { ++count; if (p.apvts.getParameter (paramId (m.id, ps.key)) == nullptr) { allFound = false; std::cout << "    missing " << paramId (m.id, ps.key) << "\n"; } }
    }
    CHECK (allFound, "every module parameter exists in the APVTS (" + juce::String (count) + " params)");
}

static void testPresets (VocalystProcessor& p)
{
    std::cout << "Presets\n";
    auto& f = presets::factory();
    CHECK (f.size() == 98, "98 factory presets: 2 init + 59 main + 37 ad-lib (" + juce::String ((int) f.size()) + ")");
    CHECK (presets::count (false) == 61, "61 main vocal presets incl. 2 Init");
    CHECK (f[0].name == "Init - Empty" && f[0].chain.empty(), "Init - Empty is first and has no modules");
    CHECK (f[1].name == "Init - Clean Chain" && f[1].chain.size() == 4, "Init - Clean Chain has 4 transparent modules");
    CHECK (presets::count (true) == 37, "37 ad-lib presets");
    bool idsOk = true, chainOk = true, namesOk = true;
    std::set<juce::String> names;
    for (auto& pr : f)
    {
        if (! names.insert (pr.name).second) namesOk = false;
        if (pr.chain.empty() && pr.genre != "Init") chainOk = false;
        for (auto& c : pr.chain) if (findModuleByKey (c) == nullptr) chainOk = false;
        for (auto& kv : pr.values) if (p.apvts.getParameter (kv.first) == nullptr) { idsOk = false; std::cout << "    bad id " << kv.first << " in " << pr.name << "\n"; }
    }
    CHECK (namesOk, "preset names are unique");
    CHECK (chainOk, "preset chains reference valid modules");
    CHECK (idsOk, "preset values reference valid parameters");
    CHECK (presets::genres (false).contains ("Pop") && presets::genres (false).contains ("Rage"), "genre list includes Pop and Rage");

    // load every preset: chain order and values applied
    bool loadOk = true;
    for (int i = 0; i < (int) f.size(); ++i)
    {
        p.loadFactoryPreset (i);
        if (p.getChainOrder().size() != f[(size_t) i].chain.size()) loadOk = false;
        if (p.getCurrentPresetName() != f[(size_t) i].name) loadOk = false;
    }
    CHECK (loadOk, "all presets load with the expected chain length");
    p.loadFactoryPreset (presets::indexOf ("Pop - Radio Ready"));
    CHECK (std::abs (p.apvts.getRawParameterValue ("tune_speed")->load() - 30.f) < 0.01f, "preset value applied (Pop tune speed = 30 ms)");

    // JSON round trip
    auto cap = p.captureCurrentPreset ("RT", "Test", "d", "me", false);
    bool ok = false;
    auto back = Preset::fromJsonString (cap.toJsonString(), &ok);
    CHECK (ok && back.chain == cap.chain && back.values.size() == cap.values.size(), "preset JSON round trip");
}

static void testChainStability (VocalystProcessor& p)
{
    std::cout << "DSP stability (all 27 modules active)\n";
    const double sr = 44100.0; const int block = 512;
    p.prepareToPlay (sr, block);
    std::vector<ModuleId> all;
    for (int i = 0; i < kNumModules; ++i) all.push_back ((ModuleId) i);
    p.setChainOrder (all);
    for (auto id : all) p.apvts.getParameter (enableId (id))->setValueNotifyingHost (1.f);
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    juce::Random rng (1);
    double phase = 0.0;
    bool finite = true; float energy = 0.f;
    for (int b = 0; b < 200; ++b)
    {
        fillSine (buf, 220.0, sr, phase, 0.3f);
        for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < block; ++i) buf.getWritePointer (ch)[i] += 0.05f * (rng.nextFloat() * 2.f - 1.f);
        p.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < block; ++i) { const float v = buf.getReadPointer (ch)[i]; if (! std::isfinite (v)) finite = false; if (b > 50) energy += v * v; }
    }
    CHECK (finite, "output is finite");
    CHECK (energy > 1.0e-3f, "output carries signal");
    CHECK (p.currentLatencySamples() > 0.f, "latency reported while Tune is active (" + juce::String ((int) p.currentLatencySamples()) + " smp)");

    // silence in -> (near) silence out
    p.prepareToPlay (sr, block);
    float maxOut = 0.f;
    for (int b = 0; b < 100; ++b)
    {
        buf.clear();
        p.processBlock (buf, midi);
        if (b > 60) maxOut = juce::jmax (maxOut, buf.getMagnitude (0, block));
    }
    CHECK (maxOut < 1.0e-3f, "silence stays silent (" + juce::String (maxOut, 6) + ")");
}

static void testEachModule (VocalystProcessor& p)
{
    std::cout << "Each module alone passes signal\n";
    const double sr = 44100.0; const int block = 512;
    bool allOk = true;
    for (int id = 0; id < kNumModules; ++id)
    {
        p.prepareToPlay (sr, block);
        p.setChainOrder ({ (ModuleId) id });
        p.apvts.getParameter (enableId ((ModuleId) id))->setValueNotifyingHost (1.f);
        juce::AudioBuffer<float> buf (2, block);
        juce::MidiBuffer midi;
        juce::Random rng (3);
        double phase = 0.0;
        float energy = 0.f; bool finite = true;
        for (int b = 0; b < 120; ++b)
        {
            fillSine (buf, 220.0, sr, phase, 0.3f);
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < block; ++i) buf.getWritePointer (ch)[i] += 0.02f * (rng.nextFloat() * 2.f - 1.f);
            p.processBlock (buf, midi);
            for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < block; ++i) { const float v = buf.getReadPointer (ch)[i]; if (! std::isfinite (v)) finite = false; if (b > 40) energy += v * v; }
        }
        const float rms = std::sqrt (energy / (80.f * block * 2.f));
        const bool ok = finite && rms > 0.01f && rms < 1.5f;
        if (! ok) { allOk = false; std::cout << "    " << moduleSpec ((ModuleId) id).name << " rms=" << rms << " finite=" << finite << "\n"; }
    }
    CHECK (allOk, "all 27 modules produce finite, non-silent output on their own");
}

static void testPitchDetector()
{
    std::cout << "Pitch detector\n";
    const double sr = 44100.0;
    std::vector<float> sig ((size_t) sr * 2);
    double phase = 0.0;
    for (auto& v : sig) { v = 0.3f * (float) (2.0 * (phase / (2.0 * juce::MathConstants<double>::pi)) - 1.0); phase += 2.0 * juce::MathConstants<double>::pi * 220.0 / sr; if (phase > 2.0 * juce::MathConstants<double>::pi) phase -= 2.0 * juce::MathConstants<double>::pi; }
    const float f = measurePitch (sig, sr, 4410);
    CHECK (std::abs (f - 220.f) < 2.5f, "220 Hz sawtooth detected as " + juce::String (f, 2) + " Hz");

    std::vector<float> hi ((size_t) sr);
    phase = 0.0;
    for (auto& v : hi) { v = 0.3f * (float) std::sin (phase); phase += 2.0 * juce::MathConstants<double>::pi * 660.0 / sr; }
    const float f2 = measurePitch (hi, sr, 4410);
    CHECK (std::abs (f2 - 660.f) < 7.f, "660 Hz sine detected as " + juce::String (f2, 2) + " Hz");
}

static void testTune (VocalystProcessor& p)
{
    std::cout << "Tune (pitch correction)\n";
    const double sr = 44100.0; const int block = 256;
    p.prepareToPlay (sr, block);
    p.setChainOrder ({ ModuleId::Tune });
    p.apvts.getParameter ("tune_on")->setValueNotifyingHost (1.f);
    p.apvts.getParameter ("tune_amount")->setValueNotifyingHost (1.f);
    p.apvts.getParameter ("tune_speed")->setValueNotifyingHost (0.f);
    p.apvts.getParameter (ids::scale)->setValueNotifyingHost (0.f); // chromatic
    p.apvts.getParameter (ids::mix)->setValueNotifyingHost (1.f);

    // 226 Hz is 46 cents above A3 (220 Hz): expect the output to land on 220 Hz
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    std::vector<float> out;
    double phase = 0.0;
    for (int b = 0; b < 400; ++b)
    {
        fillSine (buf, 226.0, sr, phase, 0.4f);
        p.processBlock (buf, midi);
        for (int i = 0; i < block; ++i) out.push_back (buf.getReadPointer (0)[i]);
    }
    const float f = measurePitch (out, sr, (int) sr);
    CHECK (std::abs (f - 220.f) < 2.0f, "226 Hz input corrected to " + juce::String (f, 2) + " Hz (target 220)");

    // amount 0 -> untouched
    p.apvts.getParameter ("tune_amount")->setValueNotifyingHost (0.f);
    out.clear();
    for (int b = 0; b < 400; ++b)
    {
        fillSine (buf, 226.0, sr, phase, 0.4f);
        p.processBlock (buf, midi);
        for (int i = 0; i < block; ++i) out.push_back (buf.getReadPointer (0)[i]);
    }
    const float f0 = measurePitch (out, sr, (int) sr);
    CHECK (std::abs (f0 - 226.f) < 2.0f, "amount 0 % leaves pitch at " + juce::String (f0, 2) + " Hz");
    p.apvts.getParameter ("tune_amount")->setValueNotifyingHost (1.f);
}

static void testKeyDetector()
{
    std::cout << "Key detector\n";
    const double sr = 44100.0;
    KeyDetector det;
    det.prepare (sr);
    // C major progression: C - F - G - C triads (with a couple of harmonics each), 1 s per chord
    const int chords[4][3] = { { 60, 64, 67 }, { 65, 69, 72 }, { 67, 71, 74 }, { 60, 64, 67 } };
    std::vector<float> sig;
    for (auto& ch : chords)
    {
        for (int i = 0; i < (int) sr; ++i)
        {
            float v = 0.f;
            for (int n : ch)
            {
                const double f = 440.0 * std::pow (2.0, (n - 69) / 12.0);
                v += 0.15f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * f * i / sr);
                v += 0.05f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * 2.0 * f * i / sr);
            }
            sig.push_back (v);
        }
    }
    det.push (sig.data(), (int) sig.size());
    auto r = det.estimate();
    CHECK (r.valid, "estimate available after 4 s");
    CHECK (r.key == 0 && r.scale == scales::Major, "C major progression detected as " + scales::keyName (r.key, r.scale) + " (" + juce::String ((int) (r.confidence * 100)) + "%)");

    // A minor: Am - Dm - E - Am
    det.reset();
    const int minor[4][3] = { { 57, 60, 64 }, { 62, 65, 69 }, { 64, 68, 71 }, { 57, 60, 64 } };
    sig.clear();
    for (auto& ch : minor)
        for (int i = 0; i < (int) sr; ++i)
        {
            float v = 0.f;
            for (int n : ch) { const double f = 440.0 * std::pow (2.0, (n - 69) / 12.0); v += 0.15f * (float) std::sin (2.0 * juce::MathConstants<double>::pi * f * i / sr); }
            sig.push_back (v);
        }
    det.push (sig.data(), (int) sig.size());
    r = det.estimate();
    CHECK (r.valid && r.key == 9 && r.scale == scales::Minor, "A minor progression detected as " + scales::keyName (r.key, r.scale));

    // shared key bridge
    CHECK (sharedkey::write (9, scales::Minor, 0.8f, "test"), "shared key file written");
    auto info = sharedkey::read();
    CHECK (info.valid && info.key == 9 && info.scale == scales::Minor, "shared key file read back");
}

static void testScales()
{
    std::cout << "Scales\n";
    CHECK (scales::nearestScaleNote (60.4f, 0, scales::Major) == 60, "60.4 -> C in C major");
    CHECK (scales::nearestScaleNote (61.0f, 0, scales::Major) == 60 || scales::nearestScaleNote (61.0f, 0, scales::Major) == 62, "C# snaps to a C major note");
    CHECK (scales::nearestScaleNote (61.0f, 0, scales::Chromatic) == 61, "chromatic keeps C#");
    CHECK (scales::nearestScaleNote (63.4f, 9, scales::Minor) == 64, "63.4 in A minor -> E");
}

static void testAnalyzer (VocalystProcessor& p)
{
    std::cout << "Analyzer\n";
    const double sr = 44100.0; const int block = 512;
    p.prepareToPlay (sr, block);
    p.analyzer().reset();
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    juce::Random rng (7);
    double phase = 0.0;
    for (int b = 0; b < 200; ++b)
    {
        fillSine (buf, 200.0, sr, phase, 0.2f);
        for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < block; ++i) buf.getWritePointer (ch)[i] += 0.1f * (rng.nextFloat() * 2.f - 1.f);
        p.processBlock (buf, midi);
    }
    auto st = p.analyzer().snapshot();
    CHECK (st.valid, "stats valid after ~2 s of audio");
    auto sug = VocalAnalyzer::suggest (st);
    CHECK (sug.size() >= 3, juce::String ((int) sug.size()) + " suggestions produced");
    const auto before = p.getChainOrder().size();
    p.setChainOrder ({ ModuleId::Tune, ModuleId::Limiter });
    p.applySuggestions (sug);
    CHECK (p.isInChain (ModuleId::LowCut) && p.isInChain (ModuleId::Compressor), "suggestions add missing modules to the chain");
    CHECK (p.getChainOrder().back() == ModuleId::Limiter, "limiter stays last after applying suggestions");
    juce::ignoreUnused (before);
}

static void testNewFeatures (VocalystProcessor& p)
{
    std::cout << "Analyzer gating, note mask, bypass, loudness match, MIDI\n";
    const double sr = 44100.0; const int block = 512;
    p.prepareToPlay (sr, block);
    p.setChainOrder ({ ModuleId::Tune });
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;

    // analyzer must not count silence
    p.analyzer().reset();
    for (int b = 0; b < 100; ++b) { buf.clear(); p.processBlock (buf, midi); }
    CHECK (p.analyzer().snapshot().seconds < 0.01f, "silence adds no analysed seconds");
    double phase = 0.0;
    for (int b = 0; b < 100; ++b) { fillSine (buf, 220.0, sr, phase, 0.3f); p.processBlock (buf, midi); }
    const float secs = p.analyzer().snapshot().seconds;
    CHECK (secs > 0.9f && secs < 1.4f, "signal adds analysed seconds (" + juce::String (secs, 2) + " s)");

    // custom note mask: only A allowed -> 233 Hz (A#3) is pulled down to 220 Hz
    p.apvts.getParameter ("tune_on")->setValueNotifyingHost (1.f);
    p.apvts.getParameter ("tune_amount")->setValueNotifyingHost (1.f);
    p.apvts.getParameter ("tune_speed")->setValueNotifyingHost (0.f);
    p.setNoteMask (1 << 9);
    CHECK (p.getNoteMask() == (1 << 9), "note mask stored in parameters");
    std::vector<float> out;
    for (int b = 0; b < 400; ++b) { fillSine (buf, 233.08, sr, phase, 0.4f); p.processBlock (buf, midi); for (int i = 0; i < block; ++i) out.push_back (buf.getReadPointer (0)[i]); }
    const float f = measurePitch (out, sr, (int) sr);
    CHECK (std::abs (f - 220.f) < 2.5f, "mask {A}: 233 Hz corrected to " + juce::String (f, 2) + " Hz");
    p.setKey (0, scales::Chromatic);
    CHECK (p.getNoteMask() == 0xFFF, "selecting a scale rebuilds the mask");

    // transpose +12 moves a 220 Hz note to 440 Hz
    p.apvts.getParameter ("tune_transpose")->setValueNotifyingHost (p.apvts.getParameter ("tune_transpose")->convertTo0to1 (12.f));
    out.clear();
    for (int b = 0; b < 400; ++b) { fillSine (buf, 220.0, sr, phase, 0.4f); p.processBlock (buf, midi); for (int i = 0; i < block; ++i) out.push_back (buf.getReadPointer (0)[i]); }
    const float ft = measurePitch (out, sr, (int) sr);
    CHECK (std::abs (ft - 440.f) < 6.f, "transpose +12: 220 Hz -> " + juce::String (ft, 1) + " Hz");
    p.apvts.getParameter ("tune_transpose")->setValueNotifyingHost (p.apvts.getParameter ("tune_transpose")->convertTo0to1 (0.f));

    // MIDI held notes are tracked
    midi.addEvent (juce::MidiMessage::noteOn (1, 69, (juce::uint8) 100), 0);
    buf.clear(); p.processBlock (buf, midi); midi.clear();
    CHECK (p.heldMidiMask() == (1 << 9), "held MIDI note A reported in the MIDI mask");
    midi.addEvent (juce::MidiMessage::noteOff (1, 69), 0);
    p.processBlock (buf, midi); midi.clear();
    CHECK (p.heldMidiMask() == 0, "note off clears the MIDI mask");

    // bypass passes the input through untouched (no latency when Tune is off)
    p.setChainOrder ({ ModuleId::Compressor });
    p.prepareToPlay (sr, block);
    p.apvts.getParameter ("comp_makeup")->setValueNotifyingHost (p.apvts.getParameter ("comp_makeup")->convertTo0to1 (12.f));
    p.apvts.getParameter (ids::bypass)->setValueNotifyingHost (1.f);
    float maxDiff = 0.f;
    for (int b = 0; b < 50; ++b)
    {
        fillSine (buf, 220.0, sr, phase, 0.3f);
        juce::AudioBuffer<float> copy; copy.makeCopyOf (buf);
        p.processBlock (buf, midi);
        if (b > 10) for (int i = 0; i < block; ++i) maxDiff = juce::jmax (maxDiff, std::abs (buf.getReadPointer (0)[i] - copy.getReadPointer (0)[i]));
    }
    CHECK (maxDiff < 1.0e-4f, "bypass passes the input through (max diff " + juce::String (maxDiff, 6) + ")");
    p.apvts.getParameter (ids::bypass)->setValueNotifyingHost (0.f);

    // loudness match pulls a +12 dB chain back to the input level
    p.apvts.getParameter (ids::match)->setValueNotifyingHost (1.f);
    float inRms = 0.f, outRms = 0.f;
    for (int b = 0; b < 400; ++b)
    {
        fillSine (buf, 220.0, sr, phase, 0.3f);
        p.processBlock (buf, midi);
        if (b > 300) { inRms += 0.3f * 0.3f * 0.5f; outRms += buf.getRMSLevel (0, 0, block) * buf.getRMSLevel (0, 0, block); }
    }
    const float diffDb = 10.f * std::log10 (outRms / inRms);
    CHECK (std::abs (diffDb) < 1.5f, "loudness match keeps output within 1.5 dB of input (" + juce::String (diffDb, 2) + " dB)");
    p.apvts.getParameter (ids::match)->setValueNotifyingHost (0.f);
    p.apvts.getParameter ("comp_makeup")->setValueNotifyingHost (p.apvts.getParameter ("comp_makeup")->getDefaultValue());

    // meters produce a spectrum
    MeterEngine::Snapshot snap;
    p.meters().getSnapshot (snap);
    CHECK (snap.valid && snap.outDb[(size_t) std::lround (220.0 * MeterEngine::fftSize / sr)] > -30.f, "spectrum analyzer shows the 220 Hz tone");
}

static void testState()
{
    std::cout << "State\n";
    VocalystProcessor a;
    CHECK (a.getCurrentPresetName() == "Init - Clean Chain", "new instance starts on Init - Clean Chain");
    a.loadFactoryPreset (5);
    a.setChainOrder ({ ModuleId::Reverb, ModuleId::Gate, ModuleId::Tune });
    a.setKey (6, scales::Minor);
    a.setSimpleMode (false);
    a.apvts.getParameter ("reverb_mix")->setValueNotifyingHost (0.77f);
    juce::MemoryBlock mb;
    a.getStateInformation (mb);

    VocalystProcessor b;
    b.setStateInformation (mb.getData(), (int) mb.getSize());
    const auto order = b.getChainOrder();
    CHECK (order.size() == 3 && order[0] == ModuleId::Reverb && order[1] == ModuleId::Gate && order[2] == ModuleId::Tune, "chain order restored");
    CHECK (b.getKey() == 6 && b.getScale() == scales::Minor, "key restored");
    CHECK (! b.isSimpleMode(), "mode restored");
    CHECK (std::abs (b.apvts.getParameter ("reverb_mix")->getValue() - 0.77f) < 1.0e-4f, "parameter restored");
    CHECK (b.getCurrentPresetName() == a.getCurrentPresetName(), "preset name restored");
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;   // message manager for AudioProcessor / parameter attachments
    std::cout << "Vocalyst test runner\n";
    {
        VocalystProcessor p;
        testRegistry (p);
        testPresets (p);
        testChainStability (p);
        testEachModule (p);
        testTune (p);
        testAnalyzer (p);
        testNewFeatures (p);
    }
    testPitchDetector();
    testKeyDetector();
    testScales();
    testState();
    std::cout << (failures == 0 ? "\nALL TESTS PASSED\n" : "\n" + juce::String (failures) + " TEST(S) FAILED\n");
    return failures == 0 ? 0 : 1;
}
