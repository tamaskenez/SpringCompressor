#include "engine_util.h"

#include "meadow/math.h"
#include "meadow/matlab.h"

#include <meadow/cppext.h>

#include <cmath>

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

AnalysePeriodicSignalHarmonicsResult analyse_periodic_signal_harmonics(std::span<const double> signal, int num_periods)
{
    const int N = iicast<int>(signal.size());
    assert(N > 0 && num_periods > 0 && num_periods < N);
    const double N_d = ifcast<double>(N);

    // Compute |X[k]|² for a single DFT bin k using argument reduction.
    // X[k] = sum_n x[n] * exp(-2πi*k*n/N)
    // (k*n) mod N is computed exactly in integer arithmetic, keeping the trig argument
    // in [0, 2π] regardless of n, which avoids large-argument precision loss entirely.
    const double two_pi_over_N = 2.0 * num::pi / N_d;
    auto dft_bin_power = [&](int k) -> double {
        double re = 0, im = 0;
        for (int n = 0; n < N; ++n) {
            const auto kn_mod_N = ifcast<double>((static_cast<int64_t>(k) * n) % N);
            const double angle = -two_pi_over_N * kn_mod_N;
            re += signal[sucast(n)] * cos(angle);
            im += signal[sucast(n)] * sin(angle);
        }
        return re * re + im * im;
    };

    // Spectral symmetry factors for a real signal:
    //   DC (k=0):           reported = parseval = 1.0  — single bin, no mirror
    //   0 < k < N/2:        reported = parseval = 2.0  — conjugate-mirror pair ±k
    //   Nyquist (k=N/2):    parseval = 1.0 (discrete-time truth)
    //                       reported = 0.5 (-Nyquist aliases onto +Nyquist, doubling
    //                                  X[N/2]; halving recovers the continuous-time A²/2)
    const bool has_nyquist_bin = is_even(N);
    auto parseval_factor = [&](int k) -> double {
        return (k == 0 || (has_nyquist_bin && k == N / 2)) ? 1.0 : 2.0;
    };
    auto reported_factor = [&](int k) -> double {
        return (has_nyquist_bin && k == N / 2) ? 0.5 : parseval_factor(k);
    };

    // Partition all bins [0..N/2] into DC, fundamental, harmonics, and rest.
    // rest_power is accumulated as a direct sum of non-special bin powers rather than
    // computed as (total − f0 − …), which would suffer catastrophic cancellation when
    // most energy is in the fundamental.
    double dc_power = 0, f0_power = 0, harmonics_power = 0, rest_power = 0;
    for (int k = 0; k <= N / 2; k++) {
        const double bp = dft_bin_power(k) / (N_d * N_d);
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
    const double wc = 2 * num::pi * freq_hps;
    const double wn = wc / sqrt(num::sqrt2 - 1); // This line provides normalizing to -3 dB instead of -6 dB
    const std::array<double, 1> b = {square(wn)};
    const std::array<double, 3> a = {1, 2 * wn, square(wn)};
    return matlab::bilinear(b, a, 2, freq_hps);
}
