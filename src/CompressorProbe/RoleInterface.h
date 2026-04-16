#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class RoleInterface
{
public:
    virtual ~RoleInterface() = default;

    virtual void prepare_to_play(double sample_rate, int samples_per_block) = 0;
    virtual void process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) = 0;
    virtual void release_resources() = 0;
};
