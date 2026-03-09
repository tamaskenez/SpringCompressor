#include "engine.h"

#include "RmsDetector.h"
#include "SpringLowPass.h"
#include "TransferCurve.h"

#include <meadow/math.h>
#include <meadow/matlab.h>

#include <cassert>
#include <vector>

// #define DO_LOG

namespace
{
constexpr double k_rms_detector_hz = 2.1;
} // namespace

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
    GainControlApplication gain_control_application = GainControlApplication::on_gr_db;
    RmsDetector input_rms, output_rms;
    int rms_sample_counter = 0;
    int rms_sample_counter_period = 100;
    std::vector<AF2> rms_samples; // [0] is input, [1] is output.

    void update_gain_filter_pars()
    {
        // Note: this might be called on audio thread.
        for (auto& chs : channel_states) {
            chs.gain_filter.set_critically_damped_with_time_constant(attack_ms / 1000, release_ms / 1000);
            chs.gain_filter.set_state(0.0, 0.0);
        }
    }

    void prepare_to_play(double sr, int maxBlockSize, int num_channels) override
    {
        sample_rate = sr;
        channel_states.assign(sucast(num_channels), ChannelState(sample_rate));
        for (auto& chs : channel_states) {
            chs.gain_filter.set_state(0.0, 0.0);
        }
        update_gain_filter_pars();
        rms_sample_counter = 0;
        rms_sample_counter_period = std::max(1, iround<int>(sample_rate * k_rms_sample_period_sec));
        rms_samples.clear();
        const auto max_rms_samples_in_block =
          sucast((maxBlockSize + rms_sample_counter_period - 1) / rms_sample_counter_period);
        rms_samples.reserve(max_rms_samples_in_block);
        input_rms = RmsDetector(RmsDetector::Flavor::exponential_moving_average, k_rms_detector_hz / sample_rate * 2);
        output_rms = input_rms;
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
        rms_samples.resize(sucast((rms_sample_counter + num_samples) / rms_sample_counter_period));
        const auto measure_rms =
          [num_samples, channel_data, period = rms_sample_counter_period, &rms_samples_ = rms_samples](
            RmsDetector& rms_detector, unsigned offset, int sample_counter
          ) -> int {
            const auto channel_data_size = ifcast<float>(channel_data.size());
            unsigned rms_sample_ix = 0;
            for (int i = 0; i < num_samples; ++i) {
                float sum = 0;
                for (auto* chd : channel_data) {
                    sum += chd[i];
                }
                rms_detector.process(sum / channel_data_size);
                if (++sample_counter >= period) {
                    assert(rms_sample_ix < rms_samples_.size());
                    rms_samples_[rms_sample_ix++][offset] = rms_detector.get_rms();
                    sample_counter = 0;
                }
            }
            assert(rms_sample_ix == rms_samples_.size());
            return sample_counter;
        };
        measure_rms(input_rms, 0, rms_sample_counter);
#ifdef DO_LOG
        static vector<AD3> log;
        log.clear();
#endif
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
                    const auto input_db = matlab::mag2db(abs(input_sample));
                    const auto gain_db = transfer_curve.gain_db_for_input_db(input_db);
                    const auto smoothed_gain_db = -gf.process(-gain_db);
#ifdef DO_LOG
                    if (ch_ix == 0) {
                        log.push_back(AD3{input_db, gain_db, smoothed_gain_db});
                    }
#endif
                    gain = matlab::db2mag(smoothed_gain_db);
                } break;
                case GainControlApplication::on_gr_mag: {
                    const auto gain_db = transfer_curve.gain_db_for_input_db(matlab::mag2db(abs(input_sample)));
                    const auto smoothed_gain = 1 - gf.process(1 - matlab::db2mag(gain_db));
                    gain = std::max(0.0, smoothed_gain);
                } break;
                }
                channel_buf[i] = ffcast<float>(input_sample * gain);
            }
            chs.gain = gain; // The latest gain.
        }
#ifdef DO_LOG
        {
            FILE* f = fopen("/Users/tamas/tmp/log.m", "wt");
            assert(f);
            println(f, "A = [");
            for (const auto& l : log) {
                println(f, "\t{} {} {}", l[0], l[1], l[2]);
            }
            println(f, "];");
            fclose(f);
        }
#endif
        rms_sample_counter = measure_rms(output_rms, 1, rms_sample_counter);
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

    std::optional<TransferCurveState> set_transfer_curve(const TransferCurvePars& p) override
    {
        return transfer_curve.set(p);
    }
    void set_attack_ms(float ms) override
    {
        if (attack_ms != ms) {
            attack_ms = ms;
            update_gain_filter_pars();
        }
    }
    void set_release_ms(float ms) override
    {
        if (release_ms != ms) {
            release_ms = ms;
            update_gain_filter_pars();
        }
    }
    void set_gain_control_application(GainControlApplication application) override
    {
        if (gain_control_application != application) {
            gain_control_application = application;
            update_gain_filter_pars();
        }
    }
    [[nodiscard]] TransferCurveState get_transfer_curve_state() const override
    {
        return transfer_curve.get_state();
    }
    [[nodiscard]] float get_rms_sample_period_sec() const override
    {
        return ffcast<float>(rms_sample_counter_period / sample_rate);
    }
    [[nodiscard]] const std::vector<AF2>& get_rms_samples_of_last_block() const override
    {
        return rms_samples;
    }
};

std::unique_ptr<Engine> make_engine()
{
    return std::make_unique<EngineImpl>();
}
