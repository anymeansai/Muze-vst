// All 27 Vocalyst effect modules. Each one reads its parameters from the APVTS raw values
// fetched in Module::attachParameters(), in the order declared in ModuleRegistry.cpp.

#include "../Module.h"
#include "../DspUtil.h"
#include "../PitchDetector.h"
#include "../PitchShifter.h"
#include "../../Core/Scales.h"
#include <juce_dsp/juce_dsp.h>

namespace vocalyst
{
using namespace dsp;

namespace
{

inline float pct (float v) { return juce::jlimit (0.f, 1.f, v * 0.01f); }

//==============================================================================
class GateModule final : public Module
{
public:
    GateModule() : Module (ModuleId::Gate) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; env.set (0.3f, 25.f, sr); aOpen = msToCoeff (1.5f, sr); reset(); }
    void reset() override { env.reset(); gain = 1.f; open = true; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float threshDb  = p (0) + (info.cleanFactor - 1.f) * 12.f;
        const float openLin   = dbToGain (threshDb);
        const float closeLin  = dbToGain (threshDb - 5.f);
        const float rangeGain = dbToGain (p (2));
        const float aClose    = msToCoeff (p (1), sr);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float level = env.process (juce::jmax (std::abs (L[i]), std::abs (R[i])));
            if (open && level < closeLin) open = false;
            else if (! open && level > openLin) open = true;
            const float target = open ? 1.f : rangeGain;
            const float a = target > gain ? aOpen : aClose;
            gain = target + a * (gain - target);
            L[i] *= gain; R[i] *= gain;
        }
    }
private:
    double sr = 44100.0; EnvelopeFollower env; float gain = 1.f, aOpen = 0.f; bool open = true;
};

//==============================================================================
class TuneModule final : public Module
{
public:
    TuneModule() : Module (ModuleId::Tune) {}
    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        detector.prepare (sr);
        shifter.prepare (sr, 2);
        shifter.setNominalWindowMs (40.f);
        lastRange = -1;
        reset();
    }
    void reset() override
    {
        detector.reset(); shifter.reset();
        correction = targetCorrection = 0.f; vibOffset = 0.f; midiSlow = 0.f;
        stableTime = 0.f; silentTime = 1.f; lastTarget = -1; voiced = false;
    }
    int latencySamples() const override { return shifter.latencySamples(); }

    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float amount    = juce::jlimit (0.f, 1.f, pct (p (0)) * info.tuneFactor);
        const float speedMs   = p (1) / juce::jmax (0.25f, info.tuneFactor);
        const float humanize  = pct (p (2));
        const float flexCents = p (3);
        const float vibrato   = p (4) * 0.01f;
        const float transpose = std::round (p (5));
        const int   range     = (int) std::round (p (6));
        const float hopSec    = (float) detector.hopSamples() / (float) sr;

        if (range != lastRange)
        {
            static const float lo[5] = { 55.f, 150.f, 100.f, 60.f, 40.f };
            static const float hi[5] = { 1200.f, 1400.f, 900.f, 500.f, 350.f };
            const int r = juce::jlimit (0, 4, range);
            detector.setRange (lo[r], hi[r]);
            lastRange = range;
        }

        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        const int n = b.getNumSamples();
        for (int i = 0; i < n; ++i)
        {
            detector.push (0.5f * (L[i] + R[i]));
            if (detector.hasNewEstimate())
            {
                const float f = detector.frequencyHz();
                if (f > 0.f && detector.confidence() > 0.55f)
                {
                    const float midi = hzToMidi (f);
                    if (! voiced || silentTime > 0.2f) midiSlow = midi;
                    voiced = true; silentTime = 0.f;
                    midiSlow += 0.3f * (midi - midiSlow);                 // ~70 ms trend of the sung pitch
                    const int   target = scales::nearestNoteInMask (midi, info.noteMask);
                    const float devCents = std::abs ((float) target - midi) * 100.f;
                    stableTime = (target == lastTarget) ? stableTime + hopSec : 0.f;
                    lastTarget = target;
                    const bool leaveIt = flexCents > 0.f && devCents > flexCents;   // Flex-Tune: far-off notes stay expressive
                    targetCorrection = leaveIt ? 0.f : ((float) target - midi) * amount;
                    vibOffset = (vibrato - 1.f) * (midi - midiSlow);                 // natural vibrato scaling
                    shifter.setPeriodHint ((float) sr / f);
                    if (info.trace != nullptr)
                    {
                        info.trace->inHz.store (f);
                        info.trace->push (midi, midi + correction + vibOffset + transpose);
                    }
                }
                else
                {
                    silentTime += hopSec;
                    if (silentTime > 0.5f) { targetCorrection = 0.f; vibOffset = 0.f; stableTime = 0.f; lastTarget = -1; voiced = false; }
                    if (info.trace != nullptr) { info.trace->inHz.store (0.f); info.trace->push (0.f, 0.f); }
                }
                const float effSpeed = speedMs * (1.f + humanize * 4.f * juce::jmin (stableTime, 1.f));
                coeff = msToCoeff (effSpeed, sr);
            }
            correction += (1.f - coeff) * (targetCorrection - correction);
            shifter.setRatio (std::pow (2.f, (correction + vibOffset + transpose) / 12.f));
            float frame[2] { L[i], R[i] };
            shifter.processFrame (frame, frame, 2);
            L[i] = frame[0]; R[i] = frame[1];
        }
    }
