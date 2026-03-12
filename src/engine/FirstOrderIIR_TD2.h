#pragma once

#include "meadow/matlab_signal.h"

#include <span>

// Digital first-order filter in transposed direct form II.
class FirstOrderIIR_TDF2
{
public:
    // Initialize filter for bypass.
    FirstOrderIIR_TDF2() = default;

    // Initialize the filter with direct-form IIR coefficients [b, a].
    // a[0] need not be 1; coefficients are normalised internally.
    FirstOrderIIR_TDF2(std::span<const double, 2> b, std::span<const double, 2> a)
    {
        const double inv_a0 = 1.0 / a[0];
        b0 = b[0] * inv_a0;
        b1 = b[1] * inv_a0;
        a1 = a[1] * inv_a0;
    }
    explicit FirstOrderIIR_TDF2(const matlab::TransferFunctionCoeffs&& coeffs)
        : FirstOrderIIR_TDF2(coeffs.b_as_static_span<2>(), coeffs.a_as_static_span<2>())
    {
    }

    double process(double x)
    {
        const double y = b0 * x + s1;
        s1 = b1 * x - a1 * y;
        return y;
    }
    void process(std::span<double> xs)
    {
        for (auto& x : xs) {
            x = process(x);
        }
    }

    void reset()
    {
        s1 = 0.0;
    }

private:
    double b0 = 1.0, b1 = 0.0;
    double a1 = 0.0;
    double s1 = 0.0;
};
