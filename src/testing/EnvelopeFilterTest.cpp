#include "EnvelopeFilter.h"

#include <meadow/cppext.h>
#include <meadow/math.h>

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

    // EMA release coefficient (same formula as engine_util.cpp).
    const double c_rel = cos(num::pi * release_freq_hps);
    const double coeff_release = 2.0 - c_rel - sqrt(square(c_rel - 2.0) - 1.0);
    const double tau_release = -1.0 / log(coeff_release); // samples

    // EMA attack coefficient for asymmetric case 2.
    const double coeff_attack = exp(-1.0 / attack_time_samples);
    // tau_attack == attack_time_samples by definition of coeff_attack.

    FILE* f = fopen("/tmp/envelopefiltertest.m", "w");
    assert(f);
    println(f, "A = [");
    for (auto x : step_up_and_down_signal) {
        print(f, "{} ", x);
    }
    println(f);
    string legend = "'step'";
    for (bool use_power : {true, false}) {
        for (int order : {1, 2}) {
            for (int asymmetric : {0, 1, 2}) {
                optional<double> attack_time_samples_arg;
                string label;
                switch (asymmetric) {
                case 0:
                    // Not asymmetric.
                    label = "symm.";
                    break;
                case 1:
                    // Use asymmetric, but the attack and release times are identical.
                    attack_time_samples_arg = fs / (2.0 * num::pi * release_freq_hz);
                    label = "assym-same";
                    break;
                case 2:
                    // Use asymmetric with markedly different attack time.
                    attack_time_samples_arg = attack_time_samples;
                    label = "assym-diff";
                    break;
                default:
                    assert(false);
                }
                label = format("'O{}/{}/{}'", order, label, use_power ? "pow" : "amp");
                legend += ", ";
                legend += label;
                {
                    auto ef = EnvelopeFilter<float>(
                      use_power ? EnvelopeFilterOutputType::rms : EnvelopeFilterOutputType::amplitude,
                      order,
                      attack_time_samples_arg,
                      release_freq_hps
                    );
                    auto samples = step_up_and_down_signal;
                    ef.process(samples);
                    for (auto x : samples) {
                        print(f, "{} ", x);
                    }
                    println(f);

                    // DC convergence: the middle segment is ~62 release time constants long, so the
                    // filter must be fully settled at test_amplitude at its end, and back at 0 at the
                    // end of the final segment.
                    EXPECT_NEAR(
                      samples[2 * N - 1], ffcast<float>(test_amplitude), ffcast<float>(test_amplitude * 1e-3)
                    );
                    EXPECT_NEAR(samples[3 * N - 1], 0.0f, ffcast<float>(test_amplitude * 1e-3));

                    if (order == 1 && asymmetric == 0) {
                        // For 1st-order EMA the step response is exact: y[n] = A_filter * (1 - coeff^n).
                        // The filter operates on A (amplitude mode) or A² (power mode), with sqrt on
                        // output in power mode.
                        const unsigned n_tau = iround<unsigned>(tau_release);
                        // samples[N + n_tau] is the output after processing n_tau+1 samples with
                        // input A, so the correct EMA decay is coeff^(n_tau+1).
                        const double decay = pow(coeff_release, n_tau + 1);

                        // Step up: output at n_tau samples after the rising edge.
                        const auto expected_step_up = ffcast<float>(
                          use_power ? test_amplitude * sqrt(1.0 - decay) : test_amplitude * (1.0 - decay)
                        );
                        EXPECT_NEAR(samples[N + n_tau], expected_step_up, expected_step_up * 5e-3f);

                        // Step down: filter state decays from A (amplitude) or A² (power) back to 0.
                        // Power mode: output = sqrt(A² * coeff^n) = A * sqrt(coeff^n).
                        const auto expected_step_down =
                          ffcast<float>(use_power ? test_amplitude * sqrt(decay) : test_amplitude * decay);
                        EXPECT_NEAR(samples[2 * N + n_tau], expected_step_down, expected_step_down * 5e-3f);
                    }

                    if (order == 1 && asymmetric == 2) {
                        // With fast attack (tau = attack_time_samples ≈ 48) the step-up should reach
                        // ~63% of the settled value at attack_time_samples, well before tau_release
                        // (~76 samples) would get there.
                        const unsigned n_attack = iround<unsigned>(attack_time_samples);
                        const double decay_attack = pow(coeff_attack, n_attack + 1); // ≈ 1/e

                        const auto expected_at_attack_tau = ffcast<float>(
                          use_power ? test_amplitude * sqrt(1.0 - decay_attack) : test_amplitude * (1.0 - decay_attack)
                        );
                        EXPECT_NEAR(samples[N + n_attack], expected_at_attack_tau, expected_at_attack_tau * 5e-3f);

                        // At the same point in time, a symmetric release-rate filter would be lower
                        // (since tau_attack < tau_release means a faster rise).
                        const double release_frac = 1.0 - pow(coeff_release, n_attack + 1);
                        const auto release_rate_output = ffcast<float>(
                          use_power ? test_amplitude * sqrt(release_frac) : test_amplitude * release_frac
                        );
                        EXPECT_GT(samples[N + n_attack], release_rate_output);
                    }
                }
                {
                    auto ef = EnvelopeFilter<float>(
                      use_power ? EnvelopeFilterOutputType::rms : EnvelopeFilterOutputType::amplitude,
                      order,
                      attack_time_samples_arg,
                      release_freq_hps
                    );
                    auto samples = sine_signal;
                    ef.process(samples);

                    if (asymmetric == 0) {
                        // Check the settled envelope in the middle of the sine segment (~31 time
                        // constants after onset). The filter smooths |sin| or sin² to its DC value:
                        //   power mode:     sqrt(mean(sin²)) = A / sqrt(2)   (RMS)
                        //   amplitude mode: mean(|sin|)      = A * 2 / pi
                        // Tolerance of 2% covers residual ripple from the 2*sine_freq harmonic.
                        const unsigned n_check = iround<unsigned>(N + N / 2.0);
                        const auto expected =
                          ffcast<float>(use_power ? test_amplitude / sqrt(2.0) : test_amplitude * 2.0 / num::pi);
                        EXPECT_NEAR(samples[n_check], expected, expected * 0.02f);
                    }
                }
            }
        }
    }
    println(f, "]';");
    println(f, "L = {{{}}}';", legend);
    fclose(f);
}
