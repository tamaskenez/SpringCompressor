#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

#include <magic_enum/magic_enum.hpp>

#include <string_view>

juce::String to_juce_string(std::string_view sv);
juce::StringArray to_juce_string_array(const std::vector<std::string>& xs);
juce::StringArray choices_for(juce::AudioProcessorValueTreeState& apvts, const char* parameter_id);

struct ComboBoxWithAttachment {
    juce::ComboBox combo;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment attachment;

    ComboBoxWithAttachment(
      juce::AudioProcessorValueTreeState& apvts, const juce::String& parameter_id, const juce::StringArray& items
    );
};

template<class E>
juce::StringArray get_enum_names_in_StringArray()
{
    juce::StringArray labels;
    for (auto [_, n] : magic_enum::enum_entries<E>()) {
        labels.add(juce::String(n.data(), n.size()));
    }
    return labels;
}

template<class E>
juce::StringArray get_labels_for_enum_in_StringArray()
{
    juce::StringArray labels;
    for (auto [e, _] : magic_enum::enum_entries<E>()) {
        auto sv = get_label_for_enum(e);
        labels.add(juce::String(sv.data(), sv.size()));
    }
    return labels;
}
