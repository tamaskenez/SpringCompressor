#include <gtest/gtest.h>

#include "SpringLowPass.h"

#include "Biquad_TD2.h"
#include "StateVariableTPTFilter.h"
#include "engine_util.h"
#include "meadow/cppext.h"

#include <cmath>
#include <fstream>
#include <numbers>
#include <span>

TEST(SpringLowPass, always_pass)
{
    SUCCEED();
}

TEST(SpringLowPass, save_cosine_response)
{
    constexpr double sample_rate = 1000.0;
    constexpr int num_samples = static_cast<int>(sample_rate); // 1 second

    SpringLowPass filter(sample_rate);
    filter.set_critically_damped_with_time_constant(0.1);

    std::ofstream file(TESTING_OUTPUT_DIR "/SpringLowPass_cosine.txt");
    ASSERT_TRUE(file.is_open());

    for (int i = 0; i < num_samples; ++i) {
        const double input = std::cos(2.0 * std::numbers::pi * i / 3.0);
        file << filter.process(input) << "\n";
    }
}

TEST(SpringLowPass, save_IR)
{
    constexpr double sample_rate = 1000.0;
    constexpr int num_samples = static_cast<int>(sample_rate); // 1 second

    SpringLowPass filter(sample_rate);
    filter.set_critically_damped_with_time_constant(0.1);

    std::ofstream file(TESTING_OUTPUT_DIR "/SpringLowPass_IR.txt");
    ASSERT_TRUE(file.is_open());

    for (int i = 0; i < num_samples; ++i) {
        const double input = (i == 0) ? 1.0 : 0.0;
        file << filter.process(input) << "\n";
    }
}

TEST(SpringLowPass, save_steps)
{
    constexpr double sample_rate = 1000.0;
    constexpr int num_samples = static_cast<int>(sample_rate); // 1 second

    SpringLowPass filter(sample_rate);
    filter.set_critically_damped_with_time_constant(0.01, 0.1);

    std::ofstream file(TESTING_OUTPUT_DIR "/SpringLowPass_steps.txt");
    ASSERT_TRUE(file.is_open());

    for (int i = 0; i < num_samples; ++i) {
        const double input = (i < 100) ? 1.0 : 0.0;
        file << filter.process(input) << "\n";
    }
}

TEST(SpringLowPass, compare_to_tustin_critically_damped_lpf)
{
    const double fs = 48000;
    SpringLowPass slp(fs);
    const double f_hz = 30;
    slp.set_critically_damped_with_time_constant(1 / (2 * std::numbers::pi * f_hz));
    std::vector<double> tustin_b = {0.384021890953201e-5, 0.768043781906403e-5, 0.384021890953201e-5};
    std::vector<double> tustin_a = {1.000000000000000, -1.992161409402673, 0.992176770278311};
    auto tustin = Biquad_TDF2(std::span<const double, 3>(tustin_b), std::span<const double, 3>(tustin_a));
    constexpr unsigned N = 1000;
    std::vector<double> slp_out;
    std::vector<double> tustin_out;
    for (unsigned i = 0; i < N; ++i) {
        const double s = i == 0 ? 1.0 : 0.0;
        tustin_out.push_back(tustin.process(s));
        slp_out.push_back(slp.process(s));
    }
    FILE* f = fopen("/tmp/slp_tustin.m", "w");
    std::println(f, "A = [");
    for (unsigned i = 0; i < N; ++i) {
        std::println(f, " {} {}", slp_out[i], tustin_out[i]);
    }
    std::println(f, "];");
}

TEST(SpringLowPass, compare_to_2nd_orders_lpf)
{
    const double fs = 48000;
    SpringLowPass slp(fs);
    const double f_hz = 300;
    // slp.set_critically_damped_with_time_constant(1 / (2 * std::numbers::pi * f_hz));
    slp.set_critically_damped_with_cutoff_freq(f_hz);
    const auto f_hps = hz_fs_to_hps(f_hz, fs);
    auto critdamp_coeffs = critically_damped_second_order_lowpass(f_hps);
    auto b2_coeffs = matlab::butter(2, matlab::FilterType::LowPass{f_hps});
    auto critdamp = Biquad_TDF2(critdamp_coeffs);
    auto b2 = Biquad_TDF2(b2_coeffs);
    auto tpt = StateVariableTPTFilter<double>();
    tpt.set_lowpass_3db_cutoff_freq_hz_Q(f_hz, 0.5);
    tpt.prepare_to_play(fs);

    const unsigned N = iround<unsigned>(fs);
    std::vector<double> slp_out, critdamp_out, b2_out, tpt_out;
    for (unsigned i = 0; i < N; ++i) {
        const double s = i == 0 ? 1.0 : 0.0;
        slp_out.push_back(slp.process(s));
        critdamp_out.push_back(critdamp.process(s));
        b2_out.push_back(b2.process(s));
        double x = s;
        tpt.process(span(&x, 1));
        tpt_out.push_back(x);
    }
    FILE* f = fopen("/tmp/slp_critdamp_b2.m", "w");
    std::println(f, "A = [");
    for (unsigned i = 0; i < N; ++i) {
        std::println(f, " {} {} {} {}", slp_out[i], critdamp_out[i], b2_out[i], tpt_out[i]);
    }
    std::println(f, "];");
}
