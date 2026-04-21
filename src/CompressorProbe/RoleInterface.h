#pragma once

#include "Role.h"

#include <juce_audio_basics/juce_audio_basics.h>

class RoleInterface
{
public:
    virtual ~RoleInterface() = default;

    virtual Role get_role() const = 0;
    virtual void prepare_to_play(double sample_rate, int samples_per_block) = 0;
    virtual void process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) = 0;
    virtual void release_resources() = 0;
    virtual void on_ui_refresh_timer_elapsed_mt() = 0;
};
