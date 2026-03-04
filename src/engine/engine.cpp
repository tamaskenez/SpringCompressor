#include "engine.h"

#include "SpringLowPass.h"
#include "TransferCurve.h"

#include <meadow/math.h>
#include <meadow/matlab.h>

#include <cassert>
#include <vector>

struct ChannelState {
    explicit ChannelState(double sample_rate)
        : gain_filter(sample_rate)
    {
    }
    SpringLowPass gain_filter;
    double gain = NAN;
};

struct EngineImpl : public Engine {
    double sample_rate = 44100.0;

    std::vector<ChannelState> channel_states;
    TransferCurve transfer_curve;

    float attack_ms = 10.0f;
    float release_ms = 100.0f;
    float makeup_gain = 1.0f;
    GainControlApplication gain_control_application = GainControlApplication::on_gr_db;

    void update_gain_filter_pars()
    {
        // Note: this might be called on audio thread.
        for (auto& chs : channel_states) {
            chs.gain_filter.set_critically_damped_with_time_constant(attack_ms / 1000, release_ms / 1000);
            switch (gain_control_application) {
            case GainControlApplication::on_squared_input:
            case GainControlApplication::on_gr_db:
                chs.gain_filter.set_state(0.0, 0.0);
                break;
            case GainControlApplication::on_gr_mag:
                chs.gain_filter.set_state(1.0, 0.0);
                break;
            }
        }
    }

    void prepare_to_play(double sr, int /*maxBlockSize*/, int num_channels) override
    {
        sample_rate = sr;
        channel_states.assign(sucast(num_channels), ChannelState(sample_rate));
        for (auto& chs : channel_states) {
            chs.gain_filter.set_state(0.0, 0.0);
        }
        update_gain_filter_pars();
    }

    void release_resources() override
    {
        channel_states.clear();
    }

    void process_block(std::span<float* const> channel_data, int num_samples) override
    {
        assert(channel_data.size() == channel_states.size());
        if (num_samples == 0) {
            return;
        }
        for (size_t ch_ix = 0; ch_ix < channel_data.size(); ++ch_ix) {
            auto* channel_buf = channel_data[ch_ix];
            auto& chs = channel_states[ch_ix];
            auto& gf = chs.gain_filter;
            double gain = NAN;
            for (int i = 0; i < num_samples; ++i) {
                const double input_sample = channel_buf[i];
                switch (gain_control_application) {
                case GainControlApplication::on_squared_input: {
                    const auto smoothed_signal_power = gf.process(square(input_sample));
                    gain = matlab::db2mag(transfer_curve.gain_db_for_input_db(matlab::pow2db(smoothed_signal_power)));
                } break;
                case GainControlApplication::on_gr_db: {
                    const auto gain_db = transfer_curve.gain_db_for_input_db(matlab::mag2db(abs(input_sample)));
                    const auto smoothed_gain_db = gf.process(gain_db);
                    gain = matlab::db2mag(smoothed_gain_db);
                } break;
                case GainControlApplication::on_gr_mag: {
                    const auto gain_db = transfer_curve.gain_db_for_input_db(matlab::mag2db(abs(input_sample)));
                    const auto smoothed_gain = gf.process(matlab::db2mag(gain_db));
                    gain = std::max(0.0, smoothed_gain);
                } break;
                }
                channel_buf[i] = ffcast<float>(input_sample * gain * makeup_gain);
            }
            chs.gain = gain; // The latest gain.
        }
    }
    std::vector<Trace> process_block_with_trace(std::span<float* const> channel_data, int num_samples) override
    {
        assert(channel_data.size() == 1);
        std::vector<Trace> ts;
        float* cd = channel_data[0];
        for (int j = 0; j < num_samples; ++j, ++cd) {
            process_block(std::span<float* const>(&cd, 1), 1);
            ts.push_back(Trace{.gain = channel_states[0].gain});
        }
        return ts;
    }

    std::optional<TransferCurveUpdateResult> set_transfer_curve(const TransferCurvePars& p) override
    {
        return transfer_curve.set(p);
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
    void set_gain_control_application(GainControlApplication application) override
    {
        if (gain_control_application != application) {
            gain_control_application = application;
            update_gain_filter_pars();
        }
    }
};

std::unique_ptr<Engine> make_engine()
{
    return std::make_unique<EngineImpl>();
}
