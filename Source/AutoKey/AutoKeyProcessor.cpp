#include "AutoKeyProcessor.h"
#include "AutoKeyEditor.h"

namespace vocalyst
{

AutoKeyProcessor::AutoKeyProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    startTimer (1000);
}

AutoKeyProcessor::~AutoKeyProcessor() { stopTimer(); }

void AutoKeyProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    keyDetector.prepare (sampleRate);
    mono.assign ((size_t) juce::jmax (1, samplesPerBlock), 0.f);
}

bool AutoKeyProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet()
        && (layouts.getMainInputChannelSet() == juce::AudioChannelSet::mono() || layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}

void AutoKeyProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    if (n == 0 || ch == 0) return;
    if ((int) mono.size() < n) mono.resize ((size_t) n);
    float pk = 0.f;
    for (int i = 0; i < n; ++i)
    {
        float s = 0.f;
        for (int c = 0; c < ch; ++c) s += buffer.getReadPointer (c)[i];
        s /= (float) ch;
        mono[(size_t) i] = s;
        pk = juce::jmax (pk, std::abs (s));
    }
    level.store (pk);
    keyDetector.push (mono.data(), n);
    // audio passes through untouched
}

bool AutoKeyProcessor::sendToVocalyst()
{
    auto r = keyDetector.estimate();
    if (! r.valid) return false;
    if (sharedkey::write (r.key, r.scale, r.confidence, "Vocalyst Auto Key"))
    {
        lastKey = r.key; lastScale = r.scale;
        lastSent = scales::keyName (r.key, r.scale) + " at " + juce::Time::getCurrentTime().toString (false, true, false);
        return true;
    }
    return false;
}

void AutoKeyProcessor::timerCallback()
{
    if (! autoSend.load()) return;
    auto r = keyDetector.estimate();
    if (r.valid && r.confidence > 0.25f && (r.key != lastKey || r.scale != lastScale))
        sendToVocalyst();
}

void AutoKeyProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream s (dest, false);
    s.writeBool (autoSend.load());
}

void AutoKeyProcessor::setStateInformation (const void* data, int size)
{
    juce::MemoryInputStream s (data, (size_t) size, false);
    if (size >= 1) autoSend.store (s.readBool());
}

juce::AudioProcessorEditor* AutoKeyProcessor::createEditor() { return new AutoKeyEditor (*this); }

} // namespace vocalyst

#ifndef VOCALYST_STANDALONE_TEST
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new vocalyst::AutoKeyProcessor();
}
#endif
