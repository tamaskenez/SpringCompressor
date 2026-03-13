#include "RmsDetector.h"

#include "meadow/cppext.h"
#include "meadow/math.h"

#include <cmath>

RmsDetector::RmsDetector(Flavor f, double freq_hps)
{
    switch (f) {
    case Flavor::exponential_moving_average:
        filter = exponential_moving_average_filter_coeff_from_cutoff_freq(freq_hps);
        break;
    case Flavor::second_order_critically_damped:
        filter = Biquad_TDF2(critically_damped_second_order_lowpass(freq_hps));
        break;
    }
}

float RmsDetector::process(float f)
{
    switch (filter.index()) {
    case 0:
        mean_square = std::lerp(square(ffcast<double>(f)), mean_square, std::get<double>(filter));
        break;
    case 1:
        mean_square = std::get<Biquad_TDF2>(filter).process(f);
        break;
    default:
        assert(false);
    }
    return ffcast<float>(sqrt(mean_square));
}

float RmsDetector::get_rms() const
{
    return ffcast<float>(std::sqrt(mean_square));
}
