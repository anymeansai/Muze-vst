#include "MeterEngine.h"
#include <cmath>

namespace vocalyst
{

static const float kBandEdges[MeterEngine::numBands + 1] = { 40.f, 120.f, 250.f, 500.f, 1000.f, 2000.f, 4000.f, 8000.f, 16000.f };

float MeterEngine::bandLowHz (int band)  { return kBandEdges[juce::jlimit (0, numBands - 1, band)]; }
float MeterEngine::bandHighHz (int band) { return kBandEdges[juce::jlimit (0, numBands - 1, band) + 1]; }

MeterEngine::MeterEngine()
{
    for (int i = 0; i < fftSize; ++i)
        window[(size_t) i] = 0.5f - 0.5f * std::cos (2.f * juce::MathConstants<float>::pi * (float) i / (float) fftSize);
    reset();
}

void MeterEngine::prepare (double sampleRate)
{
    sr = sampleRate;
    reset();
}

void MeterEngine::reset()
{
    inFifo.fill (0.f); outL.fill (0.f); outR.fill (0.f);
    pos = 0;
    const juce::SpinLock::ScopedLockType sl (lock);
    result = Snapshot();
    result.sampleRate = sr;
}

void MeterEngine::push (const float* inL, const float* inR, const float* oL, const float* oR, int n)
{
    for (int i = 0; i < n; ++i)
    {
        inFifo[(size_t) pos] = 0.5f * (inL[i] + inR[i]);
        outL[(size_t) pos] = oL[i];
        outR[(size_t) pos] = oR[i];
        if (++pos >= fftSize)
        {
            analyse();
            // keep the newest half for 50 % overlap
            std::copy (inFifo.begin() + hopSize, inFifo.end(), inFifo.begin());
            std::copy (outL.begin() + hopSize, outL.end(), outL.begin());
            std::copy (outR.begin() + hopSize, outR.end(), outR.begin());
            pos = hopSize;
        }
    }
}

void MeterEngine::analyse()
{
    for (int i = 0; i < fftSize; ++i)
    {
        const float w = window[(size_t) i];
        work[(size_t) i]  = inFifo[(size_t) i] * w;
        workL[(size_t) i] = outL[(size_t) i] * w;
        workR[(size_t) i] = outR[(size_t) i] * w;
    }
    std::fill (work.begin() + fftSize, work.end(), 0.f);
    std::fill (workL.begin() + fftSize, workL.end(), 0.f);
    std::fill (workR.begin() + fftSize, workR.end(), 0.f);

    fft.performRealOnlyForwardTransform (work.data(), true);
    fft.performRealOnlyForwardTransform (workL.data(), true);
    fft.performRealOnlyForwardTransform (workR.data(), true);

    const float norm = 2.f / (float) fftSize * 2.f;   // Hann window coherent gain compensation
    Snapshot snap;
    snap.sampleRate = sr;
    std::array<double, numBands> em {}, es {}, elr {}, el {}, er {};
    double totalM = 0.0, totalS = 0.0;

    for (int k = 0; k < numBins; ++k)
    {
        const float reI = work[(size_t) (2 * k)],  imI = work[(size_t) (2 * k + 1)];
        const float reL = workL[(size_t) (2 * k)], imL = workL[(size_t) (2 * k + 1)];
        const float reR = workR[(size_t) (2 * k)], imR = workR[(size_t) (2 * k + 1)];
        const float magI = std::sqrt (reI * reI + imI * imI) * norm;
        const float magL = std::sqrt (reL * reL + imL * imL) * norm;
        const float magR = std::sqrt (reR * reR + imR * imR) * norm;
        snap.inDb[(size_t) k]  = juce::Decibels::gainToDecibels (magI, -100.f);
        snap.outDb[(size_t) k] = juce::Decibels::gainToDecibels (0.5f * (magL + magR), -100.f);

        const float reM = 0.5f * (reL + reR), imM = 0.5f * (imL + imR);
        const float reS = 0.5f * (reL - reR), imS = 0.5f * (imL - imR);
        const double eM = reM * reM + imM * imM, eS = reS * reS + imS * imS;
        const double eL = reL * reL + imL * imL, eR = reR * reR + imR * imR;
        const double lr = reL * reR + imL * imR;
        const double f = (double) k * sr / fftSize;
        totalM += eM; totalS += eS;
        for (int b = 0; b < numBands; ++b)
            if (f >= kBandEdges[b] && f < kBandEdges[b + 1]) { em[(size_t) b] += eM; es[(size_t) b] += eS; elr[(size_t) b] += lr; el[(size_t) b] += eL; er[(size_t) b] += eR; break; }
    }
    for (int b = 0; b < numBands; ++b)
    {
        const double tot = em[(size_t) b] + es[(size_t) b];
        snap.width[(size_t) b] = tot > 1.0e-12 ? (float) (es[(size_t) b] / tot) : 0.f;
        const double den = std::sqrt (el[(size_t) b] * er[(size_t) b]);
        snap.corr[(size_t) b]  = den > 1.0e-12 ? (float) juce::jlimit (-1.0, 1.0, elr[(size_t) b] / den) : 1.f;
    }
    const double tot = totalM + totalS;
    snap.overallCorr = tot > 1.0e-12 ? (float) ((totalM - totalS) / tot) : 1.f;
    snap.valid = true;

    const juce::SpinLock::ScopedTryLockType tl (lock);
    if (tl.isLocked())
        result = snap;
}

void MeterEngine::getSnapshot (Snapshot& out) const
{
    const juce::SpinLock::ScopedLockType sl (lock);
    out = result;
}

} // namespace vocalyst
