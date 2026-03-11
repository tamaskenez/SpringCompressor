#pragma once

#include "Biquad_TD2.h"
#include "SpringLowPass.h"

#include <optional>
#include <span>
#include <variant>

// Low-pass filter for calculating the envelope of input samples.
// It provides a number of algorithms in one class:
// - can compute RMS (filter the squared input and take square root) or amplitude (absolute value of input samples)
// - it can use a first order filter (exponential moving average) or a second order critically damped filter as a
//   low-pass filter
// - it can use a regular, linear filter or an asymmetric one (different behavior for attack and release)

class EnvelopeFilter
{
public:
    EnvelopeFilter(bool use_power, int lpf_order, std::optional<double> attack_time_samples, double release_freq_hps);

    // Can't be copied or moved since the type erased function pointer contains a function (closure) with a reference to
    // `this`.
    EnvelopeFilter(const EnvelopeFilter&) = delete;
    EnvelopeFilter(EnvelopeFilter&&) = delete;

    // Take the input samples and write back the envelope values in place.
    // The envelope values will be either RMS values (if use_power = true) or amplitude values (use_power = false)
    void process(std::span<float> samples);

private:
    // Function pointer for the type-erased templated process function.
    void (*f_process)(EnvelopeFilter* that, std::span<float> samples);

    struct ExponentialMovingAverage {
        double release_coeff, attack_coeff = NAN, state = 0.0;
    };
    std::variant<ExponentialMovingAverage, Biquad_TDF2, SpringLowPass> filter;

    template<bool t_use_power, int t_order, bool t_asymmetric>
    static void t_process(EnvelopeFilter* that, std::span<float> samples);
};
