#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"

class SpringCompressorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpringCompressorEditor(SpringCompressorProcessor&);
    ~SpringCompressorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    SpringCompressorProcessor& processorRef;

    juce::Slider thresholdSlider, ratioSlider, attackSlider, releaseSlider, makeupSlider, kneeWidthSlider;
    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel, makeupLabel, kneeWidthLabel;

    juce::ComboBox gainFilterComboBox;
    juce::Label gainFilterLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    SliderAttachment thresholdAttachment, ratioAttachment, attackAttachment, releaseAttachment, makeupAttachment,
      kneeWidthAttachment;

    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> gainFilterAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringCompressorEditor)
};
