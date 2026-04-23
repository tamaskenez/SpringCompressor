#include "GeneratorRole.h"

#include "Command.h"
#include "CompressorProbeProcessor.h"
#include "ProbeProtocol.h"
#include "juce_util/logging.h"
#include "meadow/matlab.h"
#include "pipes.h"
#include "sync.h"

#include <magic_enum/magic_enum.hpp>
#include <meadow/cppext.h>

namespace
{
pair<int, unique_ptr<Pipe>> create_id_and_pipe(FileLogSink& file_log_sink)
{
    // Pick a random 16-bit starting ID and search forward until we find a pipe name
    // that doesn't exist yet. With 65536 possible IDs, this nearly always succeeds
    // on the first try; stale files from a previous crash are simply skipped.
    const auto start = static_cast<uint16_t>(std::random_device{}());
    auto candidate = start;
    do {
        if (auto p = Pipe::create_new(candidate, file_log_sink)) {
            return {candidate, MOVE(p)};
        }
        ++candidate; // uint16_t wraps naturally at 65535 → 0
    } while (candidate != start); // full wrap means all 65536 are taken (practically impossible)
    return {-1, nullptr};
}

vector<float> build_tone_buffer(uint16_t id)
{ // Build the ID tone buffer: a block of nfft samples of exact on-bin sinusoids.
    // Each sinusoid completes an integer number of cycles in nfft samples, so the
    // buffer tiles seamlessly with no discontinuity at the loop point.
    vector<float> tone_buf(ProbeProtocol::nfft);
    const float amplitude = matlab::db2mag(-40.0f);

    auto add_tone = [&](int bin) {
        for (int n = 0; n < ProbeProtocol::nfft; ++n) {
            const double phase = 2.0 * juce::MathConstants<double>::pi * bin * n / ProbeProtocol::nfft;
            tone_buf[static_cast<size_t>(n)] += amplitude * ffcast<float>(std::sin(phase));
        }
    };

    add_tone(ProbeProtocol::id_bins[0]); // sync_on: always present
    // id_bins[1] (sync_off) is intentionally never added.
    for (int b = 0; b < 16; ++b) {
        if (id & (1 << b))
            add_tone(ProbeProtocol::id_bins[sucast(2 + b)]);
    }
    return tone_buf;
}
} // namespace

GeneratorRole::GeneratorRole(CompressorProbeProcessor& processor_arg)
    : processor(processor_arg)
    , decibel_cycle_loop_generator(1.0)
{
    // The constructor is expected to be called on the message thread, but with processor.mutex locked.
    JUCE_ASSERT_MESSAGE_THREAD

    auto [new_id, new_pipe] = create_id_and_pipe(*processor.ts_state.file_log_sink);
    if (!new_pipe) {
        processor.call_async_on_mt([this] {
            processor.mt_state.error = format(
              "Failed to find an available pipe name with ID 0-65535 (starting with \"{}\").", get_pipe_name_for_id(0)
            );
        });

        return;
    }
    new_pipe->on_message_received = [this](span<const char> memory_block) {
        on_pipe_message_received_mt(memory_block);
    };
    processor.ts_state.generator_id_in_generator = new_id;
    mt.pipe = MOVE(new_pipe);

    tone_buf = build_tone_buffer(iicast<uint16_t>(new_id));
    processor.ts_state.generator_status.store(GeneratorStatus::TransmittingId);
}

GeneratorRole::~GeneratorRole() = default;

// Might be called on audio thread, but strictly before process_block.
void GeneratorRole::prepare_to_play(double sample_rate, int samples_per_block)
{
    tone_playhead = 0;
    decibel_cycle_loop_generator = DecibelCycleLoopGenerator(sample_rate);
    output_block.resize(sucast(samples_per_block));
}

void GeneratorRole::release_resources()
{
    processor.ts_state.generator_status.store(GeneratorStatus::Idle);
}

