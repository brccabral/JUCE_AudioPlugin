/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics &,
                          int x, int y, int width, int height,
                          float sliderPosProportional,
                          float rotaryStartAngle,
                          float rotaryEndAngle,
                          juce::Slider &) override;

    void drawToggleButton(juce::Graphics &g,
                          juce::ToggleButton &toggleButton,
                          bool shouldDrawButtonAsHighlighted,
                          bool shouldDrawButtonAsDown) override{};
};

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels(juce::RangedAudioParameter &rap, const juce::String &unitSuffix);
    ~RotarySliderWithLabels();

    void paint(juce::Graphics &g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

private:
    juce::RangedAudioParameter *param;
    juce::String suffix;
    LookAndFeel lnf;
};

// * Draw chart
// * we need as separated component so it doesn't draw on top of other components
struct ResponseCurveComponent : juce::Component,
                                juce::AudioProcessorParameter::Listener,
                                juce::Timer
{
    ResponseCurveComponent(AudioPlugin_JUCEAudioProcessor &);
    ~ResponseCurveComponent();

    void paint(juce::Graphics &) override;
    void resized() override;

    // * juce::AudioProcessorParameter::Listener
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

    // * juce::Timer
    void timerCallback() override;

private:
    AudioPlugin_JUCEAudioProcessor &audioProcessor;

    MonoChain monoChain;

    // * AudioProcessorParameter::Listener needs to be thread-safe and non-blocking
    juce::Atomic<bool> parametersChanged{false};

    void updateChain();
};

//==============================================================================
/**
 */
class AudioPlugin_JUCEAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AudioPlugin_JUCEAudioProcessorEditor(AudioPlugin_JUCEAudioProcessor &);
    ~AudioPlugin_JUCEAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPlugin_JUCEAudioProcessor &audioProcessor;

    RotarySliderWithLabels peakFreqSlider,
        peakGainSlider,
        peakQualitySlider,
        lowCutFreqSlider,
        highCutFreqSlider,
        lowCutSlopeSlider,
        highCutSlopeSlider;

    std::vector<juce::Component *> getComps();

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peakFreqSliderAttachment,
        peakGainSliderAttachment,
        peakQualitySliderAttachment,
        lowCutFreqSliderAttachment,
        highCutFreqSliderAttachment,
        lowCutSlopeSliderAttachment,
        highCutSlopeSliderAttachment;

    ResponseCurveComponent responseCurveComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlugin_JUCEAudioProcessorEditor)
};
