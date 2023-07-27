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

    bool lowCutBypassed{false}, peakBypassed{false}, highCutBypassed{false};
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

enum Channel
{
    Right, // effectively 0
    Left   // effectively 1
};

template <typename T>
struct Fifo
{
    void prepare(int numChannels, int numSamples)
    {
        static_assert(std::is_same_v<T, juce::AudioBuffer<float>>,
                      "prepare(numChannels, numSamples) should only be used when the Fifo is holding juce::AudioBuffer<float>");
        for (auto &buffer : buffers)
        {
            buffer.setSize(numChannels,
                           numSamples,
                           false, // clear everything?
                           true,  // including the extra space?
                           true); // avoid reallocating if you can?
            buffer.clear();
        }
    }

    void prepare(size_t numElements)
    {
        static_assert(std::is_same_v<T, std::vector<float>>,
                      "prepare(numElements) should only be used when the Fifo is holding std::vector<float>");
        for (auto &buffer : buffers)
        {
            buffer.clear();
            buffer.resize(numElements, 0);
        }
    }

    bool push(const T &t)
    {
        auto write = fifo.write(1);
        if (write.blockSize1 > 0)
        {
            buffers[write.startIndex1] = t;
            return true;
        }

        return false;
    }

    bool pull(T &t)
    {
        auto read = fifo.read(1);
        if (read.blockSize1 > 0)
        {
            t = buffers[read.startIndex1];
            return true;
        }

        return false;
    }

    int getNumAvailableForReading() const
    {
        return fifo.getNumReady();
    }

private:
    static constexpr int Capacity = 30;
    std::array<T, Capacity> buffers;
    juce::AbstractFifo fifo{Capacity};
};

template <typename BlockType>
struct SingleChannelSampleFifo
{
    SingleChannelSampleFifo(Channel ch) : channelToUse(ch)
    {
        prepared.set(false);
    }

    void update(const BlockType &buffer)
    {
        jassert(prepared.get());
        jassert(buffer.getNumChannels() > channelToUse);
        auto *channelPtr = buffer.getReadPointer(channelToUse);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            pushNextSampleIntoFifo(channelPtr[i]);
        }
    }

    void prepare(int bufferSize)
    {
        prepared.set(false);
        size.set(bufferSize);

        bufferToFill.setSize(1,          // channel
                             bufferSize, // num samples
                             false,      // keepExistingContent
                             true,       // clear extra space
                             true);      // avoid reallocating
        audioBufferFifo.prepare(1, bufferSize);
        fifoIndex = 0;
        prepared.set(true);
    }
    //==============================================================================
    int getNumCompleteBuffersAvailable() const { return audioBufferFifo.getNumAvailableForReading(); }
    bool isPrepared() const { return prepared.get(); }
    int getSize() const { return size.get(); }
    //==============================================================================
    bool getAudioBuffer(BlockType &buf) { return audioBufferFifo.pull(buf); }

private:
    Channel channelToUse;
    int fifoIndex = 0;
    Fifo<BlockType> audioBufferFifo;
    BlockType bufferToFill;
    juce::Atomic<bool> prepared = false;
    juce::Atomic<int> size = 0;

    void pushNextSampleIntoFifo(float sample)
    {
        if (fifoIndex == bufferToFill.getNumSamples())
        {
            auto ok = audioBufferFifo.push(bufferToFill);

            juce::ignoreUnused(ok);

            fifoIndex = 0;
        }

        bufferToFill.setSample(0, fifoIndex, sample);
        ++fifoIndex;
    }
};