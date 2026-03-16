#include "FilterBankBandPass2.h"

#include "meadow/math.h"
#include <meadow/matlab_signal.h>

#include <cassert>
#include <cmath>
#include <complex>

FilterBankBandPass2::FilterBankBandPass2(double freq_lo_arg, double freq_hi, double filters_per_octave_arg, double Q)
    : freq_lo(freq_lo_arg)
    , filters_per_octave(filters_per_octave_arg)
{
    const auto beta = (1 + sqrt(1 + 4 * square(Q))) / (2 * Q);
    UNUSED const auto max_fc = std::min(1 / beta, freq_hi);
    assert(freq_lo <= max_fc);
    const auto num_filters = ifloor<size_t>(filters_per_octave * log2(freq_hi / freq_lo)) + 1;
    filters.reserve(num_filters);
    // Normalize with the central filter.
    const double fmiddle = center_freq(num_filters / 2);

    double total_power = 0; // Simulate a sine of freq fmiddle going through all filters.

    for (unsigned k = 0; k < num_filters; ++k) {
        const double fc = center_freq(k);
        assert(fc <= freq_hi);
        const double hi = fc * beta;
        assert(hi <= 1);
        const auto coeffs = matlab::butter(1, matlab::FilterType::BandPass{fc / beta, hi});
        filters.emplace_back(coeffs);
        total_power += norm(matlab::freqz(coeffs.b, coeffs.a, fmiddle * num::pi));
    }
    sqrt_output_power_normalizer = sqrt(1.0 / total_power);
}

double FilterBankBandPass2::center_freq(size_t i) const
{
    return freq_lo * std::pow(2.0, ifcast<double>(i) / filters_per_octave);
}

void FilterBankBandPass2::process(double input_sample, span<double> output_samples)
{
    assert(output_samples.size() == filters.size());
    for (size_t i = 0; i < filters.size(); ++i) {
        output_samples[i] = sqrt_output_power_normalizer * filters[i].process(input_sample);
    }
}

void FilterBankBandPass2::reset()
{
    for (auto& f : filters) {
        f.reset();
    }
}
