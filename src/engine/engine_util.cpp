#include "engine_util.h"

#include "meadow/math.h"
#include "meadow/matlab.h"

#include <meadow/cppext.h>

double exponential_moving_average_filter_coeff_from_cutoff_freq(double freq_hps)
{
    // The usual exp(-num::pi * freq_hps) formula gives the same result for
    // lower frequencies but not exact at higher ones.
    const auto c = cos(num::pi * freq_hps);
    return 2 - c - sqrt(square(c - 2) - 1);
}

double exponential_moving_average_filter_coeff_from_time_constant(double time_constant_samples)
{
    return exp(-1.0 / time_constant_samples);
}

ExponentialMovingAverageFilter::ExponentialMovingAverageFilter(double freq_hps)
    : coeff(exponential_moving_average_filter_coeff_from_cutoff_freq(freq_hps))
{
}

matlab::TransferFunctionCoeffs exponential_moving_average_filter(double freq_hps)
{
    const auto coeff = exponential_moving_average_filter_coeff_from_cutoff_freq(freq_hps);
    return matlab::TransferFunctionCoeffs{.b = vector<double>({1 - coeff}), .a = vector<double>({1, -coeff})};
}

matlab::TransferFunctionCoeffs critically_damped_second_order_lowpass(double freq_hps)
{
    const double wc = 2 * num::pi * freq_hps;
    const double wn = wc / sqrt(num::sqrt2 - 1); // This line provides normalizing to -3 dB instead of -6 dB
    const std::array<double, 1> b = {square(wn)};
    const std::array<double, 3> a = {1, 2 * wn, square(wn)};
    return matlab::bilinear(b, a, 2, freq_hps);
}
