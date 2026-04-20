#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <string_view>

juce::String to_juce_string(std::string_view sv);
juce::StringArray choices_for(juce::AudioProcessorValueTreeState& apvts, const char* parameter_id);

struct ComboBoxWithAttachment {
    juce::ComboBox combo;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment attachment;

    ComboBoxWithAttachment(
      juce::AudioProcessorValueTreeState& apvts, const juce::String& parameter_id, const juce::StringArray& items
    );
};
