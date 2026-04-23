#pragma once

#include "juce_util/misc.h"

#include <juce_audio_processors/juce_audio_processors.h>

class DecibelCyclePanel : public juce::Component
{
public:
    explicit DecibelCyclePanel(juce::AudioProcessorValueTreeState& apvts);
    void resized() override;

private:
    juce::Label freq_label, waveform_label, level_ref_label;
    juce::Label min_dbfs_label, max_dbfs_label, length_label;
    juce::Slider freq_slider, min_dbfs_slider, max_dbfs_slider;
    juce::AudioProcessorValueTreeState::SliderAttachment freq_attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment min_dbfs_attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment max_dbfs_attachment;
    ComboBoxWithAttachment waveform, level_ref, length;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DecibelCyclePanel)
};
