/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

using namespace juce;

//==============================================================================
/**
*/
class MoorerReverbAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    MoorerReverbAudioProcessor();
    ~MoorerReverbAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
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

private:
    //==============================================================================
    juce::AudioBuffer<float> inputDelayBuffer;
    juce::AudioBuffer<float> firDelayBuffer;
    juce::AudioBuffer<float> combDelayBuffers;
    juce::AudioBuffer<float> combDelayBuffer;
    juce::AudioBuffer<float> allpassDelayBuffers;
    
    juce::AudioParameterFloat* reverbTime;
    juce::AudioParameterFloat* predelay1;
    juce::AudioParameterFloat* damping;
    juce::AudioParameterFloat* wetMix;
    
    int delayBufferLength, sampleRate;
    int delayWrite {0};
    
    float iirDelayLengths[6];
    float iirCoefficients[6];
    float firDelayLengths[6];
    float firCoefficients[6];
    
    void iirCombFilter (float* dest, float* input, int writePtr, float delay, float feedback);
    void allpassFilter(float* output, float* input, int writePtr, float delay);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoorerReverbAudioProcessor)
};
