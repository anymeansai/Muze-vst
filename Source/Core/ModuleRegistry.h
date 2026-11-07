#pragma once
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>

namespace vocalyst
{

// The 27 vocal effect modules that can be added, removed and re-ordered in the chain.
enum class ModuleId : int
{
    Gate = 0, Tune, LowCut, HighCut, EQ, Air, DeEsser,
    Compressor, Glue, Limiter, Transient,
    Saturation, Distortion, Exciter, LoFi, Telephone,
    Doubler, Harmonizer, Chorus, Flanger, Phaser, Tremolo, AutoPan, Widener,
    Slap, Delay, Reverb,
    Count
};

constexpr int kNumModules = static_cast<int> (ModuleId::Count);

enum class ModuleCategory : int { Clean = 0, Dynamics, Tone, Color, Modulation, Space, NumCategories };

struct ParamSpec
{
    const char* key;      // suffix, e.g. "thresh"  -> full id "gate_thresh"
    const char* name;     // display name
    float min, max, def;  // range and default (in display units)
    float skew;           // NormalisableRange skew (1 = linear)
    const char* unit;     // "dB", "ms", "Hz", "%", "st", ""
    int   step;           // 0 = continuous, 1 = integer steps
    const char* choices;  // comma-separated list -> AudioParameterChoice (min/max ignored)
};

struct ModuleSpec
{
    ModuleId        id;
    const char*     key;          // short id used in parameter ids and preset chains
    const char*     name;         // display name
    ModuleCategory  category;
    const char*     description;
    std::vector<ParamSpec> params;
};

const std::vector<ModuleSpec>& allModules();
const ModuleSpec& moduleSpec (ModuleId id);
const ModuleSpec* findModuleByKey (const juce::String& key);
const ParamSpec*  findParam (ModuleId id, const juce::String& paramKey);

juce::String paramId  (ModuleId id, const char* paramKey);   // "gate_thresh"
juce::String enableId (ModuleId id);                         // "gate_on"
const char*  categoryName (ModuleCategory c);
juce::Colour categoryColour (ModuleCategory c);

// Global (non-module) parameter ids
namespace ids
{
    inline constexpr const char* inGain     = "in_gain";
    inline constexpr const char* outGain    = "out_gain";
    inline constexpr const char* mix        = "mix";
    inline constexpr const char* key        = "key";
    inline constexpr const char* scale      = "scale";
    inline constexpr const char* macroTune  = "macro_tune";
    inline constexpr const char* macroClean = "macro_clean";
    inline constexpr const char* macroTone  = "macro_tone";
    inline constexpr const char* macroSpace = "macro_space";
    inline constexpr const char* macroWidth = "macro_width";
    inline constexpr const char* macroDrive = "macro_drive";
    inline constexpr const char* bypass     = "bypass";
    inline constexpr const char* match      = "match";
    inline juce::String note (int pitchClass) { return "note_" + juce::String (pitchClass); }   // note_0 .. note_11
}

} // namespace vocalyst
