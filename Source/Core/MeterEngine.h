#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

namespace vocalyst
{

// Real-time spectrum (input vs output) and per-band stereo width, fed from the audio thread
// and read by the meter strip. Results are published under a try-lock so audio never blocks.
class MeterEngine
{
public:
    static constexpr int fftOrder = 11;
    static constexpr int fftSize  = 1 << fftOrder;   // 2048
    static constexpr int numBins  = fftSize / 2;
    static constexpr int hopSize  = fftSize / 2;
    static constexpr int numBands = 8;

    struct Snapshot
    {
        std::array<float, numBins>  inDb {}, outDb {};      // magnitude per bin, dBFS
        std::array<float, numBands> width {}, corr {};      // 0 = mono .. 1 = all side ; correlation -1..1
        float overallCorr = 1.f;
        double sampleRate = 44100.0;
        bool valid = false;
    };

    MeterEngine();
    void prepare (double sampleRate);
    void reset();

    // audio thread: the block's input (after input gain) and final output, same length
    void push (const float* inL, const float* inR, const float* outL, const float* outR, int n);
    void getSnapshot (Snapshot& out) const;                     // any thread

    static float bandLowHz (int band);
    static float bandHighHz (int band);

private:
    void analyse();

    double sr = 44100.0;
    juce::dsp::FFT fft { fftOrder };
    std::array<float, fftSize> window {};
    std::array<float, fftSize> inFifo {}, outL {}, outR {};
    std::array<float, fftSize * 2> work {}, workL {}, workR {};
    int pos = 0;

    mutable juce::SpinLock lock;
    Snapshot result;
};

} // namespace vocalyst