private:
    double sr = 44100.0;
    PitchDetector detector; PitchShifter shifter;
    float correction = 0.f, targetCorrection = 0.f, vibOffset = 0.f, midiSlow = 0.f, coeff = 0.f, stableTime = 0.f, silentTime = 1.f;
    int lastTarget = -1, lastRange = -1;
    bool voiced = false;
};

//==============================================================================
class CutModule final : public Module
{
public:
    CutModule (ModuleId id, bool highpass) : Module (id), hp (highpass) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { f[0].reset(); f[1].reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        for (auto& s : f) s.set (p (0), p (1), sr);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = b.getWritePointer (ch);
            auto& s = f[ch];
            if (hp) for (int i = 0; i < b.getNumSamples(); ++i) d[i] = s.highpass (d[i]);
            else    for (int i = 0; i < b.getNumSamples(); ++i) d[i] = s.lowpass (d[i]);
        }
    }
private:
    double sr = 44100.0; bool hp; SVF f[2];
};

//==============================================================================
class EQModule final : public Module
{
public:
    EQModule() : Module (ModuleId::EQ) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; for (auto& v : cache) v = -1.0e9f; reset(); }
    void reset() override { for (auto& ch : filters) for (auto& f : ch) f.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        updateCoefficients();
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = b.getWritePointer (ch);
            for (int band = 0; band < 4; ++band)
            {
                auto& f = filters[ch][band];
                for (int i = 0; i < b.getNumSamples(); ++i) d[i] = f.processSample (d[i]);
            }
        }
    }
private:
    void updateCoefficients()
    {
        bool changed = false;
        for (int i = 0; i < 10; ++i) if (! juce::approximatelyEqual (cache[i], p (i))) { cache[i] = p (i); changed = true; }
        if (! changed) return;
        using C = juce::dsp::IIR::Coefficients<float>;
        auto set = [this] (int band, C::Ptr c) { for (int ch = 0; ch < 2; ++ch) *filters[ch][band].coefficients = *c; };
        set (0, C::makeLowShelf   (sr, p (1), 0.707f, dbToGain (p (0))));
        set (1, C::makePeakFilter (sr, p (3), p (4), dbToGain (p (2))));
        set (2, C::makePeakFilter (sr, p (6), p (7), dbToGain (p (5))));
        set (3, C::makeHighShelf  (sr, p (9), 0.707f, dbToGain (p (8))));
    }
    double sr = 44100.0;
    juce::dsp::IIR::Filter<float> filters[2][4];
    float cache[10] {};
};

//==============================================================================
class AirModule final : public Module
{
public:
    AirModule() : Module (ModuleId::Air) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; cf = ca = -1.f; reset(); }
    void reset() override { f[0].reset(); f[1].reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        if (! juce::approximatelyEqual (cf, p (0)) || ! juce::approximatelyEqual (ca, p (1)))
        {
            cf = p (0); ca = p (1);
            auto c = juce::dsp::IIR::Coefficients<float>::makeHighShelf (sr, cf, 0.5f, dbToGain (ca));
            for (auto& x : f) *x.coefficients = *c;
        }
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = b.getWritePointer (ch);
            for (int i = 0; i < b.getNumSamples(); ++i) d[i] = f[ch].processSample (d[i]);
        }
    }
private:
    double sr = 44100.0; juce::dsp::IIR::Filter<float> f[2]; float cf = -1.f, ca = -1.f;
};

//==============================================================================
class DeEsserModule final : public Module
{
public:
    DeEsserModule() : Module (ModuleId::DeEsser) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; env.set (0.3f, 40.f, sr); aG = msToCoeff (1.f, sr); reset(); }
    void reset() override { env.reset(); side.reset(); split[0].reset(); split[1].reset(); gain = 1.f; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float freq = p (0);
        const float thresh = dbToGain (p (1));
        const float maxRed = juce::jlimit (0.f, 30.f, p (2) * info.cleanFactor);
        side.set (freq, 2.0f, sr);
        for (auto& s : split) s.set (freq * 0.7f, 0.5f, sr);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float mono = 0.5f * (L[i] + R[i]);
            const float lvl  = env.process (side.bandpass (mono));
            float red = 0.f;
            if (lvl > thresh) red = juce::jmin (maxRed, (gainToDb (lvl) - gainToDb (thresh)) * 3.f);
            const float target = dbToGain (-red);
            gain = target + aG * (gain - target);
            const float lowL = split[0].lowpass (L[i]);
            const float lowR = split[1].lowpass (R[i]);
            L[i] = lowL + (L[i] - lowL) * gain;
            R[i] = lowR + (R[i] - lowR) * gain;
        }
    }
