#include "RecursiveCrossoverRMSDetector.h"

#include <gtest/gtest.h>

TEST(RecursiveCrossoverRMSDetector, T1)
{
    constexpr double fs = 48000.0;
    constexpr int lpf_order = 1;
    constexpr int bpf_order = 2;
    constexpr double crossovers_per_octaves = 1.0;
    constexpr double freq_lo = 20.0;
    constexpr double freq_hi = 12000.0;
    const optional<double> attack_time_samples;
    constexpr double release_ratio = 1;

    constexpr double test_sine_freq_hz = 5000.0;
    constexpr double test_one_part_length_sec = 0.1;
    constexpr double test_amplitude = 2.0; // Need a number which changes when squared, so not 1.0.

    const auto lpf = RecursiveCrossoverRMSDetectorPars::LowPassFilter{
      .order = lpf_order, .attack_time_samples = attack_time_samples, .release_ratio_to_period = release_ratio
    };

    const auto pars = RecursiveCrossoverRMSDetectorPars{
      .freq_lo_hps = 2 * freq_lo / fs,
      .freq_hi_hps = 2 * freq_hi / fs,
      .crossovers_per_octaves = crossovers_per_octaves,
      .bpf_order = bpf_order,
      .low_pass_filter = lpf,
      .min_release_time_samples = 0
    };

    // The input test signal has three parts of equal lengths:
    // zeros, constant signal (sine) and zeros again.
    const auto N = iround<unsigned>(test_one_part_length_sec * fs);
    vector<float> sine_signal(3 * N);
    for (unsigned i = 0; i < N; ++i) {
        sine_signal[N + i] = ffcast<float>(test_amplitude * sin(2.0 * num::pi * i * test_sine_freq_hz / fs));
    }

    const auto max_block_size = sine_signal.size();
    auto d = RecursiveCrossoverRMSDetector<float>(pars, max_block_size);

    auto sine_signal_output = sine_signal;
    d.process(sine_signal_output);

    FILE* f = fopen("/tmp/rcrmsdetector_test.m", "w");

    println(f, "A=[");
    for (auto x : sine_signal) {
        print(f, " {}", x);
    }
    println(f);
    for (auto x : sine_signal_output) {
        print(f, " {}", x);
    }
    println(f);
    println(f, "]';");

    vector<float> sine_sweep;
    const unsigned M = iround<unsigned>(fs * 4);
    double omega = 0;
    for (unsigned i = 0; i < M; ++i) {
        sine_sweep.push_back(ffcast<float>(test_amplitude * sin(omega)));
        const auto freq = exp(std::lerp(log(freq_lo), log(freq_hi), ifcast<double>(i) / M));
        const auto d_omega = 2.0 * num::pi * freq / fs;
        omega += d_omega;
        if (omega >= 2.0 * num::pi) {
            omega -= 2.0 * num::pi;
        }
    }
    auto sine_sweep_output = sine_sweep;
    for (size_t i = 0; i < sine_sweep.size(); i += max_block_size) {
        d.process(span(sine_sweep_output).subspan(i, std::min(sine_sweep.size() - i, max_block_size)));
    }

    println(f, "B=[");
    for (auto x : sine_sweep) {
        print(f, " {}", x);
    }
    println(f);
    for (auto x : sine_sweep_output) {
        print(f, " {}", x);
    }
    println(f);
    println(f, "]';");

    fclose(f);
}
