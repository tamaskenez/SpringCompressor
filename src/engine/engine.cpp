#include "engine.h"

#include "EnvelopeFilter.h"
#include "RecursiveCrossoverRMSDetector.h"
#include "RmsDetector.h"
#include "TransferCurve.h"
#include "meadow/math.h"

#include <meadow/matlab.h>
#include <meadow/ranges.h>

#include <cassert>
#include <vector>

namespace
{
constexpr double k_rms_detector_hz = 2.1;

template<class T>
bool is_normal_or_zero(T x)
{
    switch (std::fpclassify(x)) {
    case FP_INFINITE:
    case FP_NAN:
    case FP_SUBNORMAL:
        return false;
    case FP_NORMAL:
    case FP_ZERO:
        return true;
    default:
        assert(false);
        return false;
    }
}

} // namespace

struct EngineImpl : public Engine {
    struct PreparedToPlay {
        double sample_rate;
        size_t max_block_size;
        RecursiveCrossoverRMSDetector<double> input_level_multiband;
    };

    EnginePars pars;
    optional<PreparedToPlay> prepared_to_play;
    vector<double> sidechain_buf;

    EnvelopeFilter<double> input_level_lpf;
    TransferCurve transfer_curve;
    EnvelopeFilter<double> gr_filter;

    RmsDetector input_rms, output_rms;
    int rms_sample_counter = 0;
    int rms_sample_counter_period = 100;
    std::vector<AF2> rms_samples; // [0] is input, [1] is output.

    bool debug_mode = false;

    void set_input_level_lpf(double sample_rate_arg)
    {
        optional<double> attack_time_samples;
        if (pars.input_level_lpf.attack_time_sec) {
            attack_time_samples = *pars.input_level_lpf.attack_time_sec * sample_rate_arg;
        }
        const auto release_time_samples = pars.input_level_lpf.release_time_sec * sample_rate_arg;
        input_level_lpf = EnvelopeFilter<double>(
          pars.input_level_lpf.rms ? EnvelopeFilterOutputType::power : EnvelopeFilterOutputType::amplitude,
          pars.input_level_lpf.order,
          attack_time_samples,
          release_time_samples
        );
    }

    [[nodiscard]] RecursiveCrossoverRMSDetector<double>
    make_input_level_multiband(double sample_rate_arg, size_t max_block_size) const
    {
        return RecursiveCrossoverRMSDetector<double>(
          RecursiveCrossoverRMSDetectorPars{
            .freq_lo_hps = hz_fs_to_hps(pars.input_level_multiband.freq_lo_hz, sample_rate_arg),
            .freq_hi_hps = hz_fs_to_hps(pars.input_level_multiband.freq_hi_hz, sample_rate_arg),
            .crossovers_per_octave = pars.input_level_multiband.crossovers_per_octave,
            .bpf_order = pars.input_level_multiband.crossover_order,
            .low_pass_filter =
              RecursiveCrossoverRMSDetectorPars::LowPassFilter{
                                                               .order = pars.input_level_multiband.lp_order,
                                                               .attack_time_samples = nullopt,
                                                               .release_ratio_to_period = pars.input_level_multiband.lp_ratio
              },
            .min_release_time_samples = pars.input_level_multiband.min_release_time_sec * sample_rate_arg,
            .output_power = true
        },
          max_block_size
        );
    }

    void set_gr_filter(double sample_rate_arg)
    {
        optional<double> attack_time_samples;
        if (pars.gr_filter.attack_time_sec) {
            attack_time_samples = *pars.gr_filter.attack_time_sec * sample_rate_arg;
        }
        const auto release_time_samples = pars.gr_filter.release_time_sec * sample_rate_arg;
        gr_filter = EnvelopeFilter<double>(
          EnvelopeFilterOutputType::lowpass, pars.gr_filter.order, attack_time_samples, release_time_samples
        );
    }

