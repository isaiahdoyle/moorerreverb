/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

using namespace juce;

//==============================================================================
/**
*/
class MoorerReverbAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                             public juce::Slider::Listener
{
public:
    MoorerReverbAudioProcessorEditor (MoorerReverbAudioProcessor&);
    ~MoorerReverbAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void sliderValueChanged(Slider*) override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MoorerReverbAudioProcessor& audioProcessor;
    
    Slider timeSlider;
    Label timeLabel;
    
    Slider predelay1Slider;
    Label predelay1Label;
    
    Slider dampingSlider;
    Label dampingLabel;
    
    Slider wetSlider;
    Label wetLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MoorerReverbAudioProcessorEditor)
};
