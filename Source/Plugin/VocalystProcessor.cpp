#include "VocalystProcessor.h"
#include "VocalystEditor.h"

namespace vocalyst
{

//==============================================================================
static juce::String formatValue (const ParamSpec& spec, float v)
{
    const juce::String unit (spec.unit);
    if (unit == "Hz")
        return v >= 1000.f ? juce::String (v / 1000.f, 2) + " kHz" : juce::String ((int) std::round (v)) + " Hz";
    if (unit == "ms")
        return v >= 1000.f ? juce::String (v / 1000.f, 2) + " s" : juce::String (v, v < 10.f ? 1 : 0) + " ms";
    if (unit == "dB")  return juce::String (v, 1) + " dB";
    if (unit == ":1")  return juce::String (v, 1) + ":1";
    if (unit == "%")   return juce::String ((int) std::round (v)) + " %";
    if (unit == "st")  return (v > 0 ? "+" : "") + juce::String ((int) std::round (v)) + " st";
    if (unit == "ct")  return juce::String ((int) std::round (v)) + " ct";
    if (unit == "bit") return juce::String ((int) std::round (v)) + " bit";
    if (unit.isEmpty() && spec.max - spec.min >= 50.f) return juce::String ((int) std::round (v));
    return juce::String (v, 2);
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalystProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    auto makeFloat = [] (const String& id, const String& name, float min, float max, float def, float skew, const char* unit, int step)
    {
        NormalisableRange<float> range (min, max, step > 0 ? (float) step : 0.f, skew);
        const ParamSpec spec { "", "", min, max, def, skew, unit, step, nullptr };
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, def,
            AudioParameterFloatAttributes().withLabel (unit)
                .withStringFromValueFunction ([spec] (float v, int) { return formatValue (spec, v); }));
    };

    for (auto& m : allModules())
    {
        params.push_back (std::make_unique<AudioParameterBool> (ParameterID { enableId (m.id), 1 }, String (m.name) + " On", true));
        for (auto& p : m.params)
        {
            const String id = paramId (m.id, p.key);
            const String name = String (m.name) + " " + p.name;
            if (p.choices != nullptr)
            {
                StringArray choices;
                choices.addTokens (p.choices, ",", "");
                params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, choices, (int) p.def));
            }
            else
            {
                const ParamSpec spec = p;
                NormalisableRange<float> range (p.min, p.max, p.step > 0 ? (float) p.step : 0.f, p.skew);
                params.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, range, p.def,
                    AudioParameterFloatAttributes().withLabel (p.unit)
                        .withStringFromValueFunction ([spec] (float v, int) { return formatValue (spec, v); })));
            }
        }
    }

    params.push_back (makeFloat (ids::inGain,  "Input",  -24.f, 24.f, 0.f,   1.f, "dB", 0));
    params.push_back (makeFloat (ids::outGain, "Output", -24.f, 24.f, 0.f,   1.f, "dB", 0));
    params.push_back (makeFloat (ids::mix,     "Mix",    0.f,   100.f, 100.f, 1.f, "%", 0));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ids::key, 1 },   "Key",   scales::keyNames(),   0));
    params.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ids::scale, 1 }, "Scale", scales::scaleNames(), scales::Chromatic));
    params.push_back (makeFloat (ids::macroTune,  "Tune",  0.f,    100.f, 50.f, 1.f, "%", 0));
    params.push_back (makeFloat (ids::macroClean, "Clean", 0.f,    100.f, 50.f, 1.f, "%", 0));
    params.push_back (makeFloat (ids::macroTone,  "Tone",  -100.f, 100.f, 0.f,  1.f, "",  0));
    params.push_back (makeFloat (ids::macroSpace, "Space", 0.f,    100.f, 50.f, 1.f, "%", 0));
    params.push_back (makeFloat (ids::macroWidth, "Width", 0.f,    100.f, 50.f, 1.f, "%", 0));
    params.push_back (makeFloat (ids::macroDrive, "Drive", 0.f,    100.f, 50.f, 1.f, "%", 0));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::bypass, 1 }, "Bypass", false));
    params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::match, 1 },  "Loudness Match", false));
    for (int i = 0; i < 12; ++i)
        params.push_back (std::make_unique<AudioParameterBool> (ParameterID { ids::note (i), 1 }, "Note " + scales::keyNames()[i], true));
    return { params.begin(), params.end() };
}

