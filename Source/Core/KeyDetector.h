#pragma once
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>

namespace muze
{

// Chroma-based key finder (Krumhansl-Schmuckler profiles). Feed it mono audio from the
// audio thread; read estimates from any thread.
class KeyDetector
{
public:
    struct Result
    {
        bool  valid = false;
        int   key = 0;          // 0 = C
        int   scale = 1;        // 1 = Major, 2 = Minor (indices into scales::scaleNames())
        float confidence = 0.f; // 0..1
        std::array<float, 12> chroma {};
        float seconds = 0.f;
    };

    KeyDetector();
    void prepare (double sampleRate);
    void reset();

    void push (const float* mono, int numSamples);     // audio thread
    void pushSample (float s);                          // audio thread

    Result estimate() const;                            // any thread
    float  secondsAnalysed() const { return seconds.load(); }

    static constexpr int fftOrder = 13;
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int hopSize  = fftSize / 2;

private:
    void analyseFrame();

    double sr = 44100.0;
    juce::dsp::FFT fft { fftOrder };
    std::vector<float> window, fifo, fftData;
    int binLo = 1, binHi = 1;
    int fifoPos = 0;
    std::array<std::atomic<float>, 12> chroma;
    std::atomic<float> seconds { 0.f };
};

} // namespace muze
