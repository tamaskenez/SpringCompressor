#include "ProbeRole.h"

#include "Command.h"
#include "CommonState.h"
#include "pipes.h"

#include <magic_enum/magic_enum.hpp>
#include <meadow/cppext.h>

namespace
{
constexpr double k_seconds_of_received_audio_blocks = 1.0;
}

ProbeRole::ProbeRole(CommonState& common_state_arg)
    : common_state(common_state_arg)
{
    for (size_t i = 0; i < ProbeProtocol::id_bins.size(); ++i)
        filters[i].coeff =
          2.0 * std::cos(2.0 * juce::MathConstants<double>::pi * ProbeProtocol::id_bins[i] / ProbeProtocol::nfft);
}

// Might be called on audio thread, but strictly before process_block.
void ProbeRole::prepare_to_play(double sample_rate, int samples_per_block)
{
    for (auto& f : filters) {
        f.reset();
    }
    sample_count = 0;
    last_decoded_id = -1;
    confirm_count = 0;
    confirmed_generator_id.reset();

    CHECK(common_state.prepared_to_play);

    vector<vector<float>> channels_samples(
      sucast(common_state.prepared_to_play->num_channels), vector<float>(sucast(samples_per_block))
    );
    const auto N = iceil<size_t>(k_seconds_of_received_audio_blocks * sample_rate / samples_per_block);
    received_audio_blocks = vector<ReceivedAudioBlock>(N);
    for (auto& x : received_audio_blocks) {
        x.channels_samples = channels_samples;
    }
}

void ProbeRole::release_resources()
{
    state.pipe.reset();
    common_state.generator_id.reset();
    received_audio_blocks = vector<ReceivedAudioBlock>();
}

void ProbeRole::process_frame()
{
    const double sync_on_power = filters[0].power();
    const double sync_off_power = filters[1].power();

    int decoded_id = 0;
    for (int b = 0; b < 16; ++b) {
        if (filters[sucast(2 + b)].power() > ProbeProtocol::detection_threshold)
            decoded_id |= (1 << b);
    }

    for (auto& f : filters)
        f.reset();

    // Require sync_on present and sync_off absent to accept this frame.
    if (sync_on_power < ProbeProtocol::detection_threshold || sync_off_power > ProbeProtocol::detection_threshold) {
        last_decoded_id = -1;
        confirm_count = 0;
        return;
    }

    // Require 3 consecutive frames with the same ID before confirming.
    if (decoded_id == last_decoded_id) {
        if (++confirm_count >= 3) {
            confirmed_generator_id = decoded_id;
            juce::MessageManager::callAsync([this, decoded_id, weak_token = weak_ptr(common_state.token)] {
                if (weak_token.lock()) {
                    on_generator_id_decoded(decoded_id);
                }
            });
        }
    } else {
        last_decoded_id = decoded_id;
        confirm_count = 1;
    }
}

void ProbeRole::process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi_messages*/)
{
    if (confirmed_generator_id) {
        // Find an available ReceivedAudioBlock.
        optional<size_t> rab_index;
        for (unsigned i = 0; i < received_audio_blocks.size(); ++i) {
            const auto j = next_rab_to_check;
            next_rab_to_check = (next_rab_to_check + 1) % received_audio_blocks.size();
            if (!received_audio_blocks[j].allocated_for_message_thread.load()) {
                rab_index = j;
                break;
            }
        }
        if (!rab_index) {
            DCHECK(rab_index);
            LOG(ERROR) << "No available ReceivedAudioBlock";
        } else {
            auto& rab = received_audio_blocks[*rab_index];
            rab.allocated_for_message_thread.store(true);
            CHECK(cmp_equal(rab.channels_samples.size(), buffer.getNumChannels()));
            CHECK(!rab.channels_samples.empty());
            const auto N = rab.channels_samples[0].size();
            CHECK(cmp_equal(buffer.getNumSamples(), N));
            for (unsigned i = 0; i < rab.channels_samples.size(); ++i) {
                CHECK(rab.channels_samples[i].size() == N);
                std::copy_n(buffer.getReadPointer(uscast(i)), N, rab.channels_samples[i].data());
            }
            audio_to_message_queue.enqueue(*rab_index);
        }
    } else {
        // Accumulate samples and run Goertzel detection frame by frame.
        const int num_samples = buffer.getNumSamples();
        const int num_channels = buffer.getNumChannels();
        for (int i = 0; i < num_samples; ++i) {
            double mono = 0.0;
            for (int ch = 0; ch < num_channels; ++ch)
                mono += buffer.getSample(ch, i);
            mono /= num_channels;
            for (auto& f : filters)
                f.feed(mono);
            if (++sample_count == ProbeProtocol::nfft) {
                process_frame();
                sample_count = 0;
            }
        }
    }
    buffer.clear();
}

void ProbeRole::on_generator_id_decoded(int id)
{
    auto a = common_state.file_log_sink->activate();
    LOG(INFO) << format("ProbeRole::on_generator_id_decode({})", id);

    {
        auto p = Pipe::connect(id, *common_state.file_log_sink);
        if (!p) {
            common_state.error = format(
              "Generator ID #{} received but failed to connect to named pipe \"{}\"", id, get_pipe_name_for_id(id)
            );
            return;
        }
        state.pipe = MOVE(p);
    }
    state.pipe->on_message_received = [this](span<const char> memory_block) {
        on_pipe_message_received(memory_block);
    };

    auto new_command = Command(Mode::Bypass{});
    if (!state.pipe->send_message(command_as_span(new_command))) {
        common_state.error = format(
          "Generator ID #{} received but failed to send message to named pipe \"{}\"", id, get_pipe_name_for_id(id)
        );
        return;
    }
    state.pending_command = new_command;
    LOG(INFO) << format("Sent Bypass command#{}", new_command.command_index);

    common_state.generator_id = id;
}

void ProbeRole::on_pipe_message_received(span<const char> memory_block) const
{
    auto a = common_state.file_log_sink->activate();

    LOG(INFO) << format("ProbeRole::on_pipe_message_received(size: {})", memory_block.size());

    auto response_or = response_from_span(memory_block);
    if (!response_or) {
        LOG(ERROR) << format("ProbeRole::on_pipe_message_received: {}", response_or.error());
        return;
    }
    auto& response = *response_or;
    if (state.pending_command && response.command_index == state.pending_command->command_index) {
        LOG(INFO) << format("Pending command response received, command_index: {}", response.command_index);
    }
}

namespace
{
Mode::V get_mode_as_v(const ProbeRoleState::Modes& modes, Mode::E e)
{
    switch (e) {
    case Mode::E::Bypass:
        return modes.bypass;
    case Mode::E::DecibelCycle:
        return modes.decibel_cycle;
    }
}
} // namespace

void ProbeRole::on_mode_changed(Mode::E mode_e)
{
    auto a = common_state.file_log_sink->activate();

    LOG(INFO) << format("ProbeRole::on_mode_changed({})", magic_enum::enum_name(mode_e));

    auto new_command = Command(get_mode_as_v(state.modes, mode_e));
    if (state.pipe->send_message(command_as_span(new_command))) {
        state.pending_command = new_command;
        state.current_mode = mode_e;
    } else {
        common_state.error = format("Failed to send command#{} to generator.", new_command.command_index);
    }
}

void ProbeRole::on_ui_refresh_timer_elapsed()
{
    auto a = common_state.file_log_sink->activate();
    size_t rab_index;
    while (audio_to_message_queue.try_dequeue(rab_index)) {
        auto& rab = received_audio_blocks[rab_index];
        rab.allocated_for_message_thread.store(false); // Give back to audio thread.
    }
}
