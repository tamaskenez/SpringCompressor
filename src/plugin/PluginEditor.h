#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "JuceTimer.h"
#include "PluginProcessor.h"
#include "TransferCurveComponent.h"

class SpringCompressorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpringCompressorEditor(SpringCompressorProcessor&);
    ~SpringCompressorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void set_transfer_curve(const TransferCurveState& tcur);

private:
    SpringCompressorProcessor& processorRef;

    juce::Slider thresholdSlider, ratioSlider, attackSlider, releaseSlider, makeupSlider, referenceLevelSlider,
      kneeWidthSlider;
    juce::Label thresholdLabel, ratioLabel, attackLabel, releaseLabel, makeupLabel, referenceLevelLabel, kneeWidthLabel;

    juce::ComboBox gainFilterComboBox;
    juce::Label gainFilterLabel;

    TransferCurveComponent transfer_curve_component;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    SliderAttachment thresholdAttachment, ratioAttachment, attackAttachment, releaseAttachment, makeupAttachment,
      referenceLevelAttachment, kneeWidthAttachment;

    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<ComboBoxAttachment> gainFilterAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpringCompressorEditor)
};
