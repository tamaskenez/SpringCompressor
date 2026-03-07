#include "FilterBankBandPass2.h"

#include "meadow/math.h"
#include <meadow/matlab_signal.h>

#include <cassert>
#include <cmath>

FilterBankBandPass2::FilterBankBandPass2(double freq_lo, double freq_hi, double filters_per_octave, double B)
{
    const double half_bw_octaves = B / (2.0 * filters_per_octave);
    for (int k = 0;; ++k) {
        const double fc = freq_lo * std::pow(2.0, ifcast<double>(k) / filters_per_octave);
        if (fc > freq_hi) {
            break;
        }
        const double lo = fc * std::pow(2.0, -half_bw_octaves);
        const double hi = fc * std::pow(2.0, +half_bw_octaves);
        if (hi >= 1) {
            break;
        }
        filters.emplace_back(matlab::butter(1, matlab::FilterType::BandPass{lo, hi}));
    }
#if 0
    // Establish a normalization value for output power.
    double output_power = process_and_get_total_power(1.0);
    for (;;) {
        const double o = process_and_get_total_power(0.0);
        if (o < 1e-17) {
            break;
        }
        output_power += o;
    }
    output_power_normalizer = 1.0 / output_power;
#endif
}

void FilterBankBandPass2::process(double input_sample, span<double> output_samples)
{
    assert(output_samples.size() == filters.size());
    for (size_t i = 0; i < filters.size(); ++i) {
        output_samples[i] = filters[i].process(input_sample);
    }
}

double FilterBankBandPass2::process_and_get_total_power(double input_sample)
{
    double power = 0.0;
    for (auto& f : filters) {
        power += square(f.process(input_sample));
    }
    return power * output_power_normalizer;
}

void FilterBankBandPass2::reset()
{
    for (auto& f : filters) {
        f.reset();
    }
}
