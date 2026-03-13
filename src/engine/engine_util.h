#pragma once
#include "meadow/matlab_signal.h"

#include <numbers>

class ExponentialMovingAverageFilter
{
public:
    ExponentialMovingAverageFilter() = default;

    // `freq_hps` is the -3 dB cutoff frequency, in half-cycles-per-sample (MATLAB convention, Nyquist-normalized)
    explicit ExponentialMovingAverageFilter(double freq_hps);
    double process(double x)
    {
        s1 = coeff * s1 + (1.0 - coeff) * x;
        return s1;
    }

private:
    double coeff = 0.0;
    double s1 = 0.0;
};

// Return the coefficient of the previous output, while the coefficient of the current input sample is one minus this:
// y[n] = lerp(x[n], y[n-1], coeff);
double exponential_moving_average_filter_coeff_from_cutoff_freq(double freq_hps);
double exponential_moving_average_filter_coeff_from_time_constant(double tau_samples);

// `freq_hps` is the -3 dB cutoff frequency, in half-cycles-per-sample (MATLAB convention, Nyquist-normalized)
matlab::TransferFunctionCoeffs exponential_moving_average_filter(double freq_hps);

// Return the discrete transfer function for a critically damped second order filter.
// `freq_hps` is the -3 dB cutoff frequency, in half-cycles-per-sample (MATLAB convention, Nyquist-normalized)
// Note that this filter is usually normalized to -6 dB at cutoff frequency, which is not the case here.
matlab::TransferFunctionCoeffs critically_damped_second_order_lowpass(double freq_hps);

// There's a wave, whose frequency is given in half-cycles-per-sample.
// Return the length of one period of this wave, in samples.
inline double freq_hps_to_samples(double freq_hps)
{
    return 2.0 / freq_hps;
}

// There's a wave, whose length of period is given in samples.
// Return the frequency of this wave, in half-cycles-per-samples.
inline double samples_to_freq_hps(double samples)
{
    return 2.0 / samples;
}

inline double hz_fs_to_hps(double hz, double fs)
{
    return hz / fs * 2;
}

inline double time_constant_sec_to_cutoff_hz(double time_constant_sec)
{
    return 1.0 / (2.0 * std::numbers::pi * time_constant_sec);
}

inline double time_constant_samples_to_cutoff_hps(double time_constant_samples)
{
    return 1.0 / (std::numbers::pi * time_constant_samples);
}