void GeneratorRole::process_block(int64_t /*block_sample_index*/, juce::AudioBuffer<float>& buffer)
{
    auto& prepared_to_play = processor.common_state.prepared_to_play;
    LOG_IF(FATAL, !prepared_to_play) << "GeneratorRole::process_block called without prepare_to_play";
    const auto sample_rate = prepared_to_play->sample_rate;

    bool got_new_command = false;
    {
        Command cmd(nullopt);
        while (message_to_audio_queue.try_dequeue(cmd)) {
            current_command = cmd;
            got_new_command = true;
        }
    }
    if (got_new_command) {
        processor.call_async_on_mt([this,
                                    response = Response{
                                      .command_index = current_command->command_index,
                                      .command_received_timestamp = chr::steady_clock::now()
                                    }] {
            mt.pipe->send_message(response_as_span(response));
        });
        tone_playhead = 0;
        silent_samples_after_new_command = iround<size_t>(sample_rate * k_silence_after_new_command_sec);
        sync_signal_to_transmit = get_sync_signal();
    }
    const unsigned N = sucast(buffer.getNumSamples());
    CHECK(buffer.getNumChannels() == 1 || buffer.getNumChannels() == 2);
    switch (processor.ts_state.generator_status.load()) {
    case GeneratorStatus::Idle:
        buffer.clear();
        break;
    case GeneratorStatus::TransmittingId: {
        const int num_samples = buffer.getNumSamples();
        const int num_channels = buffer.getNumChannels();
        for (int i = 0; i < num_samples; ++i) {
            const float s = tone_buf[static_cast<size_t>(tone_playhead)];
            tone_playhead = (tone_playhead + 1) % ProbeProtocol::nfft;
            for (int ch = 0; ch < num_channels; ++ch)
                buffer.setSample(ch, i, s);
        }
    } break;
    case GeneratorStatus::Connected:
        output_block.reserve(N);
        output_block.clear();
        if (silent_samples_after_new_command > 0) {
            const auto M = std::min<size_t>(N, silent_samples_after_new_command);
            output_block.assign(M, 0.0f);
            silent_samples_after_new_command -= M;
        }
        if (silent_samples_after_new_command == 0 && !sync_signal_to_transmit.empty()) {
            const auto num_samples_left = N - output_block.size();
            const auto M = std::min<size_t>(num_samples_left, sync_signal_to_transmit.size());
            output_block.insert(
              output_block.end(), sync_signal_to_transmit.begin(), sync_signal_to_transmit.begin() + uscast(M)
            );
            remove_prefix(sync_signal_to_transmit, M);
        }
        // Overwrite the first samples with output block.
        // Leave the rest.
        std::copy_n(output_block.begin(), output_block.size(), buffer.getArrayOfWritePointers()[0]);
        if (buffer.getNumChannels() == 2) {
            std::copy_n(output_block.begin(), output_block.size(), buffer.getArrayOfWritePointers()[1]);
        }
        if (output_block.size() < N) {
            switch (enum_of(current_command->mode)) {
            case Mode::E::Bypass:
                // TODO add high frequency GR track signal.
                break;
            EVARIANT_CASE(current_command->mode, Mode, DecibelCycle, x) {
                const auto M = N - output_block.size();
                decibel_cycle_loop_generator.generate_block(
                  x, tone_playhead, span(buffer.getArrayOfWritePointers()[0] + output_block.size(), M)
                );
                tone_playhead = (tone_playhead + M) % decibel_cycle_loop_generator.cycle_length_samples;
                if (buffer.getNumChannels() == 2) {
                    std::copy_n(
                      buffer.getArrayOfWritePointers()[0] + output_block.size(),
                      M,
                      buffer.getArrayOfWritePointers()[1] + output_block.size()
                    );
                }
            } break;
            }
        }
    }
}

void GeneratorRole::on_pipe_message_received_mt(span<const char> memory_block)
{
    JUCE_ASSERT_MESSAGE_THREAD

    auto a = processor.ts_state.file_log_sink->activate();

    auto cmd_or = command_from_span(memory_block);
    if (!cmd_or) {
        LOG(ERROR) << format("GeneratorRole::on_pipe_message_received: {}", cmd_or.error());
        return;
    }

    processor.ts_state.generator_status.store(GeneratorStatus::Connected);

    auto& cmd = *cmd_or;
    processor.mt_state.generator_command =
      format("Received command#{}: {}", cmd.command_index, magic_enum::enum_name(enum_of(cmd.mode)));
    message_to_audio_queue.enqueue(cmd);
}