//==============================================================================
VocalystProcessor::VocalystProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "VocalystParams", createLayout())
{
    for (int i = 0; i < kNumModules; ++i)
    {
        modules[(size_t) i] = createModule ((ModuleId) i);
        modules[(size_t) i]->attachParameters (apvts);
    }

    pInGain = apvts.getRawParameterValue (ids::inGain);
    pOutGain = apvts.getRawParameterValue (ids::outGain);
    pMix = apvts.getRawParameterValue (ids::mix);
    pKey = apvts.getRawParameterValue (ids::key);
    pScale = apvts.getRawParameterValue (ids::scale);
    pMTune = apvts.getRawParameterValue (ids::macroTune);
    pMClean = apvts.getRawParameterValue (ids::macroClean);
    pMTone = apvts.getRawParameterValue (ids::macroTone);
    pMSpace = apvts.getRawParameterValue (ids::macroSpace);
    pMWidth = apvts.getRawParameterValue (ids::macroWidth);
    pMDrive = apvts.getRawParameterValue (ids::macroDrive);
    pBypass = apvts.getRawParameterValue (ids::bypass);
    pMatch = apvts.getRawParameterValue (ids::match);
    pTuneMidi = apvts.getRawParameterValue (paramId (ModuleId::Tune, "midi"));
    for (int i = 0; i < 12; ++i) pNotes[(size_t) i] = apvts.getRawParameterValue (ids::note (i));
    apvts.addParameterListener (ids::key, this);
    apvts.addParameterListener (ids::scale, this);

    uiState.setProperty ("mode", "simple", nullptr);
    uiState.setProperty ("tab", 0, nullptr);
    uiState.setProperty ("autoKeyLink", false, nullptr);
    apvts.state.addChild (uiState, -1, nullptr);

    const int init = juce::jmax (0, presets::indexOf ("Init - Clean Chain"));
    if (! presets::factory().empty())
        loadFactoryPreset (init);
    else
        setChainOrder (defaultChain());

    startTimer (150);
}

VocalystProcessor::~VocalystProcessor()
{
    stopTimer();
    apvts.removeParameterListener (ids::key, this);
    apvts.removeParameterListener (ids::scale, this);
}

std::vector<ModuleId> VocalystProcessor::defaultChain()
{
    return { ModuleId::Gate, ModuleId::Tune, ModuleId::LowCut, ModuleId::DeEsser, ModuleId::EQ, ModuleId::Compressor,
             ModuleId::Saturation, ModuleId::Glue, ModuleId::Air, ModuleId::Delay, ModuleId::Reverb, ModuleId::Limiter };
}

//==============================================================================
void VocalystProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    const int block = juce::jmax (1, samplesPerBlock);
    work.setSize (2, block);
    dry.setSize (2, block);
    monoScratch.assign ((size_t) block, 0.f);
    for (auto& m : modules) m->prepare (sampleRate, block);
    wasActive.fill (false);
    for (auto& t : tilt) t.prepare (sampleRate, 1000.f);
    keyDetector.prepare (sampleRate);
    vocalAnalyzer.prepare (sampleRate);
    meterEngine.prepare (sampleRate);
    for (auto& d : dryDelay) d.prepare ((int) (0.15 * sampleRate) + 16);
    dryPow = wetPow = 0.f; matchGain = 1.f; matchGainDb.store (0.f);
    inGainSm.reset (sampleRate, 0.02); outGainSm.reset (sampleRate, 0.02); mixSm.reset (sampleRate, 0.02);
    inGainSm.setCurrentAndTargetValue (dsp::dbToGain (pInGain->load()));
    outGainSm.setCurrentAndTargetValue (dsp::dbToGain (pOutGain->load()));
    mixSm.setCurrentAndTargetValue (pMix->load() * 0.01f);
    orderDirty.store (true);
}

