#include "engine_util.h"

#include "meadow/math.h"
#include "meadow/matlab.h"

#include <meadow/cppext.h>

#include <cmath>
#include <complex>

#include "pocketfft_hdronly.h"

double exponential_moving_average_filter_coeff_from_cutoff_freq(double freq_hps)
{
    assert(in_oo_range(freq_hps, 0, 1));
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

AnalysePeriodicSignalHarmonicsResult analyse_periodic_signal_harmonics(std::span<const double> signal, int num_periods)
{
    assert(!signal.empty());
    const int N = iicast<int>(signal.size());
    assert(0 < num_periods && 2 * num_periods <= N);
    const double N_d = ifcast<double>(N);

    // Compute the real-to-complex FFT: output has N/2+1 bins (0..N/2).
    const int num_bins = N / 2 + 1;
    vector<std::complex<double>> spectrum(sucast(num_bins));
    pocketfft::r2c(
      {sucast(N)},                                            // shape_in
      {iicast<std::ptrdiff_t>(sizeof(double))},               // stride_in (bytes)
      {iicast<std::ptrdiff_t>(sizeof(std::complex<double>))}, // stride_out (bytes)
      0,                                                      // axis
      true,                                                   // forward
      signal.data(),
      spectrum.data(),
      1.0 // fct (no normalization; matches our DFT convention)
    );

    // Spectral symmetry factors — same as the direct DFT implementation.
    const bool has_nyquist_bin = is_even(N);
    auto parseval_factor = [&](int k) -> double {
        return (k == 0 || (has_nyquist_bin && k == N / 2)) ? 1.0 : 2.0;
    };
    auto reported_factor = [&](int k) -> double {
        return (has_nyquist_bin && k == N / 2) ? 0.5 : parseval_factor(k);
    };

    // Partition bins [0..N/2] into DC, fundamental, harmonics, rest.
    double dc_power = 0, f0_power = 0, harmonics_power = 0, rest_power = 0;
    for (int k = 0; k < num_bins; k++) {
        const double bp = std::norm(spectrum[sucast(k)]) / (N_d * N_d);
        if (k == 0) {
            dc_power = parseval_factor(0) * bp;
        } else if (k == num_periods) {
            f0_power = reported_factor(k) * bp;
        } else if (k % num_periods == 0) {
            harmonics_power += reported_factor(k) * bp;
        } else {
            rest_power += parseval_factor(k) * bp;
        }
    }

    return AnalysePeriodicSignalHarmonicsResult{
      .dc_db = matlab::pow2db(dc_power),
      .f0_db = matlab::pow2db(f0_power),
      .harmonics_db = matlab::pow2db(harmonics_power),
      .rest_db = matlab::pow2db(rest_power)
    };
}

matlab::TransferFunctionCoeffs critically_damped_second_order_lowpass(double freq_hps)
{
    assert(in_oo_range(freq_hps, 0, 1));
    const double wc = 2 * num::pi * freq_hps;
    const double wn = wc / sqrt(num::sqrt2 - 1); // This line provides normalizing to -3 dB instead of -6 dB
    const std::array<double, 1> b = {square(wn)};
    const std::array<double, 3> a = {1, 2 * wn, square(wn)};
    return matlab::bilinear(b, a, 2, freq_hps);
}
