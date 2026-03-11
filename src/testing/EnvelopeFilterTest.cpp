#include "EnvelopeFilter.h"

#include <meadow/cppext.h>

#include <gtest/gtest.h>

TEST(EnvelopeFilter, T1)
{
    constexpr double fs = 48000;
    constexpr double release_freq_hz = 100.0;
    constexpr double attack_time_sec = 0.001;
    constexpr double attack_time_samples = attack_time_sec * fs;
    constexpr double release_freq_hps = release_freq_hz / fs * 2;
    constexpr double test_sine_freq_hz = 5000.0;
    constexpr double test_one_part_length_sec = 0.1;
    constexpr double test_amplitude = 2.0; // Need a number which changes when squared, so not 1.0.

    // The input test signal has three parts of equal lengths:
    // zeros, constant signal (either DC or sine) and zeros again.
    const auto N = iround<unsigned>(test_one_part_length_sec * fs);
    vector<float> step_up_and_down_signal(3 * N), sine_signal(3 * N);
    std::fill(
      step_up_and_down_signal.begin() + N, step_up_and_down_signal.begin() + 2 * N, ffcast<float>(test_amplitude)
    );
    for (unsigned i = 0; i < N; ++i) {
        sine_signal[N + i] = ffcast<float>(test_amplitude * sin(2.0 * num::pi * i * test_sine_freq_hz / fs));
    }
    FILE* f = fopen("/tmp/envelopefiltertest.m", "w");
    assert(f);
    println(f, "A = [");
    for (auto x : step_up_and_down_signal) {
        print(f, "{} ", x);
    }
    println(f);
    for (bool use_power : {true, false}) {
        for (int order : {1, 2}) {
            for (int asymmetric : {0, 1, 2}) {
                optional<double> attack_time_samples_arg;
                switch (asymmetric) {
                case 0:
                    // Not asymmetric.
                    break;
                case 1:
                    // Use asymmetric, but the attack and release times are identical.
                    attack_time_samples_arg = fs / (2.0 * num::pi * release_freq_hz);
                    break;
                case 2:
                    // Use asymmetric with markedly different attack time.
                    attack_time_samples_arg = attack_time_samples;
                    break;
                default:
                    assert(false);
                }
                {
                    auto ef = EnvelopeFilter(use_power, order, attack_time_samples_arg, release_freq_hps);
                    auto samples = step_up_and_down_signal;
                    ef.process(samples);
                    for (auto x : samples) {
                        print(f, "{} ", x);
                    }
                    println(f);
                    // TODO: verify that `samples` is the correct output.
                }
                {
                    auto ef = EnvelopeFilter(use_power, order, attack_time_samples_arg, release_freq_hps);
                    auto samples = sine_signal;
                    ef.process(samples);
                    // TODO: verify that `samples` is the correct output.
                }
            }
        }
    }
    println(f, "]';");
    fclose(f);
}
