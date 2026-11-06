#pragma once
#include <juce_core/juce_core.h>
#include "../DSP/DspUtil.h"
#include <atomic>
#include <vector>

namespace muze
{

// Listens to the raw vocal and produces mix suggestions (the "Analyze" feature).
class VocalAnalyzer
{
public:
    struct Stats
    {
        bool  valid = false;
        float seconds = 0.f;
        float rmsDb = -100.f, peakDb = -100.f, noiseFloorDb = -100.f, crestDb = 0.f;
        float lowRatio = 0.f, mudRatio = 0.f, presenceRatio = 0.f, sibilanceRatio = 0.f, airRatio = 0.f;
    };

    struct Suggestion
    {
        juce::String text;                                   // human readable
        std::vector<std::pair<juce::String, float>> values;  // parameter id -> value (display units)
        std::vector<juce::String> modules;                   // module keys that must be in the chain
    };

    void prepare (double sampleRate);
    void reset();
    void push (const float* mono, int numSamples);          // audio thread

    Stats snapshot() const;                                  // any thread
    static std::vector<Suggestion> suggest (const Stats& s);

private:
    void finishFrame();

    double sr = 44100.0;
    dsp::SVF lowLp, mudBp, presBp, sibBp, airHp;
    int frameLen = 1024, framePos = 0;
    float eTot = 0.f, eLow = 0.f, eMud = 0.f, ePres = 0.f, eSib = 0.f, eAir = 0.f, framePeak = 0.f;

    std::atomic<float> sumSq { 0.f }, sumLow { 0.f }, sumMud { 0.f }, sumPres { 0.f }, sumSib { 0.f }, sumAir { 0.f };
    std::atomic<float> peak { 0.f }, noiseFloor { 1.f }, frames { 0.f }, activeFrames { 0.f };
};

} // namespace muze
