#include "PitchDetector.h"

namespace vocalyst::dsp
{

void PitchDetector::prepare (double sampleRate)
{
    sr = sampleRate;
    decim = sr > 30000.0 ? 2 : 1;
    if (sr > 70000.0) decim = 4;
    srInternal = sr / decim;

    window = 1024;
    half   = window / 2;
    hop    = 256;
    tauMin = juce::jmax (2, (int) (srInternal / 1200.0));   // ~1.2 kHz upper bound
    tauMax = juce::jmin (half - 1, (int) (srInternal / 55.0)); // ~55 Hz lower bound

    ringSize = 4096;
    ringMask = ringSize - 1;
    ring.assign ((size_t) ringSize, 0.f);
    frame.assign ((size_t) window, 0.f);
    diff.assign ((size_t) (tauMax + 1), 0.f);
    antiAlias.set ((float) (srInternal * 0.42), 0.6f, sr);
    reset();
}

void PitchDetector::reset()
{
    std::fill (ring.begin(), ring.end(), 0.f);
    writePos = 0; sinceLast = 0; decimCounter = 0;
    antiAlias.reset();
    freq = 0.f; conf = 0.f; rmsLevel = 0.f; newEstimate = false;
}

void PitchDetector::push (float sample)
{
    float x = sample;
    if (decim > 1)
    {
        x = antiAlias.lowpass (sample);
        if (++decimCounter < decim)
            return;
        decimCounter = 0;
    }

    ring[(size_t) writePos] = x;
    writePos = (writePos + 1) & ringMask;

    if (++sinceLast >= hop)
    {
        sinceLast = 0;
        analyse();
    }
}

void PitchDetector::analyse()
{
    // Copy the last `window` samples into a linear frame
    int start = (writePos - window + ringSize) & ringMask;
    float energy = 0.f;
    for (int i = 0; i < window; ++i)
    {
        const float v = ring[(size_t) ((start + i) & ringMask)];
        frame[(size_t) i] = v;
        energy += v * v;
    }
    rmsLevel = std::sqrt (energy / (float) window);
    newEstimate = true;

    if (rmsLevel < 0.0005f) // below about -66 dBFS: treat as silence
    {
        freq = 0.f; conf = 0.f;
        return;
    }

    // YIN difference function over the first half of the frame
    const float* x = frame.data();
    diff[0] = 0.f;
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        float sum = 0.f;
        const float* a = x;
        const float* b = x + tau;
        for (int j = 0; j < half; ++j)
        {
            const float d = a[j] - b[j];
            sum += d * d;
        }
        diff[(size_t) tau] = sum;
    }

    // Cumulative mean normalised difference
    float running = 0.f;
    std::vector<float>& cmnd = diff; // in place
    cmnd[0] = 1.f;
    for (int tau = 1; tau <= tauMax; ++tau)
    {
        running += diff[(size_t) tau];
        cmnd[(size_t) tau] = running > 1.0e-12f ? diff[(size_t) tau] * (float) tau / running : 1.f;
    }

    // Absolute threshold search
    const float threshold = 0.15f;
    int tauEst = -1;
    for (int tau = tauMin; tau <= tauMax; ++tau)
    {
        if (cmnd[(size_t) tau] < threshold)
        {
            while (tau + 1 <= tauMax && cmnd[(size_t) (tau + 1)] < cmnd[(size_t) tau])
                ++tau;
            tauEst = tau;
            break;
        }
    }

    if (tauEst < 0)
    {
        // fall back to global minimum, but only accept if it is reasonably periodic
        float best = 1.f;
        for (int tau = tauMin; tau <= tauMax; ++tau)
            if (cmnd[(size_t) tau] < best) { best = cmnd[(size_t) tau]; tauEst = tau; }
        if (best > 0.35f || tauEst < 0)
        {
            freq = 0.f; conf = 0.f;
            return;
        }
    }

    // Parabolic interpolation around the minimum
    float betterTau = (float) tauEst;
    if (tauEst > 0 && tauEst < tauMax)
    {
        const float s0 = cmnd[(size_t) (tauEst - 1)], s1 = cmnd[(size_t) tauEst], s2 = cmnd[(size_t) (tauEst + 1)];
        const float denom = 2.f * (2.f * s1 - s2 - s0);
        if (std::abs (denom) > 1.0e-9f)
            betterTau += (s2 - s0) / denom;
    }

    freq = (float) (srInternal / betterTau);
    conf = juce::jlimit (0.f, 1.f, 1.f - cmnd[(size_t) tauEst]);
}

} // namespace vocalyst::dsp
