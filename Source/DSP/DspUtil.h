#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <vector>

namespace muze::dsp
{

constexpr float kPi = 3.14159265358979323846f;

inline float dbToGain (float db)        { return std::pow (10.0f, db * 0.05f); }
inline float gainToDb (float g)         { return 20.0f * std::log10 (std::max (g, 1.0e-9f)); }
inline float midiToHz (float m)         { return 440.0f * std::pow (2.0f, (m - 69.0f) / 12.0f); }
inline float hzToMidi (float hz)        { return 69.0f + 12.0f * std::log2 (std::max (hz, 1.0e-6f) / 440.0f); }
inline float msToCoeff (float ms, double sr)
{
    if (ms <= 0.0f) return 0.0f;
    return (float) std::exp (-1.0 / (0.001 * ms * sr));
}

// Andrew Simper's trapezoidal state variable filter (gives LP / BP / HP at once).
struct SVF
{
    float g = 0.f, k = 1.f, a1 = 0.f, a2 = 0.f, a3 = 0.f;
    float ic1eq = 0.f, ic2eq = 0.f;
    float lp = 0.f, bp = 0.f, hp = 0.f;

    void set (float cutoffHz, float Q, double sr)
    {
        cutoffHz = juce::jlimit (10.0f, (float) sr * 0.49f, cutoffHz);
        g  = std::tan (kPi * cutoffHz / (float) sr);
        k  = 1.0f / std::max (Q, 0.05f);
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
    void reset() { ic1eq = ic2eq = lp = bp = hp = 0.f; }

    inline void process (float v0)
    {
        const float v3 = v0 - ic2eq;
        const float v1 = a1 * ic1eq + a2 * v3;
        const float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.f * v1 - ic1eq;
        ic2eq = 2.f * v2 - ic2eq;
        lp = v2; bp = v1; hp = v0 - k * v1 - v2;
    }
    inline float lowpass  (float x) { process (x); return lp; }
    inline float highpass (float x) { process (x); return hp; }
    inline float bandpass (float x) { process (x); return bp; }
};

// One-pole smoother / lowpass
struct OnePole
{
    float a = 0.f, z = 0.f;
    void setTime (float ms, double sr) { a = msToCoeff (ms, sr); }
    void setCutoff (float hz, double sr) { a = (float) std::exp (-2.0 * juce::MathConstants<double>::pi * hz / sr); }
    void reset (float v = 0.f) { z = v; }
    inline float process (float x) { z = x + a * (z - x); return z; }
};

// Peak envelope follower with separate attack / release
struct EnvelopeFollower
{
    float aA = 0.f, aR = 0.f, env = 0.f;
    void set (float attackMs, float releaseMs, double sr) { aA = msToCoeff (attackMs, sr); aR = msToCoeff (releaseMs, sr); }
    void reset() { env = 0.f; }
    inline float process (float x)
    {
        x = std::abs (x);
        const float a = x > env ? aA : aR;
        env = x + a * (env - x);
        return env;
    }
};

struct LFO
{
    double phase = 0.0, inc = 0.0;
    void setRate (float hz, double sr) { inc = hz / sr; }
    void reset (double p = 0.0) { phase = p; }
    inline float sine (double offset = 0.0) const { return (float) std::sin (2.0 * juce::MathConstants<double>::pi * (phase + offset)); }
    inline void advance() { phase += inc; if (phase >= 1.0) phase -= 1.0; }
};

// Simple fractional delay line (linear interpolation), one channel.
class DelayLine
{
public:
    void prepare (int maxSamples)
    {
        size = 1;
        while (size < maxSamples + 4) size <<= 1;
        mask = size - 1;
        buf.assign ((size_t) size, 0.f);
        w = 0;
    }
    void reset() { std::fill (buf.begin(), buf.end(), 0.f); w = 0; }
    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    // delay in samples (>= 1), relative to the most recently pushed sample
    inline float read (float delay) const
    {
        delay = juce::jlimit (1.0f, (float) (size - 3), delay);
        const int   i  = (int) delay;
        const float f  = delay - (float) i;
        const int   r0 = (w - 1 - i + size) & mask;
        const int   r1 = (r0 - 1 + size) & mask;
        return buf[(size_t) r0] + f * (buf[(size_t) r1] - buf[(size_t) r0]);
    }
    inline float readInt (int delay) const
    {
        delay = juce::jlimit (1, size - 2, delay);
        return buf[(size_t) ((w - 1 - delay + size) & mask)];
    }
    int capacity() const { return size - 4; }
private:
    std::vector<float> buf;
    int size = 0, mask = 0, w = 0;
};

// Tilt filter around a pivot frequency: t in [-1, 1], +1 = bright, -1 = dark (about +/- 6 dB)
struct Tilt
{
    OnePole lp;
    float t = 0.f;
    void prepare (double sr, float pivotHz = 1000.f) { lp.setCutoff (pivotHz, sr); lp.reset(); }
    void setTilt (float tilt) { t = juce::jlimit (-1.f, 1.f, tilt); }
    inline float process (float x)
    {
        const float low  = lp.process (x);
        const float high = x - low;
        return low * (1.f - t) + high * (1.f + t);
    }
};

inline float softClip (float x)
{
    // cubic soft clipper, unity gain for small signals, hard limit at +/-1
    if (x > 1.f)  return 2.f / 3.f;
    if (x < -1.f) return -2.f / 3.f;
    return x - x * x * x / 3.f;
}

inline float fastTanh (float x)
{
    const float x2 = x * x;
    return juce::jlimit (-1.f, 1.f, x * (27.f + x2) / (27.f + 9.f * x2));
}

} // namespace muze::dsp
