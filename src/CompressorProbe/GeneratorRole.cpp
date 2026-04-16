#include "GeneratorRole.h"

#include "Command.h"
#include "CommonState.h"
#include "ProbeProtocol.h"
#include "pipes.h"

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
    constexpr float amplitude = 0.1f;

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

GeneratorRole::GeneratorRole(CommonState& common_state_arg)
    : common_state(common_state_arg)
{
    auto [new_id, new_pipe] = create_id_and_pipe(*common_state.file_log_sink);
    if (!new_pipe) {
        common_state.error = format(
          "Failed to find an available pipe name with ID 0-65535 (starting with \"{}\").", get_pipe_name_for_id(0)
        );
        return;
    }
    new_pipe->on_message_received = [this](span<const char> memory_block) {
        on_pipe_message_received(memory_block);
    };
    common_state.generator_id = new_id;
    pipe = MOVE(new_pipe);

    tone_buf = build_tone_buffer(iicast<uint16_t>(new_id));
    status.store(GeneratorStatus::TransmittingId);
}

// Might be called on audio thread, but strictly before process_block.
void GeneratorRole::prepare_to_play(double /*sample_rate*/, int /*samples_per_block*/)
{
    tone_playhead = 0;
}

void GeneratorRole::release_resources() {}

void GeneratorRole::process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midi_messages*/)
{
    switch (status.load()) {
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
        buffer.clear();
        break;
    }
}

void GeneratorRole::on_pipe_message_received(span<const char> memory_block)
{
    auto a = common_state.file_log_sink->activate();

    auto cmd_or = command_from_span(memory_block);
    if (!cmd_or) {
        LOG(ERROR) << format("GeneratorRole::on_pipe_message_received: {}", cmd_or.error());
        return;
    }

    auto& cmd = *cmd_or;
    switch (enum_of(cmd)) {
    case Command::E::Stop:
        LOG(INFO) << "GeneratorRole::on_pipe_message_received: Stop";
        status.store(GeneratorStatus::Connected);
        break;
    EVARIANT_CASE2(cmd, Command, DecibelCycle, x) {
        LOG(INFO) << format("GeneratorRole::on_pipe_message_received: DecibelCycle: {}", x.to_string());
        // todo
    } break;
    }
}