private:
    double sr = 44100.0; EnvelopeFollower env; SVF side, split[2]; float gain = 1.f, aG = 0.f;
};

//==============================================================================
inline float kneeGainDb (float levelDb, float threshDb, float ratio, float kneeDb)
{
    const float over = levelDb - threshDb;
    const float slope = 1.f - 1.f / juce::jmax (1.f, ratio);
    if (over <= -kneeDb * 0.5f) return 0.f;
    if (over >= kneeDb * 0.5f)  return -over * slope;
    const float x = over + kneeDb * 0.5f;
    return -slope * x * x / (2.f * kneeDb);
}

class CompressorModule final : public Module
{
public:
    CompressorModule() : Module (ModuleId::Compressor) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { env.reset(); grDb = 0.f; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float thresh = p (0) - (info.cleanFactor - 1.f) * 8.f;
        const float ratio = p (1);
        env.set (p (2), p (3), sr);
        const float makeup = dbToGain (p (4));
        const float mix = pct (p (5));
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float lvl = env.process (juce::jmax (std::abs (L[i]), std::abs (R[i])));
            grDb = kneeGainDb (gainToDb (lvl), thresh, ratio, 6.f);
            const float g = dbToGain (grDb) * makeup;
            L[i] = L[i] * (1.f - mix) + L[i] * g * mix;
            R[i] = R[i] * (1.f - mix) + R[i] * g * mix;
        }
        lastGr.store (grDb);
    }
    std::atomic<float> lastGr { 0.f };
private:
    double sr = 44100.0; EnvelopeFollower env; float grDb = 0.f;
};

class GlueModule final : public Module
{
public:
    GlueModule() : Module (ModuleId::Glue) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { sq = 0.f; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float thresh = p (0) - (info.cleanFactor - 1.f) * 4.f;
        const float ratio = p (1);
        const float aA = msToCoeff (p (2), sr), aR = msToCoeff (p (3), sr);
        const float makeup = dbToGain (p (4));
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float x2 = 0.5f * (L[i] * L[i] + R[i] * R[i]);
            const float a = x2 > sq ? aA : aR;
            sq = x2 + a * (sq - x2);
            const float g = dbToGain (kneeGainDb (gainToDb (std::sqrt (sq)), thresh, ratio, 12.f)) * makeup;
            L[i] *= g; R[i] *= g;
        }
    }
private:
    double sr = 44100.0; float sq = 0.f;
};

class LimiterModule final : public Module
{
public:
    LimiterModule() : Module (ModuleId::Limiter) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { env.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        const float gIn = dbToGain (p (0));
        const float ceil = dbToGain (p (1));
        env.set (0.02f, p (2), sr);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float l = L[i] * gIn, r = R[i] * gIn;
            const float lvl = env.process (juce::jmax (std::abs (l), std::abs (r)));
            const float g = lvl > ceil ? ceil / lvl : 1.f;
            L[i] = juce::jlimit (-ceil, ceil, l * g);
            R[i] = juce::jlimit (-ceil, ceil, r * g);
        }
    }
private:
    double sr = 44100.0; EnvelopeFollower env;
};

class TransientModule final : public Module
{
public:
    TransientModule() : Module (ModuleId::Transient) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; fast.set (0.4f, 25.f, sr); slow.set (25.f, 180.f, sr); aG = msToCoeff (0.5f, sr); reset(); }
    void reset() override { fast.reset(); slow.reset(); gain = 1.f; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        const float att = p (0) * 0.01f, sus = p (1) * 0.01f;
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float m = juce::jmax (std::abs (L[i]), std::abs (R[i]));
            const float d = gainToDb (fast.process (m) + 1.0e-6f) - gainToDb (slow.process (m) + 1.0e-6f);
            const float gDb = juce::jlimit (-18.f, 18.f, att * juce::jmax (d, 0.f) + sus * juce::jmax (-d, 0.f));
            const float target = dbToGain (gDb);
            gain = target + aG * (gain - target);
            L[i] *= gain; R[i] *= gain;
        }
    }
private:
    double sr = 44100.0; EnvelopeFollower fast, slow; float gain = 1.f, aG = 0.f;
};

