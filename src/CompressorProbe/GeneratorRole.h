#pragma once

#include "Command.h"
#include "GeneratorStatus.h"
#include "RoleInterface.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <memory>

struct CommonState;

class GeneratorRole : public RoleInterface
{
public:
    explicit GeneratorRole(CommonState& common_state);

    void prepare_to_play(double sample_rate, int samples_per_block) override;
    void process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) override;
    void release_resources() override;

    void on_pipe_command_received(const Command::V& command);

    GeneratorStatus get_status() const noexcept
    {
        return status.load();
    }
    std::optional<std::string> get_current_command_to_string() const
    {
        if (current_command) {
            return to_string(*current_command);
        } else {
            return std::nullopt;
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneratorRole)

private:
    CommonState& common_state;

    std::atomic<GeneratorStatus> status{GeneratorStatus::Idle};
    std::unique_ptr<juce::InterprocessConnection> pipe;
    vector<float> tone_buf;
    int tone_playhead = 0;
    std::optional<Command::V> current_command;
};
