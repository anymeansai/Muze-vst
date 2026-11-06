#include "ModuleRegistry.h"
#include <juce_graphics/juce_graphics.h>
#include <algorithm>

namespace muze
{

static std::vector<ModuleSpec> buildRegistry()
{
    using C = ModuleCategory;
    std::vector<ModuleSpec> m;
    m.reserve (kNumModules);

    // ---- Clean -------------------------------------------------------------
    m.push_back ({ ModuleId::Gate, "gate", "Gate", C::Clean,
        "Removes room noise and bleed between phrases.",
        { { "thresh",  "Threshold", -80.f, 0.f,   -45.f, 1.f,  "dB", 0, nullptr },
          { "release", "Release",    5.f,  1000.f, 120.f, 0.4f, "ms", 0, nullptr },
          { "range",   "Range",     -80.f, 0.f,   -80.f, 1.f,  "dB", 0, nullptr } } });

    m.push_back ({ ModuleId::Tune, "tune", "Tune", C::Clean,
        "Real-time pitch correction locked to the song key.",
        { { "amount",   "Amount",   0.f, 100.f, 100.f, 1.f,  "%",  0, nullptr },
          { "speed",    "Speed",    0.f, 400.f, 25.f,  0.5f, "ms", 0, nullptr },
          { "humanize", "Humanize", 0.f, 100.f, 0.f,   1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::LowCut, "lowcut", "Low Cut", C::Clean,
        "High-pass filter that clears rumble and mud below the voice.",
        { { "freq", "Frequency", 20.f, 1000.f, 90.f,  0.4f, "Hz", 0, nullptr },
          { "q",    "Resonance", 0.5f, 2.f,    0.71f, 1.f,  "",   0, nullptr } } });

    m.push_back ({ ModuleId::HighCut, "highcut", "High Cut", C::Clean,
        "Low-pass filter that tames harsh top end.",
        { { "freq", "Frequency", 1000.f, 20000.f, 16000.f, 0.4f, "Hz", 0, nullptr },
          { "q",    "Resonance", 0.5f,   2.f,     0.71f,   1.f,  "",   0, nullptr } } });

    m.push_back ({ ModuleId::DeEsser, "deess", "De-Esser", C::Clean,
        "Tames sharp S and T sounds.",
        { { "freq",   "Frequency", 3000.f, 12000.f, 6500.f, 0.5f, "Hz", 0, nullptr },
          { "thresh", "Threshold", -60.f,  0.f,     -30.f,  1.f,  "dB", 0, nullptr },
          { "amount", "Amount",    0.f,    24.f,    10.f,   1.f,  "dB", 0, nullptr } } });

    // ---- Tone --------------------------------------------------------------
    m.push_back ({ ModuleId::EQ, "eq", "EQ", C::Tone,
        "Four-band parametric equalizer.",
        { { "lowgain",  "Low Gain",   -12.f,  12.f,    0.f,    1.f,  "dB", 0, nullptr },
          { "lowfreq",  "Low Freq",    40.f,  500.f,   150.f,  0.5f, "Hz", 0, nullptr },
          { "mid1gain", "Mid 1 Gain", -12.f,  12.f,    0.f,    1.f,  "dB", 0, nullptr },
          { "mid1freq", "Mid 1 Freq",  150.f, 5000.f,  700.f,  0.4f, "Hz", 0, nullptr },
          { "mid1q",    "Mid 1 Q",     0.3f,  5.f,     1.f,    0.5f, "",   0, nullptr },
          { "mid2gain", "Mid 2 Gain", -12.f,  12.f,    0.f,    1.f,  "dB", 0, nullptr },
          { "mid2freq", "Mid 2 Freq",  1000.f, 12000.f, 3500.f, 0.4f, "Hz", 0, nullptr },
          { "mid2q",    "Mid 2 Q",     0.3f,  5.f,     1.f,    0.5f, "",   0, nullptr },
          { "highgain", "High Gain",  -12.f,  12.f,    0.f,    1.f,  "dB", 0, nullptr },
          { "highfreq", "High Freq",   3000.f, 18000.f, 9000.f, 0.5f, "Hz", 0, nullptr } } });

    m.push_back ({ ModuleId::Air, "air", "Air", C::Tone,
        "Silky high-shelf lift for openness and sparkle.",
        { { "freq",   "Frequency", 6000.f, 16000.f, 12000.f, 1.f, "Hz", 0, nullptr },
          { "amount", "Amount",    0.f,    12.f,    4.f,     1.f, "dB", 0, nullptr } } });

    // ---- Dynamics ----------------------------------------------------------
    m.push_back ({ ModuleId::Compressor, "comp", "Compressor", C::Dynamics,
        "Fast peak compressor that evens out the performance.",
        { { "thresh",  "Threshold", -60.f, 0.f,    -20.f, 1.f,  "dB", 0, nullptr },
          { "ratio",   "Ratio",     1.f,   20.f,   4.f,   0.5f, ":1", 0, nullptr },
          { "attack",  "Attack",    0.1f,  100.f,  5.f,   0.3f, "ms", 0, nullptr },
          { "release", "Release",   10.f,  1000.f, 80.f,  0.4f, "ms", 0, nullptr },
          { "makeup",  "Makeup",    0.f,   24.f,   0.f,   1.f,  "dB", 0, nullptr },
          { "mix",     "Mix",       0.f,   100.f,  100.f, 1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Glue, "glue", "Glue Comp", C::Dynamics,
        "Slow, smooth RMS compressor that glues the vocal together.",
        { { "thresh",  "Threshold", -60.f, 0.f,    -24.f, 1.f,  "dB", 0, nullptr },
          { "ratio",   "Ratio",     1.f,   10.f,   2.f,   0.5f, ":1", 0, nullptr },
          { "attack",  "Attack",    1.f,   100.f,  15.f,  0.4f, "ms", 0, nullptr },
          { "release", "Release",   50.f,  1500.f, 250.f, 0.4f, "ms", 0, nullptr },
          { "makeup",  "Makeup",    0.f,   24.f,   0.f,   1.f,  "dB", 0, nullptr } } });

    m.push_back ({ ModuleId::Limiter, "limiter", "Limiter", C::Dynamics,
        "Catches peaks so the vocal sits loud and safe.",
        { { "gain",    "Gain",    0.f,   12.f,  0.f,   1.f,  "dB", 0, nullptr },
          { "ceiling", "Ceiling", -12.f, 0.f,   -0.5f, 1.f,  "dB", 0, nullptr },
          { "release", "Release", 5.f,   500.f, 60.f,  0.4f, "ms", 0, nullptr } } });

    m.push_back ({ ModuleId::Transient, "transient", "Transient", C::Dynamics,
        "Shapes the attack and sustain of consonants and words.",
        { { "attack",  "Attack",  -100.f, 100.f, 0.f, 1.f, "%", 0, nullptr },
          { "sustain", "Sustain", -100.f, 100.f, 0.f, 1.f, "%", 0, nullptr } } });

    // ---- Color -------------------------------------------------------------
    m.push_back ({ ModuleId::Saturation, "sat", "Saturation", C::Color,
        "Warm tape/tube style harmonics.",
        { { "drive", "Drive", 0.f,    100.f, 30.f,  1.f, "%", 0, nullptr },
          { "tone",  "Tone",  -100.f, 100.f, 0.f,   1.f, "",  0, nullptr },
          { "mix",   "Mix",   0.f,    100.f, 100.f, 1.f, "%", 0, nullptr } } });

    m.push_back ({ ModuleId::Distortion, "dist", "Distortion", C::Color,
        "Aggressive clipping for rage and hyperpop textures.",
        { { "drive", "Drive", 0.f,    100.f,   40.f,   1.f,  "%",  0, nullptr },
          { "tone",  "Tone",  1000.f, 20000.f, 8000.f, 0.4f, "Hz", 0, nullptr },
          { "mix",   "Mix",   0.f,    100.f,   50.f,   1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Exciter, "exciter", "Exciter", C::Color,
        "Adds brand new harmonics above the chosen frequency.",
        { { "freq",   "Frequency", 2000.f, 12000.f, 5000.f, 0.5f, "Hz", 0, nullptr },
          { "amount", "Amount",    0.f,    100.f,   30.f,   1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::LoFi, "lofi", "Lo-Fi", C::Color,
        "Bit-crushing and sample-rate reduction.",
        { { "bits", "Bits",  2.f,    16.f,    8.f,     1.f,  "bit", 1, nullptr },
          { "rate", "Rate",  1000.f, 48000.f, 12000.f, 0.4f, "Hz",  0, nullptr },
          { "mix",  "Mix",   0.f,    100.f,   100.f,   1.f,  "%",   0, nullptr } } });

    m.push_back ({ ModuleId::Telephone, "phone", "Telephone", C::Color,
        "Narrow band-pass with grit for phone and radio effects.",
        { { "low",   "Low",   200.f,  1500.f, 500.f,  0.5f, "Hz", 0, nullptr },
          { "high",  "High",  1500.f, 6000.f, 3400.f, 0.5f, "Hz", 0, nullptr },
          { "drive", "Drive", 0.f,    100.f,  20.f,   1.f,  "%",  0, nullptr },
          { "mix",   "Mix",   0.f,    100.f,  100.f,  1.f,  "%",  0, nullptr } } });

    // ---- Modulation --------------------------------------------------------
    m.push_back ({ ModuleId::Doubler, "doubler", "Doubler", C::Modulation,
        "Two detuned copies for a wide, layered vocal.",
        { { "detune", "Detune", 0.f, 50.f,  12.f,  1.f, "ct", 0, nullptr },
          { "delay",  "Delay",  5.f, 60.f,  20.f,  1.f, "ms", 0, nullptr },
          { "width",  "Width",  0.f, 100.f, 100.f, 1.f, "%",  0, nullptr },
          { "mix",    "Mix",    0.f, 100.f, 40.f,  1.f, "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Harmonizer, "harmony", "Harmonizer", C::Modulation,
        "Pitch-shifted layer, octave down for weight or up for shine.",
        { { "interval", "Interval", -12.f,  12.f,  -12.f, 1.f, "st", 1, nullptr },
          { "mix",      "Mix",      0.f,    100.f, 30.f,  1.f, "%",  0, nullptr },
          { "pan",      "Pan",      -100.f, 100.f, 0.f,   1.f, "",   0, nullptr } } });

    m.push_back ({ ModuleId::Chorus, "chorus", "Chorus", C::Modulation,
        "Classic shimmering chorus.",
        { { "rate",  "Rate",  0.05f, 5.f,   0.6f, 0.4f, "Hz", 0, nullptr },
          { "depth", "Depth", 0.f,   100.f, 35.f, 1.f,  "%",  0, nullptr },
          { "mix",   "Mix",   0.f,   100.f, 30.f, 1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Flanger, "flanger", "Flanger", C::Modulation,
        "Jet-like sweeping comb filter.",
        { { "rate",     "Rate",     0.05f, 5.f,   0.3f, 0.4f, "Hz", 0, nullptr },
          { "depth",    "Depth",    0.f,   100.f, 50.f, 1.f,  "%",  0, nullptr },
          { "feedback", "Feedback", -95.f, 95.f,  40.f, 1.f,  "%",  0, nullptr },
          { "mix",      "Mix",      0.f,   100.f, 30.f, 1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Phaser, "phaser", "Phaser", C::Modulation,
        "Swirling all-pass phaser.",
        { { "rate",     "Rate",     0.05f, 5.f,    0.4f,   0.4f, "Hz", 0, nullptr },
          { "depth",    "Depth",    0.f,   100.f,  60.f,   1.f,  "%",  0, nullptr },
          { "center",   "Center",   200.f, 5000.f, 1000.f, 0.4f, "Hz", 0, nullptr },
          { "feedback", "Feedback", -95.f, 95.f,   20.f,   1.f,  "%",  0, nullptr },
          { "mix",      "Mix",      0.f,   100.f,  40.f,   1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Tremolo, "tremolo", "Tremolo", C::Modulation,
        "Rhythmic volume modulation.",
        { { "rate",  "Rate",  0.1f, 20.f,  5.f,  0.5f, "Hz", 0, nullptr },
          { "depth", "Depth", 0.f,  100.f, 50.f, 1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::AutoPan, "autopan", "Auto-Pan", C::Modulation,
        "Moves the vocal across the stereo field.",
        { { "rate",  "Rate",  0.05f, 10.f,  0.5f, 0.4f, "Hz", 0, nullptr },
          { "depth", "Depth", 0.f,   100.f, 70.f, 1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Widener, "widener", "Widener", C::Modulation,
        "Mid/side stereo width with mono-safe low end.",
        { { "width",   "Width",    0.f, 200.f, 130.f, 1.f, "%",  0, nullptr },
          { "lowmono", "Low Mono", 0.f, 300.f, 120.f, 1.f, "Hz", 0, nullptr } } });

    // ---- Space -------------------------------------------------------------
    m.push_back ({ ModuleId::Slap, "slap", "Slap", C::Space,
        "Short slapback echo for vintage depth.",
        { { "time",     "Time",     40.f,   200.f,   110.f,  1.f,  "ms", 0, nullptr },
          { "feedback", "Feedback", 0.f,    60.f,    10.f,   1.f,  "%",  0, nullptr },
          { "tone",     "Tone",     1000.f, 16000.f, 5000.f, 0.4f, "Hz", 0, nullptr },
          { "mix",      "Mix",      0.f,    100.f,   20.f,   1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Delay, "delay", "Delay", C::Space,
        "Tempo-synced stereo delay with ducking.",
        { { "sync",     "Sync",     0.f,    6.f,     3.f,    1.f,  "",   1, "Free,1/4,1/8,1/8 Dotted,1/16,1/4 Triplet,1/8 Triplet" },
          { "time",     "Time",     20.f,   2000.f,  375.f,  0.4f, "ms", 0, nullptr },
          { "feedback", "Feedback", 0.f,    95.f,    35.f,   1.f,  "%",  0, nullptr },
          { "spread",   "Spread",   0.f,    100.f,   100.f,  1.f,  "%",  0, nullptr },
          { "tone",     "Tone",     1000.f, 20000.f, 6000.f, 0.4f, "Hz", 0, nullptr },
          { "lowcut",   "Low Cut",  20.f,   2000.f,  200.f,  0.4f, "Hz", 0, nullptr },
          { "duck",     "Duck",     0.f,    100.f,   0.f,    1.f,  "%",  0, nullptr },
          { "mix",      "Mix",      0.f,    100.f,   25.f,   1.f,  "%",  0, nullptr } } });

    m.push_back ({ ModuleId::Reverb, "reverb", "Reverb", C::Space,
        "Lush plate/hall reverb with pre-delay and ducking.",
        { { "size",     "Size",      0.f,    100.f,   60.f,   1.f,  "%",  0, nullptr },
          { "decay",    "Decay",     0.f,    100.f,   50.f,   1.f,  "%",  0, nullptr },
          { "predelay", "Pre-Delay", 0.f,    200.f,   20.f,   1.f,  "ms", 0, nullptr },
          { "tone",     "Tone",      1000.f, 20000.f, 8000.f, 0.4f, "Hz", 0, nullptr },
          { "width",    "Width",     0.f,    100.f,   100.f,  1.f,  "%",  0, nullptr },
          { "duck",     "Duck",      0.f,    100.f,   0.f,    1.f,  "%",  0, nullptr },
          { "mix",      "Mix",       0.f,    100.f,   25.f,   1.f,  "%",  0, nullptr } } });

    std::sort (m.begin(), m.end(), [] (const ModuleSpec& a, const ModuleSpec& b) { return (int) a.id < (int) b.id; });
    jassert ((int) m.size() == kNumModules);
    for (int i = 0; i < (int) m.size(); ++i)
        jassert ((int) m[(size_t) i].id == i);
    return m;
}

const std::vector<ModuleSpec>& allModules()
{
    static const std::vector<ModuleSpec> registry = buildRegistry();
    return registry;
}

const ModuleSpec& moduleSpec (ModuleId id)
{
    return allModules()[(size_t) id];
}

const ModuleSpec* findModuleByKey (const juce::String& key)
{
    for (auto& m : allModules())
        if (key == m.key)
            return &m;
    return nullptr;
}

const ParamSpec* findParam (ModuleId id, const juce::String& paramKey)
{
    for (auto& p : moduleSpec (id).params)
        if (paramKey == p.key)
            return &p;
    return nullptr;
}

juce::String paramId (ModuleId id, const char* paramKey)
{
    return juce::String (moduleSpec (id).key) + "_" + paramKey;
}

juce::String enableId (ModuleId id)
{
    return juce::String (moduleSpec (id).key) + "_on";
}

const char* categoryName (ModuleCategory c)
{
    switch (c)
    {
        case ModuleCategory::Clean:      return "Clean";
        case ModuleCategory::Dynamics:   return "Dynamics";
        case ModuleCategory::Tone:       return "Tone";
        case ModuleCategory::Color:      return "Color";
        case ModuleCategory::Modulation: return "Modulation";
        case ModuleCategory::Space:      return "Space";
        case ModuleCategory::NumCategories:
        default:                         return "";
    }
}

juce::Colour categoryColour (ModuleCategory c)
{
    switch (c)
    {
        case ModuleCategory::Clean:      return juce::Colour (0xff22d3ee);
        case ModuleCategory::Dynamics:   return juce::Colour (0xfff59e0b);
        case ModuleCategory::Tone:       return juce::Colour (0xff4ade80);
        case ModuleCategory::Color:      return juce::Colour (0xfff43f5e);
        case ModuleCategory::Modulation: return juce::Colour (0xffa78bfa);
        case ModuleCategory::Space:      return juce::Colour (0xff60a5fa);
        case ModuleCategory::NumCategories:
        default:                         return juce::Colours::grey;
    }
}

} // namespace muze