    void prepare_to_play(double sr, int maxBlockSize, UNUSED int num_channels) override
    {
        prepared_to_play = PreparedToPlay{
          .sample_rate = sr,
          .max_block_size = sucast(maxBlockSize),
          .input_level_multiband = make_input_level_multiband(sr, sucast(maxBlockSize))
        };

        sidechain_buf.reserve(prepared_to_play->max_block_size);

        set_input_level_lpf(prepared_to_play->sample_rate);
        transfer_curve.set(pars.transfer_curve);
        set_gr_filter(prepared_to_play->sample_rate);

        // Initialize RMS sample counter.
        rms_sample_counter = 0;
        rms_sample_counter_period = std::max(1, iround<int>(prepared_to_play->sample_rate * k_rms_sample_period_sec));
        rms_samples.clear();
        const auto max_rms_samples_in_block =
          sucast((maxBlockSize + rms_sample_counter_period - 1) / rms_sample_counter_period);
        rms_samples.reserve(max_rms_samples_in_block);
        input_rms = RmsDetector(
          RmsDetector::Flavor::exponential_moving_average,
          hz_fs_to_hps(k_rms_detector_hz, prepared_to_play->sample_rate)
        );
        output_rms = input_rms;
    }

    void release_resources() override
    {
        prepared_to_play = nullopt;
        rms_samples = vector<AF2>();
        sidechain_buf = vector<double>();
    }

    void debug_verify_samples(UNUSED span<const double> samples) const
    {
#ifndef NDEBUG
        if (debug_mode) {
            for (auto x : samples) {
                assert(is_normal_or_zero(x) && std::abs(x) <= 1.0);
            }
        }
#endif
    }

    void debug_verify_decibels(UNUSED span<const double> samples) const
    {
#ifndef NDEBUG
        if (debug_mode) {
            for (auto x : samples) {
                assert(is_normal_or_zero(x) && x <= 0.0);
            }
        }
#endif
    }

    void debug_verify_gain_values(UNUSED span<const double> samples) const
    {
#ifndef NDEBUG
        if (debug_mode) {
            for (auto x : samples) {
                assert(is_normal_or_zero(x) && 0 < x && x <= 1.0);
            }
        }
#endif
    }

