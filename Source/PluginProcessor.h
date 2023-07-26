/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

// * chain will process single audio to all defined processors
using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

// * structure to hold our parameters
struct ChainSettings
{
    float peakFreq{0}, peakGainInDecibels{0}, peakQuality{1.f};
    float lowCutFreq{0}, highCutFreq{0};
    Slope lowCutSlope{Slope::Slope_12}, highCutSlope{Slope::Slope_12};
};

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts);

enum ChainPositions
{
    LowCut,
    Peak,
    HighCut
};

using Coefficients = Filter::CoefficientsPtr;

// * we need to be free function because we will use it in the Editor.h
void updateCoefficients(Coefficients &old, const Coefficients &replacements);

// * we need to be free function because we will use it in the Editor.h
Coefficients makePeakFilter(const ChainSettings &chainSettings, double sampleRate);

auto makeLowCutFilter(const ChainSettings &chainSettings, double sampleRate);
auto makeHighCutFilter(const ChainSettings &chainSettings, double sampleRate);

// * we need to be free function because we will use it in the Editor.h
template <typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType &lowCut,
                     const CoefficientType &cutCoefficients,
                     const Slope &slope);

//==============================================================================
/**
 */
class AudioPlugin_JUCEAudioProcessor : public juce::AudioProcessor
#if JucePlugin_Enable_ARA
    ,
                                       public juce::AudioProcessorARAExtension
#endif
{
public:
    //==============================================================================
    AudioPlugin_JUCEAudioProcessor();
    ~AudioPlugin_JUCEAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    //==============================================================================
    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String &newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    // * adds a control panel
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{*this, nullptr, "Parameters", createParameterLayout()};

private:
    MonoChain leftChain, rightChain;

    void updatePeakFilter(const ChainSettings &chainSettings);

    void updateLowCutFilters(const ChainSettings &chainSettings);
    void updateHighCutFilters(const ChainSettings &chainSettings);

    void updateFilters();

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlugin_JUCEAudioProcessor)
};
