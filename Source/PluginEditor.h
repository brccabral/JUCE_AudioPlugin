/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct RotarySliderWithLabels : juce::Slider
{
    RotarySliderWithLabels() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,
                                            juce::Slider::TextEntryBoxPosition::NoTextBox)
    {
    }
};

//==============================================================================
/**
 */
class AudioPlugin_JUCEAudioProcessorEditor : public juce::AudioProcessorEditor,
                                             public juce::AudioProcessorParameter::Listener,
                                             public juce::Timer
{
public:
    AudioPlugin_JUCEAudioProcessorEditor(AudioPlugin_JUCEAudioProcessor &);
    ~AudioPlugin_JUCEAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;

    // * juce::AudioProcessorParameter::Listener
    void parameterValueChanged(int parameterIndex, float newValue) override;
    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}

    // * juce::Timer
    void timerCallback() override;

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

    MonoChain monoChain;

    // * AudioProcessorParameter::Listener needs to be thread-safe and non-blocking
    juce::Atomic<bool> parametersChanged{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlugin_JUCEAudioProcessorEditor)
};
