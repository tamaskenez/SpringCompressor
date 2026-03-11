#include "EnvelopeFilter.h"

#include "engine_util.h"

#include <meadow/math.h>

EnvelopeFilter::EnvelopeFilter(
  bool use_power, int lpf_order, optional<double> attack_time_samples, double release_freq_hps
)
{
    switch (lpf_order) {
    case 1:
        if (attack_time_samples) {
            filter = ExponentialMovingAverage{
              .release_coeff = exponential_moving_average_filter_coeff_from_cutoff_freq(release_freq_hps),
              .attack_coeff = exponential_moving_average_filter_coeff_from_time_constant(*attack_time_samples)
            };
            if (use_power) {
                f_process = t_process<true, 1, true>;
            } else {
                f_process = t_process<false, 1, true>;
            }
        } else {
            filter = ExponentialMovingAverage{
              .release_coeff = exponential_moving_average_filter_coeff_from_cutoff_freq(release_freq_hps)
            };
            if (use_power) {
                f_process = t_process<true, 1, false>;
            } else {
                f_process = t_process<false, 1, false>;
            }
        }
        break;
    case 2:
        if (attack_time_samples) {
            // SpringLowPass needs sampling rate and frequencies in Hz.
            // Here we only have normalized frequencies. If we pretend the sample rate is 2.0 Hz
            // we can use the normalized frequencies as if they were in Hz.
            constexpr double fs = 2.0;
            filter = SpringLowPass(fs);
            auto& spring = std::get<SpringLowPass>(filter);
            // We convert the release frequency to Hz with `freq_hz = freq_hps * 2 / fs` which is still
            // the same value. From the frequency we arrive to the time constant with the
            // `tau = 1 / (2 * pi * f)` formula.
            spring.set_critically_damped_with_time_constant(
              *attack_time_samples / fs, 1.0 / (2.0 * num::pi * release_freq_hps)
            );
            if (use_power) {
                f_process = t_process<true, 2, true>;
            } else {
                f_process = t_process<false, 2, true>;
            }
        } else {
            filter = Biquad_TDF2(critically_damped_second_order_lowpass(release_freq_hps));
            if (use_power) {
                f_process = t_process<true, 2, false>;
            } else {
                f_process = t_process<false, 2, false>;
            }
        }
        break;
    default:
        assert(false);
    }
}

void EnvelopeFilter::process(span<float> samples)
{
    f_process(this, samples);
}

template<bool t_use_power, int t_order, bool t_asymmetric>
void EnvelopeFilter::t_process(EnvelopeFilter* that, span<float> samples)
{
    // Extract the appropriate type from the variant.
    ExponentialMovingAverage* ema{};
    Biquad_TDF2* biquad{};
    SpringLowPass* spring{};

    if constexpr (t_order == 1) {
        ema = &std::get<ExponentialMovingAverage>(that->filter);
    } else if constexpr (t_order == 2) {
        if constexpr (t_asymmetric) {
            spring = &std::get<SpringLowPass>(that->filter);
        } else {
            biquad = &std::get<Biquad_TDF2>(that->filter);
        }
    } else {
        static_assert(false);
    }

    for (auto& x : samples) {
        // Apply abs or square on the input sample.
        double xx;
        if constexpr (t_use_power) {
            xx = square(x);
        } else {
            xx = std::abs(x);
        }

        // Apply the low-pass filter.
        double y;
        if constexpr (t_order == 1) {
            double coeff;
            if constexpr (t_asymmetric) {
                coeff = xx >= ema->state ? ema->attack_coeff : ema->release_coeff;
            } else {
                coeff = ema->release_coeff;
            }
            ema->state = std::lerp(xx, ema->state, coeff);
            y = ema->state;
        } else if constexpr (t_order == 2) {
            if constexpr (t_asymmetric) {
                y = spring->process(xx);
            } else {
                y = biquad->process(xx);
            }
        } else {
            static_assert(false);
        }

        // Apply sqrt if necessary on the output sample.
        if constexpr (t_use_power) {
            x = ffcast<float>(sqrt(y));
        } else {
            x = ffcast<float>(y);
        }
    }
}
