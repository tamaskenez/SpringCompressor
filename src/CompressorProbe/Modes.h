#pragma once

#include "meadow/evariant.h"

#include <meadow/cppext.h>

constexpr inline array k_decibel_cycle_lengths_msec = {500, 1000, 2000, 4000, 8000};
enum class Waveform {
    sine,
    square,
    hi_crest_square
};
enum class LevelMethod {
    peak,
    rms
};

namespace Mode
{
struct Bypass {
};
struct DecibelCycle {
    LevelMethod level_method;
    int min_dbfs, max_dbfs;
    int freq;
    unsigned cycle_index; // Into k_decibel_cycle_lengths_msec
    Waveform waveform;
};
EVARIANT_DECLARE_E_V(Bypass, DecibelCycle)
} // namespace Mode

struct DecibelCycleLoop {
    unsigned period_samples;
    vector<float> samples;
};

DecibelCycleLoop compute_decibel_cycle(double fs, const Mode::DecibelCycle& p);