//==============================================================================
class SaturationModule final : public Module
{
public:
    SaturationModule() : Module (ModuleId::Saturation) {}
    void prepare (double sampleRate, int) override { for (auto& t : tilt) t.prepare (sampleRate, 1200.f); reset(); }
    void reset() override { for (auto& t : tilt) t.lp.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float drive = juce::jlimit (0.f, 1.f, pct (p (0)) * info.driveFactor);
        const float g = 1.f + drive * 14.f;
        const float comp = 0.3f / fastTanh (0.3f * g);
        const float mix = pct (p (2));
        for (auto& t : tilt) t.setTilt (p (1) * 0.01f);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = b.getWritePointer (ch);
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const float x = d[i];
                const float y = fastTanh (g * tilt[ch].process (x)) * comp;
                d[i] = x + (y - x) * mix;
            }
        }
    }
private:
    Tilt tilt[2];
};

class DistortionModule final : public Module
{
public:
    DistortionModule() : Module (ModuleId::Distortion) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { lp[0].reset(); lp[1].reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float drive = juce::jlimit (0.f, 1.f, pct (p (0)) * info.driveFactor);
        const float g = 1.f + drive * 30.f;
        const float comp = 0.3f / softClip (juce::jmin (0.3f * g, 1.f));
        const float mix = pct (p (2));
        for (auto& f : lp) f.set (p (1), 0.707f, sr);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = b.getWritePointer (ch);
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const float x = d[i];
                const float y = lp[ch].lowpass (softClip (g * x) * comp);
                d[i] = x + (y - x) * mix;
            }
        }
    }
private:
    double sr = 44100.0; SVF lp[2];
};

class ExciterModule final : public Module
{
public:
    ExciterModule() : Module (ModuleId::Exciter) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { hp[0].reset(); hp[1].reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float amt = juce::jlimit (0.f, 1.5f, pct (p (1)) * info.driveFactor);
        for (auto& f : hp) f.set (p (0), 0.707f, sr);
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = b.getWritePointer (ch);
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const float h = hp[ch].highpass (d[i]);
                d[i] += amt * 0.8f * fastTanh (h * 5.f) * 0.5f;
            }
        }
    }
private:
    double sr = 44100.0; SVF hp[2];
};

class LoFiModule final : public Module
{
public:
    LoFiModule() : Module (ModuleId::LoFi) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { phase = 0.f; held[0] = held[1] = 0.f; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        const float step = std::pow (2.f, juce::jlimit (2.f, 16.f, p (0)) - 1.f);
        const float inc = juce::jlimit (0.001f, 1.f, p (1) / (float) sr);
        const float mix = pct (p (2));
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            phase += inc;
            if (phase >= 1.f)
            {
                phase -= 1.f;
                held[0] = std::round (L[i] * step) / step;
                held[1] = std::round (R[i] * step) / step;
            }
            L[i] += (held[0] - L[i]) * mix;
            R[i] += (held[1] - R[i]) * mix;
        }
    }
private:
    double sr = 44100.0; float phase = 0.f, held[2] {};
};

class TelephoneModule final : public Module
{
public:
    TelephoneModule() : Module (ModuleId::Telephone) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { for (auto& f : hp) f.reset(); for (auto& f : lp) f.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        for (auto& f : hp) f.set (p (0), 0.9f, sr);
        for (auto& f : lp) f.set (p (1), 0.9f, sr);
        const float g = 1.f + pct (p (2)) * 8.f;
        const float comp = 0.3f / fastTanh (0.3f * g);
        const float mix = pct (p (3));
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = b.getWritePointer (ch);
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const float x = d[i];
                const float y = fastTanh (g * lp[ch].lowpass (hp[ch].highpass (x))) * comp;
                d[i] = x + (y - x) * mix;
            }
        }
    }
private:
    double sr = 44100.0; SVF hp[2], lp[2];
};

//==============================================================================
class DoublerModule final : public Module
{
public:
    DoublerModule() : Module (ModuleId::Doubler) {}
    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        for (auto& s : shifters) { s.prepare (sr, 1); s.setNominalWindowMs (30.f); }
        for (auto& d : delays) d.prepare ((int) (0.1 * sr) + 8);
        reset();
    }
    void reset() override { for (auto& s : shifters) s.reset(); for (auto& d : delays) d.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float cents = p (0);
        shifters[0].setRatio (std::pow (2.f,  cents / 1200.f));
        shifters[1].setRatio (std::pow (2.f, -cents / 1200.f));
        const float dA = (float) (p (1) * 0.001 * sr), dB = dA * 1.5f;
        const float w = juce::jlimit (0.f, 1.f, pct (p (2)) * info.widthFactor);
        const float mix = juce::jlimit (0.f, 1.f, pct (p (3)) * juce::jmax (0.f, info.widthFactor * 0.5f + 0.5f)) * 0.7f;
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float mono = 0.5f * (L[i] + R[i]);
            float a = mono, c = mono;
            shifters[0].processFrame (&a, &a, 1);
            shifters[1].processFrame (&c, &c, 1);
            delays[0].push (a); delays[1].push (c);
            a = delays[0].read (juce::jmax (1.f, dA));
            c = delays[1].read (juce::jmax (1.f, dB));
            L[i] += mix * (a + c * (1.f - w));
            R[i] += mix * (c + a * (1.f - w));
        }
    }
