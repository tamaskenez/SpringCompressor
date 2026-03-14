#include <gtest/gtest.h>

#include "engine.h"

#include <cmath>
#include <format>
#include <fstream>
#include <numbers>
#include <span>
#include <vector>

TEST(Engine, save_sine_compression)
{
    constexpr double sample_rate = 48000.0;
    constexpr int num_samples = static_cast<int>(sample_rate); // 1 second
    constexpr double freq = 100.0;

    auto engine = make_engine();
    engine->prepare_to_play(sample_rate, num_samples, 1);

    std::vector<float> input(num_samples);
    double A = 1.0;
    for (size_t i = 0; i < static_cast<size_t>(num_samples); ++i) {
        input[i] =
          static_cast<float>(A * std::sin(2.0 * std::numbers::pi * freq * static_cast<double>(i) / sample_rate));
        if (i >= 200) {
            if (i < 800) {
                A *= 0.999;
            } else {
                A = 0;
            }
        }
        input[i] = 1;
    }

    std::vector<float> output = input;
    float* channel = output.data();
    auto trace_block = engine->process_block_with_trace(std::span<float* const>(&channel, 1), num_samples);

    std::ofstream file(TESTING_OUTPUT_DIR "/Engine_sine_compression.txt");
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(trace_block.size() == static_cast<size_t>(num_samples));
    for (size_t i = 0; i < static_cast<size_t>(num_samples); ++i) {
        const auto& ti = trace_block[i];
        file << std::format("{} {} {}\n", input[i], output[i], ti.gain);
    }
}
