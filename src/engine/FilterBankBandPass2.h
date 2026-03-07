#pragma once

#include "Biquad_TD2.h"
#include "meadow/cppext.h"

#include <vector>

// A filter bank of 2nd order bandpass filters.
class FilterBankBandPass2
{
public:
    // Initialize the filter bank with 2nd-order butterworth band pass filters.
    // freq_lo and freq_hi are normalized to the Nyquist frequency (MATLAB convention)
    // The first filter is centered on freq_lo. The last filter will be centered on a frequency less or equal to
    // freq_hi. By default (when B = 0.0) the filters' cutoff frequencies are arranged such that one filter high cutoff
    // is placed
    // when the next filter's low cutoff is. If B != 0.0 the low and high cutoff frequencies are
    // multiplied by 2^(-B/2) and 2^(B/2), respectively.
    FilterBankBandPass2(double freq_lo, double freq_hi, double filters_per_octave, double B = 0.0);

    // output_samples.size() is expected to be num_filters()
    void process(double input_sample, span<double> output_samples);
    double process_and_get_total_power(double input_sample);

    size_t num_filters() const
    {
        return filters.size();
    }
    void reset(); // Set all filter states to zero.

private:
    std::vector<Biquad_TDF2> filters;
    double output_power_normalizer = 1.0;
};