private:
    double sr = 44100.0; PitchShifter shifters[2]; DelayLine delays[2];
};

class HarmonizerModule final : public Module
{
public:
    HarmonizerModule() : Module (ModuleId::Harmonizer) {}
    void prepare (double sampleRate, int) override { shifter.prepare (sampleRate, 1); shifter.setNominalWindowMs (50.f); reset(); }
    void reset() override { shifter.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        shifter.setRatio (std::pow (2.f, std::round (p (0)) / 12.f));
        const float mix = pct (p (1));
        const float pan = juce::jlimit (-1.f, 1.f, p (2) * 0.01f);
        const float ang = (pan * 0.5f + 0.5f) * kPi * 0.5f;
        const float gL = std::cos (ang) * 1.41421f * mix, gR = std::sin (ang) * 1.41421f * mix;
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            float v = 0.5f * (L[i] + R[i]);
            shifter.processFrame (&v, &v, 1);
            L[i] += v * gL; R[i] += v * gR;
        }
    }
private:
    PitchShifter shifter;
};

// Shared modulated delay used by Chorus and Flanger
class ModDelayModule : public Module
{
public:
    ModDelayModule (ModuleId id, float centreMs, float depthMs, bool hasFeedback)
        : Module (id), centre (centreMs), depthRange (depthMs), feedback (hasFeedback) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; for (auto& d : lines) d.prepare ((int) (0.05 * sr) + 8); reset(); }
    void reset() override { for (auto& d : lines) d.reset(); lfo.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        lfo.setRate (p (0), sr);
        const float depth = pct (p (1)) * depthRange * 0.001f * (float) sr;
        const float centreS = centre * 0.001f * (float) sr;
        const float fb  = feedback ? juce::jlimit (-0.95f, 0.95f, p (2) * 0.01f) : 0.f;
        const float mix = juce::jlimit (0.f, 1.f, pct (p (feedback ? 3 : 2)) * (feedback ? 1.f : info.widthFactor));
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float dl = centreS + depth * (0.5f + 0.5f * lfo.sine (0.0));
            const float dr = centreS + depth * (0.5f + 0.5f * lfo.sine (0.25));
            lfo.advance();
            const float wl = lines[0].read (juce::jmax (1.f, dl));
            const float wr = lines[1].read (juce::jmax (1.f, dr));
            lines[0].push (L[i] + fb * wl);
            lines[1].push (R[i] + fb * wr);
            if (feedback) { L[i] += (wl - L[i]) * mix; R[i] += (wr - R[i]) * mix; }
            else          { L[i] += wl * mix * 0.7f;   R[i] += wr * mix * 0.7f; }
        }
    }
private:
    double sr = 44100.0; DelayLine lines[2]; LFO lfo; float centre, depthRange; bool feedback;
};

class PhaserModule final : public Module
{
public:
    PhaserModule() : Module (ModuleId::Phaser) {}
    void prepare (double sampleRate, int maxBlock) override
    {
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) juce::jmax (1, maxBlock), 2 };
        phaser.prepare (spec);
        reset();
    }
    void reset() override { phaser.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        phaser.setRate (p (0));
        phaser.setDepth (pct (p (1)));
        phaser.setCentreFrequency (p (2));
        phaser.setFeedback (juce::jlimit (-0.95f, 0.95f, p (3) * 0.01f));
        phaser.setMix (pct (p (4)));
        juce::dsp::AudioBlock<float> block (b);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        phaser.process (ctx);
    }
private:
    juce::dsp::Phaser<float> phaser;
};

class TremoloModule final : public Module
{
public:
    TremoloModule() : Module (ModuleId::Tremolo) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { lfo.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        lfo.setRate (p (0), sr);
        const float depth = pct (p (1));
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float g = 1.f - depth * (0.5f + 0.5f * lfo.sine());
            lfo.advance();
            L[i] *= g; R[i] *= g;
        }
    }
private:
    double sr = 44100.0; LFO lfo;
};

