#pragma once

#include "Command.h"
#include "Modes.h"
#include "meadow/inplace_vector.h"

class Pipe;

namespace AnalyzerWorkspace
{

struct Bypass {
};

struct DecibelCycle {
    DecibelCycleLoopGenerator decibel_cycle_loop_generator;
    unsigned block_sample_index_in_cycle = 0;
    double input_period_sum2 = 0.0, output_period_sum2 = 0.0;
    double max_abs_input_period = 0.0, max_abs_output_period = 0.0;

    DecibelCycle(const Mode::DecibelCycle& decibel_cycle_params, double sample_rate);
};

EVARIANT_DECLARE_E_V(Bypass, DecibelCycle)
} // namespace AnalyzerWorkspace

struct ActiveCommand {
    Command command;
    chr::steady_clock::time_point command_received_timestamp;
    vector<float> compressor_output_blocks_tail;
    std::inplace_vector<double, 2> sync_corrs;
    optional<int64_t> test_signal_begin_sample_index;
    AnalyzerWorkspace::V analyzer_workspace;
};

struct ProbeRoleState {
    std::unique_ptr<Pipe> pipe;
    optional<Command> pending_command; // Command that has been sent but no response received.
    optional<ActiveCommand> active_command;
    int next_command_index = 1;
    unsigned test_signal_period_samples = 0;

    vector<float> compressor_input_block, compressor_output_block;

    ProbeRoleState();
    ~ProbeRoleState();
};
