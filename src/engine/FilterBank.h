#pragma once

#include "Biquad_TD2.h"

#include <vector>

// A filter bank of 2nd order bandpass filters.
class FilterBank2
{
public:
    FilterBank2(double freq_lo, double freq_hi, double filters_per_octave);

private:
    std::vector<Biquad_TDF2> filters;
};
