#pragma once
#include "meadow/matlab_signal.h"

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
