/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
MoorerReverbAudioProcessorEditor::MoorerReverbAudioProcessorEditor (MoorerReverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (360, 300);
    
    
    /* reverb time (RT60) */
    addAndMakeVisible(timeSlider);
    timeSlider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    timeSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxBelow, true, 60, 20);
    timeSlider.setColour(Slider::textBoxTextColourId, Colours::black);
    timeSlider.setTextBoxIsEditable(true);
    timeSlider.setRange(0.2, 4, 0.02);
    timeSlider.setValue(3);
    timeSlider.setTextValueSuffix(" s");
    timeSlider.addListener(this);
    // label
    addAndMakeVisible(timeLabel);
    timeLabel.setText("reverb time", juce::dontSendNotification);
    timeLabel.setJustificationType(juce::Justification::centred);
    timeLabel.attachToComponent(&timeSlider, false);
        
    
    /* pre delay 1 (early refl.) */
    addAndMakeVisible(predelay1Slider);
    predelay1Slider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    predelay1Slider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxBelow, true, 60, 20);
    predelay1Slider.setColour(Slider::textBoxTextColourId, Colours::black);
    predelay1Slider.setTextBoxIsEditable(true);
    predelay1Slider.setRange(0.5, 100, 0.5);
    predelay1Slider.setValue(20);
    predelay1Slider.setTextValueSuffix(" ms");
    predelay1Slider.addListener(this);
    // label
    addAndMakeVisible(predelay1Label);
    predelay1Label.setText("predelay 1", juce::dontSendNotification);
    predelay1Label.setJustificationType(juce::Justification::centred);
    predelay1Label.attachToComponent(&predelay1Slider, false);
    
    
    /* damping */
    addAndMakeVisible(dampingSlider);
    dampingSlider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    dampingSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxBelow, true, 60, 20);
    dampingSlider.setColour(Slider::textBoxTextColourId, Colours::black);
    dampingSlider.setTextBoxIsEditable(true);
    dampingSlider.setRange(0, 100, 1);
    dampingSlider.setValue(20);
    dampingSlider.setTextValueSuffix(" %");
    dampingSlider.addListener(this);
    // label
    addAndMakeVisible(dampingLabel);
    dampingLabel.setText("damping", juce::dontSendNotification);
    dampingLabel.setJustificationType(juce::Justification::centred);
    dampingLabel.attachToComponent(&dampingSlider, false);
    
    
    /* wet/dry mix */
    addAndMakeVisible(wetSlider);
    wetSlider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    wetSlider.setTextBoxStyle(Slider::TextEntryBoxPosition::TextBoxBelow, true, 60, 20);
    wetSlider.setColour(Slider::textBoxTextColourId, Colours::black);
    wetSlider.setTextBoxIsEditable(true);
    wetSlider.setRange(0, 100, 1);
    wetSlider.setValue(100);
    wetSlider.setTextValueSuffix(" %");
    wetSlider.addListener(this);
    // label
    addAndMakeVisible(wetLabel);
    wetLabel.setText("wet/dry", juce::dontSendNotification);
    wetLabel.setJustificationType(juce::Justification::centred);
    wetLabel.attachToComponent(&wetSlider, false);
    
    
    getLookAndFeel().setColour(Label::textColourId, Colours::black);
    getLookAndFeel().setColour(Slider::textBoxTextColourId, Colours::black);
    getLookAndFeel().setColour(Slider::textBoxOutlineColourId, Colours::transparentWhite);
    getLookAndFeel().setColour(Slider::rotarySliderFillColourId, Colours::orange);
    getLookAndFeel().setColour(Slider::trackColourId, Colours::black);
    getLookAndFeel().setColour(Slider::thumbColourId, Colours::transparentBlack);
}

MoorerReverbAudioProcessorEditor::~MoorerReverbAudioProcessorEditor()
{
}

//==============================================================================
void MoorerReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(Colours::whitesmoke); // background colour
}

void MoorerReverbAudioProcessorEditor::resized()
{
    timeSlider.setBounds(getWidth()/2-100, getHeight()/2-100, 80, 80);
    dampingSlider.setBounds(getWidth()/2+20, getHeight()/2-100, 80, 80);
    predelay1Slider.setBounds(getWidth()/2-100, getHeight()/2+20, 80, 80);
    wetSlider.setBounds(getWidth()/2+20, getHeight()/2+20, 80, 80);
}

void MoorerReverbAudioProcessorEditor::sliderValueChanged(Slider* slider)
{
    auto& audioParams = audioProcessor.getParameters();
    if (slider == &timeSlider) {
        AudioParameterFloat* timeParam = (AudioParameterFloat*) audioParams.getUnchecked(0);
        *timeParam = (float)timeSlider.getValue()/8.0f + 0.5; // scale 0-4 to 0.5-1
    } else if (slider == &predelay1Slider) {
        AudioParameterFloat* predelay1Param = (AudioParameterFloat*) audioParams.getUnchecked(1);
        *predelay1Param = predelay1Slider.getValue()/1000.0f;
    } else if (slider == &dampingSlider) {
        AudioParameterFloat* dampingParam = (AudioParameterFloat*) audioParams.getUnchecked(2);
        *dampingParam = (dampingSlider.getValue()/100.0f) + 0.5f; // scale 0-100 to 0.5-1.5
    } else if (slider == &wetSlider) {
        AudioParameterFloat* wetParam = (AudioParameterFloat*) audioParams.getUnchecked(3);
        *wetParam = wetSlider.getValue()/100.0f;
    }
}
