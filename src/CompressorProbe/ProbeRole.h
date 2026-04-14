#pragma once

#include "ProbeProtocol.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <atomic>
#include <memory>

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

class ProbeRole
{
public:
    ProbeRole(); // initialises Goertzel coefficients
    ~ProbeRole();

    // Called from prepareToPlay (audio thread). Resets all detection state.
    void prepare();

    // Called from processBlock (audio thread). Always clears the output buffer.
    void process_block(juce::AudioBuffer<float>& buffer);

    // UI-thread-safe accessor.
    int get_confirmed_id() const noexcept
    {
        return confirmed_id.load();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProbeRole)

private:
    void process_frame();
    void connect_to_generator();

    // One Goertzel filter per bin; coefficients are set in the constructor.
    std::array<GoertzelFilter, ProbeProtocol::id_bins.size()> filters{};
    int sample_count = 0;
    int last_decoded_id = -1;          // most recently decoded ID, -1 = none yet
    int confirm_count = 0;             // consecutive frames with the same decoded ID
    std::atomic<int> confirmed_id{-1}; // -1 until pairing is complete
    std::unique_ptr<juce::InterprocessConnection> pipe;
    std::atomic<bool> connection_pending{false};
};
