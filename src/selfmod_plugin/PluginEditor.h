#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class SelfModEditor : public juce::AudioProcessorEditor
{
public:
    explicit SelfModEditor(SelfModProcessor&);
    ~SelfModEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SelfModProcessor& audio_processor;

    juce::Slider lp_freq_slider{juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow};
    juce::Label lp_freq_label;
    juce::AudioProcessorValueTreeState::SliderAttachment lp_freq_attachment;

    juce::Slider intensity_slider{juce::Slider::RotaryVerticalDrag, juce::Slider::TextBoxBelow};
    juce::Label intensity_label;
    juce::AudioProcessorValueTreeState::SliderAttachment intensity_attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelfModEditor)
};
