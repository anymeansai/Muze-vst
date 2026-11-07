#pragma once
#include <juce_dsp/juce_dsp.h>
#include "../DSP/DspUtil.h"
#include <atomic>
#include <vector>
#include <array>

namespace vocalyst
{

// Listens to the raw vocal and produces mix suggestions (the "Analyze" feature).
// Only audio that is actually playing and audible is counted.
class VocalAnalyzer
{
public:
    static constexpr int numBands = 30;                 // long-term spectrum, 1/3-octave-ish from 50 Hz to 16 kHz

    struct Stats
    {
        bool  valid = false;
        float seconds = 0.f;                            // audible seconds analysed
        float rmsDb = -100.f, peakDb = -100.f, noiseFloorDb = -100.f, crestDb = 0.f;
        float loudP10 = -100.f, loudP50 = -100.f, loudP90 = -100.f;   // short-term loudness percentiles (dBFS)
        float dynamicRangeDb = 0.f;                     // P90 - P10
        float lowRatio = 0.f, mudRatio = 0.f, presenceRatio = 0.f, sibilanceRatio = 0.f, airRatio = 0.f;
        float sibilanceLevelDb = -100.f;                // level of the 5-9 kHz band while singing
        float plosiveRatio = 0.f;                       // fraction of frames with a low-frequency thump
        float tiltDbPerOct = -6.f;                      // long-term spectral slope 200 Hz .. 8 kHz
        std::array<float, numBands> spectrumDb {};      // long-term average spectrum
        std::array<float, numBands> trendDb {};         // smoothed version of it
    };

    struct Suggestion
    {
        juce::String title;                                  // short label
        juce::String text;                                   // the measurement and the reasoning
        std::vector<std::pair<juce::String, float>> values;  // parameter id -> value (display units)
        std::vector<juce::String> modules;                   // module keys that must be in the chain
    };

    VocalAnalyzer() { for (auto& h : hist) h.store (0); for (auto& b : bandPower) b.store (0.f); }
    void prepare (double sampleRate);
    void reset();
    void push (const float* mono, int numSamples);          // audio thread

    Stats snapshot() const;                                  // any thread
    static std::vector<Suggestion> suggest (const Stats& s);
    static float bandCentreHz (int band);

private:
    void finishFrame();
    void analyseSpectrum();

    double sr = 44100.0;
    dsp::SVF lowLp, mudBp, presBp, sibBp, airHp;
    int frameLen = 1024, framePos = 0;
    float eTot = 0.f, eLow = 0.f, eMud = 0.f, ePres = 0.f, eSib = 0.f, eAir = 0.f, framePeak = 0.f;
    float lowSlow = 0.f;

    // short-term loudness histogram (0.5 dB bins from -90 to 0)
    static constexpr int histBins = 180;
    std::array<std::atomic<int>, histBins> hist;
    float stRms = 0.f;   // 400 ms exponential RMS (power)

    // long-term spectrum
    static constexpr int fftOrder = 11, fftSize = 1 << fftOrder;
    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize> window {}, fifo {};
    std::array<float, fftSize * 2> fftData {};
    int fifoPos = 0;
    std::array<std::atomic<float>, numBands> bandPower;
    std::array<int, numBands> binLo {}, binHi {};
    std::atomic<int> spectrumFrames { 0 };

    std::atomic<float> sumSq { 0.f }, sumLow { 0.f }, sumMud { 0.f }, sumPres { 0.f }, sumSib { 0.f }, sumAir { 0.f };
    std::atomic<float> peak { 0.f }, noiseFloor { 1.f }, audibleFrames { 0.f }, activeFrames { 0.f }, plosiveFrames { 0.f };
};

} // namespace vocalyst
