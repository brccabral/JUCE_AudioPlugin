/*
  ==============================================================================

    PluginUtilities.cpp
    Created: 25 Jul 2023 8:10:05pm
    Author:  brccabral

  ==============================================================================
*/

#include "PluginUtilities.h"

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState &apvts)
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    settings.lowCutBypassed = apvts.getRawParameterValue("LowCut Bypassed")->load() > 0.5f;
    settings.peakBypassed = apvts.getRawParameterValue("Peak Bypassed")->load() > 0.5f;
    settings.highCutBypassed = apvts.getRawParameterValue("HighCut Bypassed")->load() > 0.5f;

    return settings;
}

// * coefficients are allocated on the Heap, we need to dereference
// * we need to be free function because we will use it in the Editor.h
void updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    *old = *replacements;
}

// * makePeakFilter() is a free function because we will use it in the Editor.h
Coefficients makePeakFilter(const ChainSettings &chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
                                                               chainSettings.peakFreq,
                                                               chainSettings.peakQuality,
                                                               juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels));
}

juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> makeLowCutFilter(const ChainSettings &chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq,
        sampleRate,
        (chainSettings.lowCutFreq + 1) * 2);
}

juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> makeHighCutFilter(const ChainSettings &chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        chainSettings.highCutFreq,
        sampleRate,
        (chainSettings.highCutFreq + 1) * 2);
}

// * we need to be free function because we will use it in the Editor.h
void updateCutFilter(CutFilter &lowCut,
                     const juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> &cutCoefficients,
                     const Slope &slope)
{
    lowCut.template setBypassed<0>(true);
    lowCut.template setBypassed<1>(true);
    lowCut.template setBypassed<2>(true);
    lowCut.template setBypassed<3>(true);

    // * leaverage switch pass-through to get all necessary case statements
    switch (slope)
    {
    case Slope_48:
    {
        lowCut.template get<3>().coefficients = cutCoefficients[3];
        lowCut.template setBypassed<3>(false);
    }
    case Slope_36:
    {
        lowCut.template get<2>().coefficients = cutCoefficients[2];
        lowCut.template setBypassed<2>(false);
    }
    case Slope_24:
    {
        lowCut.template get<1>().coefficients = cutCoefficients[1];
        lowCut.template setBypassed<1>(false);
    }
    case Slope_12:
    {
        lowCut.template get<0>().coefficients = cutCoefficients[0];
        lowCut.template setBypassed<0>(false);
    }
    }
}
