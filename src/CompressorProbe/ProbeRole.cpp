#include "ProbeRole.h"

#include "Command.h"
#include "CommonState.h"
#include "pipes.h"

#include <meadow/cppext.h>

ProbeRole::ProbeRole(CommonState& common_state_arg)
    : common_state(common_state_arg)
{
    for (size_t i = 0; i < ProbeProtocol::id_bins.size(); ++i)
        filters[i].coeff =
          2.0 * std::cos(2.0 * juce::MathConstants<double>::pi * ProbeProtocol::id_bins[i] / ProbeProtocol::nfft);
}

// Might be called on audio thread, but strictly before process_block.
void ProbeRole::prepare_to_play(double /*sample_rate*/, int /*samples_per_block*/)
{
    for (auto& f : filters)
        f.reset();
    sample_count = 0;
    last_decoded_id = -1;
    confirm_count = 0;
    confirmed_generator_id.reset();
}

void ProbeRole::release_resources()
{
    pipe.reset();
    common_state.generator_id.reset();
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
    if (!confirmed_generator_id) {
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

    auto p = Pipe::connect(id, *common_state.file_log_sink);
    if (!p) {
        common_state.error =
          format("Generator ID #{} received but failed to connect to named pipe \"{}\"", id, get_pipe_name_for_id(id));
        return;
    }
    if (!p->send_command(Command::Stop{})) {
        common_state.error = format(
          "Generator ID #{} received but failed to send message to named pipe \"{}\"", id, get_pipe_name_for_id(id)
        );
        return;
    }
    pipe = MOVE(p);
    pipe->on_message_received = [this](span<const char> memory_block) {
        on_pipe_message_received(memory_block);
    };
    common_state.generator_id = id;
}

void ProbeRole::on_pipe_message_received(span<const char> memory_block)
{
    auto a = common_state.file_log_sink->activate();

    auto response_or = response_from_span(memory_block);
    if (!response_or) {
        LOG(ERROR) << format("ProbeRole::on_pipe_message_received: {}", response_or.error());
        return;
    }
    auto& response = *response_or;
    if (pending_command && response.command_index == command_index(*pending_command)) {
        LOG(INFO) << format("Pending command response received, command_index: {}", response.command_index);
    }
}
