#include "RmsDetector.h"

#include "meadow/cppext.h"
#include "meadow/math.h"

#include <cmath>

RmsDetector::RmsDetector(Flavor f, double freq_hps)
{
    switch (f) {
    case Flavor::exponential_moving_average:
        filter = moving_average_filter_coeff(freq_hps);
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

FilterBankRmsDetector::FilterBankRmsDetector(double fs, const FilterBankRmsDetectorConfig& config_arg)
    : config(config_arg)
    , fb(
        config.filter_bank.freq_lo_hz / fs * 2,
        config.filter_bank.freq_hi_hz / fs * 2,
        config.filter_bank.filters_per_octave,
        config.filter_bank.Q
      )
    , filter_bank_output_samples(fb.num_filters())
{
    if (auto attack_time_constant = config.lpf.attack_time_constant) {
        lp.spring.reserve(fb.num_filters());
        for (unsigned j = 0; j < fb.num_filters(); ++j) {
            lp.spring.emplace_back(fs);
            const auto fc_hz = fb.center_freq(j) / 2 * fs;
            const auto tau_sec = 1 / (2 * num::pi * fc_hz);
            lp.spring.back().set_critically_damped_with_time_constant(*attack_time_constant, tau_sec * config.lpf.K);
        }
    } else {
        switch (config.lpf.order) {
        case 1:
            lp.first.reserve(fb.num_filters());
            for (unsigned j = 0; j < fb.num_filters(); ++j) {
                const auto fc = fb.center_freq(j);
                lp.first.emplace_back(matlab::butter(1, matlab::FilterType::LowPass{fc / config.lpf.K}));
            }
            break;
        case 2:
            lp.second.reserve(fb.num_filters());
            for (unsigned j = 0; j < fb.num_filters(); ++j) {
                const auto fc = fb.center_freq(j);
                lp.second.emplace_back(matlab::butter(2, matlab::FilterType::LowPass{fc / config.lpf.K}));
            }
            break;
        default:
            assert(false);
        }
    }
}

void FilterBankRmsDetector::process(span<double> samples)
{
    const auto NF = fb.num_filters();
    for (auto& s : samples) {
        fb.process(s, filter_bank_output_samples);
        double sample_power = 0;
        if (lp.first.empty()) {
            assert(lp.first.size() == NF);
            for (unsigned j = 0; j < NF; ++j) {
                sample_power += lp.first[j].process(square(filter_bank_output_samples[j]));
            }
        } else if (!lp.second.empty()) {
            assert(lp.second.size() == NF);
            for (unsigned j = 0; j < NF; ++j) {
                sample_power += lp.second[j].process(square(filter_bank_output_samples[j]));
            }
        } else {
            assert(lp.spring.size() == NF);
            for (unsigned j = 0; j < NF; ++j) {
                sample_power += lp.spring[j].process(square(filter_bank_output_samples[j]));
            }
        }
        s = std::sqrt(sample_power);
    }
}
