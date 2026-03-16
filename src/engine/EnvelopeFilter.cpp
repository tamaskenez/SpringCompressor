#include "EnvelopeFilter.h"

#include "engine_util.h"

#include <meadow/math.h>

template<class IOFloat>
EnvelopeFilter<IOFloat>::EnvelopeFilter()
    : output_type(OutputType::amplitude)
{
}

template<class IOFloat>
EnvelopeFilter<IOFloat>::EnvelopeFilter(
  OutputType output_type_arg, int lpf_order, optional<double> attack_time_samples, double release_time_samples
)
    : output_type(output_type_arg)
{
    bool square_input = false;
    switch (output_type) {
    case EnvelopeFilterOutputType::amplitude:
    case EnvelopeFilterOutputType::lowpass:
        break;
    case EnvelopeFilterOutputType::rms:
    case EnvelopeFilterOutputType::power:
        square_input = true;
        break;
    }
    switch (lpf_order) {
    case 1:
        if (attack_time_samples) {
            filter = ExponentialMovingAverage{
              .release_coeff =
                exponential_moving_average_filter_coeff_from_cutoff_freq(samples_to_freq_hps(release_time_samples)),
              .attack_coeff =
                exponential_moving_average_filter_coeff_from_cutoff_freq(samples_to_freq_hps(*attack_time_samples))
            };
            if (square_input) {
                f_process = t_process<true, 1, true>;
            } else {
                f_process = t_process<false, 1, true>;
            }
        } else {
            filter = ExponentialMovingAverage{
              .release_coeff =
                exponential_moving_average_filter_coeff_from_cutoff_freq(samples_to_freq_hps(release_time_samples))
            };
            if (square_input) {
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
            spring.set_critically_damped_with_cutoff_freq(fs / *attack_time_samples, fs / release_time_samples);

            if (square_input) {
                f_process = t_process<true, 2, true>;
            } else {
                f_process = t_process<false, 2, true>;
            }
        } else {
            filter = Biquad_TDF2(critically_damped_second_order_lowpass(samples_to_freq_hps(release_time_samples)));
            if (square_input) {
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

template<class IOFloat>
void EnvelopeFilter<IOFloat>::process(span<IOFloat> samples)
{
    switch (output_type) {
    case EnvelopeFilterOutputType::amplitude:
        for (auto& x : samples) {
            x = std::abs(x);
        }
        break;
    case EnvelopeFilterOutputType::rms:
    case EnvelopeFilterOutputType::power:
    case EnvelopeFilterOutputType::lowpass:
        break;
    }
    if (f_process) {
        f_process(this, samples);
    } else {
        assert(false); // Calling process on default-initialized object.
    }
    switch (output_type) {
    case EnvelopeFilterOutputType::rms:
        for (auto& x : samples) {
            x = sqrt(x);
        }
        break;
    case EnvelopeFilterOutputType::amplitude:
    case EnvelopeFilterOutputType::power:
    case EnvelopeFilterOutputType::lowpass:
        break;
    }
}

template<class IOFloat>
template<bool t_square_input, int t_order, bool t_asymmetric>
void EnvelopeFilter<IOFloat>::t_process(EnvelopeFilter* that, span<IOFloat> samples)
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
        if constexpr (t_square_input) {
            xx = square(ffcast<double>(x));
        } else {
            xx = ffcast<double>(x);
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

        x = ffcast<IOFloat>(y);
    }
}

template<class IOFloat>
void EnvelopeFilter<IOFloat>::reset()
{
    std::visit(
      overloaded{
        [](ExponentialMovingAverage& ema) {
            ema.state = 0.0;
        },
        [](Biquad_TDF2& biquad) {
            biquad.reset();
        },
        [](SpringLowPass& spring_low_pass) {
            spring_low_pass.reset();
        }
      },
      filter
    );
}

template class EnvelopeFilter<float>;
template class EnvelopeFilter<double>;
