#include "apvts.h"

#include "meadow/enum.h"

#include <typeinfo>

int get_int(const juce::AudioProcessorValueTreeState& apvts, juce::StringRef id)
{
    auto* rap = apvts.getParameter(id);
    LOG_IF(FATAL, !rap) << "Unknown parameter: " << to_string_view(id);
    auto* ap_int = dynamic_cast<juce::AudioParameterInt*>(rap);
    LOG_IF(FATAL, !ap_int) << format(
      "Parameter {} is not AudioParameterInt but {}", to_string_view(id), typeid(*rap).name()
    );
    return ap_int->get();
}

int get_choice_index(const juce::AudioProcessorValueTreeState& apvts, juce::StringRef id)
{
    const auto* rap = apvts.getParameter(id);
    LOG_IF(FATAL, !rap) << "Unknown parameter: " << to_string_view(id);
    const auto* ap_choice = dynamic_cast<const juce::AudioParameterChoice*>(rap);
    LOG_IF(FATAL, !ap_choice) << format(
      "Parameter {} is not AudioParameterChoice but {}", to_string_view(id), typeid(*rap).name()
    );
    return ap_choice->getIndex();
}

Mode::E get_mode(const juce::AudioProcessorValueTreeState& apvts)
{
    return get_choice<Mode::E>(apvts, "mode");
}

Mode::DecibelCycle get_mode_decibel_cycle(const juce::AudioProcessorValueTreeState& apvts)
{
    return Mode::DecibelCycle{
      .freq = get_int(apvts, "steady_curve_freq"),
      .waveform = get_choice<Waveform>(apvts, "steady_curve_waveform"),
      .level_ref = get_choice<LevelRef>(apvts, "steady_curve_level_ref"),
      .min_dbfs = get_int(apvts, "steady_curve_min_dbfs"),
      .max_dbfs = get_int(apvts, "steady_curve_max_dbfs"),
      .cycle_length_index = sucast(get_choice_index(apvts, "steady_curve_length")),
    };
}

Mode::EnvelopeFilter get_envelope_filter(const juce::AudioProcessorValueTreeState& apvts)
{
    return Mode::EnvelopeFilter{
      .carrier_freq = get_int(apvts, "envelope_filter_carrier_freq"),
      .max_carrier_amp_dbfs = get_int(apvts, "envelope_filter_max_carrier_db"),
      .min_mod_freq = get_int(apvts, "envelope_filter_min_mod_freq"),
      .max_mod_freq = get_int(apvts, "envelope_filter_max_mod_freq"),
      .mod_amp_db = get_int(apvts, "envelope_filter_mod_amp_db"),
      .cycle_length_index = sucast(get_choice_index(apvts, "envelope_filter_length")),
    };
}

Mode::V get_mode_v(const juce::AudioProcessorValueTreeState& apvts)
{
    switch (get_mode(apvts)) {
    case Mode::E::Bypass:
        return Mode::Bypass{};
    case Mode::E::DecibelCycle:
        return get_mode_decibel_cycle(apvts);
    case Mode::E::EnvelopeFilter:
        return get_envelope_filter(apvts);
    }
}
