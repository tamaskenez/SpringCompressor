#include "RmsDetector.h"

#include <meadow/cppext.h>
#include <meadow/math.h>

#include <gtest/gtest.h>

TEST(RmsDetectorTest, T1)
{
    const double fs = 48000;
    const double freq_hz = 30.0;
    const double freq_hps = freq_hz / fs * 2;
    RmsDetector d1(RmsDetector::Flavor::exponential_moving_average, freq_hps);
    RmsDetector d2(RmsDetector::Flavor::second_order_critically_damped, freq_hps);
    Biquad_TDF2 d3(matlab::butter(2, matlab::FilterType::LowPass{freq_hps}));
    SpringLowPass d4(fs);
    d4.set_critically_damped_with_time_constant(1 / (2 * std::numbers::pi * freq_hz));
    const auto N = iround<unsigned>(fs);
    vector<double> out1, out2, out3, out4;
    for (unsigned i = 0; i < N; ++i) {
        const float s = i == 0 ? 1.0 : 0.0;
        out1.push_back(square(d1.process(s)));
        out2.push_back(square(d2.process(s)));
        out3.push_back(d3.process(s));
        out4.push_back(d4.process(s));
    }
    FILE* f = fopen("/tmp/rms_test.m", "w");
    std::println(f, "A = [");
    for (unsigned i = 0; i < N; ++i) {
        std::println(f, " {} {} {} {}", out1[i], out2[i], out3[i], out4[i]);
    }
    std::println(f, "];");
    fclose(f);

    float dc1, dc2, dc3, dc4;
    for (unsigned i = 0; i < 999999; ++i) {
        dc1 = square(d1.process(1.0f));
        dc2 = square(d2.process(1.0f));
        dc3 = ffcast<float>(d3.process(1.0));
        dc4 = ffcast<float>(d4.process(1.0));
    }
    println("DC: {} {} {} {}", dc1, dc2, dc3, dc4);
}
