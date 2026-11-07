#include "PluginProcessor.h"
#include "PluginEditor.h"

using KonParam = juce::AudioParameterFloat;
using KonRange = juce::NormalisableRange<float>;

juce::AudioProcessorValueTreeState::ParameterLayout KonProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    p.push_back (std::make_unique<KonParam> (juce::ParameterID { "thresh", 1 },  "Threshold", KonRange (-40.0f, 0.0f, 0.1f), -18.0f));
    p.push_back (std::make_unique<KonParam> (juce::ParameterID { "ratio", 1 },   "Ratio",     KonRange (1.0f, 20.0f, 0.1f, 0.5f), 4.0f));
    p.push_back (std::make_unique<KonParam> (juce::ParameterID { "attack", 1 },  "Attack",    KonRange (0.1f, 100.0f, 0.1f, 0.4f), 10.0f));
    p.push_back (std::make_unique<KonParam> (juce::ParameterID { "release", 1 }, "Release",   KonRange (10.0f, 1000.0f, 1.0f, 0.4f), 120.0f));
    p.push_back (std::make_unique<KonParam> (juce::ParameterID { "drive", 1 },   "Drive",     KonRange (0.0f, 24.0f, 0.1f), 6.0f));
    p.push_back (std::make_unique<KonParam> (juce::ParameterID { "mix", 1 },     "Mix",       KonRange (0.0f, 100.0f, 1.0f), 100.0f));
    p.push_back (std::make_unique<KonParam> (juce::ParameterID { "output", 1 },  "Output",    KonRange (-24.0f, 12.0f, 0.1f), 0.0f));

    return { p.begin(), p.end() };
}

KonProcessor::KonProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

void KonProcessor::prepareToPlay (double sr, int)
{
    sampleRateHz = sr;
    grDb = 0.0f;
    driveS.reset (sr, 0.02);
    mixS.reset (sr, 0.02);
    outS.reset (sr, 0.02);

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    driveS.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (get ("drive")));
    mixS.setCurrentAndTargetValue (get ("mix") * 0.01f);
    outS.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (get ("output")));
}

bool KonProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

void KonProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float thresh  = get ("thresh");
    const float ratio   = get ("ratio");
    const float atkMs   = get ("attack");
    const float relMs   = get ("release");

    const float atkC = std::exp (-1.0f / (0.001f * atkMs * (float) sampleRateHz));
    const float relC = std::exp (-1.0f / (0.001f * relMs * (float) sampleRateHz));

    driveS.setTargetValue (juce::Decibels::decibelsToGain (get ("drive")));
    mixS.setTargetValue (get ("mix") * 0.01f);
    outS.setTargetValue (juce::Decibels::decibelsToGain (get ("output")));

    constexpr float bias = 0.12f; // чуть асимметрии = "ламповые" чётные гармоники
    float maxGr = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        // --- компрессор (peak detector, stereo link) ---
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = juce::jmax (peak, std::abs (buffer.getReadPointer (ch)[i]));

        const float levelDb = juce::Decibels::gainToDecibels (peak, -100.0f);
        const float over = levelDb - thresh;
        const float target = over > 0.0f ? over * (1.0f - 1.0f / ratio) : 0.0f;
        const float c = target > grDb ? atkC : relC;
        grDb = c * grDb + (1.0f - c) * target;
        maxGr = juce::jmax (maxGr, grDb);

        const float compGain = juce::Decibels::decibelsToGain (-grDb);

        // --- сатуратор ---
        const float d = driveS.getNextValue();
        const float mix = mixS.getNextValue();
        const float outG = outS.getNextValue();
        const float norm = 1.0f / (std::tanh (d + bias) - std::tanh (bias));

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            const float dry = data[i];
            const float x = dry * compGain;
            const float wet = (std::tanh (d * x + bias) - std::tanh (bias)) * norm;
            data[i] = (dry * (1.0f - mix) + wet * mix) * outG;
        }
    }

    grMeter.store (maxGr);
}

juce::AudioProcessorEditor* KonProcessor::createEditor()
{
    return new KonEditor (*this);
}

void KonProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void KonProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KonProcessor();
}
