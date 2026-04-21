#include "ProbeRole.h"

#include "Command.h"
#include "CompressorProbeProcessor.h"
#include "apvts.h"
#include "juce_util/logging.h"
#include "pipes.h"

#include <magic_enum/magic_enum.hpp>
#include <meadow/cppext.h>
#include <meadow/enum.h>

namespace
{
constexpr double k_seconds_of_received_audio_blocks = 1.0;
} // namespace

ProbeRole::ProbeRole(CompressorProbeProcessor& processor_arg)
    : processor(processor_arg)
{
    // The constructor is expected to be called on the message thread, but with processor.mutex locked.
    JUCE_ASSERT_MESSAGE_THREAD
    for (size_t i = 0; i < ProbeProtocol::id_bins.size(); ++i)
        at.filters[i].coeff =
          2.0 * std::cos(2.0 * juce::MathConstants<double>::pi * ProbeProtocol::id_bins[i] / ProbeProtocol::nfft);
}

// Might be called on audio thread, but strictly before process_block.
void ProbeRole::prepare_to_play(double sample_rate, int samples_per_block)
{
    for (auto& f : at.filters) {
        f.reset();
    }
    at.sample_count = 0;
    at.last_decoded_id = -1;
    at.confirm_count = 0;
    at.confirmed_generator_id.reset();

    CHECK(processor.ts_state.prepared_to_play);

    vector<vector<float>> channels_samples(
      sucast(processor.ts_state.num_channels.load()), vector<float>(sucast(samples_per_block))
    );
    const auto N = iceil<size_t>(k_seconds_of_received_audio_blocks * sample_rate / samples_per_block);
    received_audio_blocks = vector<ReceivedAudioBlock>(N);
    for (auto& x : received_audio_blocks) {
        x.channels_samples = channels_samples;
    }
}

void ProbeRole::release_resources()
{
    processor.ts_state.generator_id = k_invalid_generator_id;
    received_audio_blocks = vector<ReceivedAudioBlock>();
    processor.call_async_on_mt([this] {
        mt_state.pipe.reset();
    });
}

// Called on audio thread.
void ProbeRole::process_frame()
{
    const double sync_on_power = at.filters[0].power();
    const double sync_off_power = at.filters[1].power();

    int decoded_id = 0;
    for (int b = 0; b < 16; ++b) {
        if (at.filters[sucast(2 + b)].power() > ProbeProtocol::detection_threshold)
            decoded_id |= (1 << b);
    }

    for (auto& f : at.filters) {
        f.reset();
    }

    // Require sync_on present and sync_off absent to accept this frame.
    if (sync_on_power < ProbeProtocol::detection_threshold || sync_off_power > ProbeProtocol::detection_threshold) {
        at.last_decoded_id = -1;
        at.confirm_count = 0;
        return;
    }

    // Require 3 consecutive frames with the same ID before confirming.
    if (decoded_id == at.last_decoded_id) {
        if (++at.confirm_count >= 3) {
            at.confirmed_generator_id = decoded_id;
            processor.call_async_on_mt([this, decoded_id] {
                on_generator_id_decoded_mt(decoded_id);
            });
        }
    } else {
        at.last_decoded_id = decoded_id;
        at.confirm_count = 1;
    }
}

// Called on audio thread.
void ProbeRole::process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi_messages*/)
{
    if (at.confirmed_generator_id) {
        // Find an available ReceivedAudioBlock.
        optional<size_t> rab_index;
        for (unsigned i = 0; i < received_audio_blocks.size(); ++i) {
            const auto j = at.next_rab_to_check;
            at.next_rab_to_check = (at.next_rab_to_check + 1) % received_audio_blocks.size();
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
            for (auto& f : at.filters)
                f.feed(mono);
            if (++at.sample_count == ProbeProtocol::nfft) {
                process_frame();
                at.sample_count = 0;
            }
        }
    }
    buffer.clear();
}

void ProbeRole::on_generator_id_decoded_mt(int id)
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = processor.ts_state.file_log_sink->activate();
    LOG(INFO) << format("ProbeRole::on_generator_id_decode({})", id);

    {
        auto p = Pipe::connect(id, *processor.ts_state.file_log_sink);
        if (!p) {
            processor.mt_state.error = format(
              "Generator ID #{} received but failed to connect to named pipe \"{}\"", id, get_pipe_name_for_id(id)
            );
            return;
        }
        mt_state.pipe = MOVE(p);
    }
    mt_state.pipe->on_message_received = [this](span<const char> memory_block) {
        on_pipe_message_received_mt(memory_block);
    };

    processor.ts_state.generator_id = id;

    on_mode_changed_mt();
}

void ProbeRole::on_pipe_message_received_mt(span<const char> memory_block) const
{
    auto a = processor.ts_state.file_log_sink->activate();

    LOG(INFO) << format("ProbeRole::on_pipe_message_received(size: {})", memory_block.size());

    auto response_or = response_from_span(memory_block);
    if (!response_or) {
        LOG(ERROR) << format("ProbeRole::on_pipe_message_received: {}", response_or.error());
        return;
    }
    auto& response = *response_or;
    if (mt_state.pending_command && response.command_index == mt_state.pending_command->command_index) {
        LOG(INFO) << format("Pending command response received, command_index: {}", response.command_index);
    }
    // TODO start processing incoming frames with respect to the confirmed command and effective process block index
}

void ProbeRole::on_mode_changed_mt()
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = processor.ts_state.file_log_sink->activate();

    LOG(INFO) << format("ProbeRole::on_mode_changed({})", magic_enum::enum_name(get_mode(processor.mt_state.apvts)));

    auto new_command = Command(mt_state.next_command_index++, get_mode_v(processor.mt_state.apvts));
    if (mt_state.pipe->send_message(command_as_span(new_command))) {
        mt_state.pending_command = new_command;
    } else {
        processor.mt_state.error = format("Failed to send command#{} to generator.", new_command.command_index);
    }
}

void ProbeRole::on_ui_refresh_timer_elapsed_mt()
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = processor.ts_state.file_log_sink->activate();
    size_t rab_index;
    auto& incoming_samples = processor.mt_state.incoming_samples;
    while (audio_to_message_queue.try_dequeue(rab_index)) {
        auto& rab = received_audio_blocks[rab_index];
        switch (rab.channels_samples.size()) {
        case 1:
            incoming_samples.append_range(rab.channels_samples[0]);
            break;
        case 2:
            switch (get_choice<Channels>(processor.mt_state.apvts, "channels")) {
            case Channels::left:
                incoming_samples.append_range(rab.channels_samples[0]);
                break;
            case Channels::right:
                incoming_samples.append_range(rab.channels_samples[1]);
                break;
            case Channels::sum:
                incoming_samples.append_range(
                  vi::zip(rab.channels_samples[0], rab.channels_samples[1]) | vi::transform([](auto&& p) {
                      return std::midpoint(std::get<0>(p), std::get<1>(p));
                  })
                );
                break;
            }
            break;
        default:
            CHECK(false);
        }
        rab.allocated_for_message_thread.store(false); // Give back to audio thread.
        const auto num_remove = std::max<ptrdiff_t>(0, uscast(incoming_samples.size()) - 1000);
        incoming_samples.erase(incoming_samples.begin(), incoming_samples.begin() + num_remove);
    }
    processor.mt_state.update_wave_scope();
}