void VocalystProcessor::releaseResources() {}

bool VocalystProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo()) return false;
    if (in == juce::AudioChannelSet::stereo() && out == juce::AudioChannelSet::mono()) return false;
    return true;
}

void VocalystProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels();
    const int outCh = getTotalNumOutputChannels();
    if (n == 0 || outCh == 0) return;

    if (work.getNumSamples() < n) { work.setSize (2, n, false, false, true); dry.setSize (2, n, false, false, true); monoScratch.resize ((size_t) n); }

    // pick up a new chain order
    if (orderDirty.load())
    {
        juce::SpinLock::ScopedTryLockType tl (orderLock);
        if (tl.isLocked())
        {
            audioOrder = pendingOrder;
            audioCount = pendingCount;
            orderDirty.store (false);
        }
    }

    // ---- input stage: always work in stereo
    inGainSm.setTargetValue (dsp::dbToGain (pInGain->load()));
    outGainSm.setTargetValue (dsp::dbToGain (pOutGain->load()));
    mixSm.setTargetValue (pMix->load() * 0.01f);

    auto* wl = work.getWritePointer (0); auto* wr = work.getWritePointer (1);
    const float* inL = buffer.getReadPointer (0);
    const float* inR = inCh > 1 ? buffer.getReadPointer (1) : inL;
    float pkL = 0.f, pkR = 0.f;
    for (int i = 0; i < n; ++i)
    {
        const float g = inGainSm.getNextValue();
        wl[i] = inL[i] * g; wr[i] = inR[i] * g;
        monoScratch[(size_t) i] = 0.5f * (wl[i] + wr[i]);
        pkL = juce::jmax (pkL, std::abs (wl[i])); pkR = juce::jmax (pkR, std::abs (wr[i]));
    }
    inPeak[0].store (pkL); inPeak[1].store (pkR);
    dry.copyFrom (0, 0, work, 0, 0, n);
    dry.copyFrom (1, 0, work, 1, 0, n);

    // ---- MIDI: held notes can steer Tune
    for (const auto meta : midi)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOn())       heldNotes[(size_t) m.getNoteNumber()] = true;
        else if (m.isNoteOff()) heldNotes[(size_t) m.getNoteNumber()] = false;
        else if (m.isAllNotesOff() || m.isAllSoundOff()) heldNotes.fill (false);
    }
    {
        int hm = 0;
        for (int k = 0; k < 128; ++k) if (heldNotes[(size_t) k]) hm |= 1 << (k % 12);
        heldMask.store (hm);
    }

    // ---- per-block info
    ProcessInfo info;
    info.trace = &trace;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            info.transportPlaying = pos->getIsPlaying();

    if (info.transportPlaying)   // analysers only listen while the song is actually running
    {
        keyDetector.push (monoScratch.data(), n);
        vocalAnalyzer.push (monoScratch.data(), n);
    }
    {
        int mask = 0;
        for (int i = 0; i < 12; ++i) if (pNotes[(size_t) i]->load() > 0.5f) mask |= 1 << i;
        const int hm = heldMask.load();
        if (pTuneMidi != nullptr && pTuneMidi->load() > 0.5f && hm != 0) mask = hm;
        info.noteMask = mask == 0 ? 0xFFF : mask;
    }
    info.key = (int) pKey->load();
    info.scale = (int) pScale->load();
    info.tuneFactor  = pMTune->load() / 50.f;
    info.cleanFactor = pMClean->load() / 50.f;
    info.spaceFactor = pMSpace->load() / 50.f;
    info.widthFactor = pMWidth->load() / 50.f;
    info.driveFactor = pMDrive->load() / 50.f;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto bpm = pos->getBpm())
                if (*bpm > 20.0) info.bpm = *bpm;

    // ---- the chain
    juce::AudioBuffer<float> chainBuf (work.getArrayOfWritePointers(), 2, n);
    std::array<bool, kNumModules> nowActive {};
    int latency = 0;
    const bool bypassed = pBypass->load() > 0.5f;
    for (int k = 0; k < audioCount; ++k)
    {
        const int idx = audioOrder[(size_t) k];
        if (idx < 0 || idx >= kNumModules) continue;
        auto& m = *modules[(size_t) idx];
        if (! m.isEnabled()) continue;
        latency += m.latencySamples();          // the chain's latency is reported even while bypassed
        if (bypassed) continue;
        nowActive[(size_t) idx] = true;
        if (! wasActive[(size_t) idx]) m.reset();
        m.process (chainBuf, info);
    }
    wasActive = nowActive;
    desiredLatency.store (latency);
    const int reportedLatency = desiredLatency.load();

    // ---- loudness match: measure dry vs processed (1 s exponential RMS, gated at -60 dBFS)
    {
        float dp = 0.f, wp = 0.f;
        for (int i = 0; i < n; ++i)
        {
            const float d = 0.5f * (dry.getReadPointer (0)[i] + dry.getReadPointer (1)[i]);
            const float w = 0.5f * (wl[i] + wr[i]);
            dp += d * d; wp += w * w;
        }
        dp /= (float) n; wp /= (float) n;
        if (dp > 1.0e-6f)
        {
            const float a = 1.f - std::exp (-(float) n / (float) currentSampleRate);
            dryPow += a * (dp - dryPow);
            wetPow += a * (wp - wetPow);
        }
        dryDb.store (dsp::gainToDb (std::sqrt (dryPow)));
        wetDb.store (dsp::gainToDb (std::sqrt (wetPow)));
        float target = 1.f;
        if (pMatch->load() > 0.5f && dryPow > 1.0e-7f && wetPow > 1.0e-7f)
            target = juce::jlimit (dsp::dbToGain (-12.f), dsp::dbToGain (12.f), std::sqrt (dryPow / wetPow));
        const float a2 = 1.f - std::exp (-(float) n / (0.3f * (float) currentSampleRate));
        matchGain += a2 * (target - matchGain);
        matchGainDb.store (dsp::gainToDb (matchGain));
    }

    if (bypassed)
    {
        // keep the dry signal time-aligned with the latency the host compensates for
        for (int ch = 0; ch < 2; ++ch)
        {
            auto* d = work.getWritePointer (ch);
            const auto* src = dry.getReadPointer (ch);
            for (int i = 0; i < n; ++i)
            {
                dryDelay[ch].push (src[i]);
                d[i] = reportedLatency > 0 ? dryDelay[ch].read ((float) reportedLatency) : src[i];
            }
        }
    }
    else
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto* src = dry.getReadPointer (ch);
            for (int i = 0; i < n; ++i) dryDelay[ch].push (src[i]);
        }
    }

    // ---- tone macro (tilt), mix, output
    const float t = pMTone->load() * 0.01f;
    for (auto& x : tilt) x.setTilt (t);
    const bool useTilt = std::abs (t) > 1.0e-3f;
    auto* dl = dry.getReadPointer (0); auto* dr = dry.getReadPointer (1);
    float opL = 0.f, opR = 0.f;
    for (int i = 0; i < n; ++i)
    {
        float l = wl[i], r = wr[i];
        if (useTilt && ! bypassed) { l = tilt[0].process (l); r = tilt[1].process (r); }
        const float mix = mixSm.getNextValue();
        const float og = outGainSm.getNextValue() * (bypassed ? 1.f : matchGain);
        if (! bypassed) { l = dl[i] + (l - dl[i]) * mix; r = dr[i] + (r - dr[i]) * mix; }
        l *= og; r *= og;
        if (! std::isfinite (l)) l = 0.f;
        if (! std::isfinite (r)) r = 0.f;
        wl[i] = l; wr[i] = r;
        opL = juce::jmax (opL, std::abs (l)); opR = juce::jmax (opR, std::abs (r));
    }
    outPeak[0].store (opL); outPeak[1].store (opR);
    meterEngine.push (dry.getReadPointer (0), dry.getReadPointer (1), wl, wr, n);

    if (outCh == 1)
    {
        auto* o = buffer.getWritePointer (0);
        for (int i = 0; i < n; ++i) o[i] = 0.5f * (wl[i] + wr[i]);
    }
    else
    {
        buffer.copyFrom (0, 0, work, 0, 0, n);
        buffer.copyFrom (1, 0, work, 1, 0, n);
        for (int ch = 2; ch < outCh; ++ch) buffer.clear (ch, 0, n);
    }
}

