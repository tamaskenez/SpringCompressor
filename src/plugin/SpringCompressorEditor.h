#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "JuceTimer.h"
#include "TransferCurveComponent.h"

class SpringCompressorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SpringCompressorEditor(
      juce::AudioProcessor&, std::atomic<bool>& editor_open_arg, juce::AudioProcessorValueTreeState& apvts
    );
    ~SpringCompressorEditor() override
    {
        editor_open.store(false);
    }

    void paint(juce::Graphics&) override;
    void resized() override;

    void set_transfer_curve(const TransferCurveState& tcur);
    void update_rms_dots(
      int rms_matrix_clock, std::mdspan<int, std::dextents<int, 2>> rms_matrix, double rms_sample_period_sec
    );

private:
    std::atomic<bool>& editor_open;
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
