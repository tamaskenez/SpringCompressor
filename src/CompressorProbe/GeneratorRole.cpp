#include "GeneratorRole.h"

#include "Command.h"

#include <meadow/cppext.h>

// Holds the generator's named pipe and dispatches incoming commands via a callback.
class GeneratorPipe : public juce::InterprocessConnection
{
public:
    std::function<void(const Command::V&)> on_command;

    GeneratorPipe()
        : juce::InterprocessConnection(/*callbacksOnMessageThread=*/false)
    {
    }
    void connectionMade() override {}
    void connectionLost() override {}
    void messageReceived(const juce::MemoryBlock& mb) override
    {
        if (mb.getSize() != sizeof(Command::V))
            return;
        Command::V cmd;
        std::memcpy(&cmd, mb.getData(), sizeof(cmd));
        if (on_command)
            on_command(cmd);
    }
};

GeneratorRole::~GeneratorRole()
{
    if (pipe)
        pipe->disconnect();
}

void GeneratorRole::prepare()
{
    tone_playhead = 0;
}

void GeneratorRole::setup()
{
    // Pick a random 16-bit starting ID and search forward until we find a pipe name
    // that doesn't exist yet. With 65536 possible IDs, this nearly always succeeds
    // on the first try; stale files from a previous crash are simply skipped.
    const auto start = static_cast<uint16_t>(std::random_device{}());
    auto candidate = start;
    do {
        auto p = std::make_unique<GeneratorPipe>();
        if (p->createPipe("CompressorProbe-" + juce::String(candidate), 0, true)) {
            p->on_command = [this](const Command::V& cmd) {
                if (std::holds_alternative<Command::Stop>(cmd))
                    status.store(GeneratorStatus::Connected);
            };
            id.store(candidate);
            pipe = std::move(p);
            break;
        }
        ++candidate; // uint16_t wraps naturally at 65535 → 0
    } while (candidate != start); // full wrap means all 65536 are taken (practically impossible)

    if (id.load() < 0)
        return;

    // Build the ID tone buffer: a block of nfft samples of exact on-bin sinusoids.
    // Each sinusoid completes an integer number of cycles in nfft samples, so the
    // buffer tiles seamlessly with no discontinuity at the loop point.
    tone_buf.fill(0.0f);
    constexpr float amplitude = 0.1f;

    auto add_tone = [&](int bin) {
        for (int n = 0; n < ProbeProtocol::nfft; ++n) {
            const double phase = 2.0 * juce::MathConstants<double>::pi * bin * n / ProbeProtocol::nfft;
            tone_buf[static_cast<size_t>(n)] += amplitude * ffcast<float>(std::sin(phase));
        }
    };

    add_tone(ProbeProtocol::id_bins[0]); // sync_on: always present
    // id_bins[1] (sync_off) is intentionally never added.
    const auto bits = static_cast<uint16_t>(id.load());
    for (int b = 0; b < 16; ++b) {
        if (bits & (1 << b))
            add_tone(ProbeProtocol::id_bins[static_cast<size_t>(2 + b)]);
    }

    tone_playhead = 0;
    status.store(GeneratorStatus::TransmittingId);
}

void GeneratorRole::process_block(juce::AudioBuffer<float>& buffer)
{
    if (id.load() < 0)
        setup();
    if (id.load() < 0)
        return; // all 65536 IDs taken: pass through

    if (status.load() == GeneratorStatus::Connected)
        return; // probe acknowledged: pass audio through

    const int num_samples = buffer.getNumSamples();
    const int num_channels = buffer.getNumChannels();
    for (int i = 0; i < num_samples; ++i) {
        const float s = tone_buf[static_cast<size_t>(tone_playhead)];
        tone_playhead = (tone_playhead + 1) % ProbeProtocol::nfft;
        for (int ch = 0; ch < num_channels; ++ch)
            buffer.setSample(ch, i, s);
    }
}
