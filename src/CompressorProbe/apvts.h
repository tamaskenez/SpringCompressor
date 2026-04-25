#pragma once

#include "Modes.h"
#include "juce_util/misc.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>

int get_int(const juce::AudioProcessorValueTreeState& apvts, juce::StringRef id);
int get_choice_index(const juce::AudioProcessorValueTreeState& apvts, juce::StringRef id);

template<class E>
E get_choice(const juce::AudioProcessorValueTreeState& apvts, juce::StringRef id)
{
    const auto index = get_choice_index(apvts, id);
    const auto e = magic_enum::enum_cast<E>(index);
    LOG_IF(FATAL, !e) << format(
      "Parameter {} has invalid index {} for type {}", to_string_view(id), index, typeid(E).name()
    );
    return *e;
}

Mode::E get_mode(const juce::AudioProcessorValueTreeState& apvts);
Mode::DecibelCycle get_mode_decibel_cycle(const juce::AudioProcessorValueTreeState& apvts);
Mode::RatioByFreq get_ratio_by_freq(const juce::AudioProcessorValueTreeState& apvts);
Mode::V get_mode_v(const juce::AudioProcessorValueTreeState& apvts);
