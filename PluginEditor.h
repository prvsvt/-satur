#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// Неоновая анимированная крутилка
class KonLook : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                           float pos, float startAngle, float endAngle,
                           juce::Slider& s) override;
};

class KonEditor : public juce::AudioProcessorEditor,
                  private juce::Timer
{
public:
    explicit KonEditor (KonProcessor&);
    ~KonEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    static constexpr int numKnobs = 7;

    KonProcessor& proc;
    KonLook look;

    std::array<juce::Slider, numKnobs> sliders;
    std::array<juce::Label, numKnobs> labels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, numKnobs> attachments;

    juce::Image horseImg;
    juce::Rectangle<float> horseArea;

    // анимация коня
    float grSmooth = 0.0f, phase = 0.0f, tilt = 0.0f, zoom = 1.0f;
    float shakeX = 0.0f, shakeY = 0.0f, drive01 = 0.0f;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KonEditor)
};
