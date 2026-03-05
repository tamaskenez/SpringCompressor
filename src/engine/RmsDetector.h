#pragma once

// Exponential moving average of the squared input samples.
// The time constant controls how quickly the average responds to changes.
class RmsDetector
{
public:
    RmsDetector() = default;
    RmsDetector(double sample_rate, double time_constant_sec);

    void process(float f);

    [[nodiscard]] float get_rms() const;

private:
    double coeff = 0.0;
    double mean_square = 0.0;
};
