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

struct FilterBankRmsDetectorConfig {
    struct FilterBank {
        int half_order;
        double freq_lo_hz;
        double freq_hi_hz;
        double filters_per_octave;
        double Q;
    } filter_bank;
    struct LPF {
        int order;
        double K;
        optional<double> attack_time_constant; // When it's set, order is ignored, SpringLowPass will be used.
    } lpf;
};

class FilterBankRmsDetector
{
public:
    FilterBankRmsDetector(double fs, const FilterBankRmsDetectorConfig& config_arg);
    void process(span<double> samples);

private:
    FilterBankRmsDetectorConfig config;
    FilterBankBandPass2 fb;
    struct LP {
        std::vector<FirstOrderIIR_TDF2> first;
        std::vector<Biquad_TDF2> second;
        std::vector<SpringLowPass> spring;
    } lp;
    vector<double> filter_bank_output_samples;
};
