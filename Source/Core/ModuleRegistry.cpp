#include "ModuleRegistry.h"
#include <juce_graphics/juce_graphics.h>
#include <algorithm>

namespace vocalyst
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
        { { "amount",    "Amount",       0.f,   100.f, 100.f, 1.f,  "%",  0, nullptr },
          { "speed",     "Retune Speed", 0.f,   400.f, 25.f,  0.5f, "ms", 0, nullptr },
          { "humanize",  "Humanize",     0.f,   100.f, 0.f,   1.f,  "%",  0, nullptr },
          { "flex",      "Flex-Tune",    0.f,   100.f, 0.f,   1.f,  "ct", 0, nullptr },
          { "vibrato",   "Vibrato",      0.f,   200.f, 100.f, 1.f,  "%",  0, nullptr },
          { "transpose", "Transpose",    -12.f, 12.f,  0.f,   1.f,  "st", 1, nullptr },
          { "range",     "Input Range",  0.f,   4.f,   0.f,   1.f,  "",   1, "Auto,Soprano,Alto / Tenor,Low Male,Bass" },
          { "midi",      "MIDI Notes",   0.f,   1.f,   0.f,   1.f,  "",   1, "Off,On" } } });

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

    m.push_back ({ ModuleId::PopFilter, "pop", "Pop Filter", C::Clean,
        "Catches plosive thumps (P and B) and ducks only the low end while they last.",
        { { "sens",     "Sensitivity", 0.f,  100.f, 50.f,  1.f,  "%",  0, nullptr },
          { "strength", "Strength",    0.f,  100.f, 70.f,  1.f,  "%",  0, nullptr },
          { "release",  "Release",     20.f, 400.f, 120.f, 0.5f, "ms", 0, nullptr } } });

    m.push_back ({ ModuleId::Breath, "breath", "Breath Control", C::Clean,
        "Turns down breaths between phrases without touching the words.",
        { { "sens",      "Sensitivity", 0.f,  100.f, 50.f, 1.f,  "%",  0, nullptr },
          { "reduction", "Reduction",   0.f,  24.f,  12.f, 1.f,  "dB", 0, nullptr },
          { "speed",     "Speed",       20.f, 300.f, 80.f, 0.5f, "ms", 0, nullptr } } });

    // ---- per-module presets ---------------------------------------------
    auto presetsFor = [&m] (ModuleId id) -> std::vector<ModulePreset>& { for (auto& spec : m) if (spec.id == id) return spec.presets; return m.front().presets; };
    presetsFor (ModuleId::Gate)       = { { "Light",   { { "thresh", -60.f }, { "release", 200.f }, { "range", -30.f } } },
                                          { "Medium",  { { "thresh", -48.f }, { "release", 150.f }, { "range", -80.f } } },
                                          { "Tight",   { { "thresh", -38.f }, { "release", 80.f },  { "range", -80.f } } } };
    presetsFor (ModuleId::Tune)       = { { "Natural", { { "amount", 100.f }, { "speed", 120.f }, { "humanize", 40.f }, { "flex", 20.f } } },
                                          { "Tight",   { { "amount", 100.f }, { "speed", 20.f },  { "humanize", 0.f },  { "flex", 0.f } } },
                                          { "Hard",    { { "amount", 100.f }, { "speed", 0.f },   { "humanize", 0.f },  { "flex", 0.f } } },
                                          { "Subtle",  { { "amount", 60.f },  { "speed", 150.f }, { "humanize", 50.f }, { "flex", 30.f } } } };
    presetsFor (ModuleId::LowCut)     = { { "Gentle 70 Hz", { { "freq", 70.f } } }, { "Standard 90 Hz", { { "freq", 90.f } } }, { "Thin-out 120 Hz", { { "freq", 120.f } } }, { "Radio 180 Hz", { { "freq", 180.f } } } };
    presetsFor (ModuleId::HighCut)    = { { "Open 16 kHz", { { "freq", 16000.f } } }, { "Smooth 12 kHz", { { "freq", 12000.f } } }, { "Dark 8 kHz", { { "freq", 8000.f } } } };
    presetsFor (ModuleId::EQ)         = { { "Flat",      { { "lowgain", 0.f }, { "mid1gain", 0.f }, { "mid2gain", 0.f }, { "highgain", 0.f } } },
                                          { "Presence",  { { "lowgain", 0.f }, { "mid1gain", -1.5f }, { "mid1freq", 350.f }, { "mid2gain", 3.f }, { "mid2freq", 3500.f }, { "mid2q", 0.9f }, { "highgain", 1.5f } } },
                                          { "Warm",      { { "lowgain", 2.f }, { "lowfreq", 160.f }, { "mid1gain", 0.f }, { "mid2gain", 1.f }, { "highgain", -1.f } } },
                                          { "Air",       { { "lowgain", 0.f }, { "mid1gain", 0.f }, { "mid2gain", 1.f }, { "highgain", 3.f }, { "highfreq", 10000.f } } },
                                          { "De-mud",    { { "lowgain", -1.f }, { "mid1gain", -3.f }, { "mid1freq", 300.f }, { "mid1q", 1.2f }, { "mid2gain", 0.f }, { "highgain", 0.f } } } };
    presetsFor (ModuleId::Air)        = { { "Subtle", { { "amount", 2.f } } }, { "Open", { { "amount", 4.f } } }, { "Sparkle", { { "amount", 6.f }, { "freq", 10000.f } } } };
    presetsFor (ModuleId::DeEsser)    = { { "Gentle", { { "freq", 6500.f }, { "thresh", -30.f }, { "amount", 6.f } } },
                                          { "Medium", { { "freq", 6500.f }, { "thresh", -32.f }, { "amount", 10.f } } },
                                          { "Strong", { { "freq", 6500.f }, { "thresh", -35.f }, { "amount", 14.f } } },
                                          { "Bright voice", { { "freq", 7500.f }, { "thresh", -32.f }, { "amount", 10.f } } },
                                          { "Dark voice",   { { "freq", 5500.f }, { "thresh", -32.f }, { "amount", 10.f } } } };
    presetsFor (ModuleId::Compressor) = { { "Vocal Lead", { { "thresh", -20.f }, { "ratio", 4.f },  { "attack", 5.f },  { "release", 80.f },  { "makeup", 3.f }, { "mix", 100.f } } },
                                          { "Smooth",     { { "thresh", -24.f }, { "ratio", 2.5f }, { "attack", 15.f }, { "release", 150.f }, { "makeup", 2.f }, { "mix", 100.f } } },
                                          { "Punchy",     { { "thresh", -18.f }, { "ratio", 6.f },  { "attack", 1.f },  { "release", 60.f },  { "makeup", 4.f }, { "mix", 100.f } } },
                                          { "Aggressive", { { "thresh", -26.f }, { "ratio", 8.f },  { "attack", 2.f },  { "release", 50.f },  { "makeup", 6.f }, { "mix", 100.f } } },
                                          { "Parallel",   { { "thresh", -30.f }, { "ratio", 10.f }, { "attack", 1.f },  { "release", 80.f },  { "makeup", 8.f }, { "mix", 40.f } } } };
    presetsFor (ModuleId::Glue)       = { { "Light",  { { "thresh", -24.f }, { "ratio", 1.5f }, { "attack", 20.f }, { "release", 300.f }, { "makeup", 1.f } } },
                                          { "Medium", { { "thresh", -22.f }, { "ratio", 2.f },  { "attack", 15.f }, { "release", 250.f }, { "makeup", 2.f } } },
                                          { "Dense",  { { "thresh", -26.f }, { "ratio", 3.f },  { "attack", 10.f }, { "release", 200.f }, { "makeup", 3.f } } } };
    presetsFor (ModuleId::Limiter)    = { { "Safety", { { "gain", 0.f }, { "ceiling", -0.5f } } }, { "Loud", { { "gain", 3.f }, { "ceiling", -0.5f } } }, { "Brickwall", { { "gain", 6.f }, { "ceiling", -1.f } } } };
    presetsFor (ModuleId::Transient)  = { { "Snap", { { "attack", 40.f }, { "sustain", 0.f } } }, { "Soften", { { "attack", -30.f }, { "sustain", 0.f } } }, { "Sustain", { { "attack", 0.f }, { "sustain", 30.f } } } };
    presetsFor (ModuleId::Saturation) = { { "Subtle", { { "drive", 12.f }, { "tone", 0.f }, { "mix", 100.f } } }, { "Tape", { { "drive", 25.f }, { "tone", -20.f }, { "mix", 100.f } } },
                                          { "Tube", { { "drive", 40.f }, { "tone", 10.f }, { "mix", 100.f } } }, { "Crunch", { { "drive", 70.f }, { "tone", 0.f }, { "mix", 60.f } } } };
    presetsFor (ModuleId::Distortion) = { { "Edge", { { "drive", 30.f }, { "tone", 8000.f }, { "mix", 35.f } } }, { "Rage", { { "drive", 60.f }, { "tone", 6000.f }, { "mix", 50.f } } }, { "Fuzz", { { "drive", 90.f }, { "tone", 4000.f }, { "mix", 60.f } } } };
    presetsFor (ModuleId::Exciter)    = { { "Subtle", { { "freq", 5000.f }, { "amount", 20.f } } }, { "Bright", { { "freq", 4000.f }, { "amount", 40.f } } }, { "Air", { { "freq", 8000.f }, { "amount", 50.f } } } };
    presetsFor (ModuleId::LoFi)       = { { "Vintage", { { "bits", 12.f }, { "rate", 20000.f }, { "mix", 30.f } } }, { "Crushed", { { "bits", 8.f }, { "rate", 12000.f }, { "mix", 60.f } } }, { "Destroyed", { { "bits", 5.f }, { "rate", 6000.f }, { "mix", 100.f } } } };
    presetsFor (ModuleId::Telephone)  = { { "Telephone", { { "low", 500.f }, { "high", 3400.f }, { "drive", 20.f }, { "mix", 100.f } } }, { "Radio", { { "low", 300.f }, { "high", 5000.f }, { "drive", 30.f }, { "mix", 100.f } } }, { "Megaphone", { { "low", 700.f }, { "high", 2800.f }, { "drive", 60.f }, { "mix", 100.f } } } };
    presetsFor (ModuleId::Doubler)    = { { "Subtle", { { "detune", 6.f }, { "delay", 15.f }, { "width", 80.f }, { "mix", 25.f } } }, { "Wide", { { "detune", 12.f }, { "delay", 20.f }, { "width", 100.f }, { "mix", 40.f } } }, { "Thick", { { "detune", 18.f }, { "delay", 25.f }, { "width", 100.f }, { "mix", 55.f } } } };
    presetsFor (ModuleId::Harmonizer) = { { "Octave Down", { { "interval", -12.f }, { "mix", 25.f }, { "pan", 0.f } } }, { "Octave Up", { { "interval", 12.f }, { "mix", 20.f }, { "pan", 0.f } } }, { "Fifth", { { "interval", 7.f }, { "mix", 20.f }, { "pan", 30.f } } } };
    presetsFor (ModuleId::Chorus)     = { { "Light", { { "rate", 0.4f }, { "depth", 25.f }, { "mix", 20.f } } }, { "Lush", { { "rate", 0.6f }, { "depth", 40.f }, { "mix", 35.f } } }, { "Wobble", { { "rate", 1.5f }, { "depth", 60.f }, { "mix", 40.f } } } };
    presetsFor (ModuleId::Flanger)    = { { "Soft", { { "rate", 0.2f }, { "depth", 40.f }, { "feedback", 20.f }, { "mix", 20.f } } }, { "Jet", { { "rate", 0.3f }, { "depth", 60.f }, { "feedback", 70.f }, { "mix", 35.f } } } };
    presetsFor (ModuleId::Phaser)     = { { "Slow", { { "rate", 0.2f }, { "depth", 50.f }, { "center", 800.f }, { "feedback", 20.f }, { "mix", 35.f } } }, { "Swirl", { { "rate", 0.6f }, { "depth", 70.f }, { "center", 1200.f }, { "feedback", 40.f }, { "mix", 45.f } } } };
    presetsFor (ModuleId::Tremolo)    = { { "Gentle", { { "rate", 4.f }, { "depth", 30.f } } }, { "Chop", { { "rate", 8.f }, { "depth", 80.f } } }, { "Slow", { { "rate", 1.5f }, { "depth", 50.f } } } };
    presetsFor (ModuleId::AutoPan)    = { { "Slow", { { "rate", 0.3f }, { "depth", 60.f } } }, { "Fast", { { "rate", 2.f }, { "depth", 80.f } } } };
    presetsFor (ModuleId::Widener)    = { { "Subtle", { { "width", 115.f }, { "lowmono", 120.f } } }, { "Wide", { { "width", 140.f }, { "lowmono", 120.f } } }, { "Huge", { { "width", 180.f }, { "lowmono", 150.f } } } };
    presetsFor (ModuleId::Slap)       = { { "Vintage", { { "time", 110.f }, { "feedback", 10.f }, { "tone", 5000.f }, { "mix", 15.f } } }, { "Rockabilly", { { "time", 130.f }, { "feedback", 20.f }, { "tone", 6000.f }, { "mix", 30.f } } }, { "Tight", { { "time", 60.f }, { "feedback", 0.f }, { "tone", 8000.f }, { "mix", 20.f } } } };
    presetsFor (ModuleId::Delay)      = { { "Eighth Pop",   { { "sync", 2.f }, { "feedback", 35.f }, { "spread", 100.f }, { "tone", 6000.f }, { "lowcut", 250.f }, { "duck", 45.f }, { "mix", 18.f } } },
                                          { "Dotted Throw", { { "sync", 3.f }, { "feedback", 45.f }, { "spread", 100.f }, { "tone", 5000.f }, { "lowcut", 250.f }, { "duck", 50.f }, { "mix", 24.f } } },
                                          { "Quarter Tape", { { "sync", 1.f }, { "feedback", 40.f }, { "spread", 60.f },  { "tone", 3500.f }, { "lowcut", 200.f }, { "duck", 60.f }, { "mix", 20.f } } },
                                          { "Sixteenth",    { { "sync", 4.f }, { "feedback", 30.f }, { "spread", 100.f }, { "tone", 7000.f }, { "lowcut", 300.f }, { "duck", 40.f }, { "mix", 15.f } } },
                                          { "Slap 100 ms",  { { "sync", 0.f }, { "time", 100.f }, { "feedback", 10.f }, { "spread", 0.f }, { "tone", 6000.f }, { "lowcut", 200.f }, { "duck", 0.f }, { "mix", 20.f } } } };
    presetsFor (ModuleId::Reverb)     = { { "Vocal Plate", { { "size", 55.f }, { "decay", 45.f }, { "predelay", 25.f }, { "tone", 8000.f }, { "width", 100.f }, { "duck", 30.f }, { "mix", 22.f } } },
                                          { "Small Room",  { { "size", 30.f }, { "decay", 25.f }, { "predelay", 10.f }, { "tone", 7000.f }, { "width", 90.f },  { "duck", 20.f }, { "mix", 15.f } } },
                                          { "Hall",        { { "size", 85.f }, { "decay", 75.f }, { "predelay", 40.f }, { "tone", 6000.f }, { "width", 100.f }, { "duck", 40.f }, { "mix", 28.f } } },
                                          { "Ambience",    { { "size", 40.f }, { "decay", 30.f }, { "predelay", 5.f },  { "tone", 9000.f }, { "width", 100.f }, { "duck", 0.f },  { "mix", 12.f } } } };
    presetsFor (ModuleId::PopFilter)  = { { "Light", { { "sens", 35.f }, { "strength", 50.f }, { "release", 100.f } } }, { "Standard", { { "sens", 50.f }, { "strength", 70.f }, { "release", 120.f } } }, { "Heavy", { { "sens", 70.f }, { "strength", 100.f }, { "release", 160.f } } } };
    presetsFor (ModuleId::Breath)     = { { "Light", { { "sens", 35.f }, { "reduction", 8.f }, { "speed", 80.f } } }, { "Medium", { { "sens", 50.f }, { "reduction", 12.f }, { "speed", 80.f } } }, { "Strong", { { "sens", 65.f }, { "reduction", 18.f }, { "speed", 60.f } } } };

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

} // namespace vocalyst
