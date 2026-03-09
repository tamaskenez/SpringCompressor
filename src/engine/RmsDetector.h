#pragma once

#include "FilterBankBandPass2.h"
#include "FirstOrderIIR_TD2.h"
#include "SpringLowPass.h"

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
