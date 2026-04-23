#pragma once

#include "Command.h"
#include "Modes.h"

class Pipe;

struct ActiveCommand {
    Command command;
    chr::steady_clock::time_point command_received_timestamp;
    optional<int64_t> test_signal_begin_sample_index;
};

struct ProbeRoleState {
    std::unique_ptr<Pipe> pipe;
    optional<Command> pending_command; // Command that has been sent but no response received.
    optional<ActiveCommand> active_command;
    int next_command_index = 1;
    vector<double> corrs;
    DecibelCycleLoopGenerator decibel_cycle_loop_generator;

    vector<float> compressor_input_block, compressor_output_block, compressor_output_blocks_tail;

    ProbeRoleState();
    ~ProbeRoleState();
};
