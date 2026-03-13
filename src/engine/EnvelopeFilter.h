#pragma once

#include "Biquad_TD2.h"
#include "SpringLowPass.h"

#include <optional>
#include <span>
#include <variant>

// Low-pass filter for calculating the envelope of input samples.
// It provides a number of algorithms in one class:
// - can compute RMS/power (filter the squared input and optionally take square root) or
//   amplitude (absolute value of input samples)
// - it can use a first order filter (exponential moving average) or a second order critically damped filter as a
//   low-pass filter
// - it can use a regular, linear filter or an asymmetric one (different behavior for attack and release)

enum class EnvelopeFilterOutputType {
    amplitude, // Calculate the filtered absolute value of input samples.
    rms,       // Calculate the square root of the filtered squared value of input samples.
    power      // Calculate the filtered squared value of input samples.
};

template<class IOFloat>
class EnvelopeFilter
{
public:
    using OutputType = EnvelopeFilterOutputType;

    // The default constructor is provided to allow setting up later (by overwriting the previous instance).
    // However, it's not meant to be used that way, in Debug build an `assert(false)` will be hit in `process).
    // In release build `process` will be a no-op (bypass).
    EnvelopeFilter();
    EnvelopeFilter(
      OutputType output_type_arg, int lpf_order, std::optional<double> attack_time_samples, double release_time_samples
    );

    // Take the input samples and write back the envelope values in place.
    // The envelope values will be either RMS values (if use_power = true) or amplitude values (use_power = false)
    void process(std::span<IOFloat> samples);

private:
    OutputType output_type;
    // Function pointer for the type-erased templated process function.
    void (*f_process)(EnvelopeFilter* that, std::span<IOFloat> samples, bool take_sqrt) = nullptr;

    struct ExponentialMovingAverage {
        double release_coeff, attack_coeff = NAN, state = 0.0;
    };
    std::variant<ExponentialMovingAverage, Biquad_TDF2, SpringLowPass> filter;

    template<bool t_use_power, int t_order, bool t_asymmetric>
    static void t_process(EnvelopeFilter* that, std::span<IOFloat> samples, bool take_sqrt);
};
