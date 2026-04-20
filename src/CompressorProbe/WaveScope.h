#pragma once

#include "juce_util/misc.h"

#include <meadow/cppext.h>

#include <juce_audio_processors/juce_audio_processors.h>

class WaveScope : public juce::Component
{
public:
    explicit WaveScope(juce::AudioProcessorValueTreeState& apvts);
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Display the input samples with a green curve, the output samples with a red curve.
    //
    // If y_unit is linear, display the samples as they are, with the y = 0 positioned at the vertical center of the
    // image. For auto_scale = false, the y coordinates should range from -1 to 1. For auto_scale = true the y
    // coordinates should range from `-M` to `M` where `M` is the abs-max of the input and output samples.
    //
    // If y_unit is dB, convert the samples to dB values (matlab::mag2db(abs(sample)))
    // If the auto_scale = false, the y coordinates should range from -100 dB to 0 dB. If auto_scale = true, the y
    // coordinates should range from `M - 50` to `M`, where `M` is the max dB of the input and output samples.
    //
    // Horizontally the samples should span the entire image. `samples_in` and `samples_out` are expected to have the
    // same length.
    void update(span<const float> samples_in, span<const float> samples_out);

private:
    juce::Image waveform_image;
    ComboBoxWithAttachment channels, y_unit;
    juce::ToggleButton auto_scale_btn{"Auto scale"};
    juce::AudioProcessorValueTreeState::ButtonAttachment auto_scale_attachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveScope)
};
