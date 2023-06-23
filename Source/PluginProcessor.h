/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <complex>
//==============================================================================
/**
*/
using namespace std::literals;

class FFTExtractAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
    using ChannelSet = juce::AudioChannelSet;
    using AppProps = juce::ApplicationProperties;
    
public:
    //==============================================================================
    FFTExtractAudioProcessor();
    ~FFTExtractAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    
    juce::AudioProcessor::BusesProperties makeBusesProperties();
    
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
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    
    std::complex<float> polar2cart(float r, float theta);
    
    //==============================================================================
    void pushSampleToFifo (float sample, int channel) noexcept;
    void pushSCSampleToFifo (float sample) noexcept;
    static constexpr auto fftOrder = 8;
    static constexpr auto fftSize = 1 << fftOrder;
    juce::dsp::FFT fwdFFT1;
    juce::dsp::FFT fwdFFT2;
    juce::dsp::FFT invFFT;
    
    std::array<float, fftSize> fifo;
    std::array<float, fftSize> scfifo;
    std::array<float, fftSize * 2> fftData1;
    std::array<float, fftSize * 2> fftData2;
    
    std::vector<std::complex<float>> inputdata {fftSize};
    std::vector<std::complex<float>> scdata {fftSize};
    
    std::vector<std::complex<float>> outputdata {fftSize};
    
    int fifoInd = 0;
    int scfifoInd = 0;
    bool nextFFT1BlockReady = false;
    bool nextFFT2BlockReady = false;
    
    //==============================================================================
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FFTExtractAudioProcessor)
};
