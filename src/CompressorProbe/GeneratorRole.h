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
class CompressorProbeProcessor;

class GeneratorRole : public RoleInterface
{
public:
    explicit GeneratorRole(CompressorProbeProcessor& processor);
    ~GeneratorRole() override;

    void prepare_to_play(double sample_rate, int samples_per_block) override;
    void process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) override;
    void release_resources() override;
    void on_ui_refresh_timer_elapsed_mt() override {}
    Role get_role() const override
    {
        return Role::Generator;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GeneratorRole)

private:
    CompressorProbeProcessor& processor;
    // Message thread variables and functions
    struct MessageThreadState {
        std::unique_ptr<Pipe> pipe;
    } mt;

    // The _mt postfix means: to be called on message-thread.
    void on_pipe_message_received_mt(span<const char> memory_block);

    // Audio thread variables and functions
    vector<float> tone_buf;
    unsigned tone_playhead = 0;
    optional<Command> current_command;
    DecibelCycleLoopGenerator decibel_cycle_loop_generator;

    // Thread-safe communication between audio and message threads
    moodycamel::ReaderWriterQueue<Command> message_to_audio_queue;
};
