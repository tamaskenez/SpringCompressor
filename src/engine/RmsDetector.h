#pragma once

#include "FilterBankBandPass2.h"
#include "FirstOrderIIR_TD2.h"
#include "SpringLowPass.h"
#include "engine_util.h"

// Exponential moving average of the squared input samples.
// The time constant controls how quickly the average responds to changes.
class RmsDetector
{
public:
    enum class Flavor {
        exponential_moving_average,
        second_order_critically_damped
    };

    RmsDetector() = default;
    // freq_hps is the half-cycle-per-second (MATLAB convention), that is, normalized to Nyquist.
    RmsDetector(Flavor f, double freq_hps);

    float process(float f);

    [[nodiscard]] float get_rms() const; // Return the latest one.

private:
    double mean_square = 0.0;
    variant<double, Biquad_TDF2> filter = 0.0;
};
