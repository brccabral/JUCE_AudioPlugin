/*
  ==============================================================================

    PluginUtilities.h
    Created: 25 Jul 2023 8:09:57pm
    Author:  brccabral

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

juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> makeLowCutFilter(const ChainSettings &chainSettings, double sampleRate);
juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> makeHighCutFilter(const ChainSettings &chainSettings, double sampleRate);

// * we need to be free function because we will use it in the Editor.h
void updateCutFilter(CutFilter &lowCut,
                     const juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> &cutCoefficients,
                     const Slope &slope);
