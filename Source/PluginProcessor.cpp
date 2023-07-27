/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPlugin_JUCEAudioProcessor::AudioPlugin_JUCEAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      )
#endif
{
}

AudioPlugin_JUCEAudioProcessor::~AudioPlugin_JUCEAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPlugin_JUCEAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPlugin_JUCEAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPlugin_JUCEAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPlugin_JUCEAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPlugin_JUCEAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPlugin_JUCEAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPlugin_JUCEAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPlugin_JUCEAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String AudioPlugin_JUCEAudioProcessor::getProgramName(int index)
{
    return {};
}

void AudioPlugin_JUCEAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
}

//==============================================================================
void AudioPlugin_JUCEAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    juce::dsp::ProcessSpec spec;

    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    // spec.numChannels = 1; // * mono chain
    spec.numChannels = getTotalNumOutputChannels();

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters();

    leftChannelFifo.prepare(samplesPerBlock);
    rightChannelFifo.prepare(samplesPerBlock);

    // osc.initialise([](float x)
    //                { return std::sin(x); });
    // osc.prepare(spec);
    // osc.setFrequency(440);
}

void AudioPlugin_JUCEAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioPlugin_JUCEAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() &&
        layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void AudioPlugin_JUCEAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                                  juce::MidiBuffer &midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updateFilters();

    // * wrap the buffer into left/right blocks that can be used by the Chain process
    juce::dsp::AudioBlock<float> block(buffer);

    // buffer.clear();
    // juce::dsp::ProcessContextReplacing<float> stereoContext(block);
    // osc.setFrequency(JUCE_LIVE_CONSTANT(440));
    // * this for() sends samples to only one channel, 0 right, 1 left
    // for (int i = 0; i < buffer.getNumSamples(); ++i)
    // {
    //     buffer.setSample(JUCE_LIVE_CONSTANT(0), i, osc.processSample(0));
    //     // buffer.setSample(0, i, osc.processSample(0));
    // }
    // osc.process(stereoContext);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    leftChannelFifo.update(buffer);
    rightChannelFifo.update(buffer);
}

//==============================================================================
bool AudioPlugin_JUCEAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *AudioPlugin_JUCEAudioProcessor::createEditor()
{
    return new AudioPlugin_JUCEAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPlugin_JUCEAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void AudioPlugin_JUCEAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        apvts.replaceState(tree);
        updateFilters();
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPlugin_JUCEAudioProcessor();
}

// * configure parameters to send to our nobs/filters "Low Cut", "High Cut" and "Peak"
juce::AudioProcessorValueTreeState::ParameterLayout AudioPlugin_JUCEAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // * skew - how linear is the parameter, 0 to 1 (percentage)

    // * low freq filter, min-max 20Hz to 20kHz, default 20Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                           "LowCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20.f));

    // * high freq filter, min-max 20Hz to 20kHz, default 20kHz
    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                           "HighCut Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           20000.f));

    // * min-max 20Hz to 20kHz, default center frequency 750Hz
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                           "Peak Freq",
                                                           juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
                                                           750.f));

    // * min-max -24dB to 24dB, steps of 0.5dB, default gain 0.0f
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                           "Peak Gain",
                                                           juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
                                                           0.0f));

    // * how narrow or wide is the gain
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Quality",
                                                           "Peak Quality",
                                                           juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
                                                           1.f));

    // * 4 options - 12, 24, 36, 48
    juce::StringArray stringArray;
    for (int i = 0; i < 4; ++i)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    // * default 0 = stringArray[0] = 12
    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    layout.add(std::make_unique<juce::AudioParameterBool>("LowCut Bypassed", "LowCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Peak Bypassed", "Peak Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("HighCut Bypassed", "HighCut Bypassed", false));
    layout.add(std::make_unique<juce::AudioParameterBool>("Analyzer Enabled", "Analyzer Enabled", true));

    return layout;
}

void AudioPlugin_JUCEAudioProcessor::updatePeakFilter(const ChainSettings &chainSettings)
{
    auto peakCoefficients = makePeakFilter(chainSettings, getSampleRate());

    leftChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);
    rightChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);

    updateCoefficients(leftChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);
}

void AudioPlugin_JUCEAudioProcessor::updateLowCutFilters(const ChainSettings &chainSettings)
{
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());

    auto &leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto &rightLowCut = rightChain.get<ChainPositions::LowCut>();

    leftChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    rightChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);

    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void AudioPlugin_JUCEAudioProcessor::updateHighCutFilters(const ChainSettings &chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

    auto &leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto &rightHighCut = rightChain.get<ChainPositions::HighCut>();

    leftChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    rightChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void AudioPlugin_JUCEAudioProcessor::updateFilters()
{
    // * update parameters before process audio
    auto chainSettings = getChainSettings(apvts);

    updatePeakFilter(chainSettings);

    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
}
