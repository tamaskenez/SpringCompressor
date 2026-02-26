#include <gtest/gtest.h>

#include "SpringLowPass.h"

#include <fstream>

TEST(SpringLowPass, always_pass)
{
    SUCCEED();
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
