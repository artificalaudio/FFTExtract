/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
FFTExtractAudioProcessor::FFTExtractAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), fwdFFT(fftOrder)
#endif
{
}

FFTExtractAudioProcessor::~FFTExtractAudioProcessor()
{
}

//==============================================================================
const juce::String FFTExtractAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool FFTExtractAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool FFTExtractAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool FFTExtractAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double FFTExtractAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int FFTExtractAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

void FFTExtractAudioProcessor::pushSampleToFifo (float sample) noexcept
{
    if(fifoInd == fftSize)
    {
        if(!nextFFTBlockReady)
        {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }
        fifoInd = 0;
    }
    fifo[(size_t) fifoInd++] = sample;
}

int FFTExtractAudioProcessor::getCurrentProgram()
{
    return 0;
}

void FFTExtractAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String FFTExtractAudioProcessor::getProgramName (int index)
{
    return {};
}

void FFTExtractAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void FFTExtractAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void FFTExtractAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FFTExtractAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void FFTExtractAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    for (int channel = 0; channel < 1; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        for (auto i = 0; i < buffer.getNumSamples(); ++i)
            pushSampleToFifo(channelData[i]);
        // ..do something to the data...
    }
    
    if(nextFFTBlockReady)
    {
        fwdFFT.performRealOnlyForwardTransform(fftData.data());
        fwdFFT.performRealOnlyInverseTransform(fftData.data());
        nextFFTBlockReady = false;
        //DBG("Done");
        
    }
    
    for (int channel = 0; channel < 1; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        for (auto i = 0; i < buffer.getNumSamples(); ++i)
        {
            channelData[i] = fftData[i];
        }
        // ..do something to the data...
    }
    
    // then presumably use the data from fftData.data() in the audio block?
    // What's the step here to it get back to the output?
}


//  fwdFFT.perform(<#const Complex<float> *input#>, <#Complex<float> *output#>, <#bool inverse#>)

//==============================================================================
bool FFTExtractAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* FFTExtractAudioProcessor::createEditor()
{
    return new FFTExtractAudioProcessorEditor (*this);
}

//==============================================================================
void FFTExtractAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void FFTExtractAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FFTExtractAudioProcessor();
}
