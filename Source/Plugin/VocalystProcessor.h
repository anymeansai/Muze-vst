#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/ModuleRegistry.h"
#include "../Core/Scales.h"
#include "../Core/KeyDetector.h"
#include "../Core/VocalAnalyzer.h"
#include "../Core/SharedKey.h"
#include "../DSP/Module.h"
#include "../DSP/DspUtil.h"
#include "../Presets/Presets.h"
#include <array>
#include <atomic>
#include <vector>

namespace vocalyst
{

class VocalystProcessor final : public juce::AudioProcessor,
                            private juce::Timer
{
public:
    VocalystProcessor();
    ~VocalystProcessor() override;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                  { return true; }
    const juce::String getName() const override      { return "Vocalyst"; }
    bool acceptsMidi() const override                { return false; }
    bool producesMidi() const override               { return false; }
    bool isMidiEffect() const override               { return false; }
    double getTailLengthSeconds() const override     { return 4.0; }

    int getNumPrograms() override                    { return (int) presets::factory().size(); }
    int getCurrentProgram() override                 { return juce::jmax (0, currentFactoryIndex); }
    void setCurrentProgram (int index) override      { loadFactoryPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    // Listeners (message thread)
    struct Listener
    {
        virtual ~Listener() = default;
        virtual void chainChanged() {}
        virtual void presetChanged() {}
        virtual void keyChanged() {}
    };
    void addVocalystListener (Listener* l)    { listeners.add (l); }
    void removeVocalystListener (Listener* l) { listeners.remove (l); }

    //==========================================================================
    // Chain (message thread)
    std::vector<ModuleId> getChainOrder() const      { return uiOrder; }
    void setChainOrder (const std::vector<ModuleId>& order);
    void addModule (ModuleId id, int index = -1);
    void removeModule (ModuleId id);
    void moveModule (int fromIndex, int toIndex);
    bool isInChain (ModuleId id) const;
    static std::vector<ModuleId> defaultChain();

    //==========================================================================
    // Presets (message thread)
    void   loadPreset (const Preset& p);
    void   loadFactoryPreset (int index);
    void   nextPreset();
    void   prevPreset();
    Preset captureCurrentPreset (const juce::String& name, const juce::String& genre,
                                 const juce::String& description, const juce::String& author, bool adlib) const;
    juce::String getCurrentPresetName() const;
    int    getCurrentFactoryIndex() const            { return currentFactoryIndex; }

    //==========================================================================
    // UI state kept with the plugin state
    bool isSimpleMode() const;
    void setSimpleMode (bool simple);
    juce::ValueTree getUiState() const               { return uiState; }
    void setUiProperty (const juce::Identifier& id, const juce::var& v) { uiState.setProperty (id, v, nullptr); }
    juce::var getUiProperty (const juce::Identifier& id, const juce::var& def = {}) const { return uiState.getProperty (id, def); }

    //==========================================================================
    // Key
    int  getKey() const;
    int  getScale() const;
    void setKey (int key, int scale);
    KeyDetector& inputKeyDetector()                  { return keyDetector; }
    bool applySharedKey();                            // pull from Vocalyst Auto Key
    bool isAutoKeyLinked() const                     { return (bool) uiState.getProperty ("autoKeyLink", false); }
    void setAutoKeyLinked (bool b)                   { uiState.setProperty ("autoKeyLink", b, nullptr); }
    sharedkey::Info lastSharedKey() const            { return lastShared; }

    //==========================================================================
    // Analyze
    VocalAnalyzer& analyzer()                        { return vocalAnalyzer; }
    void applySuggestions (const std::vector<VocalAnalyzer::Suggestion>& s);

    //==========================================================================
    // Meters / monitoring (any thread)
    float inputLevel (int ch) const                  { return inPeak[(size_t) juce::jlimit (0, 1, ch)].load(); }
    float outputLevel (int ch) const                 { return outPeak[(size_t) juce::jlimit (0, 1, ch)].load(); }
    float currentLatencySamples() const              { return (float) desiredLatency.load(); }

    juce::AudioProcessorValueTreeState apvts;

    static constexpr int kMaxChain = kNumModules;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void publishOrder();           // uiOrder -> audio thread
    void chainToState();
    void stateToChain();
    void setParamValue (const juce::String& id, float displayValue);
    float getParamValue (const juce::String& id) const;

    std::array<std::unique_ptr<Module>, kNumModules> modules;
    std::array<bool, kNumModules> wasActive {};

    // chain order handoff
    std::vector<ModuleId> uiOrder;
    juce::SpinLock orderLock;
    std::array<int, kMaxChain> pendingOrder {};
    int pendingCount = 0;
    std::atomic<bool> orderDirty { false };
    std::array<int, kMaxChain> audioOrder {};
    int audioCount = 0;

    juce::AudioBuffer<float> work, dry;
    std::vector<float> monoScratch;
    dsp::Tilt tilt[2];
    juce::SmoothedValue<float> inGainSm, outGainSm, mixSm;
    double currentSampleRate = 44100.0;

    KeyDetector keyDetector;
    VocalAnalyzer vocalAnalyzer;
    sharedkey::Info lastShared;
    juce::int64 lastSharedTime = 0;

    std::array<std::atomic<float>, 2> inPeak { { 0.f, 0.f } }, outPeak { { 0.f, 0.f } };
    std::atomic<int> desiredLatency { 0 };

    juce::ValueTree uiState { "ui" };
    int currentFactoryIndex = -1;
    juce::ListenerList<Listener> listeners;

    std::atomic<float>* pInGain = nullptr; std::atomic<float>* pOutGain = nullptr; std::atomic<float>* pMix = nullptr;
    std::atomic<float>* pKey = nullptr;    std::atomic<float>* pScale = nullptr;
    std::atomic<float>* pMTune = nullptr;  std::atomic<float>* pMClean = nullptr; std::atomic<float>* pMTone = nullptr;
    std::atomic<float>* pMSpace = nullptr; std::atomic<float>* pMWidth = nullptr; std::atomic<float>* pMDrive = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalystProcessor)
};

} // namespace vocalyst