    void process_block(std::span<float* const> channel_data, int num_samples) override
    {
#ifndef NDEBUG
        if (debug_mode) {
            for (auto chd : channel_data) {
                for (int i = 0; i < num_samples; i++) {
                    const float x = chd[i];
                    assert(is_normal_or_zero(x) && std::abs(x) <= 1.0f);
                }
            }
        }
#endif
        if (!prepared_to_play) {
            assert(false);
            return;
        }

        if (channel_data.empty() || num_samples <= 0) {
            return;
        }

        // Lambda for measuring the RMS of the average of the channels in channel_data_ and write it
        // into rms_samples[][rms_channel_ix]. It will be called twice, before and after the actual processing.
        // The rms_samples vector must already be resized to the exact number of rms samples that will be written into
        // it.
        const auto measure_rms =
          [num_samples, period = rms_sample_counter_period, &rms_samples_ = rms_samples](
            span<float* const> channel_data_, RmsDetector& rms_detector, unsigned rms_channel_ix, int sample_counter
          ) -> int {
            const auto channel_data_size = ifcast<float>(channel_data_.size());
            unsigned rms_sample_ix = 0;
            for (int i = 0; i < num_samples; ++i) {
                float sum = 0;
                for (auto* chd : channel_data_) {
                    sum += chd[i];
                }
                rms_detector.process(sum / channel_data_size);
                if (++sample_counter >= period) {
                    assert(rms_sample_ix < rms_samples_.size());
                    rms_samples_[rms_sample_ix++][rms_channel_ix] = rms_detector.get_rms();
                    sample_counter = 0;
                }
            }
            assert(rms_sample_ix == rms_samples_.size());
            return sample_counter;
        };

        // Create sidechain data from the sum of inputs.
        sidechain_buf.resize(sucast(num_samples));
        std::copy_n(channel_data[0], num_samples, sidechain_buf.data());
        for (unsigned i = 1; i < channel_data.size(); ++i) {
            range_plus_equals(sidechain_buf, span(channel_data[i], sucast(num_samples)));
        }
        range_divide_equals(sidechain_buf, ifcast<double>(channel_data.size()));

        // TODO: once we have an actual, separate sidechain we need to rethink what to
        // measure instead of input and output RMS.

        // Now measure input sum of channel data RMS. We're summing again, just to keep this separate from the
        // sidechain summing.
        rms_samples.resize(sucast((rms_sample_counter + num_samples) / rms_sample_counter_period));
        measure_rms(channel_data, input_rms, 0, rms_sample_counter);

        // sidechain_buf is ready to be processed into the gain signal.

        // First, convert it to an input level and apply the transfer curve.
        switch (pars.input_level_method) {
        case EnginePars::InputLevelMethod::direct:
            debug_verify_samples(sidechain_buf);
            for (auto& s : sidechain_buf) {
                s = transfer_curve.gain_db_for_input_db_without_makeup(matlab::mag2db(std::abs(s)));
            }
            break;
        case EnginePars::InputLevelMethod::lpf:
            input_level_lpf.process(sidechain_buf);
            debug_verify_samples(sidechain_buf);
            if (pars.input_level_lpf.rms) {
                for (auto& s : sidechain_buf) {
                    s = transfer_curve.gain_db_for_input_db_without_makeup(matlab::pow2db(s));
                }
            } else {
                for (auto& s : sidechain_buf) {
                    s = transfer_curve.gain_db_for_input_db_without_makeup(matlab::mag2db(s));
                }
            }
            break;
        case EnginePars::InputLevelMethod::multiband:
            prepared_to_play->input_level_multiband.process(sidechain_buf);
            debug_verify_samples(sidechain_buf);
            for (auto& s : sidechain_buf) {
                const auto s0 = transfer_curve.gain_db_for_input_db_without_makeup(matlab::pow2db(s));
                assert(s0 <= 0);
                s = std::min(s0, 0.0);
            }
            break;
        }

        debug_verify_decibels(sidechain_buf);

        // Apply GR-filtering.
        switch (pars.gr_filter_mode) {
        case EnginePars::GRFilterMode::off:
            for (auto& s : sidechain_buf) {
                const auto s0 = matlab::db2mag(s);
                assert(in_cc_range(s0, 0, 1));
                s = clamp(s0, 0.0, 1.0);
            }
            // At this point, sidechain_buf contains linear gain values.
            break;
        case EnginePars::GRFilterMode::mag:
            // Map the -INF .. 0 dB range to 1 .. 0 (reverse) because the 0 dB = 1.0 is the "no-signal" value.
            for (auto& s : sidechain_buf) {
                const auto s0 = 1 - matlab::db2mag(s);
                assert(in_cc_range(s0, 0, 1));
                s = clamp(s0, 0.0, 1.0);
            }
            // At this point, sidechain_buf contains linear gain values.
            // Note: gr filter should be set up for amplitude filtering.
            gr_filter.process(sidechain_buf);
            // Reverse back.
            for (auto& s : sidechain_buf) {
                const auto s0 = 1 - s;
                assert(in_cc_range(s0, 0, 1));
                s = clamp(s0, 0.0, 1.0);
            }
            // At this point, sidechain_buf contains linear gain values.
            break;
        case EnginePars::GRFilterMode::pow:
            // Map the -INF .. 0 dB range to 1 .. 0 (reverse) because the 0 dB = 1.0 is the "no-signal" value.
            for (auto& s : sidechain_buf) {
                const auto s0 = 1 - matlab::db2pow(s);
                assert(in_cc_range(s0, 0, 1));
                s = clamp(s0, 0.0, 1.0);
            }
            // Note: gr filter should be set up for amplitude filtering to prevent squaring the already squared
            // signal.
            gr_filter.process(sidechain_buf);
            // At this point, sidechain_buf contains squared gain values.
            // Reverse back.
            for (auto& s : sidechain_buf) {
                const auto s0 = 1 - s;
                assert(in_cc_range(s0, 0, 1));
                s = sqrt(clamp(s0, 0.0, 1.0));
            }
            // At this point, sidechain_buf contains linear gain values.
            break;
        case EnginePars::GRFilterMode::db:
            // Note: gr filter should be set up for amplitude filtering.
            // TODO: remove this
            {
                const auto before = sidechain_buf;
                gr_filter.process(sidechain_buf);
                // At this point, sidechain_buf contains dB gain values.
                for (auto& s : sidechain_buf) {
                    const auto s0 = matlab::db2mag(s);
                    assert(s0 <= 1);
                    s = std::min(s0, 1.0);
                }
            }
            // At this point, sidechain_buf contains linear gain values.
            break;
        }
        // At this point, sidechain_buf contains linear gain values.
        debug_verify_gain_values(sidechain_buf);

        // Apply linear gain to channel data.
        const auto make_up_gain_linear = matlab::db2mag(transfer_curve.get_makeup_gain_below_threshold_db());
        for (auto chd : channel_data) {
            for (unsigned i = 0; i < sucast(num_samples); ++i) {
                chd[i] = ffcast<float>(chd[i] * sidechain_buf[i] * make_up_gain_linear);
            }
        }

        // Measure output RMS.
        rms_sample_counter = measure_rms(channel_data, output_rms, 1, rms_sample_counter);
    }

