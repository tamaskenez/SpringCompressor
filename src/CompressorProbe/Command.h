#pragma once

#include <meadow/evariant.h>

#include <string>
#include <variant>

// Commands from the Probe to the Generator
namespace Command
{
struct Header {
    double fs;
    int command_index;
};

// Stop the current output mode.
struct Stop : Header {
    string to_string() const;
};

// Emit periods of sine/square wave, the amplitude going from min_dbfs to max_dbfs and back in num_periods, which is one
// full cycle, then it repeats.
struct DecibelCycle : Header {
    double min_dbfs, max_dbfs;
    int period_samples;
    int num_periods;
    optional<double> duty_cycle; // Sine wave if nullopt.
    string to_string() const;
};
EVARIANT_DECLARE_E_V(Stop, DecibelCycle)

string to_string(const V& v);
int command_index(const Command::V& v);
} // namespace Command

struct Response {
    int command_index;
    int effective_from_process_block_index;
};

expected<Command::V, string> command_from_span(span<const char> memory_block);
expected<Response, string> response_from_span(span<const char> memory_block);
