#include "KeyDetector.h"
#include <cmath>

namespace muze
{

KeyDetector::KeyDetector()
{
    for (auto& c : chroma) c.store (0.f);
    prepare (44100.0);
}

void KeyDetector::prepare (double sampleRate)
{
    sr = sampleRate;
    window.resize ((size_t) fftSize);
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * (float) i / (float) fftSize);
    fifo.assign ((size_t) fftSize, 0.f);
    fftData.assign ((size_t) fftSize * 2, 0.f);
    binLo = juce::jmax (1, (int) std::floor (55.0 * fftSize / sr));
    binHi = juce::jmin (fftSize / 2 - 2, (int) std::ceil (5000.0 * fftSize / sr));
    reset();
}

void KeyDetector::reset()
{
    std::fill (fifo.begin(), fifo.end(), 0.f);
    fifoPos = 0;
    for (auto& c : chroma) c.store (0.f);
    seconds.store (0.f);
}

void KeyDetector::pushSample (float s)
{
    fifo[(size_t) fifoPos++] = s;
    if (fifoPos >= fftSize)
    {
        analyseFrame();
        // keep the second half for 50 % overlap
        std::copy (fifo.begin() + hopSize, fifo.end(), fifo.begin());
        fifoPos = hopSize;
    }
}

void KeyDetector::push (const float* mono, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        pushSample (mono[i]);
}

void KeyDetector::analyseFrame()
{
    float energy = 0.f;
    for (int i = 0; i < fftSize; ++i)
    {
        const float v = fifo[(size_t) i] * window[(size_t) i];
        fftData[(size_t) i] = v;
        energy += v * v;
    }
    std::fill (fftData.begin() + fftSize, fftData.end(), 0.f);
    if (energy / fftSize < 1.0e-9f)    // silence: do not accumulate
        return;

    fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

    // pick spectral peaks, interpolate their frequency and vote for a pitch class
    std::array<float, 12> frame {};
    float frameMax = 0.f;
    for (int k = binLo; k <= binHi; ++k) frameMax = juce::jmax (frameMax, fftData[(size_t) k]);
    const float floorMag = frameMax * 0.02f;
    for (int k = binLo; k <= binHi; ++k)
    {
        const float m0 = fftData[(size_t) k - 1], m1 = fftData[(size_t) k], m2 = fftData[(size_t) k + 1];
        if (m1 < floorMag || m1 <= m0 || m1 < m2) continue;          // not a local maximum
        const float denom = m0 - 2.f * m1 + m2;
        const float delta = std::abs (denom) > 1.0e-9f ? 0.5f * (m0 - m2) / denom : 0.f;   // parabolic interpolation
        const double f = ((double) k + delta) * sr / fftSize;
        if (f < 55.0 || f > 5000.0) continue;
        const double midi = 69.0 + 12.0 * std::log2 (f / 440.0);
        const int note = (int) std::lround (midi);
        if (std::abs (midi - note) > 0.4) continue;
        const float weight = (float) (1.0 / (1.0 + std::pow (f / 1500.0, 2.0)));   // de-emphasise upper harmonics
        frame[(size_t) (((note % 12) + 12) % 12)] += std::log1p (m1 * 20.f) * weight;
    }
    float sum = 0.f;
    for (auto v : frame) sum += v;
    if (sum <= 1.0e-6f) return;
    for (int i = 0; i < 12; ++i)
    {
        const float add = frame[(size_t) i] / sum;
        chroma[(size_t) i].store (chroma[(size_t) i].load() + add);
    }
    seconds.store (seconds.load() + (float) hopSize / (float) sr);
}

KeyDetector::Result KeyDetector::estimate() const
{
    Result r;
    std::array<float, 12> c {};
    float total = 0.f;
    for (int i = 0; i < 12; ++i) { c[(size_t) i] = chroma[(size_t) i].load(); total += c[(size_t) i]; }
    r.seconds = seconds.load();
    if (total <= 1.0e-6f || r.seconds < 0.5f)
        return r;
    for (auto& v : c) v /= total;
    r.chroma = c;

    static const float major[12] = { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
    static const float minor[12] = { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };

    auto pearson = [] (const float* profile, const std::array<float, 12>& x, int shift)
    {
        float mx = 0.f, mp = 0.f;
        for (int i = 0; i < 12; ++i) { mx += x[(size_t) i]; mp += profile[i]; }
        mx /= 12.f; mp /= 12.f;
        float num = 0.f, dx = 0.f, dp = 0.f;
        for (int i = 0; i < 12; ++i)
        {
            const float a = x[(size_t) ((i + shift) % 12)] - mx;
            const float b = profile[i] - mp;
            num += a * b; dx += a * a; dp += b * b;
        }
        return num / std::sqrt (juce::jmax (1.0e-12f, dx * dp));
    };

    float best = -2.f, second = -2.f; int bestKey = 0, bestScale = 1;
    for (int k = 0; k < 12; ++k)
    {
        const float cm = pearson (major, c, k);
        const float cn = pearson (minor, c, k);
        if (cm > best) { second = best; best = cm; bestKey = k; bestScale = 1; } else if (cm > second) second = cm;
        if (cn > best) { second = best; best = cn; bestKey = k; bestScale = 2; } else if (cn > second) second = cn;
    }
    r.valid = true;
    r.key = bestKey;
    r.scale = bestScale;
    r.confidence = juce::jlimit (0.f, 1.f, 0.5f * best + 2.f * (best - second));
    return r;
}

} // namespace muze