class AutoPanModule final : public Module
{
public:
    AutoPanModule() : Module (ModuleId::AutoPan) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { lfo.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo&) override
    {
        lfo.setRate (p (0), sr);
        const float depth = pct (p (1));
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float pan = depth * lfo.sine();
            lfo.advance();
            const float ang = (pan * 0.5f + 0.5f) * kPi * 0.5f;
            L[i] *= std::cos (ang) * 1.41421f;
            R[i] *= std::sin (ang) * 1.41421f;
        }
    }
private:
    double sr = 44100.0; LFO lfo;
};

class WidenerModule final : public Module
{
public:
    WidenerModule() : Module (ModuleId::Widener) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; reset(); }
    void reset() override { hp.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float w = juce::jlimit (0.f, 2.5f, p (0) * 0.01f * info.widthFactor);
        const float lowMono = p (1);
        hp.set (juce::jmax (10.f, lowMono), 0.707f, sr);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float m = 0.5f * (L[i] + R[i]);
            const float s = 0.5f * (L[i] - R[i]);
            float sOut = s * w;
            if (lowMono > 0.f)
            {
                const float sHigh = hp.highpass (s);
                sOut = (s - sHigh) + sHigh * w; // keep lows at unity (mono-safe), widen the rest
            }
            L[i] = m + sOut; R[i] = m - sOut;
        }
    }
private:
    double sr = 44100.0; SVF hp;
};

//==============================================================================
class SlapModule final : public Module
{
public:
    SlapModule() : Module (ModuleId::Slap) {}
    void prepare (double sampleRate, int) override { sr = sampleRate; line.prepare ((int) (0.25 * sr) + 8); reset(); }
    void reset() override { line.reset(); tone.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float d = (float) (p (0) * 0.001 * sr);
        const float fb = pct (p (1));
        tone.set (p (2), 0.707f, sr);
        const float mix = juce::jlimit (0.f, 1.f, pct (p (3)) * info.spaceFactor);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float w = tone.lowpass (line.read (d));
            line.push (0.5f * (L[i] + R[i]) + fb * w);
            L[i] += w * mix; R[i] += w * mix;
        }
    }
private:
    double sr = 44100.0; DelayLine line; SVF tone;
};

class DelayModule final : public Module
{
public:
    DelayModule() : Module (ModuleId::Delay) {}
    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        for (auto& l : lines) l.prepare ((int) (2.2 * sr) + 8);
        duckEnv.set (5.f, 300.f, sr);
        aTime = msToCoeff (120.f, sr);
        reset();
    }
    void reset() override { for (auto& l : lines) l.reset(); for (auto& f : lp) f.reset(); for (auto& f : hp) f.reset(); duckEnv.reset(); time = -1.f; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        float ms = p (1);
        const int sync = (int) std::round (p (0));
        if (sync > 0 && info.bpm > 20.0)
        {
            const float beat = (float) (60000.0 / info.bpm);
            static const float mult[7] = { 1.f, 1.f, 0.5f, 0.75f, 0.25f, 2.f / 3.f, 1.f / 3.f };
            ms = beat * mult[juce::jlimit (0, 6, sync)];
        }
        const float targetTime = juce::jlimit (1.f, (float) lines[0].capacity(), (float) (ms * 0.001 * sr));
        if (time < 0.f) time = targetTime;
        const float fb = pct (p (2)) * 0.95f;
        const float spread = pct (p (3));
        for (auto& f : lp) f.set (p (4), 0.707f, sr);
        for (auto& f : hp) f.set (p (5), 0.707f, sr);
        const float duck = pct (p (6));
        const float mix = juce::jlimit (0.f, 1.f, pct (p (7)) * info.spaceFactor);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            time = targetTime + aTime * (time - targetTime);
            const float yl = lines[0].read (time);
            const float yr = lines[1].read (time);
            const float fl = hp[0].highpass (lp[0].lowpass ((1.f - spread) * yl + spread * yr));
            const float fr = hp[1].highpass (lp[1].lowpass ((1.f - spread) * yr + spread * yl));
            lines[0].push (L[i] + fb * fl);
            lines[1].push (R[i] + fb * fr);
            const float inLvl = duckEnv.process (juce::jmax (std::abs (L[i]), std::abs (R[i])));
            const float dg = 1.f - duck * juce::jlimit (0.f, 1.f, (gainToDb (inLvl) + 40.f) / 30.f);
            L[i] += yl * mix * dg; R[i] += yr * mix * dg;
        }
    }
private:
    double sr = 44100.0; DelayLine lines[2]; SVF lp[2], hp[2]; EnvelopeFollower duckEnv; float time = -1.f, aTime = 0.f;
};

