#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Core/KeyDetector.h"
#include "../Core/SharedKey.h"
#include "../Core/Scales.h"

namespace muze
{

// Muze Auto Key: put it on the instrumental or master bus, it finds the song key and
// hands it to Muze on the vocal track.
class AutoKeyProcessor final : public juce::AudioProcessor, private juce::Timer
{
public:
    AutoKeyProcessor();
    ~AutoKeyProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                  { return true; }
    const juce::String getName() const override      { return "Muze Auto Key"; }
    bool acceptsMidi() const override                { return false; }
    bool producesMidi() const override               { return false; }
    bool isMidiEffect() const override               { return false; }
    double getTailLengthSeconds() const override     { return 0.0; }
    int getNumPrograms() override                    { return 1; }
    int getCurrentProgram() override                 { return 0; }
    void setCurrentProgram (int) override            {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    KeyDetector& detector()                          { return keyDetector; }
    KeyDetector::Result current() const              { return keyDetector.estimate(); }
    bool sendToMuze();
    bool isAutoSend() const                          { return autoSend.load(); }
    void setAutoSend (bool b)                        { autoSend.store (b); }
    juce::String lastSentText() const                { return lastSent; }
    float inputLevel() const                         { return level.load(); }

private:
    void timerCallback() override;
    KeyDetector keyDetector;
    std::vector<float> mono;
    std::atomic<bool> autoSend { true };
    std::atomic<float> level { 0.f };
    int lastKey = -1, lastScale = -1;
    juce::String lastSent;
};

} // namespace muze