//==============================================================================
void VocalystProcessor::timerCallback()
{
    const int want = desiredLatency.load();
    if (want != getLatencySamples())
        setLatencySamples (want);

    if (maskDirty.exchange (false))
        setNotesFromScale();

    if (isAutoKeyLinked())
    {
        auto f = sharedkey::file();
        const auto mod = f.getLastModificationTime().toMilliseconds();
        if (f.existsAsFile() && mod != lastSharedTime)
        {
            lastSharedTime = mod;
            applySharedKey();
        }
    }
}

//==============================================================================
void VocalystProcessor::publishOrder()
{
    {
        const juce::SpinLock::ScopedLockType sl (orderLock);
        pendingCount = juce::jmin ((int) uiOrder.size(), kMaxChain);
        for (int i = 0; i < pendingCount; ++i) pendingOrder[(size_t) i] = (int) uiOrder[(size_t) i];
        orderDirty.store (true);
    }
    chainToState();
    listeners.call ([] (Listener& l) { l.chainChanged(); });
}

void VocalystProcessor::setChainOrder (const std::vector<ModuleId>& order)
{
    std::vector<ModuleId> clean;
    for (auto id : order)
        if ((int) id >= 0 && (int) id < kNumModules && std::find (clean.begin(), clean.end(), id) == clean.end())
            clean.push_back (id);
    uiOrder = clean;
    publishOrder();
}

