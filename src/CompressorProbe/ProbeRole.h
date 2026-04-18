#pragma once

#include "CommonState.h"
#include "ProbeProtocol.h"
#include "ProbeRoleState.h"
#include "RoleInterface.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <readerwriterqueue/readerwriterqueue.h>

#include <array>

struct CommonState;

struct ReceivedAudioBlock {
    std::atomic<bool> allocated_for_message_thread;
    vector<vector<float>> channels_samples;
};

struct GoertzelFilter {
    double coeff = 0.0;
    double s1 = 0.0;
    double s2 = 0.0;

    void reset() noexcept
    {
        s1 = s2 = 0.0;
    }

    void feed(double x) noexcept
    {
        const double s0 = x + coeff * s1 - s2;
        s2 = s1;
        s1 = s0;
    }

    // Returns |X[k]|^2 for the accumulated samples.
    double power() const noexcept
    {
        return s1 * s1 + s2 * s2 - coeff * s1 * s2;
    }
};

class ProbeRole : public RoleInterface
{
public:
    explicit ProbeRole(CommonState& common_state); // initialises Goertzel coefficients

    void prepare_to_play(double sample_rate, int samples_per_block) override;
    void process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) override;
    void release_resources() override;
    void on_ui_refresh_timer_elapsed() override;

    void on_mode_changed(Mode::E mode);

    const ProbeRoleState* get_state() const
    {
        return &state;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProbeRole)

private:
    // Message thread variables and functions
    CommonState& common_state;
    ProbeRoleState state;
    void on_generator_id_decoded(int id);
    void on_pipe_message_received(span<const char> memory_block) const;

    // Audio thread variables and functions

    // One Goertzel filter per bin; coefficients are set in the constructor.
    std::array<GoertzelFilter, ProbeProtocol::id_bins.size()> filters{};
    int sample_count = 0;
    int last_decoded_id = -1; // most recently decoded ID, -1 = none yet
    int confirm_count = 0;    // consecutive frames with the same decoded ID
    std::optional<int> confirmed_generator_id;
    size_t next_rab_to_check = 0;

    void process_frame();

    // Shared between audio and message thread.
    moodycamel::ReaderWriterQueue<size_t> audio_to_message_queue; // Indices into received_audio_blocks.
    vector<ReceivedAudioBlock> received_audio_blocks;
};
