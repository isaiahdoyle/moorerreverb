/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
MoorerReverbAudioProcessor::MoorerReverbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    inputDelayBuffer(2,1),
    firDelayBuffer(2,1),
    combDelayBuffers(12,1),
    combDelayBuffer(2,1),
    allpassDelayBuffers(2,1)
{
    addParameter(reverbTime = new AudioParameterFloat("reverbtime", "Reverb Time", 
                                                      NormalisableRange<float>(0.5f, 1.f), 0.875f));
    addParameter(predelay1 = new AudioParameterFloat("predelay1", "Predelay 1",
                                                     NormalisableRange<float>(0.0005f, 0.1f), 0.02f));
    addParameter(damping = new AudioParameterFloat("damping", "Damping", 0.0f, 1.8f, 0.7f));
    addParameter(wetMix = new AudioParameterFloat("wetmix", "Wet Mix", 0.0f, 1.0f, 1.0f));
    
    
    /* these delay times and gains are recommended for the 7-tap reverb in
       [1] J. A. Moorer, "About This Reverberation Business," Computer Music Journal,
           vol. 3, no. 2, pp. 13-28 (1979 Jun.). https://doi.org/10.2307/3680280.
     */
    
    // 0.0199, 0.0354, 0.0389, 0.0414, 0.0699, 0.0796 w/ compensation for predelay
    const float firLengths[6] = { 0.0f, 0.0155f, 0.019f,
                                  0.0215f, 0.05f, 0.0597f };
    memcpy(firDelayLengths, firLengths, sizeof(firLengths));
    const float firGains[6] = { 0.921f,  0.818f, 0.635f,
                                0.719f, 0.267f, 0.242f };
    memcpy(firCoefficients, firGains, sizeof(firGains));
    
    
    // Moorer, 1965: 0.03, 0.034, 0.037, 0.041
    const float iirLengths[6] = { 0.05f,  0.056f, 0.061f,
                                  0.068f, 0.072f, 0.078f };
    memcpy(iirDelayLengths, iirLengths, sizeof(iirLengths));
    // damping is 0-1.8 to map these values to 0-0.99
    const float iirGains[6] = { 0.46f, 0.48f, 0.5f, 0.52f, 0.53f, 0.55f };
    memcpy(iirCoefficients, iirGains, sizeof(iirGains));
    
    delayWrite = 0;
}

MoorerReverbAudioProcessor::~MoorerReverbAudioProcessor()
{
}

//==============================================================================
const juce::String MoorerReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MoorerReverbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MoorerReverbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MoorerReverbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MoorerReverbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MoorerReverbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MoorerReverbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MoorerReverbAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MoorerReverbAudioProcessor::getProgramName (int index)
{
    return {};
}

void MoorerReverbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MoorerReverbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    /* init delay buffers */
    delayBufferLength = (int)(0.2*sampleRate); // max delay 200 ms
    
    inputDelayBuffer.setSize(2, delayBufferLength);
    inputDelayBuffer.clear();
    
    firDelayBuffer.setSize(2, delayBufferLength);
    firDelayBuffer.clear();
    
    combDelayBuffers.setSize(12, delayBufferLength);
    combDelayBuffers.clear();
    
    combDelayBuffer.setSize(2, delayBufferLength);
    combDelayBuffer.clear();
    
    allpassDelayBuffers.setSize(2, delayBufferLength);
    allpassDelayBuffers.clear();
}

void MoorerReverbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MoorerReverbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void MoorerReverbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();
    const int sampleRate = getSampleRate();
    int dpr, dpw;
    
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);
        auto* inputDelayData = inputDelayBuffer.getWritePointer(channel);
        auto* firDelayData = firDelayBuffer.getWritePointer(channel);
        auto* combDelayData = combDelayBuffer.getWritePointer(channel);
        auto* allpassDelayData1 = allpassDelayBuffers.getWritePointer(channel);
        
        dpw = delayWrite;
        
        for (int sample = 0; sample < numSamples; ++sample) {
            const float in = channelData[sample];
            float out = 0.0f;
            inputDelayData[dpw] = in;
            firDelayData[dpw] = in;
            
            
            /* FIR DELAY TAPS */
            for (int i = 0; i < 6; i++) {
                dpr = (int)(dpw - ((*predelay1)+firDelayLengths[i])*sampleRate + 2*delayBufferLength) % delayBufferLength;
                float tap = firCoefficients[i] * inputDelayData[dpr];
                firDelayData[dpw] += tap;
            }
            /* ============== */
            

            /* PARALLEL IIR COMB FILTERS */
            float combSum = 0.0f;
            for (int i = 0; i < 6; i++) {
                float* currentComb = combDelayBuffers.getWritePointer(channel + 2*i);
                iirCombFilter(currentComb, firDelayData, dpw, iirDelayLengths[i], (*damping)*iirCoefficients[i]);
                combSum += currentComb[dpw];
            }
            combDelayData[dpw] = combSum;
            /* ========================= */
            
            
            /* ALLPASS SECTION */
            allpassFilter(allpassDelayData1, combDelayData, dpw, 0.006f);
            /* =============== */
            
            
            /* this delay lines up first late reflection with the last early reflection (as recommended in [1]) */
            dpr = (int)(dpw - (0.029f+(*predelay1))*sampleRate + delayBufferLength) % delayBufferLength;
            out = 0.15f*(firDelayData[dpw] + allpassDelayData1[dpr]);
            channelData[sample] = (1.0f-(*wetMix))*in + (*wetMix)*out;
            
            dpw = (dpw + 1) % delayBufferLength;
        }
    }
    
    delayWrite = dpw;
}

//==============================================================================
bool MoorerReverbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MoorerReverbAudioProcessor::createEditor()
{
    return new MoorerReverbAudioProcessorEditor (*this);
}

//==============================================================================
void MoorerReverbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void MoorerReverbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MoorerReverbAudioProcessor();
}

//==============================================================================
void MoorerReverbAudioProcessor::iirCombFilter(float* output, float* input, int writePtr, float delay, float feedback)
{
    // y[n] = x[n-d] + g*y[n-d]
    int dpr = (int)(writePtr - delay*getSampleRate() + delayBufferLength) % delayBufferLength;
    float xd = input[dpr];  // delayed input
    float fb = output[dpr]; // previous output

    // lowpass feedback line: y[n] = (1-g)*(x[n] + g*x[n-1])
    dpr = (int)(dpr - 1 + delayBufferLength) % delayBufferLength;
    float fblp = (1-feedback)*(fb + feedback*(output[dpr]));
    
    // uses comb output to dictate reverb time
    output[writePtr] = (*reverbTime)*(xd + fblp);
}

void MoorerReverbAudioProcessor::allpassFilter(float* output, float* input, int writePtr, float delay)
{
    // y[n] = -g*x[n] + x[n-d] + g*y[n-d]
    int dpr = (int)(writePtr - delay*getSampleRate() + delayBufferLength) % delayBufferLength;
    output[writePtr] = -0.7*input[writePtr] + input[dpr] + 0.7*output[dpr];
}
