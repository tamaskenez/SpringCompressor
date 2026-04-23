#pragma once

#include "juce_util/misc.h"

#include <juce_audio_processors/juce_audio_processors.h>

class EnvelopeFilterPanel : public juce::Component
{
public:
    explicit EnvelopeFilterPanel(juce::AudioProcessorValueTreeState& apvts);
    void resized() override;

private:
    juce::Label carrier_freq_label, max_carrier_amp_label, min_mod_freq_label;
    juce::Label max_mod_freq_label, mod_amp_label, length_label;
    juce::Slider carrier_freq_slider, max_carrier_amp_slider, min_mod_freq_slider;
    juce::Slider max_mod_freq_slider, mod_amp_slider;
    juce::AudioProcessorValueTreeState::SliderAttachment carrier_freq_attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment max_carrier_amp_attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment min_mod_freq_attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment max_mod_freq_attachment;
    juce::AudioProcessorValueTreeState::SliderAttachment mod_amp_attachment;
    ComboBoxWithAttachment length;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeFilterPanel)
};
