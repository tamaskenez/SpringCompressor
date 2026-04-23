#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

inline const auto k_default_buses_layout = juce::AudioChannelSet::stereo();
constexpr double k_wave_scope_duration_sec = 0.01;
constexpr double k_silence_after_new_command_sec = 0.1;
constexpr double k_min_corr = 0.33;
constexpr double k_seconds_of_received_audio_blocks = 1.0; // How many seconds of RABs need to be maintained.

#ifdef NDEBUG
constexpr bool k_development_mode = false;
#else
// Development mode: the plugin automatically goes into Probe mode and creates a Generator without UI and conntects to
// it.
constexpr bool k_development_mode = true;
#endif
