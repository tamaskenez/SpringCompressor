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
    constexpr double sample_rate = 1000.0;
    constexpr int num_samples = static_cast<int>(sample_rate); // 1 second
    constexpr double freq = 100.0;

    auto engine = make_engine();
    engine->prepare_to_play(sample_rate, num_samples, 1);
    engine->set_threshold_db(-20.0f);
    engine->set_ratio(4.0f);
    engine->set_attack_ms(15.0f);
    engine->set_release_ms(15.0f);

    std::vector<float> input(num_samples);
    double A = 1.0;
    for (int i = 0; i < num_samples; ++i) {
        input[i] = static_cast<float>(A * std::sin(2.0 * std::numbers::pi * freq * i / sample_rate));
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
    std::vector<Engine::Trace> trace_block;
    engine->process_block(std::span<float* const>(&channel, 1), num_samples, &trace_block);

    std::ofstream file(TESTING_OUTPUT_DIR "/Engine_sine_compression.txt");
    ASSERT_TRUE(file.is_open());

#ifdef NDEBUG
    ASSERT_TRUE(trace_block.empty());
#else
    ASSERT_TRUE(trace_block.size() == num_samples);
#endif
    for (int i = 0; i < num_samples; ++i) {
        file << std::format("{} {}", input[i], output[i]);
#ifndef NDEBUG
        const auto& ti = trace_block[i];
        file << std::format(" {} {}", ti.smoothed_signal_power, ti.gain);
#endif
        file << "\n";
    }
}
