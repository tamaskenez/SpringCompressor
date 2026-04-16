#pragma once

#include "CommonState.h"
#include "ProbeProtocol.h"
#include "RoleInterface.h"
#include "pipes.h"

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

struct CommonState;

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

class ProbeRole : public RoleInterface
{
public:
    explicit ProbeRole(CommonState& common_state); // initialises Goertzel coefficients

    void prepare_to_play(double sample_rate, int samples_per_block) override;
    void process_block(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi_messages) override;
    void release_resources() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProbeRole)

private:
    // Message thread variables and functions
    CommonState& common_state;
    std::unique_ptr<Pipe> pipe;

    void on_generator_id_decoded(int id);

    // Audio thread variables and functions

    // One Goertzel filter per bin; coefficients are set in the constructor.
    std::array<GoertzelFilter, ProbeProtocol::id_bins.size()> filters{};
    int sample_count = 0;
    int last_decoded_id = -1; // most recently decoded ID, -1 = none yet
    int confirm_count = 0;    // consecutive frames with the same decoded ID
    std::optional<int> confirmed_generator_id;

    void process_frame();
};
