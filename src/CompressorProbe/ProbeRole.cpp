#include "ProbeRole.h"

#include "Command.h"

#include <meadow/cppext.h>

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

ProbeRole::ProbeRole()
{
    for (size_t i = 0; i < ProbeProtocol::id_bins.size(); ++i)
        filters[i].coeff =
          2.0 * std::cos(2.0 * juce::MathConstants<double>::pi * ProbeProtocol::id_bins[i] / ProbeProtocol::nfft);
}

ProbeRole::~ProbeRole()
{
    if (pipe)
        pipe->disconnect();
}

void ProbeRole::prepare()
{
    for (auto& f : filters)
        f.reset();
    sample_count = 0;
    last_decoded_id = -1;
    confirm_count = 0;
    confirmed_id.store(-1);
    if (pipe) {
        pipe->disconnect();
        pipe.reset();
    }
    connection_pending.store(false);
}

void ProbeRole::process_frame()
{
    const double sync_on_power = filters[0].power();
    const double sync_off_power = filters[1].power();

    int decoded_id = 0;
    for (int b = 0; b < 16; ++b) {
        if (filters[static_cast<size_t>(2 + b)].power() > ProbeProtocol::detection_threshold)
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
        if (++confirm_count >= 3)
            confirmed_id.store(decoded_id);
    } else {
        last_decoded_id = decoded_id;
        confirm_count = 1;
    }
}

void ProbeRole::connect_to_generator()
{
    const int id = confirmed_id.load();
    if (id < 0)
        return;
    auto p = std::make_unique<ProbePipe>();
    if (!p->connectToPipe("CompressorProbe-" + juce::String(id), 1000))
        return;
    Command::V cmd = Command::Stop{};
    juce::MemoryBlock mb(sizeof(cmd));
    std::memcpy(mb.getData(), &cmd, sizeof(cmd));
    p->sendMessage(mb);
    pipe = std::move(p);
}

void ProbeRole::process_block(juce::AudioBuffer<float>& buffer)
{
    if (confirmed_id.load() < 0) {
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
    } else if (!connection_pending.exchange(true)) {
        // ID just confirmed: connect to the generator and send Stop (message thread).
        juce::MessageManager::callAsync([this] {
            connect_to_generator();
        });
    }
    buffer.clear();
}
