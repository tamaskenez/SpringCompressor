#include "Biquad_TD2.h"
#include "engine_util.h"
#include "meadow/math.h"
#include "meadow/matlab.h"

#include <meadow/cppext.h>
#include <meadow/matlab_signal.h>

#include <gtest/gtest.h>

TEST(engine_util_test, filters)
{
    constexpr double fs = 48000;
    constexpr double freq_hz = 30.0;
    constexpr double freq_hps = freq_hz / fs * 2;

    {
        // Check the two ways of calculating EMA filter, for low frequencies they must be nearly identical.
        const double c1 = exponential_moving_average_filter_coeff_from_cutoff_freq(freq_hps);
        const double c1_lower = exponential_moving_average_filter_coeff_from_cutoff_freq(1.00001 * freq_hps);
        ASSERT_LT(c1_lower, c1);
        const double c2 = exponential_moving_average_filter_coeff_from_time_constant(fs / (2 * num::pi * freq_hz));
        EXPECT_TRUE(in_cc_range(c2, c1_lower, c1));
    }

    const auto emaf_coeffs = exponential_moving_average_filter(freq_hps);
    const auto cdsolp_coeffs = critically_damped_second_order_lowpass(freq_hps);
    const auto butter2_coeffs = matlab::butter(2, matlab::FilterType::LowPass{freq_hps});

    // DC = 0 response.

    const auto emaf_0 = abs(matlab::freqz(emaf_coeffs.b, emaf_coeffs.a, 0.0));
    const auto cdsolp_0 = abs(matlab::freqz(cdsolp_coeffs.b, cdsolp_coeffs.a, 0.0));
    const auto butter2_0 = abs(matlab::freqz(butter2_coeffs.b, butter2_coeffs.a, 0.0));

    double eps = 1e-11;
    EXPECT_NEAR(emaf_0, 1.0, eps);
    EXPECT_NEAR(cdsolp_0, 1.0, eps);
    EXPECT_NEAR(butter2_0, 1.0, eps);

    const auto emaf_nyq = matlab::mag2db(abs(matlab::freqz(emaf_coeffs.b, emaf_coeffs.a, num::pi)));
    const auto cdsolp_nyq = matlab::mag2db(abs(matlab::freqz(cdsolp_coeffs.b, cdsolp_coeffs.a, num::pi)));
    const auto butter2_nyq = matlab::mag2db(abs(matlab::freqz(butter2_coeffs.b, butter2_coeffs.a, num::pi)));

    EXPECT_LT(emaf_nyq, -50);
    EXPECT_LT(cdsolp_nyq, -200);
    EXPECT_LT(butter2_nyq, -200);

    const auto emaf_fc = matlab::mag2db(abs(matlab::freqz(emaf_coeffs.b, emaf_coeffs.a, freq_hps * num::pi)));
    const auto cdsolp_fc = matlab::mag2db(abs(matlab::freqz(cdsolp_coeffs.b, cdsolp_coeffs.a, freq_hps * num::pi)));
    const auto butter2_fc = matlab::mag2db(abs(matlab::freqz(butter2_coeffs.b, butter2_coeffs.a, freq_hps * num::pi)));

    eps = 1e-5;
    const auto half_power_db = matlab::pow2db(0.5);
    EXPECT_NEAR(emaf_fc, half_power_db, eps);
    EXPECT_NEAR(cdsolp_fc, half_power_db, eps);
    EXPECT_NEAR(butter2_fc, half_power_db, eps);

    const auto N = iround<unsigned>(fs);

    ExponentialMovingAverageFilter d1(freq_hps);
    Biquad_TDF2 d2(cdsolp_coeffs);
    Biquad_TDF2 d3(butter2_coeffs);

    vector<double> out1, out2, out3;
    for (unsigned i = 0; i < N; ++i) {
        const float s = i == 0 ? 1.0 : 0.0;
        out1.push_back(d1.process(s));
        out2.push_back(d2.process(s));
        out3.push_back(d3.process(s));
    }
    FILE* f = fopen("/tmp/engine_util_test.m", "w");
    std::println(f, "A = [");
    for (unsigned i = 0; i < N; ++i) {
        std::println(f, " {} {} {}", out1[i], out2[i], out3[i]);
    }
    std::println(f, "];");
    fclose(f);

    double dc1, dc2, dc3;
    for (unsigned i = 0; i < 999999; ++i) {
        dc1 = d1.process(1.0);
        dc2 = d2.process(1.0);
        dc3 = d3.process(1.0);
    }
    println("DC: {} {} {}", dc1, dc2, dc3);

    NOP;
}

TEST(engine_util_test, time_constant_samples_to_cutoff_hps)
{
    constexpr double fs = 48000;

    const double time_constant_samples = 123.0;
    const double time_constant_sec = time_constant_samples / fs;
    const double cutoff_hz = time_constant_sec_to_cutoff_hz(time_constant_sec);
    const double cutoff_hps = hz_fs_to_hps(cutoff_hz, fs);
    const auto actual_cutoff_hps = time_constant_samples_to_cutoff_hps(time_constant_samples);
    EXPECT_NEAR(cutoff_hps, actual_cutoff_hps, 1e-16);
}
