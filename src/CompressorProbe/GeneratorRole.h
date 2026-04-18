#pragma once

#include "Command.h"
#include "GeneratorStatus.h"
#include "RoleInterface.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <readerwriterqueue/readerwriterqueue.h>

#include <atomic>
#include <memory>

struct CommonState;
class Pipe;

class GeneratorRole : public RoleInterface
{
public:
    explicit GeneratorRole(CommonState& common_state);
    ~GeneratorRole() override;

    void prepare_to_play(double sample_rate, int samples_per_block) override;
    void process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) override;
    void release_resources() override;
    void on_ui_refresh_timer_elapsed() override;

    GeneratorStatus get_status() const noexcept
    {
        return status.load();
    }
    std::optional<std::string> get_current_command_to_string() const
    {
        return last_command_received;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneratorRole)

private:
    // Message thread variables and functions
    CommonState& common_state;
    std::unique_ptr<Pipe> pipe;
    std::atomic<GeneratorStatus> status{GeneratorStatus::Idle};
    optional<string> last_command_received;

    void on_pipe_message_received(span<const char> memory_block);

    // Audio thread variables and functions
    vector<float> tone_buf;
    int tone_playhead = 0;
    optional<Command> current_command;

    // Thread-safe communication between audio and message threads
    moodycamel::ReaderWriterQueue<Command> message_to_audio_queue;
    moodycamel::ReaderWriterQueue<Response> audio_to_message_queue;
};
