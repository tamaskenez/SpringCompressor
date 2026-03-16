#pragma once

#include "engine.h"

struct ScopeData {
    // Input-output dB steady signals of different frequencies.
    struct TransferGraph {
        struct TransferAtFreq {
            float freq_hz;
            vector<AF2> input_output_db;
        };
        vector<TransferAtFreq> transfers_by_freq;
    } transfer_graph;

    // Attack and release (step-up, step-down) for different levels.
    struct StepGraph {
        struct SignalAtLevel {
            float level_db;       // Above threshold.
            vector<float> signal; // Linear value.
        };
        vector<float> time_ms;
        vector<SignalAtLevel> signals;
    };
    struct StepGraphsAtFreq {
        float freq_hz;
        StepGraph attacks, releases;
    };
    vector<StepGraphsAtFreq> step_graphs_by_freq;

    // Harmonic distortion for steady signals of different frequencies and levels.
    struct HarmonicDistortionGraph {
        vector<float> freqs_hz;
        vector<float> levels_db; // Above threshold.
        vector<float> harmonic_distortion_data, inharmonic_distortion_data;
        std::mdspan<float, std::dextents<int, 2>> harmonic_distortion_db_by_freq_and_level,
          inharmonic_distortion_db_by_freq_and_level;
    };
};

ScopeData generate_scope_data(EnginePars pars);