class ReverbModule final : public Module
{
public:
    ReverbModule() : Module (ModuleId::Reverb) {}
    void prepare (double sampleRate, int maxBlock) override
    {
        sr = sampleRate;
        reverb.setSampleRate (sr);
        for (auto& l : pre) l.prepare ((int) (0.25 * sr) + 8);
        wet.setSize (2, juce::jmax (1, maxBlock));
        duckEnv.set (5.f, 350.f, sr);
        reset();
    }
    void reset() override { reverb.reset(); for (auto& l : pre) l.reset(); for (auto& f : tone) f.reset(); duckEnv.reset(); }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float size = pct (p (0)), decay = pct (p (1));
        juce::Reverb::Parameters rp;
        rp.roomSize = juce::jmap (0.4f * size + 0.6f * decay, 0.f, 1.f, 0.35f, 0.98f);
        rp.damping  = 0.95f - 0.7f * decay;
        rp.wetLevel = 1.f; rp.dryLevel = 0.f;
        rp.width    = pct (p (4));
        rp.freezeMode = 0.f;
        reverb.setParameters (rp);
        const float preS = (float) (p (2) * 0.001 * sr);
        for (auto& f : tone) f.set (p (3), 0.707f, sr);
        const float duck = pct (p (5));
        const float mix = juce::jlimit (0.f, 1.f, pct (p (6)) * info.spaceFactor);

        const int n = b.getNumSamples();
        if (wet.getNumSamples() < n) wet.setSize (2, n, false, false, true);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        auto* wl = wet.getWritePointer (0); auto* wr = wet.getWritePointer (1);
        for (int i = 0; i < n; ++i)
        {
            pre[0].push (L[i]); pre[1].push (R[i]);
            wl[i] = preS < 1.f ? L[i] : pre[0].read (preS);
            wr[i] = preS < 1.f ? R[i] : pre[1].read (preS);
        }
        reverb.processStereo (wl, wr, n);
        for (int i = 0; i < n; ++i)
        {
            const float inLvl = duckEnv.process (juce::jmax (std::abs (L[i]), std::abs (R[i])));
            const float dg = 1.f - duck * juce::jlimit (0.f, 1.f, (gainToDb (inLvl) + 40.f) / 30.f);
            L[i] += tone[0].lowpass (wl[i]) * mix * dg;
            R[i] += tone[1].lowpass (wr[i]) * mix * dg;
        }
    }
private:
    double sr = 44100.0; juce::Reverb reverb; DelayLine pre[2]; SVF tone[2]; EnvelopeFollower duckEnv; juce::AudioBuffer<float> wet;
};

//==============================================================================
// Dynamic plosive filter: a high-pass that only rises while a low-frequency thump is happening.
class PopFilterModule final : public Module
{
public:
    PopFilterModule() : Module (ModuleId::PopFilter) {}
    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        detect.set (110.f, 0.707f, sr);
        fastEnv.set (0.3f, 60.f, sr);
        slowEnv.set (60.f, 600.f, sr);
        fullEnv.set (0.3f, 60.f, sr);
        aUp = msToCoeff (0.8f, sr);
        reset();
    }
    void reset() override { detect.reset(); fastEnv.reset(); slowEnv.reset(); fullEnv.reset(); for (auto& f : hp) f.reset(); cutoff = 20.f; lastSet = -1.f; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float sens = pct (p (0)) * juce::jlimit (0.5f, 1.5f, info.cleanFactor);
        const float ratioNeeded = 10.f - sens * 8.5f;             // 10x (quiet) .. 1.5x (eager) above the running low-end level
        const float maxCut = 60.f + pct (p (1)) * 240.f;           // up to 300 Hz while a plosive lasts
        const float aDown = msToCoeff (p (2), sr);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float mono = 0.5f * (L[i] + R[i]);
            const float low = detect.lowpass (mono);
            const float lf = fastEnv.process (low);
            const float ref = slowEnv.process (low);
            const float full = fullEnv.process (mono);
            const bool thump = lf > 0.01f && lf > ref * ratioNeeded && lf > full * 0.6f;
            const float target = thump ? maxCut : 20.f;
            cutoff = target + (target > cutoff ? aUp : aDown) * (cutoff - target);
            if (std::abs (cutoff - lastSet) > 1.f) { for (auto& f : hp) f.set (cutoff, 0.707f, sr); lastSet = cutoff; }
            L[i] = hp[0].highpass (L[i]);
            R[i] = hp[1].highpass (R[i]);
        }
    }
private:
    double sr = 44100.0; SVF detect, hp[2]; EnvelopeFollower fastEnv, slowEnv, fullEnv; float cutoff = 20.f, lastSet = -1.f, aUp = 0.f;
};