    void
    process_block_with_trace(std::span<float* const> channel_data, int num_samples, std::vector<Trace>& trace) override
    {
        process_block(channel_data, num_samples);
        assert(cmp_equal(sidechain_buf.size(), num_samples));
        trace.clear();
        trace.reserve(sidechain_buf.size());
        for (auto x : sidechain_buf) {
            trace.push_back(Trace{.gain = x});
        }
    }

    SetParsResult set_pars(const EnginePars& pars_arg) override
    {
        // transfer_curve must always need to be set because it doesn't need sample_rate and does not heap-allocate. We
        // need it because it can be queried without processing (get_transfer_curve_state).
        bool transfer_curve_changed = pars.transfer_curve != pars_arg.transfer_curve;
        if (transfer_curve_changed) {
            pars.transfer_curve = pars_arg.transfer_curve;
            transfer_curve.set(pars.transfer_curve);
        }

        // We only need to actually recompute the internals if we're prepared to play and have a sample rate.
        if (prepared_to_play) {
            pars.input_level_method = pars_arg.input_level_method;
            switch (pars.input_level_method) {
            case EnginePars::InputLevelMethod::direct:
                break;
            case EnginePars::InputLevelMethod::lpf:
                if (pars.input_level_lpf != pars_arg.input_level_lpf) {
                    pars.input_level_lpf = pars_arg.input_level_lpf;
                    set_input_level_lpf(prepared_to_play->sample_rate);
                }
                break;
            case EnginePars::InputLevelMethod::multiband: {
                if (pars.input_level_multiband != pars_arg.input_level_multiband) {
                    pars.input_level_multiband = pars_arg.input_level_multiband;
                    // TODO: RecursiveCrossoverRMSDetector heap-allocates, while set_pars might be called on audio
                    // thread. Need to find a solution later.
                    prepared_to_play->input_level_multiband =
                      make_input_level_multiband(prepared_to_play->sample_rate, prepared_to_play->max_block_size);
                }
            } break;
            }

            // Set up GR filter.
            pars.gr_filter_mode = pars_arg.gr_filter_mode;
            switch (pars.gr_filter_mode) {
            case EnginePars::GRFilterMode::off:
                // Nothing to do.
                break;
            case EnginePars::GRFilterMode::mag:
            case EnginePars::GRFilterMode::pow:
            case EnginePars::GRFilterMode::db:
                if (pars.gr_filter != pars_arg.gr_filter) {
                    pars.gr_filter = pars_arg.gr_filter;
                    set_gr_filter(prepared_to_play->sample_rate);
                }
                break;
            }
        } else {
            // Everything will be updated in prepare_to_play when we do have the sample_rate.
            pars = pars_arg;
        }
        return transfer_curve_changed ? SetParsResult::transfer_curve_changed
                                      : SetParsResult::transfer_curve_didnt_change;
    }

    [[nodiscard]] TransferCurveState get_transfer_curve_state() const override
    {
        return transfer_curve.get_state();
    }
    [[nodiscard]] const std::vector<AF2>& get_rms_samples_of_last_block() const override
    {
        return rms_samples;
    }
    void reset() override
    {
        if (prepared_to_play) {
            prepared_to_play->input_level_multiband.reset();
        }
        input_level_lpf.reset();
        gr_filter.reset();
        input_rms.reset();
        output_rms.reset();
        rms_samples.clear();
    }

    void set_debug_mode(bool debug_mode_arg) override
    {
        debug_mode = debug_mode_arg;
    }
};

std::unique_ptr<Engine> make_engine()
{
    return std::make_unique<EngineImpl>();
}