void VocalystProcessor::addModule (ModuleId id, int index)
{
    if (isInChain (id)) return;
    if (index < 0 || index > (int) uiOrder.size()) uiOrder.push_back (id);
    else uiOrder.insert (uiOrder.begin() + index, id);
    if (auto* p = apvts.getParameter (enableId (id)))
        p->setValueNotifyingHost (1.f);
    publishOrder();
}

void VocalystProcessor::removeModule (ModuleId id)
{
    auto it = std::find (uiOrder.begin(), uiOrder.end(), id);
    if (it == uiOrder.end()) return;
    uiOrder.erase (it);
    publishOrder();
}

void VocalystProcessor::moveModule (int from, int to)
{
    const int n = (int) uiOrder.size();
    if (from < 0 || from >= n || to < 0 || to >= n || from == to) return;
    auto id = uiOrder[(size_t) from];
    uiOrder.erase (uiOrder.begin() + from);
    uiOrder.insert (uiOrder.begin() + to, id);
    publishOrder();
}

bool VocalystProcessor::isInChain (ModuleId id) const
{
    return std::find (uiOrder.begin(), uiOrder.end(), id) != uiOrder.end();
}

void VocalystProcessor::chainToState()
{
    juce::StringArray keys;
    for (auto id : uiOrder) keys.add (moduleSpec (id).key);
    uiState.setProperty ("chain", keys.joinIntoString (","), nullptr);
}

void VocalystProcessor::stateToChain()
{
    juce::StringArray keys;
    keys.addTokens (uiState.getProperty ("chain").toString(), ",", "");
    std::vector<ModuleId> order;
    for (auto& k : keys)
        if (auto* m = findModuleByKey (k.trim()))
            order.push_back (m->id);
    if (order.empty() && ! keys.isEmpty()) order = defaultChain();
    setChainOrder (order);
}

//==============================================================================
void VocalystProcessor::setParamValue (const juce::String& id, float displayValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (displayValue));
}

float VocalystProcessor::getParamValue (const juce::String& id) const
{
    if (auto* p = apvts.getParameter (id))
        return p->convertFrom0to1 (p->getValue());
    return 0.f;
}