//==============================================================================
// Breath control: quiet, noisy (high zero-crossing / high-frequency heavy) passages are turned down.
class BreathModule final : public Module
{
public:
    BreathModule() : Module (ModuleId::Breath) {}
    void prepare (double sampleRate, int) override
    {
        sr = sampleRate;
        hpf.set (3000.f, 0.707f, sr);
        levelEnv.set (5.f, 80.f, sr);
        hfEnv.set (5.f, 80.f, sr);
        aAtt = msToCoeff (8.f, sr);
        zcrCoeff = msToCoeff (40.f, sr);
        reset();
    }
    void reset() override { hpf.reset(); levelEnv.reset(); hfEnv.reset(); gain = 1.f; zcr = 0.f; lastPositive = true; }
    void process (juce::AudioBuffer<float>& b, const ProcessInfo& info) override
    {
        const float sens = pct (p (0)) * juce::jlimit (0.5f, 1.5f, info.cleanFactor);
        const float threshLin = dbToGain (-48.f + sens * 24.f);   // breaths louder than this still count: -48 dB .. -24 dB
        const float reductionGain = dbToGain (-p (1));
        const float aRel = msToCoeff (p (2), sr);
        auto* L = b.getWritePointer (0); auto* R = b.getWritePointer (1);
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float mono = 0.5f * (L[i] + R[i]);
            const float level = levelEnv.process (mono);
            const float hf = hfEnv.process (hpf.highpass (mono));
            const bool positive = mono >= 0.f;
            const float crossing = positive != lastPositive ? 1.f : 0.f;
            lastPositive = positive;
            zcr = crossing + zcrCoeff * (zcr - crossing);           // fraction of samples with a sign change
            const bool noisy = hf > level * 0.35f || zcr > 0.12f;
            const bool breath = level > 0.0003f && level < threshLin && noisy;
            const float target = breath ? reductionGain : 1.f;
            gain = target + (target < gain ? aAtt : aRel) * (gain - target);
            L[i] *= gain; R[i] *= gain;
        }
    }
private:
    double sr = 44100.0; SVF hpf; EnvelopeFollower levelEnv, hfEnv; float gain = 1.f, zcr = 0.f, aAtt = 0.f, zcrCoeff = 0.f; bool lastPositive = true;
};

} // namespace

//==============================================================================
std::unique_ptr<Module> createModule (ModuleId id)
{
    switch (id)
    {
        case ModuleId::Gate:        return std::make_unique<GateModule>();
        case ModuleId::Tune:        return std::make_unique<TuneModule>();
        case ModuleId::LowCut:      return std::make_unique<CutModule> (ModuleId::LowCut, true);
        case ModuleId::HighCut:     return std::make_unique<CutModule> (ModuleId::HighCut, false);
        case ModuleId::EQ:          return std::make_unique<EQModule>();
        case ModuleId::Air:         return std::make_unique<AirModule>();
        case ModuleId::DeEsser:     return std::make_unique<DeEsserModule>();
        case ModuleId::Compressor:  return std::make_unique<CompressorModule>();
        case ModuleId::Glue:        return std::make_unique<GlueModule>();
        case ModuleId::Limiter:     return std::make_unique<LimiterModule>();
        case ModuleId::Transient:   return std::make_unique<TransientModule>();
        case ModuleId::Saturation:  return std::make_unique<SaturationModule>();
        case ModuleId::Distortion:  return std::make_unique<DistortionModule>();
        case ModuleId::Exciter:     return std::make_unique<ExciterModule>();
        case ModuleId::LoFi:        return std::make_unique<LoFiModule>();
        case ModuleId::Telephone:   return std::make_unique<TelephoneModule>();
        case ModuleId::Doubler:     return std::make_unique<DoublerModule>();
        case ModuleId::Harmonizer:  return std::make_unique<HarmonizerModule>();
        case ModuleId::Chorus:      return std::make_unique<ModDelayModule> (ModuleId::Chorus, 7.f, 4.f, false);
        case ModuleId::Flanger:     return std::make_unique<ModDelayModule> (ModuleId::Flanger, 1.5f, 2.5f, true);
        case ModuleId::Phaser:      return std::make_unique<PhaserModule>();
        case ModuleId::Tremolo:     return std::make_unique<TremoloModule>();
        case ModuleId::AutoPan:     return std::make_unique<AutoPanModule>();
        case ModuleId::Widener:     return std::make_unique<WidenerModule>();
        case ModuleId::Slap:        return std::make_unique<SlapModule>();
        case ModuleId::Delay:       return std::make_unique<DelayModule>();
        case ModuleId::Reverb:      return std::make_unique<ReverbModule>();
        case ModuleId::PopFilter:   return std::make_unique<PopFilterModule>();
        case ModuleId::Breath:      return std::make_unique<BreathModule>();
        case ModuleId::Count:
        default: jassertfalse;     return std::make_unique<GateModule>();
    }
}

} // namespace vocalyst
