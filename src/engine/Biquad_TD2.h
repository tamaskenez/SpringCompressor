#pragma once

#include <span>

// Digital biquad filter in transposed direct form II.
class Biquad_TDF2
{
public:
    // Initialize the filter with direct-form IIR coefficients [b, a].
    // a[0] need not be 1; coefficients are normalised internally.
    Biquad_TDF2(std::span<const double, 3> b, std::span<const double, 3> a)
    {
        const double inv_a0 = 1.0 / a[0];
        b0 = b[0] * inv_a0;
        b1 = b[1] * inv_a0;
        b2 = b[2] * inv_a0;
        a1 = a[1] * inv_a0;
        a2 = a[2] * inv_a0;
    }

    double process(double x)
    {
        const double y = b0 * x + s1;
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        return y;
    }

private:
    double b0 = 0.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;
    double s1 = 0.0, s2 = 0.0;
};
