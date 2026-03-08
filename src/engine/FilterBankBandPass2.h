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
    // freq_hi.
    FilterBankBandPass2(double freq_lo, double freq_hi, double filters_per_octave, double Q = 1.0);

    // Return the center frequency of filter `i`.
    double center_freq(size_t i) const;

    // output_samples.size() is expected to be num_filters()
    void process(double input_sample, span<double> output_samples);

    size_t num_filters() const
    {
        return filters.size();
    }
    void reset(); // Set all filter states to zero.

private:
    double freq_lo;
    double filters_per_octave;
    std::vector<Biquad_TDF2> filters;
    double sqrt_output_power_normalizer = 1.0;
};
