#include "RmsDetector.h"

#include "meadow/cppext.h"
#include "meadow/math.h"

#include <cmath>

RmsDetector::RmsDetector(double sample_rate, double time_constant_sec)
    : coeff(std::exp(-1.0 / (sample_rate * time_constant_sec)))
{
}

void RmsDetector::process(float f)
{
    mean_square = std::lerp(square(ffcast<double>(f)), mean_square, coeff);
}

float RmsDetector::get_rms() const
{
    return ffcast<float>(std::sqrt(mean_square));
}
