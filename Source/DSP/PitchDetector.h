#pragma once
#include "DspUtil.h"
#include <functional>

namespace vocalyst::dsp
{

// Monophonic pitch detector (YIN) operating on a 2x decimated internal stream.
// Push samples; whenever a new estimate is available, hasNewEstimate() returns true.
class PitchDetector
{
public:
    void prepare (double sampleRate);
    void reset();

    void push (float sample);

    bool  hasNewEstimate()           { const bool b = newEstimate; newEstimate = false; return b; }
    float frequencyHz() const        { return freq; }           // 0 when unvoiced
    float confidence() const         { return conf; }           // 0..1
    float level() const              { return rmsLevel; }       // linear RMS of the last frame

    // analysis hop in (undecimated) samples
    int hopSamples() const           { return hop * decim; }
    int windowSamples() const        { return window * decim; }

private:
    void analyse();

    double sr = 44100.0, srInternal = 22050.0;
    int decim = 2, decimCounter = 0;
    SVF antiAlias;
    int window = 1024, half = 512, hop = 256, tauMin = 20, tauMax = 512;
    std::vector<float> ring;      // circular buffer of decimated samples
    int ringSize = 0, ringMask = 0, writePos = 0, sinceLast = 0;
    std::vector<float> frame, diff;
    float freq = 0.f, conf = 0.f, rmsLevel = 0.f;
    bool  newEstimate = false;
};

} // namespace vocalyst::dsp
