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
     : juce::AudioProcessor(makeBusesProperties()), fwdFFT1(fftOrder),fwdFFT2(fftOrder),invFFT(fftOrder)
#endif
{
}

FFTExtractAudioProcessor::~FFTExtractAudioProcessor()
{
}

juce::AudioProcessor::BusesProperties FFTExtractAudioProcessor::makeBusesProperties()
{
    BusesProperties bp; // from florian's sidechain tutorial:
    bp.addBus(true, "Input", ChannelSet::stereo(), true);
    bp.addBus(false, "Output", ChannelSet::stereo(), true);
    if (!juce::JUCEApplicationBase::isStandaloneApp())
    {
        bp.addBus(true, "Sidechain", ChannelSet::stereo(), true);
    }
    return bp;
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

void FFTExtractAudioProcessor::pushSampleToFifo (float sample, int channel) noexcept
{
    if(fifoInd == fftSize)
    {
        if(!nextFFT2BlockReady)
        {
            
            std::fill(inputdata.begin(), inputdata.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), inputdata.begin());
            nextFFT2BlockReady = true;
            
        }
        fifoInd = 0;
    }
    fifo[(size_t) fifoInd++] = sample;
}

void FFTExtractAudioProcessor::pushSCSampleToFifo (float sample) noexcept
{
    if(scfifoInd == fftSize)
    {
        if(!nextFFT1BlockReady)
        {
            std::fill(scdata.begin(), scdata.end(), 0.0f);
            std::copy(scfifo.begin(), scfifo.end(), scdata.begin());
            nextFFT1BlockReady = true;
        }
        scfifoInd = 0;
    }
    scfifo[(size_t) scfifoInd++] = sample;
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
    inputdata.reserve((fftSize));
    outputdata.reserve((fftSize));
    scdata.reserve((fftSize));
    DBG("Hello I am a robot: " << inputdata.size());
}

void FFTExtractAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool FFTExtractAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mono = juce::AudioChannelSet::mono();
    const auto stereo = juce::AudioChannelSet::stereo();

    const auto mainSetIn = layouts.getMainInputChannelSet();
    const auto mainSetOut = layouts.getMainOutputChannelSet();

    const auto scSetIn = layouts.getChannelSet(true, 1);
    if(!scSetIn.isDisabled())
        if (scSetIn != mono && scSetIn != stereo)
            return false;

    if (mainSetOut != mono && mainSetOut != stereo)
        return false;

    return mainSetIn == mainSetOut;
}
#endif


std::complex<float> FFTExtractAudioProcessor::polar2cart(float r, float theta)
{
    //r * exp( 1j * theta ) // or is polar2z a better term, no idea.
    return r * std::exp(std::complex<float>(0, theta));
}


void FFTExtractAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    auto mainBus = getBus(true, 0);
    auto scBus = getBus(true, 1);
    auto mainBuf = mainBus->getBusBuffer(buffer);
    auto scBuf = scBus->getBusBuffer(buffer);

    for (int channel = 0; channel < 2; ++channel) // get inputs
    {
        auto* channelData = mainBuf.getWritePointer(channel);
        auto* scChannelData = scBuf.getWritePointer(channel);
        
        for (auto i = 0; i < mainBuf.getNumSamples(); ++i)
        {
            if (channel < 2)
            {
                pushSampleToFifo(channelData[i], channel);
                
                pushSCSampleToFifo(scChannelData[i]); // maybe this is jenk
            }
        }
    }

    if(nextFFT1BlockReady)
    {
        // the simple way
//        fwdFFT.performRealOnlyForwardTransform(fftData.data());
//        fwdFFT.performRealOnlyInverseTransform(fftData.data());
        
        fwdFFT1.perform(inputdata.data(), inputdata.data(), false);
        fwdFFT2.perform(scdata.data(), scdata.data(), false);
        
        // this is normalisation of mags if you need it for future reference
//        for (auto& elem : inputdata)
//            elem /= fftSize;

        for (int i = 0; i < fftSize; ++i)
        {
            auto mags1 = std::abs(inputdata[i]) ;
            auto mags2 = std::abs(scdata[i]) ;
            auto phase1 = std::arg(inputdata[i]);
            auto phase2 = std::arg(scdata[i]);
            auto finalphase = phase1 + phase2;
            outputdata[i] = polar2cart(mags1 * mags2, finalphase);
            //DBG (inputdata[i].imag() << " " << inputdata[i].real());
        }
        
        fwdFFT1.perform(outputdata.data(), outputdata.data(), true);
    
        nextFFT1BlockReady = false;
        nextFFT2BlockReady = false;
    }
    
    for (int channel = 0; channel < 2; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        for (auto i = 0; i < buffer.getNumSamples(); ++i)
        {
            channelData[i] = outputdata[i].real();
        }
    }
}

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
