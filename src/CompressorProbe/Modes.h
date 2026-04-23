#pragma once

#include "meadow/evariant.h"

#include <magic_enum/magic_enum.hpp>
#include <meadow/cppext.h>

constexpr inline array k_decibel_cycle_lengths_msec = {500, 1000, 2000, 4000, 8000};
constexpr int k_min_decibel_cycle_freq = 20;
enum class Waveform {
    sine,
    square,
    hi_crest_square
};
enum class LevelRef {
    peak,
    rms
};
enum class Channels {
    left,
    right,
    sum
};

namespace Mode
{
struct Bypass {
};
struct DecibelCycle {
    int freq = INT_MIN; // k_min_decibel_cycle_freq <= freq
    Waveform waveform;
    LevelRef level_ref;
    int min_dbfs = INT_MIN, max_dbfs = INT_MIN;
    unsigned cycle_length_index = UINT_MAX; // Into k_decibel_cycle_lengths_msec
    string to_string() const;
    bool operator==(const DecibelCycle&) const = default;
};
EVARIANT_DECLARE_E_V(Bypass, DecibelCycle)
} // namespace Mode

string_view get_label_for_enum(Mode::E e);

struct DecibelCycleLoopGenerator {
    double fs = NAN;
    Mode::DecibelCycle params;
    struct NormalizedPeriod {
        explicit NormalizedPeriod(unsigned capacity)
        {
            samples.reserve(capacity);
        }
        vector<double> samples;
        double crest_factor = NAN;
        double peak = NAN;
    } normalized_period;
    double min_dbfs = NAN, max_dbfs = NAN;
    unsigned num_periods = UINT_MAX;
    unsigned cycle_length_samples = UINT_MAX;

    // Constructor preallocates all necessary memory so we don't need to allocate in generate_block()
    explicit DecibelCycleLoopGenerator(double fs);

    // Generate samples from sample_index .. sample_index + output_block.length()
    // Expects sample_index < total_cycle_samples.
    // Caller needs to reset sample_index to 0 when new_params changes to make sure the condition applies after updating
    // total_cycle_samples.
    void generate_block(const Mode::DecibelCycle& new_params, unsigned sample_index, span<float> output_block);

private:
    void compute_normalized_period(int new_freq, Waveform new_waveform, LevelRef new_level_ref);
    double decibel_for_sample_ix(unsigned sample_ix) const;
};
