#include "CompressorProbeProcessor.h"

#include "Command.h"
#include "CompressorProbeEditor.h"

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

// Probe-side pipe: connects to the generator and sends commands; receives nothing.
class ProbePipe : public juce::InterprocessConnection
{
public:
    ProbePipe()
        : juce::InterprocessConnection(/*callbacksOnMessageThread=*/false)
    {
    }
    void connectionMade() override {}
    void connectionLost() override {}
    void messageReceived(const juce::MemoryBlock&) override {}
};

CompressorProbeProcessor::~CompressorProbeProcessor()
{
    if (generator_pipe)
        generator_pipe->disconnect();
    if (probe_pipe)
        probe_pipe->disconnect();
}

CompressorProbeProcessor::CompressorProbeProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
{
    // Goertzel coefficients depend only on the fixed bin indices and nfft — set once.
    for (size_t i = 0; i < id_bins.size(); ++i)
        probe_filters[i].coeff = 2.0 * std::cos(2.0 * juce::MathConstants<double>::pi * id_bins[i] / nfft);
}

void CompressorProbeProcessor::setup_generator()
{
    // Pick a random 16-bit starting ID and search forward until we find a pipe name
    // that doesn't exist yet. With 65536 possible IDs, this nearly always succeeds
    // on the first try; stale files from a previous crash are simply skipped.
    if (generator_id.load() < 0) {
        const auto start = static_cast<uint16_t>(std::random_device{}());
        auto id = start;
        do {
            auto pipe = std::make_unique<GeneratorPipe>();
            if (pipe->createPipe("CompressorProbe-" + juce::String(id), 0, true)) {
                pipe->on_command = [this](const Command::V& cmd) {
                    if (std::holds_alternative<Command::Stop>(cmd))
                        generator_status.store(GeneratorStatus::Connected);
                };
                generator_id.store(id);
                generator_pipe = std::move(pipe);
                break;
            }
            ++id; // uint16_t wraps naturally at 65535 → 0
        } while (id != start); // full wrap means all 65536 are taken (practically impossible)
    }

    if (generator_id.load() < 0)
        return;

    generator_status.store(GeneratorStatus::TransmittingId);

    // Build the ID tone buffer: a block of nfft samples of exact on-bin sinusoids.
    // Each sinusoid completes an integer number of cycles in nfft samples, so the
    // buffer tiles seamlessly with no discontinuity at the loop point.
    id_tone_buf.fill(0.0f);
    constexpr float amplitude = 0.1f;

    auto add_tone = [&](int bin) {
        for (int n = 0; n < nfft; ++n) {
            const double phase = 2.0 * juce::MathConstants<double>::pi * bin * n / nfft;
            id_tone_buf[static_cast<size_t>(n)] += amplitude * ffcast<float>(std::sin(phase));
        }
    };

    add_tone(id_bins[0]); // sync_on: always present
    // id_bins[1] (sync_off) is intentionally never added.
    const auto bits = static_cast<uint16_t>(generator_id);
    for (int b = 0; b < 16; ++b) {
        if (bits & (1 << b))
            add_tone(id_bins[static_cast<size_t>(2 + b)]);
    }

    tone_playhead = 0;
}

void CompressorProbeProcessor::prepareToPlay(double /*sampleRate*/, int /*samplesPerBlock*/)
{
    tone_playhead = 0;

    // Reset probe detection state so pairing runs again on each engine restart.
    for (auto& f : probe_filters)
        f.reset();
    probe_sample_count = 0;
    probe_last_decoded_id = -1;
    probe_confirm_count = 0;
    probe_confirmed_id.store(-1);
    engine_running.store(true);
}

void CompressorProbeProcessor::process_probe_frame()
{
    const double sync_on_power = probe_filters[0].power();
    const double sync_off_power = probe_filters[1].power();

    int decoded_id = 0;
    for (int b = 0; b < 16; ++b) {
        if (probe_filters[static_cast<size_t>(2 + b)].power() > detection_threshold)
            decoded_id |= (1 << b);
    }

    for (auto& f : probe_filters)
        f.reset();

    // Require sync_on present and sync_off absent to accept this frame.
    if (sync_on_power < detection_threshold || sync_off_power > detection_threshold) {
        probe_last_decoded_id = -1;
        probe_confirm_count = 0;
        return;
    }

    // Require 3 consecutive frames with the same ID before confirming.
    if (decoded_id == probe_last_decoded_id) {
        if (++probe_confirm_count >= 3)
            probe_confirmed_id.store(decoded_id);
    } else {
        probe_last_decoded_id = decoded_id;
        probe_confirm_count = 1;
    }
}

void CompressorProbeProcessor::releaseResources()
{
    engine_running.store(false);
}

void CompressorProbeProcessor::connect_to_generator()
{
    const int id = probe_confirmed_id.load();
    if (id < 0)
        return;
    auto pipe = std::make_unique<ProbePipe>();
    if (!pipe->connectToPipe("CompressorProbe-" + juce::String(id), 1000))
        return;
    Command::V cmd = Command::Stop{};
    juce::MemoryBlock mb(sizeof(cmd));
    std::memcpy(mb.getData(), &cmd, sizeof(cmd));
    pipe->sendMessage(mb);
    probe_pipe = std::move(pipe);
}

void CompressorProbeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals no_denormals;
    const auto current_role = role.load();

    if (current_role == Role::Probe) {
        if (probe_confirmed_id.load() < 0) {
            // Accumulate samples and run Goertzel detection frame by frame.
            const int num_samples = buffer.getNumSamples();
            const int num_channels = buffer.getNumChannels();
            for (int i = 0; i < num_samples; ++i) {
                double mono = 0.0;
                for (int ch = 0; ch < num_channels; ++ch)
                    mono += buffer.getSample(ch, i);
                mono /= num_channels;
                for (auto& f : probe_filters)
                    f.feed(mono);
                if (++probe_sample_count == nfft) {
                    process_probe_frame();
                    probe_sample_count = 0;
                }
            }
        } else if (!probe_connection_pending.exchange(true)) {
            // ID just confirmed: connect to the generator and send Stop (message thread).
            juce::MessageManager::callAsync([this] {
                connect_to_generator();
            });
        }
        buffer.clear();
        return;
    }

    if (current_role != Role::Generator)
        return; // unset role: pass through

    if (generator_id.load() < 0)
        setup_generator();

    if (generator_id.load() < 0)
        return; // all 65536 IDs taken: pass through

    if (generator_status.load() == GeneratorStatus::Connected)
        return; // probe acknowledged: pass audio through

    const int num_samples = buffer.getNumSamples();
    const int num_channels = buffer.getNumChannels();
    for (int i = 0; i < num_samples; ++i) {
        const float s = id_tone_buf[static_cast<size_t>(tone_playhead)];
        tone_playhead = (tone_playhead + 1) % nfft;
        for (int ch = 0; ch < num_channels; ++ch)
            buffer.setSample(ch, i, s);
    }
}

juce::AudioProcessorEditor* CompressorProbeProcessor::createEditor()
{
    return new CompressorProbeEditor(*this);
}

void CompressorProbeProcessor::getStateInformation(juce::MemoryBlock& /*destData*/) {}

void CompressorProbeProcessor::setStateInformation(const void* /*data*/, int /*sizeInBytes*/) {}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CompressorProbeProcessor();
}
