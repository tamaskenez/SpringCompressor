#include "CompressorProbeProcessor.h"

#include "CompressorProbeEditor.h"

#include <meadow/cppext.h>

// Minimal InterprocessConnection subclass used to claim and hold a generator ID pipe.
class GeneratorPipe : public juce::InterprocessConnection
{
public:
    GeneratorPipe()
        : juce::InterprocessConnection(/*callbacksOnMessageThread=*/false)
    {
    }
    void connectionMade() override {}
    void connectionLost() override {}
    void messageReceived(const juce::MemoryBlock&) override {}
};

CompressorProbeProcessor::CompressorProbeProcessor()
    : AudioProcessor(
        BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
      )
{
}

void CompressorProbeProcessor::setup_generator()
{
    // Pick a random 16-bit starting ID and search forward until we find a pipe name
    // that doesn't exist yet. With 65536 possible IDs, this nearly always succeeds
    // on the first try; stale files from a previous crash are simply skipped.
    if (generator_id < 0) {
        const auto start = static_cast<uint16_t>(std::random_device{}());
        auto id = start;
        do {
            auto pipe = std::make_unique<GeneratorPipe>();
            if (pipe->createPipe("CompressorProbe-" + juce::String(id), 0, true)) {
                generator_id = id;
                generator_pipe = std::move(pipe);
                break;
            }
            ++id; // uint16_t wraps naturally at 65535 → 0
        } while (id != start); // full wrap means all 65536 are taken (practically impossible)
    }

    if (generator_id < 0)
        return;

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
}

void CompressorProbeProcessor::releaseResources() {}

void CompressorProbeProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals no_denormals;
    const auto current_role = role.load();

    if (current_role == Role::Probe) {
        buffer.clear();
        return;
    }

    if (current_role != Role::Generator)
        return; // unset role: pass through

    if (generator_id < 0)
        setup_generator();

    if (generator_id < 0)
        return; // all 16 IDs taken: pass through

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
