#include "engine.h"

#include "SpringLowPass.h"
#include "TransferCurve.h"

#include <meadow/math.h>
#include <meadow/matlab.h>

#include <cassert>
#include <vector>

struct EngineImpl : public Engine {
    double sample_rate = 44100.0;

    std::vector<SpringLowPass> gain_filters;
    TransferCurve transfer_curve;

    float attack_ms = 10.0f;
    float release_ms = 100.0f;

    void update_gain_filter_pars()
    {
        // Note: this might be called on audio thread.
        for (auto& f : gain_filters) {
            f.set_critically_damped_with_time_constant(attack_ms / 1000, release_ms / 1000);
        }
    }
    void prepare_to_play(double sr, int /*maxBlockSize*/, int num_channels) override
    {
        sample_rate = sr;
        gain_filters.assign(num_channels, SpringLowPass(sample_rate));
        for (auto& f : gain_filters) {
            f.set_state(0.0, 0.0);
        }
        update_gain_filter_pars();
    }

    void release_resources() override
    {
        gain_filters.clear();
    }

    void process_block(std::span<float* const> channel_data, int num_samples, std::vector<Trace>* trace_block) override
    {
        assert(channel_data.size() == gain_filters.size());
#ifndef NDEBUG
        if (trace_block) {
            trace_block->clear();
        }
#endif
        for (size_t ch_ix = 0; ch_ix < channel_data.size(); ++ch_ix) {
            auto* channel_buf = channel_data[ch_ix];
            auto& gf = gain_filters[ch_ix];
            for (int i = 0; i < num_samples; ++i) {
                const auto smoothed_signal_power = static_cast<float>(gf.process(square(channel_buf[i])));
                const auto gain = transfer_curve.gain_for_input_db(matlab::pow2db(smoothed_signal_power));
                channel_buf[i] *= gain;
#ifndef NDEBUG
                if (trace_block && ch_ix == 0) {
                    trace_block->emplace_back(Trace{smoothed_signal_power, gain});
                }
#endif
            }
        }
    }

    void set_threshold_db(float dB) override
    {
        transfer_curve.set_threshold_db(dB);
    }
    void set_ratio(float r) override
    {
        transfer_curve.set_ratio(r);
    }
    void set_makeup_gain_db(float /*dB*/) override
    {
        // TODO: remove or implement.
    }
    void set_attack_ms(float ms) override
    {
        attack_ms = ms;
        update_gain_filter_pars();
    }
    void set_release_ms(float ms) override
    {
        release_ms = ms;
        update_gain_filter_pars();
    }
};

std::unique_ptr<Engine> make_engine()
{
    return std::make_unique<EngineImpl>();
}
