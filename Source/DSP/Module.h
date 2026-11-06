#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/ModuleRegistry.h"
#include <atomic>
#include <array>

namespace vocalyst
{

// Per-block information handed to every module.
struct ProcessInfo
{
    double bpm = 120.0;
    int    key = 0;             // 0 = C
    int    scale = 0;           // index into scales::scaleNames()
    // Simple-mode macro factors (1.0 = neutral, as stored in the preset)
    float tuneFactor  = 1.f;    // scales Tune amount & speed
    float cleanFactor = 1.f;    // 0..2 : gate / de-esser / compressor intensity
    float spaceFactor = 1.f;    // scales reverb / delay / slap mix
    float widthFactor = 1.f;    // scales doubler / widener / chorus
    float driveFactor = 1.f;    // scales saturation / distortion / exciter
};

// Base class for all 27 effect modules. Always stereo (2 channels).
class Module
{
public:
    explicit Module (ModuleId id) : moduleId (id) {}
    virtual ~Module() = default;

    ModuleId id() const { return moduleId; }
    const ModuleSpec& spec() const { return moduleSpec (moduleId); }

    // Called once after the APVTS is created: fetch raw parameter pointers.
    void attachParameters (juce::AudioProcessorValueTreeState& apvts);

    virtual void prepare (double sampleRate, int maxBlockSize) = 0;
    virtual void reset() = 0;
    virtual void process (juce::AudioBuffer<float>& buffer, const ProcessInfo& info) = 0;
    virtual int  latencySamples() const { return 0; }

    bool isEnabled() const { return enabled != nullptr && enabled->load() > 0.5f; }

protected:
    // value of the n-th parameter in the module spec (display units)
    float p (int index) const { return params[(size_t) index] != nullptr ? params[(size_t) index]->load() : 0.f; }

    ModuleId moduleId;
    std::atomic<float>* enabled = nullptr;
    std::array<std::atomic<float>*, 12> params {};
};

inline void Module::attachParameters (juce::AudioProcessorValueTreeState& apvts)
{
    enabled = apvts.getRawParameterValue (enableId (moduleId));
    const auto& sp = spec();
    for (size_t i = 0; i < sp.params.size() && i < params.size(); ++i)
        params[i] = apvts.getRawParameterValue (paramId (moduleId, sp.params[i].key));
}

// Factory for the 27 modules
std::unique_ptr<Module> createModule (ModuleId id);

} // namespace vocalyst