void VocalystProcessor::loadPreset (const Preset& preset)
{
    // reset every module parameter to default (keeps key/scale and I/O gains)
    for (auto& m : allModules())
    {
        if (auto* on = apvts.getParameter (enableId (m.id))) on->setValueNotifyingHost (on->getDefaultValue());
        for (auto& ps : m.params)
            if (auto* p = apvts.getParameter (paramId (m.id, ps.key))) p->setValueNotifyingHost (p->getDefaultValue());
    }
    for (const char* macro : { ids::macroTune, ids::macroClean, ids::macroTone, ids::macroSpace, ids::macroWidth, ids::macroDrive })
        if (auto* p = apvts.getParameter (macro)) p->setValueNotifyingHost (p->getDefaultValue());

    for (auto& kv : preset.values)
        setParamValue (kv.first, kv.second);

    std::vector<ModuleId> order;
    for (auto& k : preset.chain)
        if (auto* m = findModuleByKey (k)) order.push_back (m->id);

    currentFactoryIndex = presets::indexOf (preset.name);
    uiState.setProperty ("presetName", preset.name, nullptr);
    uiState.setProperty ("presetGenre", preset.genre, nullptr);
    uiState.setProperty ("presetAdlib", preset.adlib, nullptr);
    setChainOrder (order);
    listeners.call ([] (Listener& l) { l.presetChanged(); });
}

void VocalystProcessor::applyModulePreset (ModuleId id, int presetIndex)
{
    auto& spec = moduleSpec (id);
    if (presetIndex < 0 || presetIndex >= (int) spec.presets.size()) return;
    for (auto& kv : spec.presets[(size_t) presetIndex].values)
        setParamValue (paramId (id, kv.first), kv.second);
    const auto name = getCurrentPresetName();
    if (! name.endsWith (" *")) uiState.setProperty ("presetName", name + " *", nullptr);
    currentFactoryIndex = -1;
    listeners.call ([] (Listener& l) { l.presetChanged(); });
}

void VocalystProcessor::loadFactoryPreset (int index)
{
    auto& f = presets::factory();
    if (f.empty()) return;
    index = juce::jlimit (0, (int) f.size() - 1, index);
    loadPreset (f[(size_t) index]);
}

void VocalystProcessor::nextPreset()
{
    const int n = (int) presets::factory().size();
    if (n == 0) return;
    loadFactoryPreset ((currentFactoryIndex + 1 + n) % n);
}

void VocalystProcessor::prevPreset()
{
    const int n = (int) presets::factory().size();
    if (n == 0) return;
    loadFactoryPreset ((juce::jmax (0, currentFactoryIndex) - 1 + n) % n);
}

Preset VocalystProcessor::captureCurrentPreset (const juce::String& name, const juce::String& genre,
                                            const juce::String& description, const juce::String& author, bool adlib) const
{
    Preset p;
    p.name = name; p.genre = genre; p.description = description; p.author = author; p.adlib = adlib;
    for (auto id : uiOrder)
    {
        auto& spec = moduleSpec (id);
        p.chain.push_back (spec.key);
        p.values.push_back ({ enableId (id), getParamValue (enableId (id)) });
        for (auto& ps : spec.params)
            p.values.push_back ({ paramId (id, ps.key), getParamValue (paramId (id, ps.key)) });
    }
    for (const char* macro : { ids::macroTune, ids::macroClean, ids::macroTone, ids::macroSpace, ids::macroWidth, ids::macroDrive })
        p.values.push_back ({ macro, getParamValue (macro) });
    return p;
}

juce::String VocalystProcessor::getCurrentPresetName() const
{
    return uiState.getProperty ("presetName", "Init").toString();
}

const juce::String VocalystProcessor::getProgramName (int index)
{
    auto& f = presets::factory();
    if (index < 0 || index >= (int) f.size()) return {};
    return f[(size_t) index].name;
}

