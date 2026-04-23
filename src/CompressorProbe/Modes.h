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
// A constant frequency waveform is generated with amplitude linearly rising in dB from min_dbfs to max_dbfs then back.
struct DecibelCycle {
    int freq = INT_MIN; // k_min_decibel_cycle_freq <= freq
    Waveform waveform;
    LevelRef level_ref;
    int min_dbfs = INT_MIN, max_dbfs = INT_MIN;
    unsigned cycle_length_index = UINT_MAX; // Into k_decibel_cycle_lengths_msec
    string to_string() const;
    bool operator==(const DecibelCycle&) const = default;
};

// A high-frequency carrier is modulated with a low-frequency envelope, from min_mod_freq to max_mod_freq
// (logarithmically) then back.
//
// The carrier's
// - max amplitude is max_carrier_amp_dbfs
// - the min amplitude is max_carrier_amp_dbfs - mod_amp_db
//
// The whole signal:
//
//     A1 = db2mag(max_carrier_amp_dbfs);
//     A2 = db2mag(max_carrier_amp_dbfs - mod_amp_db);
//     C = (A1 + A2) / 2;
//     A = (A2 - A1) / 2;
//     y = (A + C * cos(2 * pi * mod_freq * t)) * cos(2 * pi * carrier_freq * t)
struct EnvelopeFilter {
    int carrier_freq = INT_MIN;
    int max_carrier_amp_dbfs = INT_MIN;
    int min_mod_freq = INT_MIN;
    int max_mod_freq = INT_MIN;
    int mod_amp_db = INT_MIN;
    unsigned cycle_length_index = UINT_MAX; // Into k_decibel_cycle_lengths_msec

    bool operator==(const EnvelopeFilter&) const = default;
};

EVARIANT_DECLARE_E_V(Bypass, DecibelCycle, EnvelopeFilter)
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

struct EnvelopeFilterLoopGenerator {
    double fs = NAN;
    Mode::EnvelopeFilter params;
    size_t carrier_period_samples = SIZE_T_MAX;
    size_t num_periods = SIZE_T_MAX;
    size_t cycle_length_samples = SIZE_T_MAX;
    double T = NAN;
    double f0 = NAN, f1 = NAN;
    double k1 = NAN;
    double L1 = NAN, L2 = NAN;
    double alpha_T = NAN;
    double P1 = NAN;
    double f1_over_f0 = NAN;
    double MC = NAN, MA = NAN;

    void init(const Mode::EnvelopeFilter& new_params);

    explicit EnvelopeFilterLoopGenerator(double fs);

    // Generate samples from sample_index .. sample_index + output_block.length()
    // Expects sample_index < total_cycle_samples.
    // Caller needs to reset sample_index to 0 when new_params changes to make sure the condition applies after updating
    // total_cycle_samples.
    void generate_block(const Mode::EnvelopeFilter& new_params, unsigned sample_index, span<float> output_block);

    double freq_at_sample(size_t j) const;
};
