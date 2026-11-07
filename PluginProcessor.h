#pragma once
#include <JuceHeader.h>

class KonProcessor : public juce::AudioProcessor
{
public:
    KonProcessor();
    ~KonProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Pyanyi Kon"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // для анимации интерфейса: текущее сжатие в дБ (положительное число)
    std::atomic<float> grMeter { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double sampleRateHz = 44100.0;
    float grDb = 0.0f;

    juce::SmoothedValue<float> driveS, mixS, outS;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KonProcessor)
};