//==============================================================================
bool VocalystProcessor::isSimpleMode() const { return uiState.getProperty ("mode", "simple").toString() == "simple"; }
void VocalystProcessor::setSimpleMode (bool simple) { uiState.setProperty ("mode", simple ? "simple" : "advanced", nullptr); }

int  VocalystProcessor::getKey() const   { return (int) pKey->load(); }
int  VocalystProcessor::getScale() const { return (int) pScale->load(); }

void VocalystProcessor::setKey (int key, int scale)
{
    setParamValue (ids::key, (float) juce::jlimit (0, 11, key));
    setParamValue (ids::scale, (float) juce::jlimit (0, scales::kNumScales - 1, scale));
    setNotesFromScale();
    listeners.call ([] (Listener& l) { l.keyChanged(); });
}

void VocalystProcessor::parameterChanged (const juce::String&, float)
{
    maskDirty.store (true);   // applied on the message thread by the timer
}

int VocalystProcessor::getNoteMask() const
{
    int mask = 0;
    for (int i = 0; i < 12; ++i) if (pNotes[(size_t) i] != nullptr && pNotes[(size_t) i]->load() > 0.5f) mask |= 1 << i;
    return mask;
}

void VocalystProcessor::setNoteMask (int mask12)
{
    for (int i = 0; i < 12; ++i)
        if (auto* p = apvts.getParameter (ids::note (i)))
            p->setValueNotifyingHost (((mask12 >> i) & 1) ? 1.f : 0.f);
    listeners.call ([] (Listener& l) { l.keyChanged(); });
}

void VocalystProcessor::setNotesFromScale()
{
    maskDirty.store (false);
    setNoteMask (scales::maskForKey (getKey(), getScale()));
}

bool VocalystProcessor::applySharedKey()
{
    auto info = sharedkey::read();
    lastShared = info;
    if (! info.valid) return false;
    setKey (info.key, info.scale);
    return true;
}

void VocalystProcessor::applySuggestions (const std::vector<VocalAnalyzer::Suggestion>& suggestions)
{
    for (auto& s : suggestions)
    {
        for (auto& key : s.modules)
        {
            if (auto* m = findModuleByKey (key))
            {
                if (isInChain (m->id)) continue;
                // insert at the canonical position (before the first module with a higher id)
                int pos = (int) uiOrder.size();
                for (int i = 0; i < (int) uiOrder.size(); ++i)
                    if ((int) uiOrder[(size_t) i] > (int) m->id && uiOrder[(size_t) i] != ModuleId::Limiter) { pos = i; break; }
                if (uiOrder.size() > 0 && uiOrder.back() == ModuleId::Limiter && pos == (int) uiOrder.size()) pos = (int) uiOrder.size() - 1;
                addModule (m->id, pos);
            }
        }
        for (auto& kv : s.values)
            setParamValue (kv.first, kv.second);
    }
    uiState.setProperty ("presetName", getCurrentPresetName().upToLastOccurrenceOf (" *", false, false) + " *", nullptr);
    currentFactoryIndex = -1;
    listeners.call ([] (Listener& l) { l.presetChanged(); });
}

//==============================================================================
void VocalystProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    chainToState();
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VocalystProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return;
    auto newState = juce::ValueTree::fromXml (*xml);
    auto ui = newState.getChildWithName ("ui");
    if (! ui.isValid())
    {
        ui = juce::ValueTree ("ui");
        newState.addChild (ui, -1, nullptr);
    }
    apvts.replaceState (newState);
    maskDirty.store (false);
    uiState = apvts.state.getChildWithName ("ui");
    currentFactoryIndex = presets::indexOf (uiState.getProperty ("presetName").toString());
    stateToChain();
    listeners.call ([] (Listener& l) { l.presetChanged(); l.keyChanged(); });
}

//==============================================================================
juce::AudioProcessorEditor* VocalystProcessor::createEditor()
{
    return new VocalystEditor (*this);
}

} // namespace vocalyst

//==============================================================================
#ifndef VOCALYST_STANDALONE_TEST
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new vocalyst::VocalystProcessor();
}
#endif
